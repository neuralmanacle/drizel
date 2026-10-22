#include "PluginEditor.h"

DrizelAudioProcessorEditor::DrizelAudioProcessorEditor(DrizelAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p), controls(p)
{
    addAndMakeVisible(loadButton);
    addAndMakeVisible(status);
    addAndMakeVisible(controls);
    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser>("Load a WAV", juce::File{}, "*.wav");
        auto safe = juce::Component::SafePointer<DrizelAudioProcessorEditor>(this);
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe](const juce::FileChooser& dialog)
            {
                if (safe != nullptr && dialog.getResult().existsAsFile())
                    safe->processor.requestSample(dialog.getResult());
            }); // SafePointer prevents access after the editor closes.
    };
    setSize(520, 350);
    startTimerHz(5); // UI status never reads or locks the audio engine.
    timerCallback();
}
void DrizelAudioProcessorEditor::timerCallback()
{
    status.setText(processor.loadStatus(), juce::dontSendNotification);
}
void DrizelAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff202323));
}
void DrizelAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(12);
    loadButton.setBounds(bounds.removeFromTop(30));
    status.setBounds(bounds.removeFromTop(40));
    controls.setBounds(bounds);
}
