#include "audio/GrainCloud.h" // Scheduler, pool and overlap mixer.
#include "io/WavFile.h" // Offline WAV input/output.
#include <chrono> // Measure render time, excluding loading and preparation.
#include <iostream> // Report diagnostics outside the rendering loop.
#include <string> // Parse whole numeric arguments.

double number(const char* text) // CLI parsing may allocate and throw.
{
    const std::string value(text); // Own the argument text.
    std::size_t end = 0; // Index following the parsed number.
    const double result = std::stod(value, &end); // Reject missing numeric text.
    if (end != value.size() || !std::isfinite(result))
        throw std::invalid_argument("Expected a finite number"); // No trailing junk.
    return result; // Caller applies parameter-specific limits.
}

int main(int argc, char* argv[])
{
    if (argc != 7) // Check argument count before accessing argv.
    {
        std::cerr << "Usage: drizel_grain_cloud input.wav output.wav seconds density start_ms seed\n";
        return 1; // Usage errors are visible to shell scripts.
    }
    try // All file I/O and memory preparation occurs on this offline thread.
    {
        const double seconds = number(argv[3]); // Triggering duration, excluding tail.
        const double density = number(argv[4]); // Nominal grains per second.
        const double startMs = number(argv[5]); // Centre position in source time.
        const double seed = number(argv[6]); // Integer reproducibility key.
        if (seconds <= 0 || seconds > 60 || density <= 0 || density > 1000
            || startMs < 0 || seed < 0 || seed > 4294967295.0 || std::floor(seed) != seed)
            throw std::invalid_argument("Use seconds (0,60], density (0,1000], start >= 0 and a uint32 seed");
        const auto source = drizel::loadWav(argv[1]); // Keep storage alive for the cloud.
        drizel::CloudSettings settings; // Defaults use 48 kHz stereo output.
        settings.density = density; // Control overlap through the launch interval.
        settings.position = startMs * source.sampleRate() / 1000.0; // Source frames.
        settings.positionSpread = std::min(source.sampleRate() * 0.05,
                                           static_cast<double>(source.frameCount())); // +/-50 ms.
        settings.lengthSpread = 1200; // 75, 100 or 125 ms at 48 kHz.
        settings.pitchSpread = 3.0; // +/-3 semitones.
        settings.panSpread = 0.75f; // Spread voices across the stereo field.
        settings.jitter = 0.2; // +/-20 percent of the nominal interval.
        settings.seed = static_cast<std::uint32_t>(seed); // Validated integer cast.
        drizel::GrainCloud<> cloud; // Default capacity is 64 reusable voices.
        cloud.prepare(source, settings); // Scan peak, validate and build windows.
        const auto frames = static_cast<std::size_t>(std::ceil(seconds * settings.outputRate));
        const auto tail = settings.length + settings.lengthSpread; // Longest envelope.
        drizel::SampleBuffer output(2, frames + tail, settings.outputRate); // Preallocate.
        const auto begin = std::chrono::steady_clock::now(); // Start render measurement.
        double peak = 0.0; // Track actual output peak for inspection.
        for (std::size_t frame = 0; frame < output.frameCount(); ++frame)
        {
            const auto sample = cloud.next(frame < frames); // Stop triggers, then drain.
            output.channelData(0)[frame] = sample.left; // Write stereo planar storage.
            output.channelData(1)[frame] = sample.right; // No allocation here.
            peak = std::max(peak, static_cast<double>(std::max(std::abs(sample.left), std::abs(sample.right))));
        }
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
        drizel::writeWav(argv[2], output); // Existing destinations are refused.
        std::cout << "Started " << cloud.started() << ", dropped " << cloud.dropped()
                  << ", max active " << cloud.maximumActive() << ", peak " << peak
                  << ", render seconds " << elapsed << '\n'; // Offline diagnostics.
        return 0; // Success; listening remains a separate check.
    }
    catch (const std::exception& error) // Report invalid input and I/O failures.
    {
        std::cerr << error.what() << '\n'; // Console I/O never occurs inside next().
        return 1; // Propagate failure to the caller.
    }
}
