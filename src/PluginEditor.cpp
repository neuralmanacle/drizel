#include "PluginEditor.h"
#if defined(DRIZEL_HAS_DRIZEL_LOGO)
#include <BinaryData.h>
#endif
namespace
{
constexpr juce::uint32 emerald = 0xff8eaf27;
constexpr juce::uint32 mint = 0xff8dca67;
constexpr juce::uint32 cyan = 0xffd73832;
constexpr juce::uint32 forest = 0xff103d20;
constexpr juce::uint32 ink = 0xff10271a;
constexpr juce::uint32 pearl = 0xfffff0d8;

class DrizelGlassLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::String family;
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& font) override
    {
        if (family.isNotEmpty())
        {
            auto selectedFont = font;
            selectedFont.setTypefaceName(family);
            return juce::Typeface::createSystemTypefaceFor(selectedFont);
        }
        return juce::LookAndFeel_V4::getTypefaceForFont(font);
    }
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour&, bool hovered, bool down) override
    {
        auto bounds=button.getLocalBounds().toFloat().reduced(0.7f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(down ? 0xff416653 : hovered ? 0xff345b49 : 0xff25483a),
                                               0.0f,0.0f,juce::Colour(0xff102b25),0.0f,bounds.getBottom(),false));
        g.fillRoundedRectangle(bounds,7.0f);
        g.setColour(juce::Colour(0xff86a891));
        g.drawRoundedRectangle(bounds.reduced(0.5f),7.0f,1.1f);
    }
    DrizelGlassLookAndFeel()
    {
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1a372e));
        setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe0ebd9));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff557b64));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xffffd18a));
        setColour(juce::PopupMenu::headerTextColourId, juce::Colour(0xffb8cfbd));
    }
    juce::Font getPopupMenuFont() override
    {
        return juce::Font(juce::FontOptions(15.0f));
    }
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        const auto area=juce::Rectangle<float>(0.0f,0.0f,(float)width,(float)height);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff344f40),0.0f,0.0f,
                                               juce::Colour(0xff152d27),0.0f,(float)height,false));
        g.fillRoundedRectangle(area.reduced(1.0f),9.0f);
        g.setColour(juce::Colour(0xff9db7a1));
        g.drawRoundedRectangle(area.reduced(1.5f),9.0f,1.5f);
        g.setColour(juce::Colour(0xffd73832).withAlpha(0.48f));
        g.drawLine(8.0f,3.0f,(float)width-8.0f,3.0f,1.0f);
    }
    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int, int, int, int, juce::ComboBox&) override
    {
        const auto area=juce::Rectangle<float>(0.5f,0.5f,(float)width-1.0f,(float)height-1.0f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff25483a),0.0f,0.0f,
                                               juce::Colour(0xff102b25),0.0f,(float)height,false));
        g.fillRoundedRectangle(area,6.0f);
        g.setColour(juce::Colour(0xff86a891));
        g.drawRoundedRectangle(area.reduced(0.5f),6.0f,1.2f);
        g.setColour(juce::Colour(0xffd73832));
        juce::Path arrow;
        const float cx=(float)width-15.0f, cy=(float)height*0.5f;
        arrow.addTriangle(cx-4.0f,cy-2.0f,cx+4.0f,cy-2.0f,cx,cy+3.0f);
        g.fillPath(arrow);
    }
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override
    {
        // Machined analog-synth knob: knurled edge, brushed cap, recessed
        // indicator and a calibrated dial. All geometry follows the value.
        const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
        const auto centre = bounds.getCentre();
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.38f;
        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        const auto polar = [centre](float a, float r)
        {
            return centre + juce::Point<float>(std::sin(a) * r, -std::cos(a) * r);
        };
        const auto disk = [centre](float r)
        {
            return juce::Rectangle<float>(centre.x-r, centre.y-r, 2.0f*r, 2.0f*r);
        };

        // Recessed dial and fine engraved scale marks.
        g.setColour(juce::Colour(0xff153a32).withAlpha(0.48f));
        g.fillEllipse(disk(radius * 1.20f));
        for (int i = 0; i <= 24; ++i)
        {
            const float a = startAngle + (endAngle-startAngle) * ((float)i/24.0f);
            const bool major = (i % 4) == 0;
            const auto p0 = polar(a, radius * (major ? 1.07f : 1.10f));
            const auto p1 = polar(a, radius * 1.17f);
            g.setColour(juce::Colour(0xff365f52).withAlpha(major ? 0.95f : 0.48f));
            g.drawLine(p0.x,p0.y,p1.x,p1.y,major ? 1.6f : 0.9f);
        }

        // Soft contact shadow and metallic outer bezel.
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.fillEllipse(disk(radius).translated(1.5f,2.5f));
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffbed2bc),polar(-0.65f,radius),
                                                juce::Colour(0xff4b6c5b),polar(2.3f,radius),false));
        g.fillEllipse(disk(radius));
        g.setColour(juce::Colour(0xff213f35));
        g.fillEllipse(disk(radius * 0.91f));

        // Radial ridges on the edge simulate a grippy, knurled metal skirt.
        for (int i=0;i<64;++i)
        {
            const float a=juce::MathConstants<float>::twoPi * ((float)i/64.0f);
            const auto p0=polar(a,radius*0.92f),p1=polar(a,radius*0.985f);
            g.setColour(i%2==0 ? juce::Colour(0xff91aa96) : juce::Colour(0xff476b59));
            g.drawLine(p0.x,p0.y,p1.x,p1.y,0.9f);
        }

        // Matte forest-green cap with a restrained directional highlight.
        const float capRadius=radius*0.82f;
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff426657),
                                                polar(-0.8f,capRadius),
                                                juce::Colour(0xff1d342c),
                                                polar(2.4f,capRadius),false));
        g.fillEllipse(disk(capRadius));
        g.setColour(juce::Colour(0xffa4bbaa).withAlpha(0.26f));
        g.drawEllipse(disk(capRadius).reduced(0.7f),0.9f);

        // Amber inlaid pointer moves with the host-automatable parameter.
        const auto from=polar(angle,capRadius*0.53f);
        const auto to=polar(angle,capRadius*0.87f);
        g.setColour(juce::Colour(0xff100f0b).withAlpha(0.65f));
        g.drawLine(from.x+0.6f,from.y+0.8f,to.x+0.6f,to.y+0.8f,
                   juce::jmax(2.6f,radius*0.085f));
        g.setColour(juce::Colour(0xffd73832));
        g.drawLine(from.x,from.y,to.x,to.y,juce::jmax(2.2f,radius*0.07f));

    }
};

}

