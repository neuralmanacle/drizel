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
    return layout; // Construction and allocations occur before playback.
}

DrizelAudioProcessor::DrizelAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      Thread("Drizel sample loader"), parameters(*this, nullptr, "DrizelState", makeParameters())
{
    static_assert(std::atomic<float>::is_always_lock_free, "Parameters must be lock-free");
    position = parameters.getRawParameterValue("position");
    pitch = parameters.getRawParameterValue("pitch");
    density = parameters.getRawParameterValue("density");
    gain = parameters.getRawParameterValue("gain");
    play = parameters.getRawParameterValue("play");
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
    engine.prepare(sampleRate); // The host guarantees processing is stopped here.
    const juce::ScopedLock lock(requestLock);
    requestedRate = sampleRate; // Rebuild the source/windows for a new device rate.
    ++revision;
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
    midi.clear(); // This free-running texture does not produce MIDI.
    if (buffer.getNumChannels() == 0)
        return;
    const auto pos = position->load(), semitones = pitch->load(); // One atomic snapshot per block.
    const auto grains = density->load(), level = gain->load() * play->load();
    auto* left = buffer.getWritePointer(0); // Borrow only for this callback.
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    for (int i = 0; i < buffer.getNumSamples(); ++i) // No assumed maximum block size.
    {
        const auto frame = engine.next(pos, semitones, grains, level);
        left[i] = right != nullptr ? frame.left : 0.5f * (frame.left + frame.right);
        if (right != nullptr)
            right[i] = frame.right;
    }
}
void DrizelAudioProcessor::requestSample(const juce::File& file)
{
    const juce::ScopedLock lock(requestLock);
    requestedPath = file.getFullPathName(); // No decode on the UI thread.
    status = "Loading " + file.getFileName();
    ++revision; // Newest request wins if a previous decode is still running.
    notify();
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
                const juce::ScopedLock lock(requestLock);
                if (job == revision && !threadShouldExit())
                {
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
            parameters.replaceState(state); // JUCE updates the atomic parameter values.
            const auto path = state.getProperty("samplePath").toString();
            const juce::ScopedLock lock(requestLock);
            requestedPath = path; // An empty saved path also replaces the old source.
            status = path.isEmpty() ? "Load a WAV to begin" : "Restoring sample...";
            ++revision;
            notify(); // Decode/clear remains asynchronous.
        }
    }
}
juce::AudioProcessorEditor* DrizelAudioProcessor::createEditor()
{
    return new DrizelAudioProcessorEditor(*this); // Host owns the editor lifetime.
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrizelAudioProcessor();
}
