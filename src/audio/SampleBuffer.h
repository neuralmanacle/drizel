#pragma once

/**
 * @file SampleBuffer.h
 * @brief Framework-independent storage for one decoded mono or stereo sample.
 *
 * JUCE types are deliberately absent: future grain readers can depend on this
 * header without depending on file formats, a GUI, or an audio-device API.
 */

#include <cassert>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace drizel
{
/**
 * @brief Owns floating-point audio in a contiguous, planar allocation.
 *
 * A sample is one amplitude value. A frame contains one sample per channel at
 * the same instant. For stereo audio, the vector contains every left-channel
 * frame first, followed by every right-channel frame. A value at (channel,
 * frame) therefore lives at channel * frameCount() + frame in the vector.
 *
 * This is a value type: the compiler-generated copy operations copy the sample
 * data, and move operations transfer vector storage. After moving from a
 * buffer, only destroy it or assign it a new value; its scalar fields are not
 * reset to describe the moved-from vector. Assignment can change the shape.
 *
 * Construction/copying may allocate, and assignment/destruction may free
 * memory. Keep those operations outside real-time audio processing. Accessors
 * do not allocate. Concurrent readers are only safe while the buffer stays
 * alive and no thread modifies its samples or replaces its storage.
 */
class SampleBuffer
{
public:
    /**
     * @brief Allocate and initialise an audio buffer to silence.
     * @param channels One for mono, two for stereo.
     * @param frames Number of time positions in each channel, not total samples.
     *               Zero is allowed here; WAV file I/O rejects empty audio.
     * @param sampleRate Source frames per second; must be finite and positive.
     * @throws std::invalid_argument If channels or sampleRate are unsupported.
     * @throws std::length_error If the required vector size cannot be represented.
     * @throws std::bad_alloc If allocation fails despite a valid size.
     *
     * The member initialiser list stores the dimensions before the body runs.
     * Allocation happens only after validation. Cost is O(channels * frames)
     * for zero-initialisation, with the same order of sample storage.
     */
    SampleBuffer(std::size_t channels, std::size_t frames, double sampleRate)
        : channels_(channels), frames_(frames), sampleRate_(sampleRate)
    {
        if (channels != 1 && channels != 2)
            throw std::invalid_argument("SampleBuffer requires mono or stereo audio");
        if (!std::isfinite(sampleRate) || sampleRate <= 0.0)
            throw std::invalid_argument("Sample rate must be finite and positive");
        // Check with division before multiplying; an overflowing product could
        // otherwise wrap to a small size and allocate too little storage.
        if (frames > samples_.max_size() / channels)
            throw std::length_error("Sample buffer is too large");

        samples_.resize(channels * frames, 0.0f);
    }

    /** @return Number of channel arrays: one or two. No allocation or mutation. */
    std::size_t channelCount() const noexcept { return channels_; }
    /** @return Number of samples per channel, also the number of time positions. */
    std::size_t frameCount() const noexcept { return frames_; }
    /** @return Original source rate in frames/second; this does not resample audio. */
    double sampleRate() const noexcept { return sampleRate_; }
    /**
     * @return Source duration in seconds; 48,000 frames at 48,000 Hz is 1 second.
     * Conversion to double makes the division floating-point. Channel count
     * is not a factor: both stereo channels describe the same time interval.
     */
    double durationSeconds() const noexcept
    {
        return static_cast<double>(frames_) / sampleRate_;
    }

    /**
     * @brief Borrow read-only access to one channel's contiguous sample array.
     * @param channel Zero-based channel index; stereo uses 0 = left, 1 = right.
     * @return Pointer to frame 0, or nullptr if frameCount() is zero.
     * @pre channel < channelCount(); the buffer must not be moved from.
     *
     * The pointer does not own storage. Keep this buffer alive and unchanged
     * while using it; do not move/assign it or race with a writer. Caller-side
     * frame indexing must also stay below frameCount(). The assertion checks
     * the channel in debug builds, not as a recoverable runtime error.
     */
    const float* channelData(std::size_t channel) const noexcept
    {
        assert(channel < channels_);
        return samples_.empty() ? nullptr : samples_.data() + channel * frames_;
    }

    /**
     * @brief Borrow writable channel storage for loading or offline rendering.
     * @param channel Zero-based channel index, with the same bounds/lifetime
     *                requirements as the const overload.
     * @return Pointer to frame 0, or nullptr for an empty buffer.
     *
     * Writing through this pointer does not validate or clamp sample values.
     * The WAV adapter checks finiteness at its boundaries. This overload is
     * selected for a non-const buffer; const buffers expose read-only samples.
     */
    float* channelData(std::size_t channel) noexcept
    {
        assert(channel < channels_);
        return samples_.empty() ? nullptr : samples_.data() + channel * frames_;
    }

private:
    std::size_t channels_;       ///< Channel arrays stored in samples_.
    std::size_t frames_;         ///< Length of each channel array.
    double sampleRate_;          ///< Source timing information, not a device rate.
    std::vector<float> samples_; ///< Owns channels_ * frames_ planar sample values.
};
} // namespace drizel
