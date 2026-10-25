#include "PluginEditor.h"

DrizelAudioProcessorEditor::DrizelAudioProcessorEditor(DrizelAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p)
{
    formats.registerBasicFormats();
    addAndMakeVisible(loadButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(status);
    const char* ids[] = {"position", "pitch", "density", "gain", "lfoRate", "lfoDepth"};
    const char* names[] = {"Position", "Pitch / semitones", "Density / grains per second",
                          "Output gain", "LFO rate / Hz", "LFO depth", "LFO shape", "LFO destination"};
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
    setSize(760, 650);
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
    status.setText(processor.loadStatus(), juce::dontSendNotification);
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
    loadButton.setBounds(520, 16, 130, 32);
    playButton.setBounds(660, 16, 80, 32);
    status.setBounds(20, 52, getWidth() - 40, 30);
    waveformBounds = {20, 90, getWidth() - 40, 140};
    for (int i = 0; i < 8; ++i)
    {
        const int x = 20 + (i % 2) * 370;
        const int y = 280 + (i / 2) * 76;
        labels[static_cast<std::size_t>(i)].setBounds(x, y, 350, 24);
        auto area = juce::Rectangle<int>(x, y + 26, 350, 32);
        if (i < 6) sliders[static_cast<std::size_t>(i)].setBounds(area);
        else if (i == 6) shape.setBounds(area);
        else destination.setBounds(area);
    }
    meterBounds = {20, 610, getWidth() - 40, 16};
}
