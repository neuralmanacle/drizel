#include "audio/LiveGranulator.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

static void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    try
    {
        drizel::LiveGranulator effect;
        effect.prepare(48000.0);
        constexpr double pi = 3.14159265358979323846;
        double peak = 0.0;
        double startupPeak = 0.0;
        for (int i = 0; i < 96000; ++i)
        {
            const auto input = static_cast<float>(0.35 * std::sin(2.0 * pi * 220.0 * i / 48000.0));
            const auto result = effect.process(input, -input, 0.5, 0.0, 40.0, 0.8);
            check(std::isfinite(result.left) && std::isfinite(result.right),
                  "granular output must remain finite");
            if (i < 16000)
                startupPeak = std::max(startupPeak, static_cast<double>(std::abs(result.left)));
            if (i > 48000)
            {
                peak = std::max(peak, static_cast<double>(std::abs(result.left)));
                check(std::abs(result.left + result.right) < 1.0e-4f,
                      "stereo channels should preserve opposite-phase input");
            }
        }
        check(startupPeak == 0.0, "live effect must wait for history before reading");
        check(peak > 0.001, "incoming DAW audio should produce granular output");

        effect.prepare(44100.0);
        for (int i = 0; i < 44100; ++i)
        {
            const auto result = effect.process(0.0f, 0.0f, 1.0, 12.0, 60.0, 1.0);
            check(result.left == 0.0f && result.right == 0.0f,
                  "silence must remain silent");
        }
        std::cout << "Live granular DAW input tests passed\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
