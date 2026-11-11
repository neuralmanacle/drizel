#pragma once

#include "SampleFeatures.h"

namespace drizel
{
enum class GrainSelection { manual, attack, sustain, audible, bright };

// Built and owned by the same packet as the source. Playback only reads tables.
class SampleSelection
{
public:
    template <typename Cancelled>
    bool prepare(const SampleBuffer& source, Cancelled cancelled)
    {
        regions_.clear();
        if (source.frameCount() == 0)
            return true;
        const auto frameCount = 1 + (source.frameCount() - 1) / SampleFeatures::hop;
        regions_.resize(std::clamp(frameCount / 32, std::size_t{1}, std::size_t{16}));
        sourceFrames_ = source.frameCount();
        const auto framesPerRegion = divideUp(frameCount, regions_.size());
        framesPerRegion_ = framesPerRegion;
        const auto framesPerBucket = divideUp(framesPerRegion, 128);
        const double attackLead = source.sampleRate() * 0.025;
        SampleFeatures features(source);
        std::array<Candidate, 4> best {};
        double audibleHops = 0.0;
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            if (cancelled())
            {
                regions_.clear();
                return false;
            }
            const auto center = index * SampleFeatures::hop;
            const auto frame = features.next(center);
            const auto local = index % framesPerRegion;
            if (frame.rms > SampleFeatures::silence)
            {
                ++audibleHops;
                const double relativeFlux = frame.onset / frame.rms;
                const double brightness = frame.centroid / (source.sampleRate() * 0.5);
                const std::array<double, 4> weights {
                    relativeFlux >= 0.2 ? frame.onset : 0.0,
                    index > 0 && relativeFlux < 0.2 ? frame.rms * (1.0 - relativeFlux / 0.2) : 0.0,
                    1.0,
                    brightness >= 0.05 ? brightness * brightness : 0.0
                };
                for (std::size_t mode = 0; mode < best.size(); ++mode)
                    if (weights[mode] > best[mode].weight)
                        best[mode] = {std::max(0.0, static_cast<double>(center) - (mode == 0 ? attackLead : 0.0)),
                                      weights[mode]};
            }
            if ((local + 1) % framesPerBucket == 0 || local + 1 == framesPerRegion || index + 1 == frameCount)
            {
                auto& region = regions_[index / framesPerRegion];
                for (std::size_t mode = 0; mode < best.size(); ++mode)
                {
                    auto& table = region[mode];
                    if (best[mode].weight > 0.0)
                    {
                        // Audible mode samples buckets by their eligible hop count.
                        const auto weight = mode == 2 ? audibleHops : best[mode].weight;
                        const auto previous = table.empty() ? 0.0 : table.back().weight;
                        table.push_back({best[mode].start, previous + weight});
                    }
                }
                best = {};
                audibleHops = 0;
            }
        }
        return true;
    }

    double start(GrainSelection mode, double manualStart, double draw) const noexcept
    {
        const auto slot = static_cast<std::size_t>(mode);
        if (slot == 0 || slot > 4 || regions_.empty())
            return manualStart;
        const auto sourceFrame = static_cast<std::size_t>(
            std::clamp(manualStart, 0.0, static_cast<double>(sourceFrames_ - 1)));
        const auto& table = regions_[sourceFrame / SampleFeatures::hop / framesPerRegion_][slot - 1];
        if (table.empty())
            return manualStart;
        const auto selected = std::upper_bound(table.begin(), table.end(), draw * table.back().weight,
            [](double target, const Candidate& entry) { return target < entry.weight; });
        return selected == table.end() ? table.back().start : selected->start;
    }

private:
    struct Candidate { double start = 0.0; double weight = 0.0; };
    static std::size_t divideUp(std::size_t value, std::size_t divisor) noexcept
    {
        return 1 + (value - 1) / divisor;
    }

    std::vector<std::array<std::vector<Candidate>, 4>> regions_;
    std::size_t framesPerRegion_ = 1;
    std::size_t sourceFrames_ = 0;
};
} // namespace drizel
