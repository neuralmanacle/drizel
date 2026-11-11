#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "io/WavFile.h"

juce::AudioProcessorValueTreeState::ParameterLayout DrizelAudioProcessor::makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"position", 1}, "Position", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"pitch", 1}, "Pitch (semitones)", -24.0f, 24.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"density", 1}, "Density (grains/sec)", 1.0f, 100.0f, 20.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"gain", 1}, "Output gain", 0.0f, 1.0f, 0.8f));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"play", 1}, "Play", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"lfoRate", 1}, "LFO rate (Hz)", 0.05f, 10.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"lfoDepth", 1}, "LFO depth", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"lfoShape", 1}, "LFO shape", juce::StringArray{"Sine", "Triangle"}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"lfoDestination", 1}, "LFO destination", juce::StringArray{"Off", "Position", "Pitch"}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"midiMode", 1}, "Mode", juce::StringArray{"Drone", "MIDI"}, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"attack", 1}, "Attack (seconds)", 0.001f, 2.0f, 0.01f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"release", 1}, "Release (seconds)", 0.005f, 5.0f, 0.2f));
    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"rootNote", 1}, "Root MIDI note", 0, 127, 60));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"grainSelection", 2},
        "Grain selection", juce::StringArray{"Manual", "Attack", "Sustain", "Avoid silence", "Bright"}, 0));
    return layout; // Construction and allocations occur before playback.
}

DrizelAudioProcessor::DrizelAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      Thread("Drizel sample loader"), parameters(*this, nullptr, "DrizelState", makeParameters())
{
    static_assert(std::atomic<float>::is_always_lock_free, "Parameters must be lock-free");
    static_assert(std::atomic<int>::is_always_lock_free, "Telemetry must be lock-free");
    selectionMode = parameters.getRawParameterValue("grainSelection");
    position = parameters.getRawParameterValue("position");
    pitch = parameters.getRawParameterValue("pitch");
    density = parameters.getRawParameterValue("density");
    gain = parameters.getRawParameterValue("gain");
    play = parameters.getRawParameterValue("play");
    lfoRate = parameters.getRawParameterValue("lfoRate");
    lfoDepth = parameters.getRawParameterValue("lfoDepth");
    lfoShape = parameters.getRawParameterValue("lfoShape");
    lfoDestination = parameters.getRawParameterValue("lfoDestination");
    midiMode = parameters.getRawParameterValue("midiMode");
    attack = parameters.getRawParameterValue("attack");
    release = parameters.getRawParameterValue("release");
    rootNote = parameters.getRawParameterValue("rootNote");
    startThread(); // No thread creation is deferred into the callback.
}
DrizelAudioProcessor::~DrizelAudioProcessor()
{
    signalThreadShouldExit(); // The host has stopped processing before destruction.
    notify(); // Wake a sleeping loader immediately.
    stopThread(-1); // Join without killing a worker that may own sample memory.
}
void DrizelAudioProcessor::prepareToPlay(double sampleRate, int)
{
    engine.prepare(sampleRate);
    instrument.prepare(sampleRate);
    previousMidiMode = midiMode->load() >= 0.5f;
    lfo.prepare(sampleRate); // The host guarantees processing is stopped here.
    const juce::ScopedLock lock(requestLock);
    requestedRate = sampleRate; // Rebuild the source/windows for a new device rate.
    engine.expectRevision(++revision);
    notify();
}
bool DrizelAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet().isDisabled()
        && (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
            || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}
void DrizelAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals; // Avoid expensive subnormal arithmetic.
    buffer.clear(); // Also initialise any unexpected extra channels.
    if (buffer.getNumChannels() == 0)
    {
        midi.clear();
        instrument.reset();
        peak.store(0.0f, std::memory_order_relaxed);
        grainsActive.store(0, std::memory_order_relaxed);
        return;
    }
    const bool useMidi = midiMode->load() >= 0.5f;
    if (resetInstrument.exchange(false) || useMidi != previousMidiMode)
        instrument.reset();
    previousMidiMode = useMidi;
    instrument.setEnvelope(attack->load(), release->load());
    const int root = static_cast<int>(rootNote->load());
    auto event = midi.cbegin();
    const auto eventEnd = midi.cend();
    const auto selection = static_cast<drizel::GrainSelection>(static_cast<int>(selectionMode->load()));
    const auto pos = position->load(), semitones = pitch->load(); // One atomic snapshot per block.
    const auto grains = density->load(), level = gain->load() * play->load();
    lfo.setRate(lfoRate->load());
    lfo.setShape(lfoShape->load() >= 0.5f ? drizel::LfoShape::triangle : drizel::LfoShape::sine);
    const int destination = static_cast<int>(lfoDestination->load());
    const auto route = destination == 1 ? drizel::ModDestination::position
                   : destination == 2 ? drizel::ModDestination::pitch
                                      : drizel::ModDestination::off;
    const double depth = lfoDepth->load();
    auto* left = buffer.getWritePointer(0); // Borrow only for this callback.
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    float blockPeak = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i) // No assumed maximum block size.
    {
        while (event != eventEnd && (*event).samplePosition <= i)
        {
            // Inspect borrowed bytes, avoiding MidiMessage copies (SysEx can allocate).
            const auto message = *event++;
            const auto* data = message.data;
            if (useMidi && message.numBytes >= 3 && (data[0] & 0x0f) == 0)
            {
                switch (data[0] & 0xf0)
                {
                    case 0x90: instrument.noteOn(data[1], data[2] / 127.0); break;
                    case 0x80: instrument.noteOff(data[1]); break;
                    case 0xb0: instrument.controller(data[1], data[2]); break;
                    case 0xe0: instrument.pitchBend(data[1] | (data[2] << 7)); break;
                    default: break;
                }
            }
        }
        modulation.set(route, std::min(1.0, depth + (useMidi ? instrument.wheel() : 0.0)));
        const auto targets = modulation.apply(useMidi ? instrument.position(pos) : pos, semitones, lfo.next());
        const auto frame = engine.next(targets.position,
            targets.pitch + (useMidi ? instrument.pitchOffset(root) : 0.0),
            useMidi ? instrument.density(grains) : grains, level, selection);
        // Apply the note envelope AFTER the engine's gain smoother: exact silent gate.
        const float amplitude = useMidi ? static_cast<float>(instrument.nextAmplitude()) : 1.0f;
        left[i] = amplitude * (right != nullptr ? frame.left : 0.5f * (frame.left + frame.right));
        if (right != nullptr)
            right[i] = amplitude * frame.right;
        blockPeak = std::max(blockPeak, std::abs(left[i]));
        if (right != nullptr)
            blockPeak = std::max(blockPeak, std::abs(right[i]));
    }
    midi.clear(); // Input consumed; this instrument does not emit MIDI.
    peak.store(blockPeak, std::memory_order_relaxed);
    grainsActive.store(static_cast<int>(engine.activeGrains()), std::memory_order_relaxed);
}
void DrizelAudioProcessor::requestSample(const juce::File& file)
{
    const juce::ScopedLock lock(requestLock);
    requestedPath = file.getFullPathName(); // No decode on the UI thread.
    status = "Loading " + file.getFileName();
    engine.expectRevision(++revision); // Newest request wins if a previous decode is still running.
    notify();
}
DrizelAudioProcessor::SampleDisplayState DrizelAudioProcessor::loadedSampleState()
{
    const juce::ScopedLock lock(requestLock);
    return {readyPath, readyRevision};
}
juce::String DrizelAudioProcessor::loadStatus()
{
    const juce::ScopedLock lock(requestLock);
    return status;
}
void DrizelAudioProcessor::run()
{
    std::uint64_t handled = 0; // Only the loader reads/writes this local revision.
    while (!threadShouldExit())
    {
        engine.collect(); // Free old sources/windows on this background thread.
        juce::String path;
        double rate;
        std::uint64_t job;
        {
            const juce::ScopedLock lock(requestLock);
            path = requestedPath;
            rate = requestedRate;
            job = revision; // Snapshot the entire request under one lock.
        }
        if (job != handled && rate > 0.0)
        {
            handled = job; // Failed jobs wait for an explicit new request.
            try
            {
                auto sample = path.isEmpty() ? drizel::SampleBuffer(1, 1, rate)
                    : drizel::loadWav(std::filesystem::u8path(path.toStdString()));
                auto ready = std::make_unique<drizel::PreparedSample>(std::move(sample), rate);
                ready->revision = job;
                if (path.isNotEmpty() && !ready->selection.prepare(ready->source, [this, job]
                    { return threadShouldExit() || !engine.expectsRevision(job); }))
                    continue;
                const juce::ScopedLock lock(requestLock);
                if (job == revision && !threadShouldExit())
                {
                    readyRevision = job;
                    readyPath = path; // UI waveform follows successful decoding only.
                    engine.publish(std::move(ready)); // Atomic ownership transfer.
                    status = path.isEmpty() ? "Load a WAV to begin"
                                            : "Ready: " + juce::File(path).getFileName();
                }
            }
            catch (const std::exception& error)
            {
                const juce::ScopedLock lock(requestLock);
                if (job == revision)
                    status = "Load failed: " + juce::String(error.what()); // Current audio continues.
            }
        }
        wait(20); // Poll reclamation even when no new file is requested.
    }
    engine.collect(); // Final reclamation before thread exit.
}
void DrizelAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state = parameters.copyState(); // Host state callbacks are outside processBlock.
    {
        const juce::ScopedLock lock(requestLock);
        state.setProperty("samplePath", requestedPath, nullptr);
    }
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destination);
}
void DrizelAudioProcessor::setStateInformation(const void* data, int bytes)
{
    if (auto xml = getXmlFromBinary(data, bytes))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.hasType(parameters.state.getType()))
        {
            resetInstrument.store(true);
            // Older states predate the selection parameter. Restore its default
            // explicitly even if a guided mode was active before recall.
            if (!state.getChildWithProperty("id", "grainSelection").isValid())
            {
                juce::ValueTree selectionState("PARAM");
                selectionState.setProperty("id", "grainSelection", nullptr);
                selectionState.setProperty("value", 0.0f, nullptr);
                state.appendChild(selectionState, nullptr);
            }
            parameters.replaceState(state); // JUCE updates the atomic parameter values.
            const auto path = state.getProperty("samplePath").toString();
            const juce::ScopedLock lock(requestLock);
            requestedPath = path; // An empty saved path also replaces the old source.
            status = path.isEmpty() ? "Load a WAV to begin" : "Restoring sample...";
            engine.expectRevision(++revision);
            notify(); // Decode/clear remains asynchronous.
        }
    }
}
bool DrizelAudioProcessor::savePreset(const juce::File& file)
{
    juce::MemoryBlock data;
    getStateInformation(data);
    return file.replaceWithData(data.getData(), data.getSize());
}
bool DrizelAudioProcessor::loadPreset(const juce::File& file)
{
    juce::MemoryBlock data;
    if (file.getSize() > 1024 * 1024 || !file.loadFileAsData(data)) return false;
    auto xml = getXmlFromBinary(data.getData(), static_cast<int>(data.getSize()));
    if (!xml || !juce::ValueTree::fromXml(*xml).hasType(parameters.state.getType())) return false;
    setStateInformation(data.getData(), static_cast<int>(data.getSize()));
    return true;
}
juce::AudioProcessorEditor* DrizelAudioProcessor::createEditor()
{
    return new DrizelAudioProcessorEditor(*this); // Host owns the editor lifetime.
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrizelAudioProcessor();
}
