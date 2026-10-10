#include "WavFile.h"

/**
 * @file WavFile.cpp
 * @brief JUCE codec adapter for the offline sample-buffer milestone.
 *
 * Disk operations and codec ownership stay here so future DSP can use the
 * standard-library SampleBuffer without knowing how WAV containers work.
 * Every operation in this translation unit is intended for an offline thread.
 */

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>

namespace drizel
{
namespace
{
// Private helpers have translation-unit scope through this unnamed namespace.
// A transfer size, not the audio device's block size or the total sample length.
constexpr std::size_t blockFrames = 4096;

/** Convert a C++17 filesystem path to JUCE's absolute UTF-8 path representation.
 * Relative paths are resolved against the current working directory; no file is
 * opened here. Path resolution/conversion failures propagate to the caller.
 */
juce::File asJuceFile(const std::filesystem::path& path)
{
    const auto utf8 = std::filesystem::absolute(path).u8string();
    return juce::File(juce::String::fromUTF8(utf8.c_str()));
}

/**
 * @brief Record incomplete disk reads that JUCE's WAV decoder could zero-pad.
 *
 * Inherit the normal file-stream implementation and override only read().
 * The flag is sticky: once a read is short it stays true until explicitly reset.
 * Header probing can legitimately reach the end; loadWav resets the flag after
 * creating the reader so it diagnoses the actual sample-payload transfer.
 * This object is used on one thread; the plain bool is not a synchronisation aid.
 */
class CheckedInputStream final : public juce::FileInputStream
{
public:
    // Inherit FileInputStream's constructor taking a juce::File.
    using juce::FileInputStream::FileInputStream;

    /** Read up to bytes into destination and remember whether fewer arrived.
     * Return the real count unchanged so the base codec retains its normal
     * behaviour; the adapter checks shortRead separately and rejects the load.
     */
    int read(void* destination, int bytes) override
    {
        const int count = juce::FileInputStream::read(destination, bytes);
        shortRead = shortRead || count != bytes;
        return count;
    }

    bool shortRead = false; ///< Borrowing code resets this before decoding audio.
};

/**
 * @brief Reject NaN/infinity before audio enters/leaves the file-I/O boundary.
 * @throws std::runtime_error At the first non-finite sample.
 * A full O(channels * frames) scan; finite amplitudes above unity are permitted.
 * This validates values only, not loudness, clipping safety, or audio quality.
 */
void requireFiniteSamples(const SampleBuffer& buffer)
{
    for (std::size_t channel = 0; channel < buffer.channelCount(); ++channel)
        for (std::size_t frame = 0; frame < buffer.frameCount(); ++frame)
            if (!std::isfinite(buffer.channelData(channel)[frame]))
                throw std::runtime_error("Audio contains a non-finite sample (NaN or infinity)");
}
} // namespace

SampleBuffer loadWav(const std::filesystem::path& path, std::size_t maxDecodedBytes)
{
    // Begin with a single RAII owner, so failure to open cleans up automatically.
    auto stream = std::make_unique<CheckedInputStream>(asJuceFile(path));
    if (!stream->openedOk())
        throw std::runtime_error("Cannot open input WAV: " + path.u8string());

    // get() borrows the address; it does not transfer or duplicate ownership.
    auto* checkedStream = stream.get();
    juce::WavAudioFormat format;
    // release() hands the stream to JUCE's raw-pointer factory. On success the
    // reader owns it; true asks JUCE to delete it on a normal factory failure.
    // Wrapping the returned reader in unique_ptr owns the reader and, indirectly,
    // its stream. Never delete checkedStream separately or use it after reader.
    std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(stream.release(), true));
    if (!reader)
        throw std::runtime_error("Invalid or unsupported WAV file");
    if (reader->numChannels != 1 && reader->numChannels != 2)
        throw std::runtime_error("Only mono and stereo WAV files are supported");
    // A 32-bit integer file is different from IEEE float32: only the latter is
    // supported here. Explicitly check both the bit depth and the sample kind.
    if (!((!reader->usesFloatingPointData
           && (reader->bitsPerSample == 16 || reader->bitsPerSample == 24))
          || (reader->usesFloatingPointData && reader->bitsPerSample == 32)))
        throw std::runtime_error("Supported WAV encodings: PCM16, PCM24, float32");
    if (reader->lengthInSamples <= 0)
        throw std::runtime_error("WAV contains no audio frames");

    // Convert counts only after rejecting unsupported channels and non-positive
    // lengths. Test the decoded float footprint BEFORE allocating SampleBuffer.
    const auto channels = static_cast<std::size_t>(reader->numChannels);
    const auto frames = static_cast<std::uint64_t>(reader->lengthInSamples);
    // Division avoids multiplying an untrusted frame count into an overflow.
    // Passing this size_t-based limit also bounds the subsequent size_t cast.
    if (frames > maxDecodedBytes / (channels * sizeof(float)))
        throw std::runtime_error("WAV exceeds the decoded audio memory limit");

