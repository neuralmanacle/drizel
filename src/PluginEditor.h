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
    juce::TextEditor speechText;
    juce::TextButton speechGenerate {"Generate speech"};
    juce::ComboBox speechTargetChoice;
    juce::Slider speechDepthSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> speechTargetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speechDepthAttachment;
    std::unique_ptr<juce::ChildProcess> speechProcess;
    juce::File speechOutput;
    juce::String speechMessage;
    bool speechPending = false;
    class TransportButton : public juce::TextButton
    {
    public:
        void paintButton(juce::Graphics& g, bool hovered, bool pressed) override
        {
            auto area=getLocalBounds().toFloat().reduced(1.0f);
            g.setColour(juce::Colours::white);
            const float cx=area.getCentreX(), cy=area.getCentreY();
            if(getToggleState())
            {
                g.fillRoundedRectangle(cx-9.0f,cy-9.0f,6.0f,18.0f,1.0f);
                g.fillRoundedRectangle(cx+3.0f,cy-9.0f,6.0f,18.0f,1.0f);
            }
            else
            {
                juce::Path triangle;
                triangle.addTriangle(cx-6.0f,cy-10.0f,cx-6.0f,cy+10.0f,cx+11.0f,cy);
                g.fillPath(triangle);
            }
        }
    };
    TransportButton playButton;
    juce::TextButton saveButton {"Save preset"}, presetButton {"Load preset"};
    juce::Label midiHelp, selectionLabel;
    juce::ComboBox selectionChoice, shapeChoice, destinationChoice, modeChoice;
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
    
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::array<std::unique_ptr<SliderAttachment>, 9> attachments;
    using ChoiceAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ChoiceAttachment> shapeAttachment, destinationAttachment, modeAttachment, selectionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAttachment;
    juce::AudioFormatManager formats;
    juce::AudioThumbnailCache cache {4};
    juce::AudioThumbnail thumbnail {512, formats, cache};
    juce::Rectangle<int> waveformBounds, meterBounds;
    std::uint64_t waveformRevision = 0;
    float displayedPeak = 0.0f;
    std::unique_ptr<juce::FileChooser> chooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrizelAudioProcessorEditor)
};