DrizelAudioProcessorEditor::DrizelAudioProcessorEditor(DrizelAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p),
      waveformPositionAttachment(*p.parameters.getParameter("position"), nullptr)
{
    editorLook = std::make_unique<DrizelGlassLookAndFeel>();
    setLookAndFeel(editorLook.get());
    setOpaque(false);
    formats.registerBasicFormats();
    for (auto* button : { &loadButton, &saveButton, &presetButton })
    {
        addAndMakeVisible(*button);
        button->setLookAndFeel(editorLook.get());
        button->setColour(juce::TextButton::textColourOffId,juce::Colour(mint));
    }
    for (auto* button : {&captureButton, &useCaptureButton})
    {
        addAndMakeVisible(*button);
        button->setLookAndFeel(editorLook.get());
        button->setColour(juce::TextButton::textColourOffId, juce::Colour(mint));
    }
    useCaptureButton.setEnabled(false);
    captureButton.onClick = [this]
    {
        if (!captureRecording)
        {
            renderedOutput = juce::File{};
            processor.beginDawCapture();
            captureRecording = true;
            captureButton.setButtonText("Stop capture");
            useCaptureButton.setEnabled(false);
        }
        else
        {
            processor.stopDawCapture();
            captureRecording = false;
            captureButton.setButtonText("Capture");
        }
    };
    useCaptureButton.onClick = [this]
    {
        if (!processor.dawCaptureReady()) return;
        auto destination = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getNonexistentChildFile("drizel-daw-capture", ".wav", false);
        if (processor.finishDawCapture(destination))
        {
            selectSampleMode();
            useCaptureButton.setEnabled(false);
        }
        else
            status.setText("DAW capture export failed", juce::dontSendNotification);
    };
    for (auto* button : {&renderButton, &dragOutputButton})
    {
        addAndMakeVisible(*button);
        button->setLookAndFeel(editorLook.get());
        button->setColour(juce::TextButton::textColourOffId, juce::Colour(mint));
    }
    renderButton.onClick = [this]
    {
        if (recordingOutput) processor.stopDawCapture();
        else if (!processor.captureActive())
        {
            renderedOutput = juce::File{};
            processor.beginDawCapture(true);
            recordingOutput = true;
            renderButton.setButtonText("Stop output");
        }
    };
    dragOutputButton.onClick = [this]
    {
        if (renderedOutput.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles(
                juce::StringArray{renderedOutput.getFullPathName()}, false, &dragOutputButton);
    };
    dragOutputButton.setEnabled(false);
    addAndMakeVisible(playButton);
    playButton.setClickingTogglesState(true);




    addAndMakeVisible(status);
    addAndMakeVisible(selectionLabel);

    addAndMakeVisible(keyboard);
    keyboard.setAvailableRange(48, 71); // Exactly two octaves, C3 through B4.
    keyboard.setKeyWidth(77.0f);
    // Match the plugin's mint / forest hardware palette rather than piano black/white.
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffcce9dc));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff214c40));
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff6c9e8b));
    keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, juce::Colour(0xff174a3d));
    keyboard.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colour(0xff0e3b31));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(mint));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(cyan));
    keyboard.setVisible(true);
    midiHelp.setText("MIDI CH 1  •  BEND ±2  •  CC1 LFO  •  CC74 POSITION  •  CC71 DENSITY",
                     juce::dontSendNotification);
    saveButton.onClick = [this] { choosePreset(true); };
    presetButton.onClick = [this] { choosePreset(false); };
    const char* ids[] = {"position","pitch","density","gain","lfoRate","lfoDepth","attack","release","rootNote"};
    const char* names[] = {"POSITION","PITCH","DENSITY","OUTPUT","LFO RATE","LFO DEPTH","ATTACK","RELEASE","ROOT NOTE","LFO SHAPE","LFO TARGET","PLAY MODE"};
    for (std::size_t i=0; i<labels.size(); ++i)
    {
        labels[i].setText(names[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        labels[i].setColour(juce::Label::textColourId, juce::Colour(forest));
        addAndMakeVisible(labels[i]);
    }
    for (std::size_t i=0; i<sliders.size(); ++i)
    {
        auto& s=sliders[i];
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setMouseDragSensitivity(90); // 90 pixels for the full parameter range.
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 66, 18);
        s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(mint));
        s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(forest));
        s.setColour(juce::Slider::thumbColourId, juce::Colour(cyan));
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(mint));
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(ink));
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setLookAndFeel(editorLook.get());
        addAndMakeVisible(s);
        attachments[i]=std::make_unique<SliderAttachment>(processor.parameters,ids[i],s);
    }

    // Hardware-style digital selection screens: click to choose an exact value.
    const auto configureChoice=[this](juce::ComboBox& box, const juce::StringArray& items)
    {
        for(int i=0;i<items.size();++i) box.addItem(items[i],i+1);
        box.setColour(juce::ComboBox::backgroundColourId,juce::Colour(ink));
        box.setColour(juce::ComboBox::textColourId,juce::Colour(mint));
        box.setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff729e89));
        box.setColour(juce::ComboBox::arrowColourId,juce::Colour(0xffd73832));
        box.setJustificationType(juce::Justification::centred);
        box.setLookAndFeel(editorLook.get());
        addAndMakeVisible(box);
    };
    configureChoice(modeChoice,{"Drone","MIDI"});
    configureChoice(audioInputChoice,{"Sample","Audio monitor","Audio capture","Live FX"});
    configureChoice(shapeChoice,{"Sine","Triangle"});
    configureChoice(destinationChoice,{"Off","Position","Pitch"});
    configureChoice(selectionChoice,{"Manual","Attack","Sustain","Avoid silence","Bright"});
    modeAttachment=std::make_unique<ChoiceAttachment>(processor.parameters,"midiMode",modeChoice);
    audioInputAttachment=std::make_unique<ChoiceAttachment>(processor.parameters,"audioInputMode",audioInputChoice);
    shapeAttachment=std::make_unique<ChoiceAttachment>(processor.parameters,"lfoShape",shapeChoice);
    destinationAttachment=std::make_unique<ChoiceAttachment>(processor.parameters,"lfoDestination",destinationChoice);
    selectionAttachment=std::make_unique<ChoiceAttachment>(processor.parameters,"grainSelection",selectionChoice);
    playAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"play",playButton);
    selectionLabel.setText("GRAIN SELECTION",juce::dontSendNotification);
    selectionLabel.setColour(juce::Label::textColourId,juce::Colour(pearl));
    loadButton.onClick=[this]
    {
        chooser=std::make_unique<juce::FileChooser>("Load a WAV",juce::File{},"*.wav");
        auto safe=juce::Component::SafePointer<DrizelAudioProcessorEditor>(this);
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe](const juce::FileChooser& dialog)
            {
                if(safe != nullptr && dialog.getResult().existsAsFile())
                {
                    safe->selectSampleMode();
                    safe->processor.requestSample(dialog.getResult());
                }
            });
    };
    // Keyboard note shortcuts are observed even while a knob or button has focus.
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    for (int i = 0; i < getNumChildComponents(); ++i)
        getChildComponent(i)->addKeyListener(this);
    midiHelp.setText("KEYS: A W S E D F T G Y H U J K  |  MIDI CH 1  |  CLICK WAVEFORM TO SET POSITION",
                     juce::dontSendNotification);

    movementCommand.setMultiLine(true);
    movementCommand.setFont(uiTypeface(22.0f));
    movementCommand.setIndents(10, 8);
    movementCommand.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff153e2b));
    movementCommand.setColour(juce::TextEditor::textColourId, juce::Colour(pearl));
    movementCommand.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff78977b));
    movementCommand.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(mint));
    movementCommand.setReturnKeyStartsNewLine(true);
    movementCommand.setTextToShowWhenEmpty("Type a grain modulation command", juce::Colour(pearl).withAlpha(0.72f));
    movementCommand.setTitle("Movement command");
    movementCommand.onTextChange = [this] {
#if defined(DRIZEL_MAC_SPEECH) && DRIZEL_MAC_SPEECH
        movementSpeech.cancel();
#endif
        applyButton.setEnabled(false);
        movementStatus.setText("Preview to review your changes", juce::dontSendNotification);
    };
    previewButton.onClick = [this] { previewMovement(); };
    applyButton.onClick = [this] { applyMovement(); };
    manualButton.onClick = [this] {
#if defined(DRIZEL_MAC_SPEECH) && DRIZEL_MAC_SPEECH
        movementSpeech.cancel();
#endif
        processor.setMovement({});
        movementStatus.setText("Manual scan active", juce::dontSendNotification);
        applyButton.setEnabled(false);
    };
    applyButton.setEnabled(false);
    for (auto* button : {&previewButton, &applyButton, &manualButton})
    {
        button->setLookAndFeel(editorLook.get());
        button->setColour(juce::TextButton::textColourOffId, juce::Colour(pearl));
    }
    const char* settings[] = {"Region start %", "Region end %", "Interval / ms",
                              "Step %", "Distance variation %", "Timing variation %"};
    const int minimum[] = {0, 1, 50, 1, 0, 0};
    const int maximum[] = {99, 100, 10000, 50, 90, 90};
    for (std::size_t i = 0; i < movementSettings.size(); ++i)
    {
        auto& slider = movementSettings[i];
        movementLabels[i].setText(settings[i], juce::dontSendNotification);
        movementLabels[i].setJustificationType(juce::Justification::centred);
        slider.setTitle(settings[i]);
        slider.setRange(minimum[i], maximum[i], i == 2 ? 50 : 1);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 20);
        slider.setLookAndFeel(editorLook.get());
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(mint));
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(ink));
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        movementLabels[i].setColour(juce::Label::textColourId, juce::Colour(forest));
        addAndMakeVisible(movementLabels[i]);
        addAndMakeVisible(slider);
    }
    for (juce::Component* component : std::array<juce::Component*, 5>{
             &movementCommand, &previewButton, &applyButton, &manualButton, &movementStatus})
        addAndMakeVisible(component);
    movementStatus.setText(processor.activeMovement().pattern == drizel::MovementPattern::manual
                           ? "Manual scan active" : "Movement policy active", juce::dontSendNotification);
