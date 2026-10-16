#include "audio/Grain.h" // Fixed-size grain player and its source-buffer type.
#include "audio/Window.h" // Offline Hann table preparation.
#include "io/WavFile.h" // Offline file loading and writing.
#include <iostream> // Command-line status and error reporting.
#include <string> // Strict numeric argument conversion with std::stod.

// Parse an entire argument, rejecting trailing text and non-finite values.
double number(const char* text)
{
    const std::string value(text); // Own text on the non-real-time main thread.
    std::size_t end = 0; // stod reports the index after the parsed number.
    const double result = std::stod(value, &end); // Invalid numbers throw.
    if (end != value.size() || !std::isfinite(result))
        throw std::invalid_argument("Expected a finite number");
    return result; // Bounds depend on the argument and are checked below.
}

int main(int argc, char* argv[])
{
    if (argc != 8) // Validate before reading any positional arguments.
    {
        std::cerr << "Usage: drizel_single_grain input.wav output.wav start_ms length_ms semitones pan linear|hermite\n";
        return 1; // Shell and CTest can recognise a usage error.
    }
    try // File operations and preparation may allocate and throw.
    {
        const double startMs = number(argv[3]); // Offset measured in source time.
        const double lengthMs = number(argv[4]); // Duration measured in output time.
        const double semitones = number(argv[5]); // Twelve semitones double speed.
        const double pan = number(argv[6]); // -1 left, 0 centre, +1 right.
        const std::string modeName(argv[7]); // Explicit interpolation selection.
        if (startMs < 0 || lengthMs <= 0 || lengthMs > 10000
            || semitones < -48 || semitones > 48 || pan < -1 || pan > 1
            || (modeName != "linear" && modeName != "hermite"))
            throw std::invalid_argument("Check start, length (0..10000 ms), pitch (-48..48), pan (-1..1), and mode");
        const auto source = drizel::loadWav(argv[1]); // Own source for entire render.
        if (startMs >= source.durationSeconds() * 1000.0)
            throw std::invalid_argument("Start must be inside the source");
        constexpr double outputRate = 48000.0; // Deliberately exercise rate conversion.
        const auto length = static_cast<std::size_t>(std::llround(lengthMs * outputRate / 1000.0));
        if (length < 3) // Shorter symmetric Hann windows cannot fade audibly.
            throw std::invalid_argument("Use at least three output frames");
        const auto window = drizel::makeHannWindow(length); // Allocate before playback.
        drizel::SampleBuffer output(2, length, outputRate); // Preallocate stereo output.
        drizel::Grain grain; // Default state is inactive and contains no heap storage.
        const double rate = std::pow(2.0, semitones / 12.0) * source.sampleRate() / outputRate;
        const auto mode = modeName == "hermite" ? drizel::InterpolationMode::hermite
                                               : drizel::InterpolationMode::linear;
        if (!grain.start(source, window.data(), length, startMs * source.sampleRate() / 1000.0,
                         rate, 0.5f, static_cast<float>(pan), mode))
            throw std::runtime_error("Grain settings rejected");
        for (std::size_t frame = 0; frame < length; ++frame) // No allocation in this loop.
        {
            const auto sample = grain.next(); // Interpolate, window, pan, then advance.
            output.channelData(0)[frame] = sample.left; // Write left output plane.
            output.channelData(1)[frame] = sample.right; // Write right output plane.
        }
        drizel::writeWav(argv[2], output); // Float WAV; refuses an existing destination.
        std::cout << "Saved " << length << " frames at 48000 Hz to " << argv[2] << '\n';
        return 0; // Successful render, not a claim of a listening check.
    }
    catch (const std::exception& error) // Report preparation or I/O failure clearly.
    {
        std::cerr << error.what() << '\n'; // Console work stays outside the DSP loop.
        return 1; // Let scripts detect failure.
    }
}
