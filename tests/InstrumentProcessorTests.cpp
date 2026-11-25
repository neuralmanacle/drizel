#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;
    try
    {
        DrizelAudioProcessor processor;
        if (!processor.acceptsMidi() || processor.getTotalNumInputChannels() != 0)
            throw std::runtime_error("Instrument must accept MIDI without an audio input bus");
        if (!processor.isBusesLayoutSupported(processor.getBusesLayout()))
            throw std::runtime_error("Instrument stereo output bus must be supported");
        processor.prepareToPlay(48000.0, 256);
        juce::AudioBuffer<float> audio(2, 256);
        audio.clear();
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 69, 1.0f), 0);
        processor.processBlock(audio, midi);
        if (audio.getMagnitude(0, 256) <= 0.001f)
            throw std::runtime_error("Instrument must generate MIDI sound without a WAV");
        std::cout << "Instrument MIDI routing passed\\n";
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\\n';
        return 1;
    }
    return 0;
}
