#include "PluginEditor.h"

DrizelAudioProcessorEditor::DrizelAudioProcessorEditor(DrizelAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p),
      waveformPositionAttachment(*p.parameters.getParameter("position"), nullptr)
{
    formats.registerBasicFormats();
    addAndMakeVisible(loadButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(status);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(presetButton);
    addAndMakeVisible(midiHelp);
    midiHelp.setText("MIDI ch 1 | Last note wins | Bend +/-2 | CC1 LFO | CC74 position | CC71 density", juce::dontSendNotification);
    saveButton.onClick = [this] { choosePreset(true); };
    presetButton.onClick = [this] { choosePreset(false); };
    const char* ids[] = {"position", "pitch", "density", "gain", "lfoRate", "lfoDepth", "attack", "release", "rootNote"};
    const char* names[] = {"Position", "Pitch / st", "Density / gr/s",
                          "Output gain", "LFO rate / Hz", "LFO depth", "Attack / s", "Release / s", "Root note", "LFO shape", "LFO target", "Mode"};
    for (std::size_t i = 0; i < labels.size(); ++i)
    {
        labels[i].setText(names[i], juce::dontSendNotification);
        addAndMakeVisible(labels[i]);
    }
    for (std::size_t i = 0; i < sliders.size(); ++i)
    {
        sliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        sliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
        addAndMakeVisible(sliders[i]);
        attachments[i] = std::make_unique<SliderAttachment>(processor.parameters, ids[i], sliders[i]);
    }
    shape.addItemList(juce::StringArray{"Sine", "Triangle"}, 1);
    destination.addItemList(juce::StringArray{"Off", "Position", "Pitch"}, 1);
    mode.addItemList(juce::StringArray{"Drone", "MIDI"}, 1);
    addAndMakeVisible(mode);
    modeAttachment = std::make_unique<ComboAttachment>(processor.parameters, "midiMode", mode);
    addAndMakeVisible(shape);
    addAndMakeVisible(destination);
    shapeAttachment = std::make_unique<ComboAttachment>(processor.parameters, "lfoShape", shape);
    destinationAttachment = std::make_unique<ComboAttachment>(processor.parameters, "lfoDestination", destination);
    playAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters, "play", playButton);
    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser>("Load a WAV", juce::File{}, "*.wav");
        auto safe = juce::Component::SafePointer<DrizelAudioProcessorEditor>(this);
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe](const juce::FileChooser& dialog)
            {
                if (safe != nullptr && dialog.getResult().existsAsFile())
                    safe->processor.requestSample(dialog.getResult());
            });
    };
    selectionLabel.setText("Grain selection", juce::dontSendNotification);
    selection.addItemList(juce::StringArray{"Manual", "Attack", "Sustain", "Avoid silence", "Bright"}, 1);
    selection.setTooltip("Guided modes look near the current Position. If there is no match, the grain starts at Position.");
    addAndMakeVisible(selectionLabel);
    addAndMakeVisible(selection);
    selectionAttachment = std::make_unique<ComboAttachment>(processor.parameters, "grainSelection", selection);
    movementCommand.setMultiLine(true);
    movementCommand.setReturnKeyStartsNewLine(true);
    movementCommand.setTextToShowWhenEmpty("Describe the grain movement", juce::Colours::grey);
    movementCommand.setTitle("Movement command");
    movementCommand.onTextChange = [this] {
#if DRIZEL_MAC_SPEECH
        movementSpeech.cancel();
#endif
        applyButton.setEnabled(false);
        movementStatus.setText("Preview to review your changes", juce::dontSendNotification);
    };
    previewButton.onClick = [this] { previewMovement(); };
    applyButton.onClick = [this] { applyMovement(); };
    manualButton.onClick = [this] {
        processor.setMovement({});
        movementStatus.setText("Manual scan active", juce::dontSendNotification);
        applyButton.setEnabled(false);
    };
    applyButton.setEnabled(false);
    const char* settings[] = {"Region start %", "Region end %", "Interval / ms",
                              "Step %", "Distance variation %", "Timing variation %"};
    const int minimum[] = {0, 1, 50, 1, 0, 0};
    const int maximum[] = {99, 100, 10000, 50, 90, 90};
    for (std::size_t i = 0; i < movementSettings.size(); ++i)
    {
        auto& slider = movementSettings[i];
        movementLabels[i].setText(settings[i], juce::dontSendNotification);
        slider.setTitle(settings[i]);
        slider.setRange(minimum[i], maximum[i], i == 2 ? 50 : 1);
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        addAndMakeVisible(movementLabels[i]);
        addAndMakeVisible(slider);
    }
    for (juce::Component* component : std::array<juce::Component*, 5>{
             &movementCommand, &previewButton, &applyButton, &manualButton, &movementStatus})
        addAndMakeVisible(component);
    movementStatus.setText(processor.activeMovement().pattern == drizel::MovementPattern::manual
                           ? "Manual scan active" : "Movement policy active", juce::dontSendNotification);