#if defined(DRIZEL_MAC_SPEECH) && DRIZEL_MAC_SPEECH
    if (juce::JUCEApplicationBase::isStandaloneApp())
    {
        addAndMakeVisible(speechButton);
        speechButton.setLookAndFeel(editorLook.get());
        speechButton.setColour(juce::TextButton::textColourOffId, juce::Colour(pearl));
        speechButton.onStateChange = [this] {
            const bool down = speechButton.isDown();
            if (down == speechHeld)
                return;
            speechHeld = down;
            if (!down)
            {
                movementSpeech.finish();
                return;
            }
            movementStatus.setText("Hold to speak, then release", juce::dontSendNotification);
            applyButton.setEnabled(false);
            const auto revision = processor.sampleRequestRevision();
            movementSpeech.start([this, revision](juce::String transcript, juce::String message) {
                if (revision != processor.sampleRequestRevision())
                {
                    movementStatus.setText("Sample changed. Speak or type a new command.",
                                           juce::dontSendNotification);
                    return;
                }
                if (transcript.isNotEmpty())
                    movementCommand.setText(transcript, false);
                applyButton.setEnabled(false);
                movementStatus.setText(message, juce::dontSendNotification);
            });
        };
    }
#endif
    keyboardState.addListener(this);
    keyboard.setMidiChannelsToDisplay(3);
    keyboard.setScrollButtonsVisible(false);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xfffffcf2));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff153e2b));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0xffa4d38a));
    keyboard.setTitle("MIDI keyboard");
    keyboard.setWantsKeyboardFocus(false);
    octaveDown.onClick = [this] { setKeyboardOctave(keyboardOctave - 1); };
    octaveUp.onClick = [this] { setKeyboardOctave(keyboardOctave + 1); };
    octaveDown.setTitle("Octave down");
    octaveUp.setTitle("Octave up");
    movementTitle.setText("GRAIN MOVEMENT", juce::dontSendNotification);
    movementTitle.setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::bold)));
    movementTitle.setColour(juce::Label::textColourId, juce::Colour(forest));
    movementStatus.setColour(juce::Label::textColourId, juce::Colour(forest));
    keyboardRange.setColour(juce::Label::textColourId, juce::Colour(forest));
    midiHelp.setColour(juce::Label::textColourId, juce::Colour(forest));
    for (juce::Component* component : std::array<juce::Component*, 5>{
             &keyboard, &keyboardRange, &octaveDown, &octaveUp, &movementTitle})
        addAndMakeVisible(component);
    setKeyboardOctave(3);
    addAndMakeVisible(keyboardToggle);
    keyboardToggle.setLookAndFeel(editorLook.get());
    keyboardToggle.setColour(juce::TextButton::textColourOffId, juce::Colour(pearl));
    keyboardToggle.onClick = [this] { setKeyboardExpanded(!keyboardExpanded); };
    // Keep the collapsed choice across editor reopenings in this plugin instance.
    keyboardExpanded = processor.keyboardVisible();

    applyFont();
    setKeyboardExpanded(keyboardExpanded);
    startTimerHz(30);
    timerCallback();
}
DrizelAudioProcessorEditor::~DrizelAudioProcessorEditor()
{
#if defined(DRIZEL_MAC_SPEECH) && DRIZEL_MAC_SPEECH
    movementSpeech.cancel();
#endif
    keyboardState.allNotesOff(1);
    keyboardState.removeListener(this);
    endWaveformGesture();
    for (auto& slider : sliders) slider.setLookAndFeel(nullptr);
    for (auto& slider : movementSettings) slider.setLookAndFeel(nullptr);
    for (auto* button : {&loadButton, &saveButton, &presetButton, &captureButton, &useCaptureButton, &renderButton, &dragOutputButton, &previewButton, &applyButton, &manualButton}) button->setLookAndFeel(nullptr);
#if defined(DRIZEL_MAC_SPEECH) && DRIZEL_MAC_SPEECH
    speechButton.setLookAndFeel(nullptr);
#endif
    for (auto* box : {&modeChoice, &audioInputChoice, &shapeChoice, &destinationChoice, &selectionChoice}) box->setLookAndFeel(nullptr);
    keyboardToggle.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
    for (int i = 0; i < 13; ++i) if (computerNotesDown[static_cast<std::size_t>(i)]) processor.keyboardState.noteOff(1, 60+i, 0.0f);
}

