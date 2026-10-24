#pragma once
#include <JuceHeader.h>
#include "audio/RealtimeEngine.h"
#include "audio/Modulation.h"

// Host-facing wrapper. Audio owns the engine; the loader owns all disk I/O.
class DrizelAudioProcessor : public juce::AudioProcessor, private juce::Thread
{
public:
    DrizelAudioProcessor(); // Register host parameters and start the loader.
    ~DrizelAudioProcessor() override; // Join the loader before engine destruction.
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {} // Storage remains available until reprepare/destruction.
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; } // MIDI belongs to a later step.
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    void requestSample(const juce::File&); // Control thread only; queues a path.
    juce::String loadStatus(); // UI thread only; never queried by processBlock.
    juce::AudioProcessorValueTreeState parameters; // Automatable controls/state.

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    void run() override; // Loader loop, including packet reclamation.
    drizel::Lfo lfo;
    drizel::ModulationRouter modulation;
    drizel::RealtimeEngine engine; // Destroyed only after the worker has joined.
    juce::CriticalSection requestLock; // Only UI/lifecycle/state/worker use this.
    juce::String requestedPath, status = "Load a WAV to begin";
    double requestedRate = 0.0; // Zero until the host prepares playback.
    std::uint64_t revision = 0; // Changed path/rate invalidates in-flight work.
    std::atomic<float>* position = nullptr; // Cached parameter pointers: no lookup in callback.
    std::atomic<float>* pitch = nullptr;
    std::atomic<float>* density = nullptr;
    std::atomic<float>* gain = nullptr;
    std::atomic<float>* play = nullptr;
    std::atomic<float>* lfoRate = nullptr;
    std::atomic<float>* lfoDepth = nullptr;
    std::atomic<float>* lfoShape = nullptr;
    std::atomic<float>* lfoDestination = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrizelAudioProcessor)
};
