#import <AVFoundation/AVFoundation.h>
#import <Speech/Speech.h>
#include "MovementSpeech.h"
#include <utility>

// Session state is owned by the main thread. The audio tap only supplies buffers
// to its recognition request and does not access Drizel's audio engine.
struct MovementSpeech::Session
{
    AVAudioEngine* engine = nil;
    SFSpeechRecognizer* recognizer = nil;
    SFSpeechAudioBufferRecognitionRequest* request = nil;
    SFSpeechRecognitionTask* task = nil;
    Result result;
    bool tapped = false;

    void stopCapture()
    {
        [engine stop];
        if (tapped)
            [engine.inputNode removeTapOnBus:0];
        tapped = false;
    }

    void complete(juce::String transcript, juce::String status)
    {
        auto callback = std::exchange(result, {});
        stopCapture();
        [task cancel];
        task = nil;
        request = nil;
        if (callback)
            callback(std::move(transcript), std::move(status));
    }
};

MovementSpeech::MovementSpeech() = default;
MovementSpeech::~MovementSpeech() { cancel(); }

void MovementSpeech::cancel()
{
    if (session)
    {
        session->result = {};
        session->complete({}, {});
        session.reset();
    }
}

void MovementSpeech::finish()
{
    if (!session || !session->result)
        return;
    if (!session->tapped)
    {
        session->complete({}, "Capture ended. Hold to talk again.");
        return;
    }
    session->stopCapture();
    [session->request endAudio];
    const auto current = session;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        if (current->result)
            current->complete({}, "Speech timed out. Type your command or try again.");
    });
}

void MovementSpeech::start(Result result)
{
    cancel();
    const auto current = session = std::make_shared<Session>();
    current->result = std::move(result);
    if (@available(macOS 10.15, *))
    {
        [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus speech) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (!current->result)
                    return;
                if (speech != SFSpeechRecognizerAuthorizationStatusAuthorized)
                {
                    current->complete({}, "Speech permission unavailable. Type your command.");
                    return;
                }
                [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL allowed) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        if (!current->result)
                            return;
                        if (!allowed)
                        {
                            current->complete({}, "Microphone permission unavailable. Type your command.");
                            return;
                        }
                        current->recognizer = [[SFSpeechRecognizer alloc] init];
                        const auto canRecognizeLocally = [](SFSpeechRecognizer* recognizer) {
                            return recognizer.available && recognizer.supportsOnDeviceRecognition;
                        };
                        if (!canRecognizeLocally(current->recognizer))
                        {
                            // Movement commands use English. Try available English assets
                            // when the system locale has no local recognizer.
                            for (NSString* identifier in @[@"en-IN", @"en-US", @"en-GB"])
                            {
                                auto* candidate = [[SFSpeechRecognizer alloc]
                                    initWithLocale:[NSLocale localeWithLocaleIdentifier:identifier]];
                                if (canRecognizeLocally(candidate))
                                {
                                    current->recognizer = candidate;
                                    break;
                                }
                            }
                        }
                        if (!canRecognizeLocally(current->recognizer))
                        {
                            current->complete({}, "No local English speech model. Try macOS Dictation.");
                            return;
                        }
                        current->engine = [[AVAudioEngine alloc] init];
                        AVAudioInputNode* input = current->engine.inputNode;
                        AVAudioFormat* format = [input outputFormatForBus:0];
                        if (format.sampleRate <= 0 || format.channelCount == 0)
                        {
                            current->complete({}, "No microphone input. Type your command.");
                            return;
                        }
                        SFSpeechAudioBufferRecognitionRequest* request = [[SFSpeechAudioBufferRecognitionRequest alloc] init];
                        current->request = request;
                        request.requiresOnDeviceRecognition = YES;
                        request.shouldReportPartialResults = NO;
                        [input installTapOnBus:0 bufferSize:1024 format:format
                                        block:^(AVAudioPCMBuffer* buffer, AVAudioTime*) {
                            [request appendAudioPCMBuffer:buffer];
                        }];
                        current->tapped = true;
                        NSError* error = nil;
                        if (![current->engine startAndReturnError:&error])
                        {
                            current->complete({}, "Microphone could not start. Type your command.");
                            return;
                        }
                        current->task = [current->recognizer recognitionTaskWithRequest:request
                                                                          resultHandler:^(SFSpeechRecognitionResult* recognized, NSError* failure) {
                            if (!recognized.isFinal && !failure)
                                return;
                            dispatch_async(dispatch_get_main_queue(), ^{
                                if (!current->result)
                                    return;
                                const auto text = juce::String::fromUTF8(recognized.bestTranscription.formattedString.UTF8String);
                                current->complete(text, text.isNotEmpty()
                                    ? "Edit the transcript or select Preview."
                                    : "Speech could not be transcribed. Type your command or try again.");
                            });
                        }];
                        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 15 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
                            if (current->result && current->tapped)
                                current->complete({}, "Capture timed out. Type your command or try again.");
                        });
                    });
                }];
            });
        }];
        return;
    }
    current->complete({}, "Speech requires macOS 10.15 or later.");
}
