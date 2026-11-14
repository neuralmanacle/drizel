#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace drizel
{
enum class MovementPattern : std::uint8_t { manual, cyclic, stepped, irregular };

struct MovementPolicy
{
    MovementPattern pattern = MovementPattern::manual;
    int start = 5, end = 90, interval = 5, step = 18, distanceVariation = 0, timingVariation = 0;

    bool valid() const noexcept
    {
        return pattern >= MovementPattern::manual && pattern <= MovementPattern::irregular
            && start >= 0 && start < end && end <= 100 && interval >= 1 && interval <= 200
            && step >= 1 && step <= 50 && distanceVariation >= 0 && distanceVariation <= 90
            && timingVariation >= 0 && timingVariation <= 90;
    }

    // Publish the entire numeric policy in one lock-free word.
    std::uint64_t packed() const noexcept
    {
        return static_cast<std::uint64_t>(pattern) | (std::uint64_t(start) << 8)
            | (std::uint64_t(end) << 16) | (std::uint64_t(interval) << 24)
            | (std::uint64_t(step) << 32) | (std::uint64_t(distanceVariation) << 40)
            | (std::uint64_t(timingVariation) << 48);
    }

    static MovementPolicy unpack(std::uint64_t word) noexcept
    {
        return {static_cast<MovementPattern>(word & 255), int((word >> 8) & 255),
            int((word >> 16) & 255), int((word >> 24) & 255), int((word >> 32) & 255),
            int((word >> 40) & 255), int((word >> 48) & 255)};
    }
};

class GrainMovement
{
public:
    void prepare(double rate) noexcept { sampleRate = std::max(1.0, rate); }
    void accept(std::uint64_t word) noexcept
    {
        if (word == current)
            return;
        current = word;
        policy = MovementPolicy::unpack(word);
        position = policy.pattern == MovementPattern::cyclic ? 0.0 : 0.5;
        phaseIncrement = 20.0 / (sampleRate * policy.interval);
        untilMove = sampleRate * policy.interval / 20.0;
        direction = 0;
        seed = 12013;
    }

    double next(double manual) noexcept
    {
        if (policy.pattern == MovementPattern::manual)
            return manual;
        const auto result = (policy.start + (policy.end - policy.start) * position) / 100.0;
        if (policy.pattern == MovementPattern::cyclic)
        {
            position = wrap(position + phaseIncrement);
        }
        else if (--untilMove <= 0.0)
        {
            const auto step = policy.step / 100.0;
            if (policy.pattern == MovementPattern::stepped)
            {
                constexpr std::array<int, 3> directions {1, -1, -1};
                position = wrap(position + directions[direction] * step
                                * (1.0 + bipolar() * policy.distanceVariation / 100.0));
                direction = (direction + 1) % directions.size();
            }
            else
            {
                position = wrap(position + 2.0 * step * bipolar());
            }
            untilMove += sampleRate * policy.interval / 20.0
                         * (1.0 + bipolar() * policy.timingVariation / 100.0);
        }
        return result;
    }

private:
    static double wrap(double value) noexcept { return value - std::floor(value); }
    double bipolar() noexcept
    {
        seed = seed * 1664525u + 1013904223u;
        return seed / 2147483648.0 - 1.0;
    }

    MovementPolicy policy;
    std::uint64_t current = 0;
    double sampleRate = 48000.0, position = 0.5, phaseIncrement = 0.0, untilMove = 0.0;
    std::size_t direction = 0;
    std::uint32_t seed = 12013;
};
}
