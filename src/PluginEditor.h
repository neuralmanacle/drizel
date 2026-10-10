/*
  ==============================================================================

    Editor interface for the starter plugin UI.
    Painting/layout belong to the UI path, independently of offline WAV I/O.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 * @brief Host-managed JUCE editor displaying the starter "Hello World!" UI.
 *
 * The editor is a Component through AudioProcessorEditor inheritance. JUCE calls
 * paint and resized during UI activity; these are not audio processing callbacks.
 * The processor owns the eventual sound/state lifetime, while an editor may be
 * opened, destroyed, and reopened. Optional ARA inheritance is compile-time only.
 */
class DrizelAudioProcessorEditor  : public juce::AudioProcessorEditor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorEditorARAExtension
                            #endif
{
public:
    /** Construct for an existing processor and set an initial 400x300 size.
     * The supplied processor is borrowed and must outlive the editor.
     */
    DrizelAudioProcessorEditor (DrizelAudioProcessor&);
    /** Destroy UI-owned resources; never delete the referenced processor. */
    ~DrizelAudioProcessorEditor() override;

    //==============================================================================
    /** Draw the background and centred placeholder text in the current bounds.
     * The supplied Graphics is borrowed for this paint call. No audio/DSP runs here.
     */
    void paint (juce::Graphics&) override;
    /** Recompute child-component positions after a size change; currently empty
     * because this editor has no controls or other child components yet.
     */
    void resized() override;

private:
    // Non-owning reference: this aliases the existing processor, not a copy.
    // Holding it does not make direct cross-thread parameter changes safe; future
    // controls need the processor's agreed parameter/state handoff mechanism.
    DrizelAudioProcessor& audioProcessor;

    // UI components have a single identity and cannot be copied. The additional
    // JUCE member checks for leaked instances in supported debug configurations.
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrizelAudioProcessorEditor)
};