    SampleBuffer buffer(channels, static_cast<std::size_t>(frames), reader->sampleRate);
    checkedStream->shortRead = false;
    for (std::size_t offset = 0; offset < buffer.frameCount();)
    {
        // The final transfer may contain fewer than blockFrames frames. Offset
        // is in frames in both source and destination, not in bytes or samples
        // summed across channels. Count is small enough for JUCE's int API.
        const auto count = std::min(blockFrames, buffer.frameCount() - offset);
        std::array<float*, 2> destinations {};
        // This stack array holds borrowed channel pointers, not audio storage.
        // Point each one into its channel at the current frame offset. JUCE's
        // float overload converts PCM to floats and deinterleaves file samples.
        for (std::size_t channel = 0; channel < channels; ++channel)
            destinations[channel] = buffer.channelData(channel) + offset;

        // The codec can report success even after zero-padding a short input,
        // so include the stream's short-read flag and I/O status in the check.
        if (!reader->read(destinations.data(), static_cast<int>(channels),
                          static_cast<juce::int64>(offset), static_cast<int>(count))
            || checkedStream->shortRead || checkedStream->getStatus().failed())
            throw std::runtime_error("WAV audio data is truncated or unreadable");
        offset += count;
    }
    requireFiniteSamples(buffer);
    // Return transfers ownership of the sample value (elision or move). The
    // reader and stream are then destroyed; the returned audio is independent.
    return buffer;
}

void writeWav(const std::filesystem::path& path, const SampleBuffer& buffer)
{
    // Reject invalid data before creating a file. SampleBuffer itself allows
    // zero frames and fractional positive rates for general offline use.
    if (buffer.frameCount() == 0)
        throw std::runtime_error("Cannot write an empty sample buffer");
    // JUCE writes the sample rate and byte rate through signed 32-bit casts.
    const auto maxRate = std::numeric_limits<std::int32_t>::max()
        / (buffer.channelCount() * sizeof(float));
    if (buffer.sampleRate() > static_cast<double>(maxRate)
        || std::floor(buffer.sampleRate()) != buffer.sampleRate())
        throw std::runtime_error("Sample rate must be a whole number fitting the WAV writer's header fields");
    requireFiniteSamples(buffer);

    const auto destination = asJuceFile(path);
    if (destination.exists())
        throw std::runtime_error("Output already exists; choose a new filename: " + path.u8string());

    // RAII cleanup attempts to remove the temporary sibling on failures. Its
    // destructor is not a guarantee against permission errors or process crashes.
    juce::TemporaryFile temporary(destination);
    auto fileStream = temporary.getFile().createOutputStream();
    if (!fileStream || !fileStream->openedOk())
        throw std::runtime_error("Cannot create output WAV: " + path.u8string());
    // Keep a non-owning FileOutputStream pointer to inspect its final status.
    // Move the owner into JUCE's generic OutputStream type without copying data.
    auto* output = fileStream.get();
    std::unique_ptr<juce::OutputStream> stream(std::move(fileStream));

    juce::WavAudioFormat format;
    // These with... methods return configured option values. Choosing float32
    // explicitly preserves decoded PCM16/PCM24 values without requantising.
    const auto options = juce::AudioFormatWriterOptions()
        .withSampleRate(buffer.sampleRate())
        .withNumChannels(static_cast<int>(buffer.channelCount()))
        .withBitsPerSample(32)
        .withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
    // On success JUCE moves stream into writer and sets stream to nullptr.
    // On failure stream retains ownership and is cleaned up during unwinding.
    auto writer = format.createWriterFor(stream, options);
    if (!writer)
        throw std::runtime_error("Cannot initialise the WAV writer");

    for (std::size_t offset = 0; offset < buffer.frameCount();)
    {
        const auto count = std::min(blockFrames, buffer.frameCount() - offset);
        // Const pointers borrow each channel slice. The writer interleaves them
        // into WAV data; neither SampleBuffer nor its source values are modified.
        std::array<const float*, 2> sources {};
        for (std::size_t channel = 0; channel < buffer.channelCount(); ++channel)
            sources[channel] = buffer.channelData(channel) + offset;
        if (!writer->writeFromFloatArrays(sources.data(), static_cast<int>(buffer.channelCount()),
                                         static_cast<int>(count)))
            throw std::runtime_error("Failed while writing WAV audio data");
        offset += count;
    }

    // Finalise the WAV length/header, then flush the file stream and check its
    // status while output is still alive. Stream flushes may block on the OS.
    if (!writer->flush())
        throw std::runtime_error("Failed to finalise the WAV header");
    output->flush();
    if (output->getStatus().failed())
        throw std::runtime_error("Failed to flush the output WAV");
    // Destroying the writer closes its owned stream. output now dangles and must
    // never be read again. Closing before replacement is necessary on Windows.
    writer.reset();

    // Recheck after the write in case a destination has appeared. The check and
    // JUCE replacement are separate operations, not atomic no-clobber creation.
    // JUCE can retry replacement internally, another reason this stays offline.
    if (destination.exists() || !temporary.overwriteTargetFileWithTemporary())
        throw std::runtime_error("Cannot move completed WAV to its destination");
}
} // namespace drizel
