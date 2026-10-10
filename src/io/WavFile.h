#pragma once

/**
 * @file WavFile.h
 * @brief Offline WAV operations with standard C++ types at the public boundary.
 *
 * File decoding/encoding is implemented with JUCE in WavFile.cpp. Callers only
 * need a filesystem path and SampleBuffer; they do not manage codec streams.
 */

#include "audio/SampleBuffer.h"

#include <cstddef>
#include <filesystem>

namespace drizel
{
/**
 * @brief Decode an entire supported WAV into an owned planar float buffer.
 * @param path Relative to the process's working directory, or an absolute path.
 * @param maxDecodedBytes Maximum sample-storage bytes, excluding object and
 *                        codec overhead. Defaults to 256 MiB; zero rejects
 *                        every non-empty input. This is not a file-size limit.
 * @return Independent sample storage retaining source channel count and rate.
 * @throws std::runtime_error For open/decode failures, unsupported channels or
 *         encoding, empty audio, excess decoded size, short reads, NaN or infinity.
 * @throws std::filesystem::filesystem_error If filesystem path operations fail.
 * @throws std::invalid_argument If decoded buffer dimensions/rate are invalid.
 *         Allocation/length exceptions may also propagate from SampleBuffer.
 *
 * Accepts mono/stereo PCM16, PCM24 and IEEE float32 WAV. Keeps finite float
 * headroom outside [-1, 1], with no normalisation, downmixing, or resampling.
 * Decodes in bounded transfer chunks but retains the whole sample in memory.
 *
 * @warning Offline only: allocates, blocks on disk, and may throw. Never call
 *          from processBlock or another real-time audio callback.
 */
SampleBuffer loadWav(const std::filesystem::path& path,
                     std::size_t maxDecodedBytes = 256u * 1024u * 1024u);

/**
 * @brief Save a non-empty buffer as IEEE float32 WAV without changing its samples.
 * @param path New output filename; the parent directory must already exist.
 * @param buffer Source audio, borrowed read-only for the duration of this call.
 * @throws std::runtime_error For empty/non-finite data, an unrepresentable WAV
 *         rate, an existing destination, or create/write/flush/move failures.
 *         Filesystem and allocation exceptions may also propagate.
 *
 * The source rate must be a whole number and fit the writer's header fields.
 * Sample values, channel count, frame count, and rate are preserved; the input
 * WAV's bit depth and metadata are not. No clipping or gain change is applied.
 * A temporary sibling is completed and closed before the destination is set.
 * Existing destinations are checked before and after writing, but this is not
 * an atomic no-overwrite guarantee against another process creating the same
 * destination between the final check and the underlying replacement call.
 *
 * @warning Offline only. The caller must also prevent concurrent modification
 *          of buffer; a const reference does not synchronise other threads.
 */
void writeWav(const std::filesystem::path& path, const SampleBuffer& buffer);
} // namespace drizel
