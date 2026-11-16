#pragma once
#include <JuceHeader.h>
#include <functional>
#include <memory>

class MovementSpeech
{
public:
    using Result = std::function<void(juce::String transcript, juce::String status)>;
    MovementSpeech();
    ~MovementSpeech();
    void start(Result);
    void finish();
    void cancel();

private:
    struct Session;
    std::shared_ptr<Session> session;
};
