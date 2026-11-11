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
        set(p, "position", 0.35f); set(p, "rootNote", 48); set(p, "lfoDepth", 0.7f);
        set(p, "grainSelection", 4);
        const auto preset = directory.getChildFile("patch.drizel");
        check(p.savePreset(preset), "preset write");
        set(p, "grainSelection", 0);
        set(p, "position", 0.8f); set(p, "rootNote", 60);
        check(p.loadPreset(preset), "preset read");
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
        legacyState.removeChild(legacyState.getChildWithProperty("id", "grainSelection"), nullptr);
        juce::MemoryBlock legacyData;
        juce::AudioProcessor::copyXmlToBinary(*legacyState.createXml(), legacyData);
        check(legacyPreset.replaceWithData(legacyData.getData(), legacyData.getSize()), "write legacy preset");
        check(p.loadPreset(legacyPreset), "legacy preset loads");
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
