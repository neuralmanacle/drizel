#include "PluginEditor.h"

DrizelAudioProcessorEditor::DrizelAudioProcessorEditor(DrizelAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p)
{
    formats.registerBasicFormats();
    for (auto* button : { &loadButton, &saveButton, &presetButton })
        addAndMakeVisible(*button);
    addAndMakeVisible(playButton);
    addAndMakeVisible(status);
    addAndMakeVisible(midiHelp);
    addAndMakeVisible(selectionLabel);
    addAndMakeVisible(selectionKnob);
    addAndMakeVisible(selectionDisplay);
    addAndMakeVisible(selectionTicks);
    addAndMakeVisible(keyboard);
    keyboard.setAvailableRange(36, 96);
    keyboard.setKeyWidth(30.0f);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffe8f4ef));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff0b2026));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0xff20d6c7));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0xff35bdb5));
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
        labels[i].setColour(juce::Label::textColourId, juce::Colour(0xff37666b));
        addAndMakeVisible(labels[i]);
    }
    for (std::size_t i=0; i<sliders.size(); ++i)
    {
        auto& s=sliders[i];
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 22);
        s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff20d6c7));
        s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff477d9b));
        s.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe5b96b));
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff0b2026));
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xffe8f4ef));
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(s);
        attachments[i]=std::make_unique<SliderAttachment>(processor.parameters,ids[i],s);
    }

    // Discrete, host-automatable knobs replace the three combo boxes.
    const auto configureChoice = [this](juce::Slider& knob, juce::Label& display, int maximum)
    {
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setRange(0.0, static_cast<double>(maximum), 1.0);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff20d6c7));
        knob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff477d9b));
        knob.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe5b96b));
        addAndMakeVisible(knob);
        display.setJustificationType(juce::Justification::centred);
        display.setColour(juce::Label::backgroundColourId, juce::Colour(0xff102a3a));
        display.setColour(juce::Label::textColourId, juce::Colour(0xff20d6c7));
        display.setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(display);
    };
    configureChoice(shapeKnob, shapeDisplay, 1);
    configureChoice(destinationKnob, destinationDisplay, 2);
    configureChoice(modeKnob, modeDisplay, 1);
    shapeAttachment=std::make_unique<SliderAttachment>(processor.parameters,"lfoShape",shapeKnob);
    destinationAttachment=std::make_unique<SliderAttachment>(processor.parameters,"lfoDestination",destinationKnob);
    modeAttachment=std::make_unique<SliderAttachment>(processor.parameters,"midiMode",modeKnob);
    selectionKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    selectionKnob.setRange(0.0, 4.0, 1.0);
    selectionKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    selectionKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff20d6c7));
    selectionKnob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff477d9b));
    selectionKnob.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe5b96b));
    selectionAttachment=std::make_unique<SliderAttachment>(processor.parameters,"grainSelection",selectionKnob);
    selectionDisplay.setJustificationType(juce::Justification::centred);
    selectionDisplay.setColour(juce::Label::backgroundColourId, juce::Colour(0xff102a3a));
    selectionDisplay.setColour(juce::Label::textColourId, juce::Colour(0xff20d6c7));
    selectionDisplay.setFont(juce::Font(17.0f, juce::Font::bold));
    selectionTicks.setText("MAN / ATK / SUS / SIL / BRT",juce::dontSendNotification);
    selectionTicks.setColour(juce::Label::textColourId,juce::Colour(0xffa4c2bf));
    selectionTicks.setJustificationType(juce::Justification::centred);
    playAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"play",playButton);
    selectionLabel.setText("GRAIN SELECTION",juce::dontSendNotification);
    selectionLabel.setColour(juce::Label::textColourId,juce::Colour(0xffe8f4ef));
    loadButton.onClick=[this]
    {
        chooser=std::make_unique<juce::FileChooser>("Load a WAV",juce::File{},"*.wav");
        auto safe=juce::Component::SafePointer<DrizelAudioProcessorEditor>(this);
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe](const juce::FileChooser& dialog)
            {
                if(safe != nullptr && dialog.getResult().existsAsFile())
                    safe->processor.requestSample(dialog.getResult());
            });
    };
    // Keyboard note shortcuts are observed even while a knob or button has focus.
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    for (int i = 0; i < getNumChildComponents(); ++i)
        getChildComponent(i)->addKeyListener(this);
    midiHelp.setText("KEYS: A W S E D F T G Y H U J K  |  MIDI CH 1  |  CLICK WAVEFORM TO SET POSITION",
                     juce::dontSendNotification);
    setSize(1200,930);
    startTimerHz(30);
    timerCallback();
}
bool DrizelAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    return files.size() == 1 && juce::File(files[0]).hasFileExtension("wav");
}
void DrizelAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    if (isInterestedInFileDrag(files) && juce::File(files[0]).existsAsFile())
        processor.requestSample(juce::File(files[0]));
}
void DrizelAudioProcessorEditor::timerCallback()
{
    status.setText(processor.loadStatus() + presetStatus, juce::dontSendNotification);
    const auto sample = processor.loadedSampleState();
    if (sample.revision != waveformRevision)
    {
        waveformRevision = sample.revision;
        thumbnail.clear();
        if (sample.path.isNotEmpty())
        {
            cache.clear(); // Same path may now contain different audio.
            thumbnail.setSource(new juce::FileInputSource(juce::File(sample.path)));
        }
    }
    displayedPeak = std::max(processor.outputPeak(), displayedPeak * 0.9f);
    activeGrains = processor.activeGrainCount();
    const int selected=juce::jlimit(0,4,static_cast<int>(std::lround(processor.parameters.getRawParameterValue("grainSelection")->load())));
    static const char* modes[]={"MANUAL","ATTACK","SUSTAIN","AVOID SILENCE","BRIGHT"};
    selectionDisplay.setText(modes[selected],juce::dontSendNotification);
    const auto choiceIndex = [this](const char* id, int maxValue)
    {
        return juce::jlimit(0,maxValue,static_cast<int>(std::lround(processor.parameters.getRawParameterValue(id)->load())));
    };
    shapeDisplay.setText(choiceIndex("lfoShape",1)==0?"SINE":"TRIANGLE",juce::dontSendNotification);
    static const char* targets[]={"OFF","POSITION","PITCH"};
    destinationDisplay.setText(targets[choiceIndex("lfoDestination",2)],juce::dontSendNotification);
    modeDisplay.setText(choiceIndex("midiMode",1)==0?"DRONE":"MIDI",juce::dontSendNotification);
    repaint();
}
void DrizelAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bg=juce::Colour(0xff0b2026);
    const auto surface=juce::Colour(0xff163a40);
    const auto teal=juce::Colour(0xff5cd9c6);
    const auto ivory=juce::Colour(0xffe8f4ef);
    const auto amber=juce::Colour(0xffe5b96b);
    g.fillAll(bg);
    g.setColour(surface);
    g.fillRoundedRectangle(headerBounds.toFloat(),12.0f);
    g.fillRoundedRectangle(waveformBounds.expanded(12,38).toFloat(),12.0f);
    g.setColour(ivory);
    g.setFont(juce::Font(30.0f,juce::Font::bold));
    g.drawText("drizel",headerBounds.withTrimmedLeft(22).removeFromLeft(170),juce::Justification::centredLeft);
    g.setFont(14.0f);
    g.drawText("SAMPLE  /  " + processor.loadedSampleState().path.fromLastOccurrenceOf("/",false,false),
               waveformBounds.getX(),waveformBounds.getY()-30,500,25,juce::Justification::centredLeft);
    g.setColour(teal);
    if(thumbnail.getTotalLength()>0.0)
    {
        thumbnail.drawChannels(g,waveformBounds,0.0,thumbnail.getTotalLength(),1.0f);
        const float pos=processor.parameters.getRawParameterValue("position")->load();
        const int x=waveformBounds.getX()+juce::roundToInt(pos*(waveformBounds.getWidth()-1));
        g.setColour(teal.withAlpha(0.22f));
        g.fillRoundedRectangle(juce::Rectangle<float>(static_cast<float>(x-32),static_cast<float>(waveformBounds.getY()),64.0f,static_cast<float>(waveformBounds.getHeight())),5.0f);
        g.setColour(amber);
        g.drawVerticalLine(x,static_cast<float>(waveformBounds.getY()),static_cast<float>(waveformBounds.getBottom()));
    }
    else g.drawText("Drop a WAV here or choose Load WAV",waveformBounds,juce::Justification::centred);
    g.setColour(ivory);
    g.setFont(13.0f);
    g.drawText("ACTIVE GRAINS  "+juce::String(activeGrains),waveformBounds.getX(),waveformBounds.getBottom()+8,240,22,juce::Justification::centredLeft);
    g.setColour(ivory);
    g.fillRoundedRectangle(deckBounds.toFloat(),12.0f);
    g.setColour(juce::Colour(0xff37666b));
    g.setFont(juce::Font(16.0f,juce::Font::bold));
    for(const auto& section : std::array<std::pair<const char*,int>,4>{{{"GRAIN ENGINE",24},{"PITCH / VOICE",370},{"ENVELOPE",680},{"MODULATION",925}}})
        g.drawText(section.first,deckBounds.getX()+section.second,deckBounds.getY()+14,250,25,juce::Justification::centredLeft);
    g.setColour(surface);
    g.fillRoundedRectangle(keyboardHeaderBounds.toFloat(),12.0f);
    g.setColour(juce::Colour(0xff37666b));
    g.setFont(11.0f);
    g.drawText("MODE",405,563,65,16,juce::Justification::centred);
    g.drawText("SHAPE",890,563,65,16,juce::Justification::centred);
    g.drawText("TARGET",1044,563,65,16,juce::Justification::centred);
    g.setColour(juce::Colour(0xff20535a));
    g.fillRoundedRectangle(meterBounds.toFloat(),4.0f);
    g.setColour(displayedPeak>=0.99f?amber:teal);
    g.fillRoundedRectangle(meterBounds.withWidth(juce::roundToInt(meterBounds.getWidth()*juce::jlimit(0.0f,1.0f,displayedPeak))).toFloat(),4.0f);
}
void DrizelAudioProcessorEditor::resized()
{
    const int w=getWidth();
    headerBounds={20,16,w-40,70};
    loadButton.setBounds(w-550,34,100,34);
    presetButton.setBounds(w-440,34,110,34);
    saveButton.setBounds(w-320,34,110,34);
    playButton.setBounds(w-200,34,160,34);
    status.setBounds(35,94,w-70,25);
    waveformBounds={46,157,w-92,160};
    deckBounds={20,378,w-40,278};
    const std::array<juce::Point<int>,9> pos{{{95,425},{438,435},{238,515},{1085,515},{951,435},{1047,435},{705,435},{805,435},{540,435}}};
    for(std::size_t i=0;i<sliders.size();++i)
    {
        const bool big=i==0;
        const int sw=big?130:88,sh=big?156:110;
        sliders[i].setBounds(pos[i].x,pos[i].y,sw,sh);
        labels[i].setBounds(pos[i].x-5,pos[i].y+sh,sw+10,20);
    }
    modeKnob.setBounds(405,579,62,62);
    modeDisplay.setBounds(470,592,95,32);
    shapeKnob.setBounds(890,579,62,62);
    shapeDisplay.setBounds(949,592,94,32);
    destinationKnob.setBounds(1044,579,62,62);
    destinationDisplay.setBounds(1104,592,77,32);
    selectionLabel.setBounds(30,337,150,25);
    selectionKnob.setBounds(185,324,54,54);
    selectionTicks.setBounds(248,343,260,25);
    selectionDisplay.setBounds(518,332,200,37);
    midiHelp.setBounds(730,337,w-760,25);
    keyboardHeaderBounds={20,672,w-40,64};
    midiHelp.setBounds(30,682,w-260,28);
    meterBounds={w-220,704,180,8};
    keyboard.setBounds(36,750,w-72,158);
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

void DrizelAudioProcessorEditor::setWaveformPosition(int x)
{
    const float normalized=juce::jlimit(0.0f,1.0f,
        static_cast<float>(x-waveformBounds.getX()) /
        static_cast<float>(juce::jmax(1,waveformBounds.getWidth()-1)));
    if(auto* parameter=processor.parameters.getParameter("position"))
        parameter->setValueNotifyingHost(normalized);
}
void DrizelAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if(waveformBounds.contains(e.getPosition()))
    {
        if(auto* parameter=processor.parameters.getParameter("position"))
            parameter->beginChangeGesture();
        waveformGesture=true;
        setWaveformPosition(e.x);
    }
}
void DrizelAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if(waveformGesture) setWaveformPosition(e.x);
}
void DrizelAudioProcessorEditor::mouseUp(const juce::MouseEvent& e)
{
    if(waveformGesture)
    {
        setWaveformPosition(e.x);
        if(auto* parameter=processor.parameters.getParameter("position"))
            parameter->endChangeGesture();
        waveformGesture=false;
    }
}
bool DrizelAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    // Piano mapping: A=C4, W=C#4, S=D4, E=D#4 ... K=C5.
    static constexpr char keys[]="awsedftgyhujk";
    const auto character=juce::CharacterFunctions::toLowerCase(key.getTextCharacter());
    for(int i=0;i<13;++i)
        if(character==keys[i])
        {
            if(!computerNotesDown[static_cast<std::size_t>(i)])
            {
                computerNotesDown[static_cast<std::size_t>(i)]=true;
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
    return false;
}
