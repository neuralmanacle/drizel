#pragma once
// Step 7: lightweight, JUCE-independent modulation primitives.
// All state belongs to the audio thread; construction and tick() allocate nothing.
#include <algorithm>
#include <cmath>

namespace drizel
{
enum class LfoShape { sine, triangle };
enum class ModDestination { off, position, pitch };

// Bipolar oscillator: -1..+1. Phase is stored in cycles, not radians.
class Lfo
{
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = std::isfinite(sampleRate) && sampleRate > 0 ? sampleRate : 48000.0;
        phase_ = 0.0;
    }

    void setRate(double hertz) noexcept
    {
        if (std::isfinite(hertz))
            rate_ = std::clamp(hertz, 0.05, 10.0);
    }
    void setShape(LfoShape shape) noexcept { shape_ = shape; }

    double next() noexcept
    {
        const double value = shape_ == LfoShape::triangle
            ? 1.0 - 4.0 * std::abs(phase_ - 0.5)
            : std::sin(6.28318530717958647692 * phase_);
        phase_ += rate_ / sampleRate_;
        phase_ -= std::floor(phase_); // Wrap without drifting during long sessions.
        return value;
    }

private:
    double sampleRate_ = 48000.0, rate_ = 1.0, phase_ = 0.0;
    LfoShape shape_ = LfoShape::sine;
};

struct ModulatedControls
{
    double position, pitch;
};

// Routes a bipolar source to exactly one parameter. The existing real-time
// engine smooths/clamps the combined target after routing.
class ModulationRouter
{
public:
    void set(ModDestination destination, double depth) noexcept
    {
        destination_ = destination;
        if (std::isfinite(depth))
            depth_ = std::clamp(depth, 0.0, 1.0);
    }

    ModulatedControls apply(double position, double pitch, double source) const noexcept
    {
        if (!std::isfinite(source))
            source = 0.0;
        const auto bipolar = std::clamp(source, -1.0, 1.0);
        // At full depth: position +/- 0.25 of the sample; pitch +/- 12 semitones.
        if (destination_ == ModDestination::position)
            position = std::clamp(position + bipolar * depth_ * 0.25, 0.0, 1.0);
        else if (destination_ == ModDestination::pitch)
            pitch = std::clamp(pitch + bipolar * depth_ * 12.0, -24.0, 24.0);
        return {position, pitch};
    }

private:
    ModDestination destination_ = ModDestination::off;
    double depth_ = 0.0;
};
} // namespace drizel
