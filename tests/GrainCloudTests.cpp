#include "audio/GrainCloud.h" // Production scheduler under test.
#include <iostream> // Human-readable test result.
#include <limits> // Nonfinite validation fixture.
#include <vector> // Offline reference and replay storage.

void check(bool value, const char* message) // Works with NDEBUG release builds.
{
    if (!value)
        throw std::runtime_error(message); // Stop on the first violated contract.
}

int main()
{
    try
    {
        drizel::SampleBuffer source(2, 4096, 1000); // Stereo preserves centre gain.
        for (std::size_t n = 0; n < source.frameCount(); ++n)
        {
            source.channelData(0)[n] = 1.0f; // Constant source exposes window sums.
            source.channelData(1)[n] = -1.0f; // Opposite sign checks stereo separation.
        }
        drizel::CloudSettings s; // Start with deterministic, unvaried parameters.
        s.outputRate = 1000; s.density = 200; s.length = 10; s.gain = 1;
        drizel::GrainCloud<8> cloud; // More slots than the two-grain overlap.
        check(cloud.next().left == 0, "unprepared silence");
        cloud.prepare(source, s); // Five-frame hop and ten-frame Hann.
        const auto window = drizel::makeHannWindow(10); // Independent overlap reference.
        for (std::size_t n = 0; n < 100; ++n)
        {
            double expected = 0; // Explicitly sum envelopes launched every five frames.
            for (std::size_t start = 0; start <= n; start += 5)
                if (n - start < 10)
                    expected += window[n - start] * 0.5; // Bound: two overlaps.
            const auto frame = cloud.next(); // Compare each channel to the reference.
            check(std::abs(frame.left - expected) < 1e-6, "overlap-add reference");
            check(std::abs(frame.right + expected) < 1e-6, "stereo separation");
        }
        check(cloud.started() == 20 && cloud.maximumActive() == 2, "integer scheduling");
        s.density = 300; // Noninteger interval: 3 1/3 output frames.
        cloud.prepare(source, s);
        for (int n = 0; n < 1000; ++n) cloud.next(); // Exactly one output second.
        check(cloud.started() == 300, "fractional scheduling drift");

        drizel::GrainCloud<2> small; // Deliberately force pool saturation.
        s.density = 1000; s.length = 100; // One event per frame, hundred-frame grains.
        small.prepare(source, s);
        for (int n = 0; n < 10; ++n) small.next();
        check(small.started() == 2 && small.dropped() == 8, "drop-new accounting");
        check(small.maximumActive() == 2, "capacity bound");
        for (int n = 0; n < 100; ++n) small.next(false); // Drain without new events.
        check(small.activeCount() == 0 && small.next(false).left == 0, "tail drain");

        s.density = 137; s.length = 100; s.lengthSpread = 20; // Vary every parameter.
        s.position = 100; s.positionSpread = 100; s.pitchSpread = 12;
        s.panSpread = 1; s.jitter = 0.95; s.seed = 0;
        s.mode = drizel::InterpolationMode::hermite; // Exercise overshoot headroom.
        for (std::size_t n = 0; n < source.frameCount(); ++n)
            source.channelData(0)[n] = (n % 4 < 2) ? 4.0f : -4.0f; // Peaks above unity.
        cloud.prepare(source, s);
        std::vector<drizel::GrainFrame> replay; // Test-only allocation, outside next().
        for (int n = 0; n < 10000; ++n)
        {
            const auto frame = cloud.next(); // Seeded randomized render.
            check(std::isfinite(frame.left) && std::abs(frame.left) <= s.gain + 1e-6f
                  && std::abs(frame.right) <= s.gain + 1e-6f, "peak bound");
            check(cloud.activeCount() <= 8, "randomized capacity");
            replay.push_back(frame); // Save exact float results.
        }
        cloud.prepare(source, s); // Same settings and seed must reset everything.
        for (const auto& expected : replay)
        {
            const auto frame = cloud.next();
            check(frame.left == expected.left && frame.right == expected.right, "seed replay");
        }
        ++s.seed; cloud.prepare(source, s); // A different seed should change the texture.
        bool different = false;
        for (const auto& expected : replay)
        {
            const auto frame = cloud.next();
            different = different || frame.left != expected.left || frame.right != expected.right;
        }
        check(different, "different seeds");
        s.density = std::numeric_limits<double>::quiet_NaN(); // Invalid reprepare.
        bool rejected = false;
        try { cloud.prepare(source, s); } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected && cloud.next().left == 0 && cloud.activeCount() == 0, "failed prepare stops");
        std::cout << "Cloud timing, overlap, capacity, tail, peak, randomness and validation passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n'; // Report the failed contract.
        return 1;
    }
}
