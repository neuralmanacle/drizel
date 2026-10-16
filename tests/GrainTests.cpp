#include "audio/Grain.h" // Unit under test; no JUCE or audio device needed.
#include "audio/Window.h" // Real production window rather than a mock.
#include <iostream> // Print failures for CTest.
#include <limits> // Construct invalid settings deliberately.
#include <stdexcept> // Fail even when assertions are disabled.
#include <type_traits> // Check fixed-state copy semantics at compile time.

void check(bool condition) // Small always-enabled assertion helper.
{
    if (!condition) throw std::runtime_error("Grain check failed");
}
void near(float actual, float expected) // Floating arithmetic needs tolerance.
{
    check(std::abs(actual - expected) < 0.00001f);
}
int main()
{
    static_assert(std::is_trivially_copyable<drizel::Grain>::value,
                  "Grain must remain trivially copyable");
    try
    {
        drizel::SampleBuffer source(2, 16, 24000); // Distinct stereo ramps expose channel bugs.
        for (std::size_t i = 0; i < 16; ++i)
        {
            source.channelData(0)[i] = static_cast<float>(i) / 16.0f;
            source.channelData(1)[i] = -static_cast<float>(i) / 32.0f;
        }
        const auto window = drizel::makeHannWindow(9); // Silent ends, centre unity.
        drizel::Grain grain; // Inactive grains must return silence.
        near(grain.next().left, 0);
        for (const auto mode : {drizel::InterpolationMode::linear, drizel::InterpolationMode::hermite})
            for (const double rate : {1.0, 0.5, -1.0, 0.0, 2.0})
            {
                check(grain.start(source, window.data(), 9, 2.25, rate, 0.5f, 0, mode));
                for (std::size_t i = 0; i < 9; ++i)
                {
                    check(grain.active()); // Includes both silent endpoint frames.
                    const auto value = grain.next();
                    const double position = 2.25 + static_cast<double>(i) * rate;
                    near(value.left, drizel::readSample(source.channelData(0), 16, position, mode) * window[i] * 0.5f);
                    near(value.right, drizel::readSample(source.channelData(1), 16, position, mode) * window[i] * 0.5f);
                    if (i == 0 || i == 8) near(value.left, 0); // Hann endpoints.
                }
                check(!grain.active()); // Exact duration regardless of rate/bounds.
                near(grain.next().right, 0); // Repeated reads remain safely silent.
            }
        const float unity[] = {1, 1, 1}; // Isolate timing and pan from envelope shape.
        check(grain.start(source, unity, 3, 4, 24000.0 / 48000.0));
        near(grain.next().left, 0.25f); // Same pitch across different sample rates.
        near(grain.next().left, 4.5f / 16); // Half-frame advance at twice output rate.
        check(grain.start(source, unity, 3, 8, 1)); // Retrigger before completion.
        near(grain.next().left, 0.5f); // New start must replace old position and age.
        check(grain.start(source, unity, 3, 15, 1));
        near(grain.next().left, 15.0f / 16); // Last valid source frame.
        near(grain.next().left, 0); // Beyond source is silence; duration continues.
        check(grain.active());
        drizel::SampleBuffer mono(1, 3, 48000); // Constant mono tests pan energy.
        for (std::size_t i = 0; i < 3; ++i) mono.channelData(0)[i] = 1;
        for (const float pan : {-1.0f, 0.0f, 1.0f})
        {
            check(grain.start(mono, unity, 3, 0, 1, 1, pan));
            const auto value = grain.next();
            near(value.left * value.left + value.right * value.right, 1);
            if (pan == -1) near(value.right, 0);
            if (pan == 1) near(value.left, 0);
            check(grain.start(source, unity, 3, 8, 1, 1, pan));
            const auto stereo = grain.next();
            near(stereo.left, pan == 1 ? 0 : 0.5f);
            near(stereo.right, pan == -1 ? 0 : -0.25f);
        }
        check(!grain.start(source, nullptr, 3, 0, 1)); // Invalid trigger cancels playback.
        check(!grain.active());
        check(!grain.start(source, unity, 0, 0, 1));
        check(!grain.start(source, unity, 3, std::numeric_limits<double>::infinity(), 1));
        check(!grain.start(source, unity, 3, 0, std::numeric_limits<double>::quiet_NaN()));
        check(!grain.start(source, unity, 3, 0, 1, 1, 2));
        check(!grain.start(source, unity, 3, 0, 1, -1));
        drizel::SampleBuffer empty(1, 0, 48000);
        check(!grain.start(empty, unity, 3, 0, 1));
        check(grain.start(mono, unity, 1, 0, 1)); // One-frame custom envelope policy.
        grain.next();
        check(!grain.active());
        std::cout << "All grain checks passed; state size " << sizeof(grain) << " bytes\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
