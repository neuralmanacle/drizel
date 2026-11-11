#pragma once

#include "SampleBuffer.h"
#include <algorithm>
#include <array>
#include <complex>

namespace drizel
{
// Step 10 defaults. All indices and frequencies refer to the decoded source.
class SampleFeatures
{
public:
    static constexpr std::size_t size = 2048;
    static constexpr std::size_t hop = 512;
    static constexpr double silence = 1e-4;

    struct Frame
    {
        double rms;
        double onset;
        double centroid;
    };

    explicit SampleFeatures(const SampleBuffer& source) : source_(source)
    {
        constexpr double tau = 6.28318530717958647692;
        for (std::size_t i = 0; i < size; ++i)
        {
            window_[i] = 0.5 * (1.0 - std::cos(tau * i / size));
            normalization_ += window_[i] * window_[i];
        }
        normalization_ *= size * source_.channelCount();
        for (std::size_t i = 0; i < size / 2; ++i)
            twiddles_[i] = std::polar(1.0, -tau * i / size);
        for (std::size_t i = 1; i < size; ++i)
            reversed_[i] = (reversed_[i / 2] >> 1) | ((i & 1) * (size / 2));
    }

    // Call in ascending hop order so spectral flux compares adjacent frames.
    Frame next(std::size_t center)
    {
        const auto left = std::min(center, size / 2);
        const auto right = std::min(source_.frameCount() - center, size / 2);
        const auto begin = center - left;
        const auto count = left + right;
        std::array<double, size / 2 + 1> power {};
        double squareSum = 0.0;
        for (std::size_t channel = 0; channel < source_.channelCount(); ++channel)
        {
            const auto* samples = source_.channelData(channel);
            transform_.fill({});
            for (std::size_t i = 0; i < count; ++i)
            {
                const double value = samples[begin + i];
                const auto n = size / 2 - left + i;
                squareSum += value * value;
                transform_[reversed_[n]] = value * window_[n];
            }
            fft();
            for (std::size_t k = 0; k < power.size(); ++k)
                power[k] += std::norm(transform_[k]);
        }
        const double rms = std::sqrt(squareSum / (count * source_.channelCount()));
        double total = 0.0, moment = 0.0, flux = 0.0;
        for (std::size_t k = 0; k < power.size(); ++k)
        {
            const double bin = rms <= silence ? 0.0
                : power[k] * (k == 0 || k == size / 2 ? 1.0 : 2.0) / normalization_;
            total += bin;
            moment += k * bin;
            const double magnitude = std::sqrt(bin);
            const double increase = std::max(0.0, magnitude - previous_[k]);
            flux += increase * increase;
            previous_[k] = magnitude;
        }
        return {rms, center == 0 ? 0.0 : std::sqrt(flux),
                total > 0.0 ? (moment / total) * (source_.sampleRate() / size) : 0.0};
    }

private:
    void fft()
    {
        for (std::size_t span = 2; span <= size; span *= 2)
        {
            const auto half = span / 2;
            for (std::size_t base = 0; base < size; base += span)
                for (std::size_t i = 0; i < half; ++i)
                {
                    const auto even = transform_[base + i];
                    const auto odd = transform_[base + i + half] * twiddles_[i * (size / span)];
                    transform_[base + i] = even + odd;
                    transform_[base + i + half] = even - odd;
                }
        }
    }

    const SampleBuffer& source_;
    std::array<std::complex<double>, size> transform_ {};
    std::array<std::complex<double>, size / 2> twiddles_ {};
    std::array<std::size_t, size> reversed_ {};
    std::array<double, size> window_ {};
    std::array<double, size / 2 + 1> previous_ {};
    double normalization_ = 0.0;
};
} // namespace drizel
