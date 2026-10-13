#include "audio/Interpolation.h"
#include "audio/SampleBuffer.h"
#include "audio/Window.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
void require(bool value, const std::string& message)
{
    if (!value)
        throw std::runtime_error(message);
}

void near(double actual, double expected, const std::string& message)
{
    require(std::isfinite(actual) && std::abs(actual - expected) <= 1.0e-6, message);
}

void smallWindows()
{
    require(drizel::makeHannWindow(0).empty(), "zero length");
    require(drizel::makeHannWindow(1) == std::vector<float>{1.0f}, "one frame");
    require(drizel::makeHannWindow(2) == std::vector<float>({0.0f, 0.0f}), "two frames");
    require(drizel::makeHannWindow(3) == std::vector<float>({0.0f, 1.0f, 0.0f}), "three frames");
}

void knownWindows()
{
    const auto odd = drizel::makeHannWindow(5);
    const std::array<float, 5> expected{0.0f, 0.5f, 1.0f, 0.5f, 0.0f};
    for (std::size_t i = 0; i < expected.size(); ++i)
        near(odd[i], expected[i], "five-frame reference");
    const auto even = drizel::makeHannWindow(4);
    near(even[1], 0.75, "even-length peak");
    near(even[2], 0.75, "even-length mirrored peak");
}

void windowProperties()
{
    for (std::size_t length = 2; length <= 1025; ++length)
    {
        const auto window = drizel::makeHannWindow(length);
        require(window.size() == length, "requested length");
        require(window.front() == 0.0f && window.back() == 0.0f, "exact endpoints");
        for (std::size_t n = 0; n < length; ++n)
        {
            require(window[n] >= 0.0f && window[n] <= 1.0f, "bounded envelope");
            require(window[n] == window[length - 1 - n], "exact symmetry");
            if (n > 0 && n <= (length - 1) / 2)
                require(window[n] >= window[n - 1], "rising first half");
        }
    }
}

void overlapConvention()
{
    // Symmetric odd N has a unit steady-state sum at hop (N-1)/2.
    const auto odd = drizel::makeHannWindow(9);
    for (std::size_t n = 0; n < 4; ++n)
        near(odd[n] + odd[n + 4], 1.0, "odd half-span overlap");
    // Symmetric even N is not a periodic Hann: N/2 hop is not unity.
    const auto even = drizel::makeHannWindow(4);
    near(even[0] + even[2], 0.75, "symmetric even overlap");
}

void integerAndFractionalReads()
{
    const std::array<float, 4> data{2.0f, -2.0f, 6.0f, 4.0f};
    for (std::size_t i = 0; i < data.size(); ++i)
        require(drizel::readLinear(data.data(), data.size(), static_cast<double>(i)) == data[i],
                "integer read exact");
    near(drizel::readLinear(data.data(), data.size(), 0.5), 0.0, "midpoint");
    near(drizel::readLinear(data.data(), data.size(), 1.25), 0.0, "quarter point");
    near(drizel::readLinear(data.data(), data.size(), 2.75), 4.5, "three-quarter point");
}

void hermiteInterpolation()
{
    // These samples lie on y=x^2 at x=-1,0,1,2,3. Catmull-Rom tangents
    // reproduce this quadratic exactly inside the segment from x=0 to x=1.
    const std::array<float, 5> quadratic{1.0f, 0.0f, 1.0f, 4.0f, 9.0f};
    near(drizel::readHermite(quadratic.data(), quadratic.size(), 1.25), 0.0625,
         "Hermite reconstructs a quadratic between samples");
    near(drizel::readSample(quadratic.data(), quadratic.size(), 1.25), 0.25,
         "default interpolation mode remains linear");
    near(drizel::readSample(quadratic.data(), quadratic.size(), 1.25,
                            drizel::InterpolationMode::hermite), 0.0625,
         "mode selector dispatches to Hermite");

    for (std::size_t i = 0; i < quadratic.size(); ++i)
        require(drizel::readHermite(quadratic.data(), quadratic.size(),
                                    static_cast<double>(i)) == quadratic[i],
                "integer positions preserve the stored sample exactly");

    const std::array<float, 5> ramp{0.0f, 2.0f, 4.0f, 6.0f, 8.0f};
    for (double position : {1.5, 2.5})
        near(drizel::readHermite(ramp.data(), ramp.size(), position), position * 2.0,
             "Hermite preserves an interior linear ramp");
}

