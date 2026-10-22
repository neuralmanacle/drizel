#pragma once
#include "PluginProcessor.h"

// Minimal Step 6 test panel; the instrument's designed interface comes later.
class DrizelAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DrizelAudioProcessorEditor(DrizelAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override; // Poll status on the message thread.
    DrizelAudioProcessor& processor;
    juce::TextButton loadButton {"Load WAV"};
    juce::Label status;
    juce::GenericAudioProcessorEditor controls; // JUCE supplies host parameter bindings.
    std::unique_ptr<juce::FileChooser> chooser; // Keep asynchronous chooser alive.
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrizelAudioProcessorEditor)
};
