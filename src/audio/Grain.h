#pragma once // Include this definition once per translation unit.

#include "Interpolation.h" // Reuse the selected fractional sample reader.
#include "SampleBuffer.h" // Borrow source storage without JUCE dependencies.
#include <cmath> // isfinite validates settings; sqrt prepares pan gains.
#include <cstddef> // std::size_t represents frame counts and ages.

namespace drizel
{
// One output frame; mono sources become stereo through equal-power panning.
struct GrainFrame
{
    float left = 0.0f; // Default silence also represents an inactive grain.
    float right = 0.0f; // Stereo sources retain separate channel content.
};

// Fixed-size, non-owning state. Source and window must outlive playback and
// remain unchanged. Do not copy/move/resize their storage while a grain uses it.
// start() and next() allocate nothing, lock nothing, and perform no file I/O.
// Source samples must be finite; window values must be finite and in [0,1].
// Very large sample values can overflow when gain is applied; no limiter here.
class Grain
{
public:
    // rate is SOURCE frames per OUTPUT frame: pitchRatio * sourceHz/outputHz.
    // Negative rates play backwards; zero freezes the read head under the window.
    // length is OUTPUT frames and must match the caller-owned window allocation.
    // gain is [0,1], pan is [-1,1]. Stereo uses balance: centre preserves L/R,
    // extremes mute the opposite channel rather than downmixing the whole source.
    bool start(const SampleBuffer& source, const float* window, std::size_t length,
               double position, double rate, float gain = 1.0f, float pan = 0.0f,
               InterpolationMode mode = InterpolationMode::linear) noexcept
    {
        remaining_ = 0; // Failed retrigger stops the previous grain safely.
        if (source.frameCount() == 0 || window == nullptr || length == 0
            || !std::isfinite(position) || !std::isfinite(rate)
            || !std::isfinite(gain) || gain < 0.0f || gain > 1.0f
            || !std::isfinite(pan) || pan < -1.0f || pan > 1.0f)
            return false; // Validate scalars before borrowing any channel.
        left_ = source.channelData(0); // Borrow the first planar channel.
        const bool mono = source.channelCount() == 1; // Choose pan policy once.
        right_ = mono ? left_ : source.channelData(1); // Mono shares its source.
        frames_ = source.frameCount(); // Bounds supplied to the interpolator.
        window_ = window; // No table copy or allocation at trigger time.
        remaining_ = length; // Exact duration, including silent endpoints.
        age_ = 0; // Restart the amplitude envelope at its first entry.
        position_ = position; // Fractional starting positions are supported.
        rate_ = rate; // Preserve direction and source/output rate correction.
        mode_ = mode; // Both existing interpolation modes use the same player.
        leftGain_ = gain * std::sqrt(mono ? (1.0f - pan) * 0.5f
                                               : (pan > 0.0f ? 1.0f - pan : 1.0f));
        rightGain_ = gain * std::sqrt(mono ? (1.0f + pan) * 0.5f
                                                : (pan < 0.0f ? 1.0f + pan : 1.0f));
        return true; // Square roots occur only at trigger, not per sample.
    }

    bool active() const noexcept { return remaining_ != 0; } // No mutation.

    GrainFrame next() noexcept // Render one frame; caller chooses block size.
    {
        if (!active())
            return {}; // Value-initialised silence without touching borrowed data.
        const float envelope = window_[age_]; // Age advances in output time.
        const GrainFrame result {
            readSample(left_, frames_, position_, mode_) * envelope * leftGain_,
            readSample(right_, frames_, position_, mode_) * envelope * rightGain_
        }; // Source boundaries are zero padded by the existing interpolators.
        ++age_; // Advance the window independently of pitch or direction.
        --remaining_; // Deactivate after exactly length output frames.
        position_ += rate_; // Move the source head, possibly by a fractional step.
        return result; // Return values, never a reference to temporary storage.
    }

private:
    const float* left_ = nullptr; // Borrowed first source channel.
    const float* right_ = nullptr; // Borrowed second channel or mono alias.
    const float* window_ = nullptr; // Borrowed amplitude table.
    std::size_t frames_ = 0; // Source length in frames.
    std::size_t remaining_ = 0; // Zero is also the inactive flag.
    std::size_t age_ = 0; // Current output-frame index into the window.
    double position_ = 0.0; // Current fractional source-frame position.
    double rate_ = 1.0; // Signed source-frame increment per output frame.
    float leftGain_ = 0.0f; // Prepared pan and amplitude multiplier.
    float rightGain_ = 0.0f; // Prepared pan and amplitude multiplier.
    InterpolationMode mode_ = InterpolationMode::linear; // Default reader.
};
} // namespace drizel