void hermiteEdgesAndSafety()
{
    const std::array<float, 2> data{2.0f, 4.0f};
    near(drizel::readHermite(data.data(), data.size(), -1.0), 0.0,
         "Hermite is silent before zero-padded support");
    near(drizel::readHermite(data.data(), data.size(), 2.0), 0.0,
         "Hermite is silent after zero-padded support");
    require(std::isfinite(drizel::readHermite(data.data(), data.size(), -0.5)),
            "leading zero-padded interpolation is finite");
    require(std::isfinite(drizel::readHermite(data.data(), data.size(), 1.5)),
            "trailing zero-padded interpolation is finite");
    require(drizel::readHermite(nullptr, 2, 0.5) == 0.0f,
            "null channel returns silence");
    require(drizel::readHermite(data.data(), 0, 0.5) == 0.0f,
            "empty channel returns silence");
    require(drizel::readHermite(data.data(), data.size(),
                std::numeric_limits<double>::quiet_NaN()) == 0.0f,
            "non-finite position returns silence");

    const auto maximum = std::numeric_limits<float>::max();
    const std::array<float, 4> extremes{maximum, -maximum, maximum, -maximum};
    require(std::isfinite(drizel::readHermite(extremes.data(), extremes.size(), 0.25)),
            "finite float inputs cannot overflow the Hermite result");
}

void zeroPadding()
{
    const std::array<float, 2> data{2.0f, 4.0f};
    near(drizel::readLinear(data.data(), 2, -1.0), 0.0, "before first support");
    near(drizel::readLinear(data.data(), 2, -0.5), 1.0, "leading blend with silence");
    near(drizel::readLinear(data.data(), 2, 1.5), 2.0, "trailing blend with silence");
    near(drizel::readLinear(data.data(), 2, 2.0), 0.0, "after last support");
    const float one = 0.8f;
    near(drizel::readLinear(&one, 1, -0.5), 0.4, "one-frame left edge");
    near(drizel::readLinear(&one, 1, 0.0), 0.8, "one-frame centre");
    near(drizel::readLinear(&one, 1, 0.5), 0.4, "one-frame right edge");
}

void invalidReads()
{
    const float one = 1.0f;
    const double inf = std::numeric_limits<double>::infinity();
    for (const double p : {inf, -inf, std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()})
        require(drizel::readLinear(&one, 1, p) == 0.0f, "invalid/outside position");
    require(drizel::readLinear(nullptr, 10, 0.0) == 0.0f, "null channel");
    require(drizel::readLinear(&one, 0, 0.0) == 0.0f, "empty channel");
    near(drizel::readLinear(&one, 1, std::nextafter(1.0, 0.0)), 0.0, "just inside end");
    near(drizel::readLinear(&one, 1, std::nextafter(-1.0, 0.0)), 0.0, "just inside start");
}

void finiteHeadroom()
{
    const auto maximum = std::numeric_limits<float>::max();
    const std::array<float, 2> data{-maximum, maximum};
    near(drizel::readLinear(data.data(), 2, 0.5), 0.0, "no intermediate float overflow");
    require(drizel::readLinear(data.data(), 2, 0.75) == maximum * 0.5f, "finite headroom");
}

void stereoBufferReads()
{
    drizel::SampleBuffer buffer(2, 3, 48000.0);
    for (std::size_t n = 0; n < buffer.frameCount(); ++n)
    {
        buffer.channelData(0)[n] = static_cast<float>(n);
        buffer.channelData(1)[n] = -2.0f * static_cast<float>(n);
    }
    near(drizel::readLinear(buffer.channelData(0), buffer.frameCount(), 1.5), 1.5, "left channel");
    near(drizel::readLinear(buffer.channelData(1), buffer.frameCount(), 1.5), -3.0, "right channel");
}

void rateTraversal()
{
    const std::array<float, 4> ramp{0.0f, 2.0f, 4.0f, 6.0f};
    // Expectations come from the linear ramp, independently of interpolation.
    for (int i = 0; i <= 6; ++i)
    {
        near(drizel::readLinear(ramp.data(), 4, i * 0.5), i, "half-rate forward");
        near(drizel::readLinear(ramp.data(), 4, 3.0 - i * 0.5), 6 - i, "half-rate reverse");
    }
}
} // namespace

int main()
{
    struct Test { const char* name; void (*run)(); };
    const Test tests[]{
        {"small windows", smallWindows}, {"known windows", knownWindows},
        {"window properties", windowProperties}, {"overlap convention", overlapConvention},
        {"integer and fractional reads", integerAndFractionalReads},
        {"Hermite interpolation", hermiteInterpolation},
        {"Hermite edges and safety", hermiteEdgesAndSafety}, {"zero padding", zeroPadding},
        {"invalid reads", invalidReads}, {"finite headroom", finiteHeadroom},
        {"stereo buffer reads", stereoBufferReads}, {"rate traversal", rateTraversal}};
    int failed = 0;
    for (const auto& test : tests)
    {
        try { test.run(); std::cout << "PASS " << test.name << '\n'; }
        catch (const std::exception& error)
        {
            ++failed;
            std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
        }
    }
    return failed == 0 ? 0 : 1;
}