void DrizelAudioProcessorEditor::handleNoteOn(juce::MidiKeyboardState*, int channel, int note, float velocity)
{
    if (channel == 1)
        processor.setPanelNote(note, juce::jlimit(1, 127, juce::roundToInt(velocity * 127)));
}

void DrizelAudioProcessorEditor::handleNoteOff(juce::MidiKeyboardState*, int channel, int note, float)
{
    if (channel == 1)
        processor.setPanelNote(note, 0);
}

void DrizelAudioProcessorEditor::setKeyboardOctave(int octave)
{
    keyboardState.allNotesOff(1);
    keyboardOctave = juce::jlimit(0, 7, octave);
    const int first = 12 * (keyboardOctave + 1);
    keyboard.setAvailableRange(first, first + 23);
    keyboard.setLowestVisibleKey(first);
    keyboardRange.setText("MIDI keyboard | C" + juce::String(keyboardOctave)
                          + "–B" + juce::String(keyboardOctave + 1), juce::dontSendNotification);
    octaveDown.setEnabled(keyboardOctave > 0);
    octaveUp.setEnabled(keyboardOctave < 7);
}

void DrizelAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (!event.mods.isLeftButtonDown() || waveformGestureActive
        || !waveformBounds.toFloat().contains(event.position) || thumbnail.getTotalLength() <= 0.0)
        return;

    waveformGestureActive = true;
    waveformPositionAttachment.beginGesture();
    setWaveformPosition(event.position.x);
}

void DrizelAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (waveformGestureActive)
        setWaveformPosition(event.position.x);
}

void DrizelAudioProcessorEditor::mouseUp(const juce::MouseEvent&)
{
    endWaveformGesture();
}

void DrizelAudioProcessorEditor::endWaveformGesture()
{
    if (waveformGestureActive)
    {
        waveformGestureActive = false;
        waveformPositionAttachment.endGesture();
    }
}

void DrizelAudioProcessorEditor::setWaveformPosition(float x)
{
    const auto audioBounds = waveformBounds.reduced(4);
    const auto width = static_cast<float>(juce::jmax(1, audioBounds.getWidth() - 1));
    const float position = juce::jlimit(0.0f, 1.0f, (x - audioBounds.getX()) / width);
    waveformPositionAttachment.setValueAsPartOfGesture(position);
    repaint(waveformBounds);
}

