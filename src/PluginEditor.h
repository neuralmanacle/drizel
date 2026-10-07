/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class DrizelAudioProcessorEditor  : public juce::AudioProcessorEditor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorEditorARAExtension
                            #endif
{
public:
    DrizelAudioProcessorEditor (DrizelAudioProcessor&);
    ~DrizelAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DrizelAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrizelAudioProcessorEditor)
};
