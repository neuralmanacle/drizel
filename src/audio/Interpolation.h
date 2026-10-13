#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace drizel
{
/** Selects how a fractional source-frame position is reconstructed. */
enum class InterpolationMode
{
    linear, ///< Blend the two samples surrounding the read position.
    hermite ///< Use four samples to form a cubic Hermite (Catmull-Rom) curve.
};

/**
 * Read a channel at a fractional source-frame position using linear blending.
 * Example: position 3.25 is 75% of frame 3 plus 25% of frame 4.
 *
 * The discrete signal is zero-padded: samples outside [0, frames) are zero.
 * Consequently -0.5 blends zero with frame 0, and frames-0.5 blends the last
 * frame with zero. Positions <= -1 or >= frames produce silence. Nothing wraps
 * or clamps to an edge sample. A null/empty channel or non-finite position also
 * returns silence before any indexing or floating-to-integer conversion.
 *
 * Non-null data must point to at least frames readable, finite float samples.
 * The caller owns their lifetime and prevents concurrent writes. Borrow a
 * SampleBuffer channel using channelData(channel) and frameCount().
 * No allocation, locks, exceptions, file I/O, or state changes occur here.
 * This is an interpolator, not a band-limited sample-rate converter.
 */
inline float readLinear(const float* data, std::size_t frames, double position) noexcept
{
    if (data == nullptr || frames == 0 || !std::isfinite(position)
        || position <= -1.0 || position >= static_cast<double>(frames))
        return 0.0f;

    if (position < 0.0)
        return static_cast<float>((position + 1.0) * static_cast<double>(data[0]));

    const auto left = static_cast<std::size_t>(position); // Nonnegative: truncation = floor.
    const auto fraction = position - static_cast<double>(left);
    // Besides preserving integer reads, this avoids consulting the next frame
    // unnecessarily. The preceding bounds check guarantees left < frames.
    if (fraction == 0.0)
        return data[left];
    const double a = data[left];
    const double b = left + 1 < frames ? data[left + 1] : 0.0;
    // Double intermediates prevent overflow when blending opposite large
    // finite floats. With fraction in [0,1], the result stays between a and b.
    return static_cast<float>((1.0 - fraction) * a + fraction * b);
}

namespace detail
{
/** Apply the Catmull-Rom form of a cubic Hermite segment and keep float output finite. */
inline float cubicHermite(double p0, double p1, double p2, double p3, double t) noexcept
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    // Catmull-Rom uses centred differences for endpoint tangents. This is the
    // cubic Hermite polynomial expanded into its four sample-value weights.
    const double value = 0.5 * ((2.0 * p1)
        + (-p0 + p2) * t
        + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2
        + (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
    // Cubic curves may overshoot their samples. Saturate only values beyond
    // float's representable range so extreme finite inputs cannot become infinity.
    const double limit = static_cast<double>(std::numeric_limits<float>::max());
    return static_cast<float>(std::clamp(value, -limit, limit));
}
} // namespace detail

/**
 * Read a channel at a fractional source-frame position using four-point cubic
 * Hermite interpolation. The two samples around the read position define the
 * segment; their outer neighbors define its centred-difference tangents.
 * Integer positions return the original sample exactly. Missing neighbors use
 * zero padding, and positions outside (-1, frames) return silence, like readLinear.
 *
 * This preserves more high-frequency content than linear interpolation in many
 * cases, though cubic interpolation can overshoot. It is not band-limited
 * resampling. The caller's storage, lifetime, and concurrency requirements are
 * identical to readLinear; this function allocates and locks nothing.
 */
inline float readHermite(const float* data, std::size_t frames, double position) noexcept
{
    if (data == nullptr || frames == 0 || !std::isfinite(position)
        || position <= -1.0 || position >= static_cast<double>(frames))
        return 0.0f;

    // The only valid negative positions lie between virtual zero frame -1 and frame 0.
    if (position < 0.0)
    {
        const double fraction = position + 1.0;
        const double next = frames > 1 ? static_cast<double>(data[1]) : 0.0;
        return detail::cubicHermite(0.0, 0.0, static_cast<double>(data[0]), next, fraction);
    }

    const auto left = static_cast<std::size_t>(position); // Bounds above make this a valid frame.
    const double fraction = position - static_cast<double>(left);
    if (fraction == 0.0)
        return data[left];

    // Read the four-point neighborhood, substituting zero outside the buffer.
    const double previous = left > 0 ? static_cast<double>(data[left - 1]) : 0.0;
    const double current = static_cast<double>(data[left]);
    const double next = left < frames - 1 ? static_cast<double>(data[left + 1]) : 0.0;
    const double afterNext = frames - left > 2 ? static_cast<double>(data[left + 2]) : 0.0;
    return detail::cubicHermite(previous, current, next, afterNext, fraction);
}

/** Dispatch a fractional read to the selected interpolation method. */
inline float readSample(const float* data, std::size_t frames, double position,
                        InterpolationMode mode = InterpolationMode::linear) noexcept
{
    switch (mode)
    {
        case InterpolationMode::hermite:
            return readHermite(data, frames, position);
        case InterpolationMode::linear:
        default:
            return readLinear(data, frames, position);
    }
}
} // namespace drizel
