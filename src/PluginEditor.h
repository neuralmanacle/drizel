#pragma once
#include "PluginProcessor.h"
#include <array>
#if DRIZEL_MAC_SPEECH
#include "MovementSpeech.h"
#endif

// All components, attachments and thumbnail work belong to the message thread.
class DrizelAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer, private juce::MidiKeyboardState::Listener, public juce::FileDragAndDropTarget
{
public:
    explicit DrizelAudioProcessorEditor(DrizelAudioProcessor&);
    ~DrizelAudioProcessorEditor() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;
private:
    void handleNoteOn(juce::MidiKeyboardState*, int channel, int note, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int channel, int note, float) override;
    void setKeyboardOctave(int octave);
    void previewMovement();
    void applyMovement();
    juce::TextEditor movementCommand;
    juce::TextButton previewButton {"Preview"}, applyButton {"Apply"}, manualButton {"Manual scan"};
#if DRIZEL_MAC_SPEECH
    MovementSpeech movementSpeech;
    juce::TextButton speechButton {"Hold to talk"};
    bool speechHeld = false;
#endif
    juce::Label movementStatus;
    std::array<juce::Slider, 6> movementSettings;
    std::array<juce::Label, 6> movementLabels;
    drizel::MovementPolicy proposedMovement;
    void timerCallback() override;
    void setWaveformPosition(float x);
    void endWaveformGesture();
    DrizelAudioProcessor& processor;
    juce::ParameterAttachment waveformPositionAttachment;
    bool waveformGestureActive = false;
    juce::TextButton loadButton {"Load WAV"};
    juce::ToggleButton playButton {"Output on"};
    juce::TextButton saveButton {"Save preset"}, presetButton {"Load preset"};
    juce::Label midiHelp, selectionLabel, keyboardRange, movementTitle;
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboard {keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard};
    juce::TextButton octaveDown {"−"}, octaveUp {"+"};
    int keyboardOctave = 3, highlightedNote = -1;
    std::array<juce::Rectangle<int>, 3> banks;
    void choosePreset(bool save);
    juce::String presetStatus;
    juce::Label status;
    std::array<juce::Slider, 9> sliders;
    std::array<juce::Label, 12> labels;
    juce::ComboBox shape, destination, mode, selection;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::array<std::unique_ptr<SliderAttachment>, 9> attachments;
    std::unique_ptr<ComboAttachment> shapeAttachment, destinationAttachment, modeAttachment, selectionAttachment;
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