bool DrizelAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    return files.size() == 1 && juce::File(files[0]).hasFileExtension("wav");
}
void DrizelAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    if (isInterestedInFileDrag(files) && juce::File(files[0]).existsAsFile())
    {
        selectSampleMode();
        processor.requestSample(juce::File(files[0]));
    }
}
void DrizelAudioProcessorEditor::selectSampleMode()
{
#if !JucePlugin_IsSynth
    audioInputChoice.setSelectedId(1, juce::sendNotificationSync);
    modeChoice.setSelectedId(1, juce::sendNotificationSync);
#endif
}
void DrizelAudioProcessorEditor::timerCallback()
{
    if (recordingOutput && !processor.captureActive())
    {
        recordingOutput = false;
        renderButton.setButtonText("Record output");
        auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getNonexistentChildFile("Drizel-Output", ".wav", false);
        if (processor.exportCapture(file, false)) renderedOutput = file;
    }
    if (captureRecording && !processor.captureActive())
    {
        captureRecording = false;
        captureButton.setButtonText("Capture");
    }
    useCaptureButton.setEnabled(!recordingOutput && !processor.captureActive() && processor.dawCaptureReady()
                                && processor.capturedFrames() > 0 && !renderedOutput.existsAsFile());
    captureButton.setEnabled(!recordingOutput && (captureRecording || !processor.captureActive()));
    renderButton.setEnabled(!captureRecording && (recordingOutput || !processor.captureActive()));
    dragOutputButton.setEnabled(renderedOutput.existsAsFile());
    const bool live = !JucePlugin_IsSynth && audioInputChoice.getSelectedId() == 4;
    status.setText((recordingOutput ? juce::String("Recording processed output") :
                    captureRecording ? juce::String("Recording input") :
                    live ? juce::String("Live DAW audio") :
                    JucePlugin_IsSynth && processor.loadedSampleState().path.isEmpty() ? juce::String("MIDI oscillator ready") :
                    processor.loadStatus()) + presetStatus, juce::dontSendNotification);
    if (applyButton.isEnabled() && previewRevision != processor.sampleRequestRevision())
    {
        applyButton.setEnabled(false);
        movementStatus.setText("Sample or preset changed. Preview again.", juce::dontSendNotification);
    }
    const auto sample = processor.loadedSampleState();
    if (sample.revision != waveformRevision)
    {
        endWaveformGesture();
        waveformRevision = sample.revision;
        thumbnail.clear();
        if (sample.path.isNotEmpty())
        {
            cache.clear(); // Same path may now contain different audio.
            thumbnail.setSource(new juce::FileInputSource(juce::File(sample.path)));
        }
    }
    displayedPeak = std::max(processor.outputPeak(), displayedPeak * 0.9f);

    const auto hostNote = processor.displayedHostNote();
    if (hostNote != highlightedNote)
    {
        if (highlightedNote >= 0) keyboardState.noteOff(2, highlightedNote, 0.0f);
        if (hostNote >= 0) keyboardState.noteOn(2, hostNote, 1.0f);
        // Drain the display-only channel's queued events without sending them to the processor.
        juce::MidiBuffer displayEvents;
        keyboardState.processNextMidiBuffer(displayEvents, 0, 1, true);
        highlightedNote = hostNote;
    }
    else
    {
        juce::MidiBuffer panelEvents;
        keyboardState.processNextMidiBuffer(panelEvents, 0, 1, true);
    }
    repaint();
}
static std::array<juce::Rectangle<int>,4> controlPanels(juce::Rectangle<int> deck)
{
    const int gap = 14;
    const int third = (deck.getWidth() - 2 * gap) / 3;
    const int x = deck.getX();
    const int y = deck.getY();
    const int height = deck.getHeight();
    const int engineWidth = 300;
    return {{{deck.getRight() - engineWidth, 105, engineWidth, height},
             {x, y, third, height},
             {x + third + gap, y, third, height},
             {x + 2 * (third + gap), y, deck.getWidth() - 2 * (third + gap), height}}};
}
juce::Font DrizelAudioProcessorEditor::uiTypeface(float height, int style) const
{
    auto font = juce::Font(juce::FontOptions(height, style));
    if (uiFont.isNotEmpty()) font.setTypefaceName(uiFont);
    return font;
}
void DrizelAudioProcessorEditor::applyFont()
{
    static_cast<DrizelGlassLookAndFeel*>(editorLook.get())->family = uiFont;
    movementCommand.applyFontToAllText(uiTypeface(22.0f));
    movementCommand.setFont(uiTypeface(22.0f));
    for (auto& label : labels) label.setFont(uiTypeface(14.0f));
    for (auto& label : movementLabels) label.setFont(uiTypeface(14.0f));
    for (auto* label : {&status, &selectionLabel, &movementStatus, &keyboardRange, &midiHelp}) label->setFont(uiTypeface(14.0f));
    movementTitle.setFont(uiTypeface(17.0f, juce::Font::bold));
    sendLookAndFeelChange();
    repaint();
}
void DrizelAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto teal = juce::Colour(cyan);
    const auto ivory = juce::Colour(pearl);
    const auto amber = juce::Colour(mint);
    g.setColour(juce::Colour(emerald));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 22.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(ink).withAlpha(0.88f),waveformBounds.getTopLeft().toFloat(),
                                           juce::Colour(forest).withAlpha(0.8f),waveformBounds.getBottomRight().toFloat(),false));
    g.fillRoundedRectangle(waveformBounds.expanded(12,27).toFloat(),18.0f);
    g.setColour(juce::Colour(pearl).withAlpha(0.4f));
    g.drawRoundedRectangle(waveformBounds.expanded(12,27).toFloat(),18.0f,1.2f);
    g.setColour(ivory);
    g.setFont(uiTypeface(38.0f, juce::Font::bold));
#if defined(DRIZEL_HAS_DRIZEL_LOGO)
    {
        static const juce::Image drizelLogo = juce::ImageFileFormat::loadFrom(
            BinaryData::drizel_logo_png, BinaryData::drizel_logo_pngSize);
        if (drizelLogo.isValid())
            g.drawImageWithin(drizelLogo, 30, 18, 44, 44,
                              juce::RectanglePlacement::centred);
    }
