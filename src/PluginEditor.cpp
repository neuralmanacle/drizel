#include "PluginEditor.h"
namespace
{
constexpr juce::uint32 emerald = 0xff00d879;
constexpr juce::uint32 mint = 0xff00e5ba;
constexpr juce::uint32 cyan = 0xff19e8e8;
constexpr juce::uint32 forest = 0xff006f56;
constexpr juce::uint32 ink = 0xff052b2b;
constexpr juce::uint32 pearl = 0xfff1fffc;

class DrizelGlassLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override
    {
        // Flat, forest-green hardware knobs based on the Drizel master-plan reference.
        // Keep the value indicator dynamic while matching the reference's amber tick.
        const auto bounds=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),
                                                  static_cast<float>(width),static_cast<float>(height));
        const auto centre=bounds.getCentre();
        const float radius=juce::jmin(bounds.getWidth(),bounds.getHeight())*0.43f;
        const auto outer=juce::Rectangle<float>(centre.x-radius,centre.y-radius,radius*2.0f,radius*2.0f);
        g.setColour(juce::Colour(0xff819c89));
        g.fillEllipse(outer);
        const float inset=juce::jmax(3.0f,radius*0.075f);
        g.setColour(juce::Colour(0xff2d4438));
        g.fillEllipse(outer.reduced(inset));
        const float angle=startAngle+sliderPos*(endAngle-startAngle);
        const float innerRadius=radius-inset;
        const auto from=centre+juce::Point<float>(std::sin(angle)*innerRadius*0.57f,
                                                   -std::cos(angle)*innerRadius*0.57f);
        const auto to=centre+juce::Point<float>(std::sin(angle)*innerRadius*0.85f,
                                                 -std::cos(angle)*innerRadius*0.85f);
        g.setColour(juce::Colour(0xffe7b66d));
        g.drawLine(from.x,from.y,to.x,to.y,juce::jmax(3.0f,radius*0.075f));
    }
};
// Deterministic, allocation-free visual noise. Rendered only by the UI paint callback.
void drawFineGrain(juce::Graphics& g, juce::Rectangle<int> area, float opacity, int spacing=4)
{
    juce::Graphics::ScopedSaveState saved(g);
    g.reduceClipRegion(area);
    const int right=area.getRight(), bottom=area.getBottom();
    for(int y=area.getY();y<bottom;y+=spacing)
        for(int x=area.getX();x<right;x+=spacing)
        {
            const juce::uint32 seed=static_cast<juce::uint32>(x)*73856093u
                                    ^static_cast<juce::uint32>(y)*19349663u;
            const juce::uint32 hash=(seed^(seed>>13))*1274126177u;
            const float intensity=static_cast<float>((hash>>24)&255u)/255.0f;
            g.setColour((hash&1u?juce::Colours::white:juce::Colours::black)
                            .withAlpha(opacity*(0.25f+0.75f*intensity)));
            g.fillRect(x,y,2,2);
        }
}
DrizelGlassLookAndFeel& glassLook() { static DrizelGlassLookAndFeel look; return look; }
}

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
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(pearl));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(ink));
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
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 22);
        s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(mint));
        s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(forest));
        s.setColour(juce::Slider::thumbColourId, juce::Colour(cyan));
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(ink));
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(pearl));
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setLookAndFeel(&glassLook());
        addAndMakeVisible(s);
        attachments[i]=std::make_unique<SliderAttachment>(processor.parameters,ids[i],s);
    }

    // Discrete, host-automatable knobs replace the three combo boxes.
    const auto configureChoice = [this](juce::Slider& knob, juce::Label& display, int maximum)
    {
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setMouseDragSensitivity(105);
        knob.setRange(0.0, static_cast<double>(maximum), 1.0);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(mint));
        knob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(forest));
        knob.setColour(juce::Slider::thumbColourId, juce::Colour(cyan));
        knob.setLookAndFeel(&glassLook());
        addAndMakeVisible(knob);
        display.setJustificationType(juce::Justification::centred);
        display.setColour(juce::Label::backgroundColourId, juce::Colour(ink));
        display.setColour(juce::Label::textColourId, juce::Colour(mint));
        display.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
        addAndMakeVisible(display);
    };
    configureChoice(shapeKnob, shapeDisplay, 1);
    configureChoice(destinationKnob, destinationDisplay, 2);
    configureChoice(modeKnob, modeDisplay, 1);
    shapeAttachment=std::make_unique<SliderAttachment>(processor.parameters,"lfoShape",shapeKnob);
    destinationAttachment=std::make_unique<SliderAttachment>(processor.parameters,"lfoDestination",destinationKnob);
    modeAttachment=std::make_unique<SliderAttachment>(processor.parameters,"midiMode",modeKnob);
    selectionKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    selectionKnob.setMouseDragSensitivity(105);
    selectionKnob.setRange(0.0, 4.0, 1.0);
    selectionKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    selectionKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(mint));
    selectionKnob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(forest));
    selectionKnob.setColour(juce::Slider::thumbColourId, juce::Colour(cyan));
    selectionKnob.setLookAndFeel(&glassLook());
    selectionAttachment=std::make_unique<SliderAttachment>(processor.parameters,"grainSelection",selectionKnob);
    selectionDisplay.setJustificationType(juce::Justification::centred);
    selectionDisplay.setColour(juce::Label::backgroundColourId, juce::Colour(ink));
    selectionDisplay.setColour(juce::Label::textColourId, juce::Colour(mint));
    selectionDisplay.setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::bold)));
    selectionTicks.setText("MAN / ATK / SUS / SIL / BRT",juce::dontSendNotification);
    selectionTicks.setColour(juce::Label::textColourId,juce::Colour(pearl));
    selectionTicks.setJustificationType(juce::Justification::centred);
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
DrizelAudioProcessorEditor::~DrizelAudioProcessorEditor()
{
    for(auto& slider : sliders) slider.setLookAndFeel(nullptr);
    shapeKnob.setLookAndFeel(nullptr);
    destinationKnob.setLookAndFeel(nullptr);
    modeKnob.setLookAndFeel(nullptr);
    selectionKnob.setLookAndFeel(nullptr);
    for(int i=0;i<13;++i)
        if(computerNotesDown[static_cast<std::size_t>(i)])
            processor.keyboardState.noteOff(1,60+i,0.0f);
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
    const auto bg=juce::Colour(ink);
    const auto surface=juce::Colour(emerald);
    const auto teal=juce::Colour(cyan);
    const auto ivory=juce::Colour(pearl);
    const auto amber=juce::Colour(mint);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff008f63),0.0f,0.0f,
                                           juce::Colour(0xff00d8bb),static_cast<float>(getWidth()),
                                           static_cast<float>(getHeight()),false));
    g.fillAll();
    g.setGradientFill(juce::ColourGradient(juce::Colour(cyan).withAlpha(0.38f),getWidth()*0.8f,0.0f,
                                           juce::Colour(emerald).withAlpha(0.0f),getWidth()*0.2f,getHeight()*0.7f,true));
    g.fillRect(getLocalBounds());
    drawFineGrain(g,getLocalBounds(),0.20f,4);
    g.setGradientFill(juce::ColourGradient(juce::Colour(pearl).withAlpha(0.35f),headerBounds.getTopLeft().toFloat(),
                                           juce::Colour(ink).withAlpha(0.26f),headerBounds.getBottomRight().toFloat(),false));
    g.fillRoundedRectangle(headerBounds.toFloat(),16.0f);
    g.setColour(juce::Colour(pearl).withAlpha(0.48f));
    g.drawRoundedRectangle(headerBounds.toFloat().reduced(0.8f),16.0f,1.4f);
    drawFineGrain(g,headerBounds.reduced(5),0.22f,4);
    g.setGradientFill(juce::ColourGradient(juce::Colour(ink).withAlpha(0.88f),waveformBounds.getTopLeft().toFloat(),
                                           juce::Colour(forest).withAlpha(0.8f),waveformBounds.getBottomRight().toFloat(),false));
    g.fillRoundedRectangle(waveformBounds.expanded(12,38).toFloat(),18.0f);
    g.setColour(juce::Colour(pearl).withAlpha(0.4f));
    g.drawRoundedRectangle(waveformBounds.expanded(12,38).toFloat(),18.0f,1.2f);
    drawFineGrain(g,waveformBounds.expanded(7,32),0.26f,4);
    g.setColour(ivory);
    g.setFont(juce::Font(juce::FontOptions(30.0f, juce::Font::bold)));
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
    // Four physically separate control panels, matching the reference layout.
    const int y=deckBounds.getY(), height=deckBounds.getHeight();
    const std::array<juce::Rectangle<int>,4> panels {{
        {deckBounds.getX(),y,345,height},
        {deckBounds.getX()+357,y,296,height},
        {deckBounds.getX()+665,y,238,height},
        {deckBounds.getX()+915,y,245,height}
    }};
    for(const auto& panel:panels)
    {
        g.setColour(juce::Colour(ink).withAlpha(0.20f));
        g.fillRoundedRectangle(panel.translated(0,4).toFloat(),17.0f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe2e7d8),panel.getTopLeft().toFloat(),
                                               juce::Colour(0xffc5ddcf),panel.getBottomRight().toFloat(),false));
        g.fillRoundedRectangle(panel.toFloat(),17.0f);
        g.setColour(juce::Colour(pearl).withAlpha(0.58f));
        g.drawRoundedRectangle(panel.toFloat().reduced(1.0f),17.0f,1.1f);
        drawFineGrain(g,panel.reduced(5),0.23f,4);
    }
    g.setColour(juce::Colour(forest));
    g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    for(const auto& section : std::array<std::pair<const char*,int>,4>{{{"GRAIN ENGINE",24},{"PITCH / VOICE",370},{"ENVELOPE",680},{"MODULATION",925}}})
        g.drawText(section.first,deckBounds.getX()+section.second,deckBounds.getY()+14,250,25,juce::Justification::centredLeft);
    g.setGradientFill(juce::ColourGradient(juce::Colour(ink).withAlpha(0.6f),keyboardHeaderBounds.getTopLeft().toFloat(),
                                           juce::Colour(forest).withAlpha(0.55f),keyboardHeaderBounds.getBottomRight().toFloat(),false));
    g.fillRoundedRectangle(keyboardHeaderBounds.toFloat(),12.0f);
    drawFineGrain(g,keyboardHeaderBounds.reduced(5),0.22f,4);
    g.setColour(juce::Colour(forest));
    g.setFont(11.0f);
    g.drawText("MODE",405,563,65,16,juce::Justification::centred);
    g.drawText("SHAPE",890,563,65,16,juce::Justification::centred);
    g.drawText("TARGET",1044,563,65,16,juce::Justification::centred);
    g.setColour(juce::Colour(ink).withAlpha(0.7f));
    g.fillRoundedRectangle(meterBounds.toFloat(),4.0f);
    g.setColour(displayedPeak>=0.99f?ivory:teal);
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
    return false;
}
