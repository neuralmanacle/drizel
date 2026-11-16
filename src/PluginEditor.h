#pragma once
#include "PluginProcessor.h"
#include <array>

// All components, attachments and thumbnail work belong to the message thread.
class DrizelAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer, public juce::FileDragAndDropTarget, private juce::KeyListener
{
public:
    explicit DrizelAudioProcessorEditor(DrizelAudioProcessor&);
    ~DrizelAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&, juce::Component*) override;
    bool keyStateChanged(bool, juce::Component*) override;
private:
    void timerCallback() override;
    DrizelAudioProcessor& processor;
    juce::TextButton loadButton {"Load WAV"};
    juce::ToggleButton playButton {"Output on"};
    juce::TextButton saveButton {"Save preset"}, presetButton {"Load preset"};
    juce::Label midiHelp, selectionLabel, selectionDisplay, selectionTicks;
    juce::MidiKeyboardComponent keyboard {processor.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard};
    std::array<bool, 13> computerNotesDown {};
    void setWaveformPosition(int x);
    bool waveformGesture = false;
    juce::Rectangle<int> deckBounds, headerBounds, keyboardHeaderBounds;
    void choosePreset(bool save);
    juce::String presetStatus;
    juce::Label status;
    std::array<juce::Slider, 9> sliders;
    std::array<juce::Label, 12> labels;
    juce::Slider shapeKnob, destinationKnob, modeKnob;
    juce::Label shapeDisplay, destinationDisplay, modeDisplay;
    juce::Slider selectionKnob;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::array<std::unique_ptr<SliderAttachment>, 9> attachments;
    std::unique_ptr<SliderAttachment> shapeAttachment, destinationAttachment, modeAttachment;
    std::unique_ptr<SliderAttachment> selectionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAttachment;
    juce::AudioFormatManager formats;
    juce::AudioThumbnailCache cache {4};
    juce::AudioThumbnail thumbnail {512, formats, cache};
    juce::Rectangle<int> waveformBounds, meterBounds;
    std::uint64_t waveformRevision = 0;
    float displayedPeak = 0.0f;
    int activeGrains = 0;
    std::unique_ptr<juce::FileChooser> chooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrizelAudioProcessorEditor)
};
