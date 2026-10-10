/*
  ==============================================================================

    Starter editor implementation: construction, painting, and layout.
    The editor is separate from both the real-time callback and offline experiment.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// The initialiser list runs before the body: configure the JUCE base with &p,
// initialise an optional ARA base, and bind our non-owning audioProcessor reference.
DrizelAudioProcessorEditor::DrizelAudioProcessorEditor (DrizelAudioProcessor& p)
    : AudioProcessorEditor (&p),
     #if JucePlugin_Enable_ARA
      AudioProcessorEditorARAExtension (&p),
     #endif
      audioProcessor (p)
{
   #if JucePlugin_Enable_ARA
    // ARA plugins must be resizable for proper view embedding
    setResizable (true, false);
   #endif

    // Give the host a usable initial UI size. setSize can trigger resized();
    // that callback currently has no child components to lay out.
    setSize (400, 300);
}

DrizelAudioProcessorEditor::~DrizelAudioProcessorEditor()
{
    // Member/base destructors handle the current UI. audioProcessor is a borrowed
    // reference, so closing the editor must not destroy the processor instance.
}

//==============================================================================
void DrizelAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Cover the full editor area using JUCE's current LookAndFeel background.
    // Paint may run repeatedly for exposure or UI updates; no disk reads belong
    // in this draw path. It draws existing UI state, not a stream of audio.
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Subsequent text uses this colour/font. Local bounds are relative to this
    // component; centred justification and a one-line limit place the placeholder.
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void DrizelAudioProcessorEditor::resized()
{
    // Future controls get their bounds here so layout follows size changes.
    // This starter editor has no children; painting uses getLocalBounds directly.
}