#endif
    g.drawText("drizel", juce::Rectangle<int>(82, 15, 175, 48),
               juce::Justification::centredLeft);

    // Matching dark analog readouts for the loaded input and active grain count.
    const auto drawReadout=[this,&g](juce::Rectangle<int> area,const juce::String& value)
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff25483a),
                                               area.getTopLeft().toFloat(),
                                               juce::Colour(0xff102b25),
                                               area.getBottomRight().toFloat(),false));
        g.fillRoundedRectangle(area.toFloat(),6.0f);
        g.setColour(juce::Colour(0xff86a891));
        g.drawRoundedRectangle(area.toFloat().reduced(0.6f),6.0f,1.1f);
        g.setColour(juce::Colour(mint));
        g.setFont(uiTypeface(14.0f));
        g.drawFittedText(value,area.reduced(10,2),juce::Justification::centredLeft,1);
    };
    const bool live = !JucePlugin_IsSynth && audioInputChoice.getSelectedId() == 4;
    const auto sampleName=processor.loadedSampleState().path.fromLastOccurrenceOf("/",false,false);
    drawReadout({waveformBounds.getX(),waveformBounds.getY()-32,310,27},
                "INPUT  /  " + (live ? juce::String("DAW audio") : sampleName.isEmpty() ? juce::String(JucePlugin_IsSynth ? "Oscillator" : "No WAV") : sampleName));
    drawReadout({waveformBounds.getRight()-190,waveformBounds.getY()-32,190,27},
                "ACTIVE GRAINS  " + juce::String(processor.activeGrainCount()));
    g.setColour(teal);
    if(!live && thumbnail.getTotalLength()>0.0)
    {
        thumbnail.drawChannels(g,waveformBounds,0.0,thumbnail.getTotalLength(),1.0f);
        const float pos=processor.parameters.getRawParameterValue("position")->load();
        const int x=waveformBounds.getX()+juce::roundToInt(pos*(waveformBounds.getWidth()-1));
        g.setColour(teal.withAlpha(0.22f));
        g.fillRoundedRectangle(juce::Rectangle<float>(static_cast<float>(x-32),static_cast<float>(waveformBounds.getY()),64.0f,static_cast<float>(waveformBounds.getHeight())),5.0f);
        g.setColour(amber);
        g.drawVerticalLine(x,static_cast<float>(waveformBounds.getY()),static_cast<float>(waveformBounds.getBottom()));
    }
    else if (!live && !JucePlugin_IsSynth)
        g.drawText("Drop a WAV here or choose Load WAV",waveformBounds,juce::Justification::centred);
    if (live || JucePlugin_IsSynth)
    {
        for (int i = 0; i < 128; ++i)
        {
            const float x = waveformBounds.getX() + i * waveformBounds.getWidth() / 128.0f;
            for (int out = 0; out < 2; ++out)
            {
                const float y = waveformBounds.getY() + waveformBounds.getHeight() * (out ? 0.75f : 0.25f);
                const float h = juce::jlimit(0.0f, 1.0f, processor.waveformPeak(out != 0, i)) * waveformBounds.getHeight() * 0.22f;
                g.setColour(out ? ivory : amber);
                g.drawLine(x, y-h, x, y+h, 2.0f);
            }
        }
    }
    const auto paintPanel = [&g](juce::Rectangle<int> bounds)
    {
        g.setColour(juce::Colour(0xfff6edd9));
        g.fillRoundedRectangle(bounds.toFloat(), 17.0f);
    };
    paintPanel(commandBounds);
    paintPanel(movementBounds);
    paintPanel(keyboardPanelBounds);
    const auto panels = controlPanels(deckBounds);
    for (const auto& panel : panels)
        paintPanel(panel);

    g.setColour(juce::Colour(forest));
    g.setFont(uiTypeface(18.0f));
    g.drawText("SPEECH / TEXT", commandBounds.getX() + 16, commandBounds.getY() + 12,
               commandBounds.getWidth() - 32, 24, juce::Justification::centredLeft);
    g.setFont(uiTypeface(14.0f));
    g.drawText("Grain modulation command", commandBounds.getX() + 16,
               movementCommand.getY() - 24, commandBounds.getWidth() - 32, 20,
               juce::Justification::centredLeft);
    g.setColour(juce::Colour(forest));
    g.setFont(uiTypeface(14.0f, juce::Font::bold));
    const std::array<const char*,4> titles {{"GRAIN ENGINE","PITCH / VOICE","ENVELOPE","MODULATION"}};
    for(std::size_t i=0;i<panels.size();++i)
        g.drawText(titles[i],panels[i].getX()+14,panels[i].getY()+14,
                   panels[i].getWidth()-28,25,juce::Justification::centredLeft);
    g.setColour(juce::Colour(forest));
    g.setFont(uiTypeface(11.0f));
    for(const auto& pair : std::array<std::pair<const char*,juce::Rectangle<int>>,3> {{
        {"MODE",modeChoice.getBounds()},{"SHAPE",shapeChoice.getBounds()},
        {"TARGET",destinationChoice.getBounds()}
    }})
        g.drawText(pair.first,pair.second.getX(),pair.second.getY()-18,
                   pair.second.getWidth(),18,juce::Justification::centredLeft);
    g.setColour(juce::Colour(forest));
    g.drawText("OUTPUT LEVEL",meterBounds.getX()-120,meterBounds.getY()-8,112,22,juce::Justification::centredRight);
    g.setColour(juce::Colour(ink).withAlpha(0.7f));
    g.fillRoundedRectangle(meterBounds.toFloat(),4.0f);
    g.setColour(displayedPeak >= 0.99f ? teal : juce::Colour(0xff557b64));
    g.fillRoundedRectangle(meterBounds.withWidth(juce::roundToInt(meterBounds.getWidth()*juce::jlimit(0.0f,1.0f,displayedPeak))).toFloat(),4.0f);
}
void DrizelAudioProcessorEditor::setKeyboardExpanded(bool expanded)
{
    keyboardExpanded = expanded;
    processor.setKeyboardVisible(expanded);
    keyboardToggle.setButtonText(expanded ? "Hide MIDI keyboard" : "Show MIDI keyboard");
    for (juce::Component* component : std::array<juce::Component*, 5>{
             &keyboard, &keyboardRange, &octaveDown, &octaveUp, &midiHelp})
        component->setVisible(expanded);
    setSize(compactEditorWidth, expanded ? expandedEditorHeight : collapsedEditorHeight);
    repaint();
}

