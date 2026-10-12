#pragma once

#include <cmath>
#include <cstddef>

namespace drizel
{
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
} // namespace drizel
