#include "PluginEditor.h"

DrizelAudioProcessorEditor::DrizelAudioProcessorEditor(DrizelAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p)
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
    const char* names[] = {"Position", "Pitch / semitones", "Density / grains per second",
                          "Output gain", "LFO rate / Hz", "LFO depth", "Attack / seconds", "Release / seconds", "Root MIDI note", "LFO shape", "LFO destination", "Mode"};
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
    setSize(760, 814);
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
    repaint();
}
void DrizelAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff161c20));
    g.setColour(juce::Colour(0xffb7e885));
    g.setFont(26.0f);
    g.drawText("DRIZEL", 20, 12, 300, 36, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff24323a));
    g.fillRect(waveformBounds);
    g.setColour(juce::Colour(0xffb7e885));
    if (thumbnail.getTotalLength() > 0.0)
    {
        thumbnail.drawChannels(g, waveformBounds.reduced(4), 0.0, thumbnail.getTotalLength(), 1.0f);
        // Base position marker: modulation may move new-grain starts around it.
        const float position = processor.parameters.getRawParameterValue("position")->load();
        const int x = waveformBounds.getX() + juce::roundToInt(position * (waveformBounds.getWidth() - 1));
        g.setColour(juce::Colours::white);
        g.drawVerticalLine(x, static_cast<float>(waveformBounds.getY()), static_cast<float>(waveformBounds.getBottom()));
    }
    else
        g.drawText("Drop a WAV here or choose Load WAV", waveformBounds, juce::Justification::centred);
    g.setFont(14.0f);
    g.setColour(juce::Colours::white);
    g.drawText("Active grains: " + juce::String(activeGrains) + "   |   Marker: base position", 20, 240, 700, 24, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff24323a));
    g.fillRect(meterBounds);
    g.setColour(displayedPeak >= 0.99f ? juce::Colours::orange : juce::Colour(0xffb7e885));
    g.fillRect(meterBounds.withWidth(juce::roundToInt(meterBounds.getWidth() * juce::jlimit(0.0f, 1.0f, displayedPeak))));
}
void DrizelAudioProcessorEditor::resized()
{
    loadButton.setBounds(285, 16, 100, 32);
    presetButton.setBounds(395, 16, 110, 32);
    saveButton.setBounds(515, 16, 110, 32);
    playButton.setBounds(635, 16, 105, 32);
    status.setBounds(20, 52, getWidth() - 40, 30);
    waveformBounds = {20, 90, getWidth() - 40, 140};
    for (int i = 0; i < 12; ++i)
    {
        const int x = 20 + (i % 2) * 370;
        const int y = 280 + (i / 2) * 64;
        labels[static_cast<std::size_t>(i)].setBounds(x, y, 350, 24);
        auto area = juce::Rectangle<int>(x, y + 26, 350, 32);
        if (i < 9) sliders[static_cast<std::size_t>(i)].setBounds(area);
        else if (i == 9) shape.setBounds(area);
        else if (i == 10) destination.setBounds(area);
        else mode.setBounds(area);
    }
    selectionLabel.setBounds(20, 664, 350, 24);
    selection.setBounds(20, 690, 350, 32);
    midiHelp.setBounds(20, 744, 720, 30);
    meterBounds = {20, 784, getWidth() - 40, 16};
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