void DrizelAudioProcessorEditor::resized()
{
    const int w = getWidth();
    headerBounds = {20, 12, w - 40, 75};
    loadButton.setBounds(w - 442, 24, 92, 32);
    presetButton.setBounds(w - 342, 24, 104, 32);
    saveButton.setBounds(w - 230, 24, 104, 32);
    playButton.setBounds(w - 113, 24, 82, 32);
    status.setBounds(35, 74, w - 390, 20);
    captureButton.setBounds(35, 250, 110, 25);
    useCaptureButton.setBounds(151, 250, 125, 25);
    renderButton.setBounds(282, 250, 150, 25);
    dragOutputButton.setBounds(438, 250, 115, 25);
#if JucePlugin_IsSynth
    captureButton.setVisible(false);
    useCaptureButton.setVisible(false);
    audioInputChoice.setVisible(false);
    modeChoice.setEnabled(false);
#endif
    audioInputChoice.setBounds(w - 310, 264, 280, 28);
    waveformBounds = {35, 132, w - 389, 112};

    selectionLabel.setBounds(35, 284, 140, 24);
    selectionChoice.setBounds(181, 281, 190, 30);

    deckBounds = {20, 318, w - 40, 193};
    const auto panels = controlPanels(deckBounds);
    const auto place = [&](int index, int panel, int column, int count)
    {
        const auto p = panels[(std::size_t) panel];
        const int innerLeft = p.getX() + 12;
        const int innerWidth = p.getWidth() - 24;
        const int cx = innerLeft + innerWidth * (2 * column + 1) / (2 * count);
        constexpr int knobWidth = 76;
        sliders[(std::size_t) index].setBounds(cx - knobWidth / 2,
                                               p.getY() + 37, knobWidth, 84);
        labels[(std::size_t) index].setBounds(cx - 43, p.getY() + 124, 86, 18);
    };
    place(0, 0, 0, 3);
    place(2, 0, 1, 3);
    place(3, 0, 2, 3);
    place(1, 1, 0, 2);
    place(8, 1, 1, 2);
    place(6, 2, 0, 2);
    place(7, 2, 1, 2);
    place(4, 3, 0, 2);
    place(5, 3, 1, 2);

    const auto choiceBounds = [&](int panel, int column, int count)
    {
        const auto p = panels[(std::size_t) panel];
        const int margin = 12, gap = 8;
        const int width = (p.getWidth() - 2 * margin - (count - 1) * gap) / count;
        return juce::Rectangle<int>(p.getX() + margin + column * (width + gap),
                                     p.getY() + 162, width, 27);
    };
    modeChoice.setBounds(choiceBounds(1, 0, 1));
    shapeChoice.setBounds(choiceBounds(3, 0, 2));
    destinationChoice.setBounds(choiceBounds(3, 1, 2));

    const int commandWidth = (w - 54) * 48 / 100;
    commandBounds = {20, 525, commandWidth, 300};
    movementBounds = {commandBounds.getRight() + 14, 525, w - commandWidth - 54, 300};
    const int commandX = commandBounds.getX() + 16;
    const int commandInnerWidth = commandBounds.getWidth() - 32;
    movementCommand.setBounds(commandX, 599, commandInnerWidth, 105);
    auto actions = juce::Rectangle<int>(commandX, 716, commandInnerWidth, 32);
#if defined(DRIZEL_MAC_SPEECH) && DRIZEL_MAC_SPEECH
    if (speechButton.isVisible())
        speechButton.setBounds(actions.removeFromLeft(125).reduced(2, 0));
#endif
    previewButton.setBounds(actions.removeFromLeft(110).reduced(2, 0));
    applyButton.setBounds(actions.removeFromLeft(125).reduced(2, 0));
    movementStatus.setBounds(commandX, 760, commandInnerWidth, 40);

    auto movementHeader = movementBounds.reduced(16, 10).removeFromTop(30);
    manualButton.setBounds(movementHeader.removeFromRight(120));
    movementTitle.setBounds(movementHeader);
    const int cellWidth = (movementBounds.getWidth() - 32) / 3;
    for (std::size_t i = 0; i < movementSettings.size(); ++i)
    {
        const int cx = movementBounds.getX() + 16 + cellWidth * (2 * int(i % 3) + 1) / 2;
        const int y = movementBounds.getY() + 43 + 119 * int(i / 3);
        movementSettings[i].setBounds(cx - 39, y, 78, 98);
        movementLabels[i].setBounds(cx - cellWidth / 2, y + 98, cellWidth, 18);
    }

    keyboardPanelBounds = {20, 839, w - 40, keyboardExpanded ? 143 : 28};
    keyboardHeaderBounds = keyboardPanelBounds.withHeight(32);
    keyboardToggle.setBounds(35, 842, 184, 22);
    meterBounds = {w - 207, 850, 164, 8};
    keyboardRange.setBounds(230, 847, 195, 22);
    octaveDown.setBounds(430, 847, 28, 22);
    octaveUp.setBounds(462, 847, 28, 22);
    midiHelp.setBounds(500, 847, w - 735, 22);
    keyboard.setBounds(36, 877, w - 72, 89);
    keyboard.setKeyWidth((w - 72) / 14.0f);
}
void DrizelAudioProcessorEditor::choosePreset(bool save)
{
    chooser = std::make_unique<juce::FileChooser>(save ? "Save Drizel preset" : "Load Drizel preset",
        save ? juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Preset.drizel")
             : juce::File{}, "*.drizel");
    auto safe = juce::Component::SafePointer<DrizelAudioProcessorEditor>(this);
    const int flags = juce::FileBrowserComponent::canSelectFiles | (save
        ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting
        : juce::FileBrowserComponent::openMode);
    chooser->launchAsync(flags, [safe, save](const juce::FileChooser& dialog)
    {
        if (safe == nullptr || dialog.getResult() == juce::File{}) return;
        // Keep the exact chosen path so overwrite confirmation covers the written file.
        const auto file = dialog.getResult();
        const bool ok = save ? safe->processor.savePreset(file) : safe->processor.loadPreset(file);
        safe->presetStatus = ok ? (save ? " | Preset saved" : " | Preset recalled") : " | Preset failed";
    });
}