#if DRIZEL_MAC_SPEECH
    if (juce::JUCEApplicationBase::isStandaloneApp())
    {
        addAndMakeVisible(speechButton);
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
            movementSpeech.start([this](juce::String transcript, juce::String message) {
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
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xfff2efdf));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff222b26));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0xffa4d38a));
    keyboard.setTitle("MIDI keyboard");
    keyboard.setWantsKeyboardFocus(false);
    octaveDown.onClick = [this] { setKeyboardOctave(keyboardOctave - 1); };
    octaveUp.onClick = [this] { setKeyboardOctave(keyboardOctave + 1); };
    octaveDown.setTitle("Octave down");
    octaveUp.setTitle("Octave up");
    movementTitle.setText("Movement command", juce::dontSendNotification);
    for (juce::Component* component : std::array<juce::Component*, 5>{
             &keyboard, &keyboardRange, &octaveDown, &octaveUp, &movementTitle})
        addAndMakeVisible(component);
    setKeyboardOctave(3);
    setSize(900, 775);
    startTimerHz(30);
    timerCallback();
}
DrizelAudioProcessorEditor::~DrizelAudioProcessorEditor()
{
#if DRIZEL_MAC_SPEECH
    movementSpeech.cancel();
#endif
    keyboardState.allNotesOff(1);
    keyboardState.removeListener(this);
    endWaveformGesture();
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
    keyboardRange.setText("MIDI keyboard  ·  C" + juce::String(keyboardOctave)
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
        processor.requestSample(juce::File(files[0]));
}
void DrizelAudioProcessorEditor::timerCallback()
{
    status.setText(processor.loadStatus() + presetStatus, juce::dontSendNotification);
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
    activeGrains = processor.activeGrainCount();
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
void DrizelAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff202722));
    const char* titles[] = {"Grain", "Envelope & MIDI", "Modulation"};
    for (std::size_t i = 0; i < banks.size(); ++i)
    {
        g.setColour(juce::Colour(0xff303b32));
        g.fillRoundedRectangle(banks[i].toFloat(), 7.0f);
        g.setColour(juce::Colour(0xffc6e7aa));
        g.setFont(15.0f);
        g.drawText(titles[i], banks[i].reduced(10, 0).removeFromTop(24), juce::Justification::centredLeft);
    }
    g.setColour(juce::Colour(0xffb7e885));
    g.setFont(26.0f);
    g.drawText("DRIZEL", 20, 12, 300, 36, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff11241f));
    g.fillRect(waveformBounds);
    g.setColour(juce::Colour(0xffb7e885));
    if (thumbnail.getTotalLength() > 0.0)
    {
        const auto audioBounds = waveformBounds.reduced(4);
        thumbnail.drawChannels(g, audioBounds, 0.0, thumbnail.getTotalLength(), 1.0f);
        // Base position marker: modulation may move new-grain starts around it.
        const float position = processor.parameters.getRawParameterValue("position")->load();
        const int x = audioBounds.getX() + juce::roundToInt(position * (audioBounds.getWidth() - 1));
        g.setColour(juce::Colours::white);
        g.drawVerticalLine(x, static_cast<float>(waveformBounds.getY()), static_cast<float>(waveformBounds.getBottom()));
    }
    else
        g.drawText("Drop a WAV here or choose Load WAV", waveformBounds, juce::Justification::centred);
    g.setFont(14.0f);
    g.setColour(juce::Colours::white);
    g.drawText("Active grains: " + juce::String(activeGrains) + "   |   Click or drag waveform to set base position", 20, 222, 700, 22, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff11241f));
    g.fillRect(meterBounds);
    g.setColour(displayedPeak >= 0.99f ? juce::Colours::orange : juce::Colour(0xffb7e885));
    g.fillRect(meterBounds.withWidth(juce::roundToInt(meterBounds.getWidth() * juce::jlimit(0.0f, 1.0f, displayedPeak))));
}
void DrizelAudioProcessorEditor::resized()
{
    auto header = juce::Rectangle<int>(getWidth() - 440, 16, 420, 30);
    for (auto* button : {&loadButton, &presetButton, &saveButton})
        button->setBounds(header.removeFromLeft(105).reduced(3, 0));
    playButton.setBounds(header);
    status.setBounds(20, 51, getWidth() - 40, 25);
    waveformBounds = {20, 80, getWidth() - 40, 136};
    meterBounds = {getWidth() - 147, 228, 127, 10};

    auto row = juce::Rectangle<int>(20, 250, getWidth() - 40, 206);
    const auto bankWidth = (row.getWidth() - 20) / 3;
    const int groups[][5] = {{0, 2, 1, 3, -1}, {7, 8, 6, 11, -2}, {4, 5, 9, 10, -2}};
    for (std::size_t i = 0; i < banks.size(); ++i)
    {
        banks[i] = row.removeFromLeft(i == 2 ? row.getWidth() : bankWidth);
        row.removeFromLeft(10);
        auto controls = banks[i].reduced(10, 0);
        controls.removeFromTop(26);
        for (const int index : groups[i])
        {
            if (index == -2) break;
            auto line = controls.removeFromTop(34);
            if (index == -1)
            {
                selectionLabel.setBounds(line.removeFromLeft(102));
                selection.setBounds(line.reduced(0, 4));
                continue;
            }
            labels[static_cast<std::size_t>(index)].setBounds(line.removeFromLeft(100));
            if (index < 9) sliders[static_cast<std::size_t>(index)].setBounds(line);
            else if (index == 9) shape.setBounds(line.reduced(0, 4));
            else if (index == 10) destination.setBounds(line.reduced(0, 4));
            else mode.setBounds(line.reduced(0, 4));
        }
    }
    movementTitle.setBounds(20, 462, getWidth() - 40, 20);
    movementCommand.setBounds(20, 484, getWidth() - 40, 50);
    auto actions = juce::Rectangle<int>(20, 540, getWidth() - 40, 26);
    previewButton.setBounds(actions.removeFromLeft(90).reduced(2, 0));
    applyButton.setBounds(actions.removeFromLeft(78).reduced(2, 0));
    manualButton.setBounds(actions.removeFromLeft(105).reduced(2, 0));
#if DRIZEL_MAC_SPEECH
    if (speechButton.isVisible())
        speechButton.setBounds(actions.removeFromLeft(104).reduced(2, 0));
#endif
    actions.removeFromLeft(8);
    movementStatus.setBounds(actions);
    const auto settingWidth = (getWidth() - 40) / 3;
    for (std::size_t i = 0; i < movementSettings.size(); ++i)
    {
        auto line = juce::Rectangle<int>(20 + int(i % 3) * settingWidth,
                                          572 + int(i / 3) * 25, settingWidth - 10, 24);
        movementLabels[i].setBounds(line.removeFromLeft(135));
        movementSettings[i].setBounds(line);
    }
    keyboardRange.setBounds(20, 629, 200, 24);
    octaveDown.setBounds(222, 629, 28, 24);
    octaveUp.setBounds(254, 629, 28, 24);
    midiHelp.setBounds(294, 629, getWidth() - 314, 24);
    keyboard.setKeyWidth(float(getWidth() - 40) / 14.0f);
    keyboard.setBounds(20, 660, getWidth() - 40, 95);
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
    if ((!stepped && !irregular && !cyclic) || (cyclic && (stepped || irregular)))
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
        proposedMovement.distanceVariation = relative ? std::min(90, priorDistance + 15) : 30;
        proposedMovement.timingVariation = relative ? std::min(90, priorTiming + 15) : 30;
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
    processor.setMovement(proposedMovement);
    movementStatus.setText("Movement policy active", juce::dontSendNotification);
}
