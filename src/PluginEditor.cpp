#include "PluginEditor.h"
#if defined(DRIZEL_HAS_NEURAL_MANACLE_LOGO)
#include <BinaryData.h>
#endif
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
        g.setColour(juce::Colour(0xffe7b66d).withAlpha(0.48f));
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
        g.setColour(juce::Colour(0xffe7b66d));
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
        g.setColour(juce::Colour(0xfff3c57a));
        g.drawLine(from.x,from.y,to.x,to.y,juce::jmax(2.2f,radius*0.07f));

    }
};
DrizelGlassLookAndFeel& glassLook() { static DrizelGlassLookAndFeel look; return look; }
}

DrizelAudioProcessorEditor::DrizelAudioProcessorEditor(DrizelAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p)
{
    setOpaque(false);
    formats.registerBasicFormats();
    for (auto* button : { &loadButton, &saveButton, &presetButton })
    {
        addAndMakeVisible(*button);
        button->setLookAndFeel(&glassLook());
        button->setColour(juce::TextButton::textColourOffId,juce::Colour(mint));
    }
    addAndMakeVisible(playButton);
    playButton.setClickingTogglesState(true);




    addAndMakeVisible(speechText);
    speechText.setMultiLine(false);
    speechText.setTextToShowWhenEmpty("Enter text to synthesize",juce::Colour(0xff8aa99a));
    speechText.setColour(juce::TextEditor::backgroundColourId,juce::Colour(ink));
    speechText.setColour(juce::TextEditor::textColourId,juce::Colour(mint));
    speechText.setColour(juce::TextEditor::outlineColourId,juce::Colour(0xff86a891));
    addAndMakeVisible(speechGenerate);
    speechGenerate.setLookAndFeel(&glassLook());
    speechGenerate.setColour(juce::TextButton::textColourOffId,juce::Colour(mint));
    speechGenerate.onClick=[this]
    {
#if JUCE_MAC
        if (speechPending || speechText.getText().trim().isEmpty()) return;
        speechOutput=juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getNonexistentChildFile("drizel-speech",".aiff",false);
        speechProcess=std::make_unique<juce::ChildProcess>();
        // Argument vector, not a shell command: user text cannot inject commands.
        juce::StringArray args {"/usr/bin/say","-o",speechOutput.getFullPathName(),
                                speechText.getText().substring(0,1000)};
        speechPending=speechProcess->start(args);
        speechMessage=speechPending ? "Generating speech..." : "Could not start macOS speech";
        speechGenerate.setEnabled(!speechPending);
#else
        speechMessage="Speech generation currently requires macOS";
#endif
    };
    addAndMakeVisible(speechTargetChoice);
    for(int i=0;i<4;++i)
        speechTargetChoice.addItem(juce::StringArray{"Off","Density","Position","Pitch"}[i],i+1);
    speechTargetChoice.setLookAndFeel(&glassLook());
    speechTargetChoice.setColour(juce::ComboBox::textColourId,juce::Colour(mint));
    speechTargetAttachment=std::make_unique<ChoiceAttachment>(processor.parameters,"speechTarget",speechTargetChoice);
    addAndMakeVisible(speechDepthSlider);
    speechDepthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    speechDepthSlider.setTextBoxStyle(juce::Slider::TextBoxRight,false,55,22);
    speechDepthSlider.setColour(juce::Slider::textBoxTextColourId,juce::Colour(mint));
    speechDepthSlider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(ink));
    speechDepthAttachment=std::make_unique<SliderAttachment>(processor.parameters,"speechDepth",speechDepthSlider);
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
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 22);
        s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(mint));
        s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(forest));
        s.setColour(juce::Slider::thumbColourId, juce::Colour(cyan));
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(mint));
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(ink));
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setLookAndFeel(&glassLook());
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
        box.setColour(juce::ComboBox::arrowColourId,juce::Colour(0xffe7b66d));
        box.setJustificationType(juce::Justification::centred);
        box.setLookAndFeel(&glassLook());
        addAndMakeVisible(box);
    };
    configureChoice(modeChoice,{"Drone","MIDI"});
    configureChoice(shapeChoice,{"Sine","Triangle"});
    configureChoice(destinationChoice,{"Off","Position","Pitch"});
    configureChoice(selectionChoice,{"Manual","Attack","Sustain","Avoid silence","Bright"});
    modeAttachment=std::make_unique<ChoiceAttachment>(processor.parameters,"midiMode",modeChoice);
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
    setSize(1200,985);
    startTimerHz(30);
    timerCallback();
}
DrizelAudioProcessorEditor::~DrizelAudioProcessorEditor()
{
    for(auto& slider : sliders) slider.setLookAndFeel(nullptr);
    for(auto* button : { &loadButton, &saveButton, &presetButton, &speechGenerate }) button->setLookAndFeel(nullptr);
    speechTargetChoice.setLookAndFeel(nullptr);
    for(auto* box : { &modeChoice, &shapeChoice, &destinationChoice, &selectionChoice })
        box->setLookAndFeel(nullptr);
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
    if(speechPending && speechProcess != nullptr && !speechProcess->isRunning())
    {
        speechPending=false;
        speechGenerate.setEnabled(true);
        const bool ok=speechProcess->getExitCode()==0 && speechOutput.existsAsFile();
        if(ok)
        {
            // Convert AIFF generated by macOS into WAV for the existing sample loader.
            juce::AudioFormatManager manager;
            manager.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(speechOutput));
            auto wav=speechOutput.withFileExtension(".wav");
            std::unique_ptr<juce::FileOutputStream> stream(wav.createOutputStream());
            if(reader != nullptr && stream != nullptr)
            {
                juce::WavAudioFormat format;
                std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(
                    stream.get(),
                    juce::AudioFormatWriterOptions{}
                        .withSampleRate(reader->sampleRate)
                        .withNumChannels(static_cast<int>(reader->numChannels))
                        .withBitsPerSample(16)));
                if(writer != nullptr)
                {
                    stream.release();
                    const bool written=writer->writeFromAudioReader(*reader,0,-1);
                    writer.reset();
                    if(written)
                    {
                        processor.requestSample(wav);
                        speechMessage="Speech loaded into grain engine";
                    }
                    else speechMessage="Speech WAV conversion failed";
                }
                else speechMessage="Speech WAV writer unavailable";
            }
            else speechMessage="Speech audio could not be read";
        }
        else speechMessage="Speech synthesis failed";
        speechProcess.reset();
    }
    status.setText(processor.loadStatus() + presetStatus
                   + (speechMessage.isEmpty() ? juce::String() : "  |  " + speechMessage),
                   juce::dontSendNotification);
    playButton.repaint();
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
    repaint();
}
static std::array<juce::Rectangle<int>,4> controlPanels(juce::Rectangle<int> deck)
{
    // One source of truth for panel painting and component placement.
    constexpr int gap=12;
    const int usable=deck.getWidth()-3*gap;
    const std::array<int,4> weights {{340,280,225,279}};
    constexpr int total=1124;
    const int a=usable*weights[0]/total;
    const int b=usable*weights[1]/total;
    const int c=usable*weights[2]/total;
    const int d=usable-a-b-c;
    const int x=deck.getX(),y=deck.getY(),h=deck.getHeight();
    return {{{x,y,a,h},{x+a+gap,y,b,h},{x+a+b+2*gap,y,c,h},
             {x+a+b+c+3*gap,y,d,h}}};
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
    juce::Path roundedFrame;
    roundedFrame.addRoundedRectangle(getLocalBounds().toFloat(),22.0f);
    g.reduceClipRegion(roundedFrame);
    g.fillAll();
    g.setGradientFill(juce::ColourGradient(juce::Colour(cyan).withAlpha(0.38f),getWidth()*0.8f,0.0f,
                                           juce::Colour(emerald).withAlpha(0.0f),getWidth()*0.2f,getHeight()*0.7f,true));
    g.fillRect(getLocalBounds());
    g.setGradientFill(juce::ColourGradient(juce::Colour(ink).withAlpha(0.88f),waveformBounds.getTopLeft().toFloat(),
                                           juce::Colour(forest).withAlpha(0.8f),waveformBounds.getBottomRight().toFloat(),false));
    g.fillRoundedRectangle(waveformBounds.expanded(12,27).toFloat(),18.0f);
    g.setColour(juce::Colour(pearl).withAlpha(0.4f));
    g.drawRoundedRectangle(waveformBounds.expanded(12,27).toFloat(),18.0f,1.2f);
    g.setColour(ivory);
    g.setFont(juce::Font(juce::FontOptions(30.0f, juce::Font::bold)));
    // Render the supplied PNG itself; preserve the artwork rather than tracing it.
#if defined(DRIZEL_HAS_NEURAL_MANACLE_LOGO)
    {
        static const juce::Image logo=juce::ImageFileFormat::loadFrom(
            BinaryData::neural_manacle_logo_png,
            BinaryData::neural_manacle_logo_pngSize);
        if(logo.isValid())
            g.drawImageWithin(logo,34,24,52,52,juce::RectanglePlacement::centred);
    }
#else
    // Logo asset not yet installed: leave its space clear.
#endif
    g.drawText("drizel",juce::Rectangle<int>(88,22,180,58),juce::Justification::centredLeft);
    // Matching dark analog readouts for the loaded input and active grain count.
    const auto drawReadout=[&g](juce::Rectangle<int> area,const juce::String& value)
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff25483a),
                                               area.getTopLeft().toFloat(),
                                               juce::Colour(0xff102b25),
                                               area.getBottomRight().toFloat(),false));
        g.fillRoundedRectangle(area.toFloat(),6.0f);
        g.setColour(juce::Colour(0xff86a891));
        g.drawRoundedRectangle(area.toFloat().reduced(0.6f),6.0f,1.1f);
        g.setColour(juce::Colour(mint));
        g.setFont(juce::Font(juce::FontOptions(14.0f)));
        g.drawFittedText(value,area.reduced(10,2),juce::Justification::centredLeft,1);
    };
    const auto sampleName=processor.loadedSampleState().path.fromLastOccurrenceOf("/",false,false);
    drawReadout({waveformBounds.getX(),waveformBounds.getY()-32,310,27},
                "INPUT  /  " + (sampleName.isEmpty() ? juce::String("No WAV") : sampleName));
    drawReadout({waveformBounds.getRight()-190,waveformBounds.getY()-32,190,27},
                "ACTIVE GRAINS  " + juce::String(processor.activeGrainCount()));
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
    g.drawText("SPEECH ENGINE  /  TEXT TO AUDIO + ENVELOPE MODULATION",
               35,661,700,22,juce::Justification::centredLeft);
    // Four physically separate control panels, matching the reference layout.
    const auto panels=controlPanels(deckBounds);
    for(const auto& panel:panels)
    {
        g.setColour(juce::Colour(ink).withAlpha(0.20f));
        g.fillRoundedRectangle(panel.translated(0,4).toFloat(),17.0f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe2e7d8),panel.getTopLeft().toFloat(),
                                               juce::Colour(0xffc5ddcf),panel.getBottomRight().toFloat(),false));
        g.fillRoundedRectangle(panel.toFloat(),17.0f);
        g.setColour(juce::Colour(pearl).withAlpha(0.58f));
        g.drawRoundedRectangle(panel.toFloat().reduced(1.0f),17.0f,1.1f);
    }
    g.setColour(juce::Colour(forest));
    g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    const std::array<const char*,4> titles {{"GRAIN ENGINE","PITCH / VOICE","ENVELOPE","MODULATION"}};
    for(std::size_t i=0;i<panels.size();++i)
        g.drawText(titles[i],panels[i].getX()+14,panels[i].getY()+14,
                   panels[i].getWidth()-28,25,juce::Justification::centredLeft);
    g.setColour(juce::Colour(forest));
    g.setFont(11.0f);
    for(const auto& pair : std::array<std::pair<const char*,juce::Rectangle<int>>,3> {{
        {"MODE",modeChoice.getBounds()},{"SHAPE",shapeChoice.getBounds()},
        {"TARGET",destinationChoice.getBounds()}
    }})
        g.drawText(pair.first,pair.second.getX(),pair.second.getY()-23,
                   pair.second.getWidth(),18,juce::Justification::centredLeft);
    g.setColour(ivory);
    g.drawText("OUTPUT LEVEL",meterBounds.getX()-120,meterBounds.getY()-8,112,22,juce::Justification::centredRight);
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
    playButton.setBounds(w-190,34,150,34);
    status.setBounds(35,94,w-70,25);
    waveformBounds={46,157,w-92,160};

    // Dedicated selector strip, fully outside the waveform and control panels.
    selectionLabel.setBounds(40,356,145,24);
    selectionChoice.setBounds(205,351,210,34);

    deckBounds={20,395,w-40,261};
    const auto panels=controlPanels(deckBounds);
    // All nine rotary controls share one baseline, value baseline and label baseline.
    // Centers are evenly distributed within each panel's inner horizontal padding.
    const auto place=[&](int index,int panel,int column,int count)
    {
        const auto p=panels[(std::size_t)panel];
        const int innerLeft=p.getX()+18, innerWidth=p.getWidth()-36;
        const int cx=innerLeft+(innerWidth*(2*column+1))/(2*count);
        constexpr int knobWidth=94;
        sliders[(std::size_t)index].setBounds(cx-knobWidth/2,p.getY()+61,knobWidth,105);
        labels[(std::size_t)index].setBounds(cx-54,p.getY()+170,108,20);
    };
    place(0,0,0,3); // Position
    place(2,0,1,3); // Density
    place(3,0,2,3); // Output
    place(1,1,0,2); // Pitch
    place(8,1,1,2); // Root note
    place(6,2,0,2); // Attack
    place(7,2,1,2); // Release
    place(4,3,0,2); // LFO rate
    place(5,3,1,2); // LFO depth

    const auto choiceBounds=[&](int panel,int column,int count)
    {
        const auto p=panels[(std::size_t)panel];
        const int margin=18,gap=14;
        const int width=(p.getWidth()-2*margin-(count-1)*gap)/count;
        return juce::Rectangle<int>(p.getX()+margin+column*(width+gap),
                                     p.getY()+221,width,32);
    };
    modeChoice.setBounds(choiceBounds(1,0,1));
    shapeChoice.setBounds(choiceBounds(3,0,2));
    destinationChoice.setBounds(choiceBounds(3,1,2));

    speechText.setBounds(35,686,410,32);
    speechGenerate.setBounds(455,686,150,32);
    speechTargetChoice.setBounds(625,686,160,32);
    speechDepthSlider.setBounds(800,686,355,32);
    keyboardHeaderBounds={20,735,w-40,64};
    meterBounds={w-220,757,180,8};
    keyboard.setBounds(36,803,w-72,158);
    keyboard.setKeyWidth((w-72)/14.0f);
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