void DrizelAudioProcessorEditor::previewMovement()
{
#if defined(DRIZEL_MAC_SPEECH) && DRIZEL_MAC_SPEECH
    movementSpeech.cancel();
#endif
    const auto revision = processor.sampleRequestRevision();
    using Pattern = drizel::MovementPattern;
    const auto words = juce::StringArray::fromTokens(
        movementCommand.getText().toLowerCase().retainCharacters(
            "abcdefghijklmnopqrstuvwxyz \t\r\n,.!?-").replaceCharacters(",.!?-", "     "),
        " \t\r\n", "");
    const auto has = [&words](const char* word) { return words.contains(word); };
    const bool stepped = has("step") || has("steps") || has("stepped");
    const bool irregular = has("chaotic") || has("unpredictable") || has("random")
                           || has("irregular") || has("vary") || has("jumps");
    const bool cyclic = has("circle") || has("circles") || has("loop") || has("cyclic")
                        || has("cycle");
    const bool negated = has("not") || has("never") || has("no") || has("don")
                         || has("without") || has("avoid");
    const bool stepSequence = stepped && !irregular && has("forward") && has("back") && has("two");
    if (negated || (has("then") && !stepSequence) || (!stepped && !irregular && !cyclic)
        || (cyclic && (stepped || irregular)))
    {
        applyButton.setEnabled(false);
        movementStatus.setText("Try a stepped, chaotic, or looping scan", juce::dontSendNotification);
        return;
    }
    proposedMovement = processor.activeMovement();
    if (proposedMovement.pattern == Pattern::manual)
        proposedMovement = {};
    if (cyclic && proposedMovement.pattern != Pattern::cyclic)
        proposedMovement.interval = 40;
    if (!cyclic && proposedMovement.pattern == Pattern::cyclic)
        proposedMovement.interval = 5;
    proposedMovement.pattern = stepped ? Pattern::stepped : irregular ? Pattern::irregular : Pattern::cyclic;
    if (stepped && irregular)
    {
        const int priorDistance = proposedMovement.distanceVariation;
        const int priorTiming = proposedMovement.timingVariation;
        const bool relative = has("more") && processor.activeMovement().pattern != Pattern::manual;
        const int initialVariation = has("little") || has("slightly") || has("subtle") ? 15 : 30;
        proposedMovement.distanceVariation = relative ? std::min(90, priorDistance + 15) : initialVariation;
        proposedMovement.timingVariation = relative ? std::min(90, priorTiming + 15) : initialVariation;
    }
    else
    {
        proposedMovement.distanceVariation = 0;
        proposedMovement.timingVariation = irregular ? 45 : 0;
    }
    const int values[] = {proposedMovement.start, proposedMovement.end, proposedMovement.interval * 50,
                          proposedMovement.step, proposedMovement.distanceVariation, proposedMovement.timingVariation};
    for (std::size_t i = 0; i < movementSettings.size(); ++i)
        movementSettings[i].setValue(values[i], juce::dontSendNotification);
    movementSettings[3].setEnabled(!cyclic);
    movementSettings[4].setEnabled(stepped);
    movementSettings[5].setEnabled(!cyclic);
    movementStatus.setText(stepped ? (irregular ? "Preview: stepped with variation" : "Preview: stepped")
                                  : irregular ? "Preview: irregular" : "Preview: cyclic",
                           juce::dontSendNotification);
    previewRevision = revision;
    applyButton.setEnabled(true);
}

void DrizelAudioProcessorEditor::applyMovement()
{
    proposedMovement.start = int(movementSettings[0].getValue());
    proposedMovement.end = int(movementSettings[1].getValue());
    if (proposedMovement.start >= proposedMovement.end)
    {
        movementStatus.setText("Region start must be before its end", juce::dontSendNotification);
        return;
    }
    proposedMovement.interval = int(movementSettings[2].getValue()) / 50;
    proposedMovement.step = int(movementSettings[3].getValue());
    proposedMovement.distanceVariation = int(movementSettings[4].getValue());
    proposedMovement.timingVariation = int(movementSettings[5].getValue());
    if (!processor.applyMovementForSample(proposedMovement, previewRevision))
    {
        applyButton.setEnabled(false);
        movementStatus.setText("Sample or preset changed. Preview again.", juce::dontSendNotification);
        return;
    }
    applyButton.setEnabled(false);
    movementStatus.setText("Movement policy active", juce::dontSendNotification);
}

bool DrizelAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    // Piano mapping: A=C4, W=C#4, S=D4, E=D#4 ... K=C5.
    static constexpr char keys[]="awsedftgyhujk";
    if (movementCommand.hasKeyboardFocus(true)) return false;
    const auto character=juce::CharacterFunctions::toLowerCase(key.getTextCharacter());
    for(int i=0;i<13;++i)
        if(character==keys[i])
        {
            if(!computerNotesDown[static_cast<std::size_t>(i)])
            {
                computerNotesDown[static_cast<std::size_t>(i)]=true;
                // Pressing a computer key switches to instrument mode automatically.
                if(auto* modeParameter=processor.parameters.getParameter("midiMode"))
                    modeParameter->setValueNotifyingHost(1.0f);
                processor.keyboardState.noteOn(1,60+i,0.8f);
            }
            return true;
        }
    return false;
}
bool DrizelAudioProcessorEditor::keyStateChanged(bool,juce::Component*)
{
    static constexpr char keys[]="awsedftgyhujk";
    for(int i=0;i<13;++i)
        if(computerNotesDown[static_cast<std::size_t>(i)] &&
           !juce::KeyPress::isKeyCurrentlyDown(static_cast<int>(keys[i])))
        {
            computerNotesDown[static_cast<std::size_t>(i)]=false;
            processor.keyboardState.noteOff(1,60+i,0.8f);
        }
    return false;}
