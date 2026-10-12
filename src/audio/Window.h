#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

namespace drizel
{
/**
 * Build a symmetric Hann amplitude envelope, offline or during preparation.
 * For N >= 2: w[n] = 0.5 * (1 - cos(2*pi*n/(N-1))).
 *
 * The table owns its storage and allocates here. A future grain can borrow it
 * and multiply each output frame by table[age], without allocation or cosine
 * calls in the audio callback. Keep the table alive and unchanged while read.
 *
 * Length 0 returns an empty table; length 1 returns unity (no fade possible).
 * Length 2 returns two zeros. Practical audible grains need at least 3 frames.
 * Both endpoints are exactly zero for N >= 2. This is the symmetric envelope,
 * not the periodic Hann used for an even-length, half-hop overlap sum of one.
 * Allocation failures propagate to the caller. Never build tables on the
 * audio thread; indexing a prepared table is the real-time operation.
 */
inline std::vector<float> makeHannWindow(std::size_t length)
{
    std::vector<float> window(length, 0.0f);
    if (length == 0)
        return window;
    if (length == 1)
    {
        window[0] = 1.0f;
        return window;
    }

    constexpr double pi = 3.14159265358979323846;
    // Compute one half and mirror it: symmetry is exact even after rounding.
    // Index zero and its partner remain explicitly zero.
    for (std::size_t n = 1; n < length / 2; ++n)
    {
        const auto phase = static_cast<double>(n) / static_cast<double>(length - 1);
        const auto value = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * pi * phase)));
        window[n] = value;
        window[length - 1 - n] = value;
    }
    if (length % 2 != 0)
        window[length / 2] = 1.0f;
    return window;
}
} // namespace drizel
