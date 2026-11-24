#include "PluginProcessor.h"
#include "io/WavFile.h"
#include <iostream>
#include <stdexcept>

static void check(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}
static void set(DrizelAudioProcessor& p, const char* id, float value)
{
    auto* parameter = p.parameters.getParameter(id);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
int main()
{
    juce::ScopedJuceInitialiser_GUI juce;
    const auto directory = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("drizel-processor-test", "", false);
    directory.createDirectory();
    try
    {
        DrizelAudioProcessor p;
        check(p.acceptsMidi(), "processor must advertise MIDI input");
        check(p.getTotalNumInputChannels() == 2, "processor must accept stereo DAW audio");
        check(p.isBusesLayoutSupported({juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo()}),
              "stereo input/output must be supported");
        p.prepareToPlay(1000, 64);
        drizel::SampleBuffer sample(1, 2000, 1000);
        for (std::size_t i = 0; i < sample.frameCount(); ++i) sample.channelData(0)[i] = 0.5f;
        const auto wav = directory.getChildFile("source.wav");
        drizel::writeWav(std::filesystem::u8path(wav.getFullPathName().toStdString()), sample);
        p.requestSample(wav);
        for (int i = 0; i < 500 && p.loadedSampleState().path != wav.getFullPathName(); ++i)
            juce::Thread::sleep(10);
        check(p.loadedSampleState().path == wav.getFullPathName(), "background sample must load");
        set(p, "midiMode", 1); set(p, "attack", 0.001f); set(p, "release", 0.005f);
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        for (int i = 0; i < 20; ++i) p.processBlock(audio, midi);
        check(audio.getMagnitude(0, 64) == 0, "MIDI mode is silent without notes");
        midi.addEvent(juce::MidiMessage::noteOn(2, 60, 1.0f), 0);
        p.processBlock(audio, midi);
        check(audio.getMagnitude(0, 64) == 0, "other MIDI channels ignored");
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 17);
        midi.addEvent(juce::MidiMessage::noteOff(1, 60), 41);
        p.processBlock(audio, midi);
        check(audio.getMagnitude(0, 17) == 0, "no audio before event offset");
        check(audio.getMagnitude(17, 24) > 0, "note produces sound at nonzero offset");
        check(audio.getMagnitude(47, 17) == 0, "release returns to exact silence");
        check(midi.isEmpty(), "MIDI input consumed");
        p.setPanelNote(60, 127);
        p.processBlock(audio, midi);
        check(audio.getMagnitude(0, 64) > 0, "panel key produces sound");
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
        p.processBlock(audio, midi);
        check(p.displayedHostNote() == 60, "host note is visible on the panel");
        p.setPanelNote(60, 0);
        p.processBlock(audio, midi);
        check(audio.getMagnitude(0, 64) > 0, "panel release preserves the held host note");
        midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
        p.processBlock(audio, midi);
        check(p.displayedHostNote() == -1, "host note highlight clears");
        check(audio.getMagnitude(8, 56) == 0, "last key release reaches silence");
        set(p, "audioInputMode", 1); // Monitor mode passes host audio through.
        set(p, "play", 0); // Isolate DAW audio from the granular engine.
        audio.clear();
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < audio.getNumSamples(); ++i)
                audio.setSample(ch, i, 0.25f);
        p.processBlock(audio, midi);
        check(std::abs(audio.getSample(0, 0) - 0.25f) < 0.001f,
              "monitor mode must preserve DAW audio");
        set(p, "audioInputMode", 2);
        p.beginDawCapture();
        audio.clear();
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < audio.getNumSamples(); ++i)
                audio.setSample(ch, i, 0.2f);
        p.processBlock(audio, midi);
        p.stopDawCapture();
        p.processBlock(audio, midi);
        check(p.dawCaptureReady() && p.capturedFrames() == 64,
              "DAW audio capture must finish at the next block boundary");
        const auto capturedWav = directory.getChildFile("captured.wav");
        check(p.finishDawCapture(capturedWav) && capturedWav.existsAsFile(),
              "DAW input should export to WAV");
        set(p, "play", 1);
        set(p, "audioInputMode", 0);
        set(p, "position", 0.35f); set(p, "rootNote", 48); set(p, "lfoDepth", 0.7f);
        set(p, "grainSelection", 4);
        const auto preset = directory.getChildFile("patch.drizel");
        p.setMovement({drizel::MovementPattern::stepped, 20, 70, 6, 15, 30, 40});
        const auto expectedMovement = p.activeMovement().packed();
        const auto previewRevision = p.sampleRequestRevision();
        auto invalidMovement = p.activeMovement();
        invalidMovement.end = invalidMovement.start;
        check(!p.applyMovementForSample(invalidMovement, previewRevision), "invalid preview rejected");
        check(p.activeMovement().packed() == expectedMovement, "invalid preview preserves policy");
        p.requestSample(wav);
        check(!p.applyMovementForSample({}, previewRevision), "sample request invalidates preview");
        check(p.activeMovement().packed() == expectedMovement, "stale preview preserves policy");
        check(p.applyMovementForSample(p.activeMovement(), p.sampleRequestRevision()),
              "current preview accepted");
        check(p.savePreset(preset), "preset write");
        set(p, "grainSelection", 0);
        set(p, "position", 0.8f); set(p, "rootNote", 60);
        p.setMovement({});
        check(p.loadPreset(preset), "preset read");
        check(p.activeMovement().packed() == expectedMovement, "preset restores movement policy");
        check(std::abs(p.parameters.getRawParameterValue("position")->load() - 0.35f) < 1e-5f,
              "preset restores parameter");
        check(p.parameters.getRawParameterValue("rootNote")->load() == 48, "preset restores MIDI setting");
        check(p.parameters.getRawParameterValue("grainSelection")->load() == 4,
              "preset restores guided selection");
        juce::MemoryBlock state;
        p.getStateInformation(state);
        set(p, "lfoDepth", 0);
        p.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        check(std::abs(p.parameters.getRawParameterValue("lfoDepth")->load() - 0.7f) < 1e-5f,
              "host state restores modulation");
        const auto legacyPreset = directory.getChildFile("legacy.drizel");
        auto legacyState = p.parameters.copyState();
        legacyState.removeProperty("movementPolicyV1", nullptr);
        legacyState.removeChild(legacyState.getChildWithProperty("id", "grainSelection"), nullptr);
        juce::MemoryBlock legacyData;
        juce::AudioProcessor::copyXmlToBinary(*legacyState.createXml(), legacyData);
        check(legacyPreset.replaceWithData(legacyData.getData(), legacyData.getSize()), "write legacy preset");
        check(p.loadPreset(legacyPreset), "legacy preset loads");
        check(p.activeMovement().pattern == drizel::MovementPattern::manual,
              "legacy preset restores manual movement");
        check(p.parameters.getRawParameterValue("grainSelection")->load() == 0,
              "legacy state restores manual selection");
        const auto invalid = directory.getChildFile("invalid.drizel");
        invalid.replaceWithText("invalid");
        check(!p.loadPreset(invalid), "invalid preset rejected");
        check(p.parameters.getRawParameterValue("rootNote")->load() == 48, "failed preset preserves controls");
        // State restore clears performance state on the next audio block.
        p.processBlock(audio, midi);
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
        p.processBlock(audio, midi);
        check(p.loadPreset(preset), "recall while a note is held");
        p.processBlock(audio, midi);
        check(audio.getMagnitude(0, 64) == 0, "preset recall clears held note");
        std::cout << "Processor MIDI and preset integration tests passed\n";
    }
    catch (const std::exception& error)
    {
        directory.deleteRecursively();
        std::cerr << error.what() << '\n'; return 1;
    }
    directory.deleteRecursively();
}
