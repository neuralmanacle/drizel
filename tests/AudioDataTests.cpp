#include "io/WavFile.h"

/**
 * @file AudioDataTests.cpp
 * @brief Standalone tests for sample storage, WAV conversion, and failure paths.
 *
 * Fixtures are small WAVs assembled directly from bytes, not by the production
 * writer. That independence helps expose reader/writer pairs sharing the same
 * bug. Tests use exceptions rather than assert(), so checks remain in Release.
 * All filesystem and test-runner allocations happen in an offline executable.
 */

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
/** Fail the current test with a readable message when condition is false.
 * The runner catches this exception, reports the case, and runs later cases.
 */
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

/**
 * @brief Require a callable to reject an operation with a standard exception.
 * @param action Usually a lambda containing the invalid operation.
 * @param message Failure explanation if action returns normally.
 *
 * The template accepts different callable types without prescribing a return
 * type. This helper checks rejection only, not an exact exception class/message;
 * tightening error-category contracts would require more specific assertions.
 */
template <typename Function>
void requireThrows(Function action, const std::string& message)
{
    try { action(); }
    catch (const std::exception&) { return; }
    throw std::runtime_error(message);
}

/**
 * @brief Own the temporary directory used by one invocation of the test suite.
 *
 * Creating the directory, rather than checking existence separately, chooses
 * an available name. Each retry changes the suffix. Cleanup uses an error_code
 * overload so the destructor does not throw while another exception unwinds.
 * Only the directory successfully created by this object is removed.
 */
class TestDirectory
{
public:
    /** Claim a fresh directory below the operating system's temporary path. */
    TestDirectory()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned attempt = 0; attempt < 100; ++attempt)
        {
            path = std::filesystem::temp_directory_path()
                / ("drizel-test-" + std::to_string(stamp) + "-" + std::to_string(attempt));
            if (std::filesystem::create_directory(path))
                return;
        }
        throw std::runtime_error("Cannot create a unique test directory");
    }
    /** Best-effort recursive cleanup of fixtures and generated output WAVs. */
    ~TestDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
    std::filesystem::path path; ///< Base path for test-specific filenames.
};

/**
 * @brief Write the least-significant bytes of an integer in WAV byte order.
 * @param out Binary output stream, checked by the fixture after writing.
 * @param value Unsigned bit pattern, including converted negative PCM values.
 * @param bytes Number of low-order bytes to write; test callers use 1 through 4.
 *
 * Shifting by 8*i selects byte i; masking keeps only that byte. Writing each
 * byte explicitly makes the fixture independent of the host's integer byte order.
 */
void littleEndian(std::ostream& out, std::uint32_t value, unsigned bytes)
{
    for (unsigned i = 0; i < bytes; ++i)
        out.put(static_cast<char>((value >> (i * 8)) & 0xff));
}

/**
 * @brief Build a deliberately small RIFF/WAVE fixture and its expected samples.
 * @param path Destination file inside TestDirectory; fixtures may overwrite it.
 * @param channels Encoded channel count; includes 3 for rejection tests.
 * @param bits Encoded sample width; accepted-value tests use 16, 24, or float32.
 * @param floating Write IEEE float (format tag 3) instead of PCM (tag 1).
 * @param frames Number of frames; zero deliberately tests empty audio.
 * @param sampleRate Integer header rate; zero deliberately tests invalid timing.
 * @return Planar channel vectors used as an independent numeric expectation.
 *
 * This is a bounded test helper, not a general WAV exporter. The positive
 * cases use PCM16/PCM24/float32. Expected values for unsupported PCM8/PCM32
 * fixtures are not meaningful and are never used for a decode comparison.
 */
std::vector<std::vector<float>> fixture(const std::filesystem::path& path,
    unsigned channels, unsigned bits, bool floating, unsigned frames = 19,
    unsigned sampleRate = 48000)
{
    // WAV's block alignment is bytes in one interleaved frame. RIFF chunks need
    // an even stored length; padding belongs outside the declared data length.
    const auto bytesPerFrame = channels * bits / 8;
    const auto dataBytes = frames * bytesPerFrame;
    const auto padding = dataBytes & 1u;
    std::ofstream out(path, std::ios::binary);
    // Minimal 44-byte header: 12 bytes RIFF/WAVE, 24 bytes fmt header+payload,
    // and 8 bytes data header. RIFF size excludes its own first 8 bytes.
    out.write("RIFF", 4);
    littleEndian(out, 36 + dataBytes + padding, 4);
    out.write("WAVEfmt ", 8);
    littleEndian(out, 16, 4);
    littleEndian(out, floating ? 3 : 1, 2);
    littleEndian(out, channels, 2);
    littleEndian(out, sampleRate, 4);
    littleEndian(out, sampleRate * bytesPerFrame, 4);
    littleEndian(out, bytesPerFrame, 2);
    littleEndian(out, bits, 2);
    out.write("data", 4);
    littleEndian(out, dataBytes, 4);

    // Expected values use planar storage while the file writes frame-by-frame,
    // interleaving channel samples. This tests the adapter's channel mapping.
    std::vector<std::vector<float>> expected(channels, std::vector<float>(frames));
    for (unsigned frame = 0; frame < frames; ++frame)
    {
        for (unsigned channel = 0; channel < channels; ++channel)
        {
            const auto index = frame + channel * 3;
            std::uint32_t encoded = 0;
            if (floating)
            {
                // Exact binary fractions, signed zero, and finite headroom
                // above unity exercise preservation without rounding ambiguity.
                constexpr std::array<float, 9> values { -1.5f, -1.0f, -0.5f, -0.0f,
                                                       0.0f, 0.25f, 0.75f, 1.0f, 1.5f };
                expected[channel][frame] = values[index % values.size()];
                // Preserve the float's IEEE bit pattern without pointer type
                // punning. This is different from numerically casting to int.
                std::memcpy(&encoded, &expected[channel][frame], sizeof(encoded));
            }
            else
            {
                // Signed PCM16/24 scales by 2^(bits-1): the minimum is -1.0,
                // while the largest positive integer is just below +1.0.
                const std::int32_t scale = bits == 24 ? 8388608 : 32768;
                const std::array<std::int32_t, 7> values {
                    -scale, -scale / 2, -1, 0, 1, scale / 2, scale - 1 };
                const auto value = values[index % values.size()];
                expected[channel][frame] = static_cast<float>(value) / static_cast<float>(scale);
                encoded = static_cast<std::uint32_t>(value);
            }
            littleEndian(out, encoded, bits / 8);
        }
    }
    if (padding != 0)
        out.put(0);
    // Closing before checking also catches failures flushing the output stream.
    out.close();
    require(out.good(), "Could not write test fixture");
    return expected;
}

/** Compare shape/rate before sample indexing, then require numeric equality.
 * Positive fixture cases always contain at least one channel. Exact equality
 * is suitable for PCM16/PCM24 -> float32 and unchanged float32 data; it is not
 * a universal comparison policy for future DSP calculations. Signed zeros
 * compare equal, so these checks do not require identical float bit patterns.
 */
void compare(const drizel::SampleBuffer& actual,
             const std::vector<std::vector<float>>& expected, double rate)
{
    require(actual.channelCount() == expected.size(), "Channel count changed");
    require(actual.frameCount() == expected.front().size(), "Frame count changed");
    require(actual.sampleRate() == rate, "Sample rate changed");
    for (std::size_t channel = 0; channel < expected.size(); ++channel)
        for (std::size_t frame = 0; frame < expected[channel].size(); ++frame)
            require(actual.channelData(channel)[frame] == expected[channel][frame],
                    "Sample value or channel order changed");
}
} // namespace

/** Run 21 named cases; return nonzero if any case fails.
 * The format/rate loops account for 12 cases and the frame-length loop for 2.
 * Directory-creation failure occurs before the per-case runner and aborts setup.
 */
int main()
{
    TestDirectory temp;
    unsigned passed = 0;
    unsigned failed = 0;
    // [&] borrows passed/failed counters from main. Each callable runs
    // immediately, so test lambdas may safely borrow the current loop values.
    // std::function type-erases the different lambdas; any resulting allocation
    // is acceptable here because this runner is never part of the audio thread.
    const auto test = [&](const std::string& name, const std::function<void()>& run)
    {
        try
        {
            run();
            ++passed;
            std::cout << "PASS " << name << '\n';
        }
        catch (const std::exception& error)
        {
            ++failed;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    };

    // Verify storage invariants before relying on them in the file-I/O cases.
    // The huge count exercises overflow rejection without a huge allocation.
    test("sample buffer layout, silence, and duration", []
    {
        drizel::SampleBuffer buffer(2, 480, 48000.0);
        require(buffer.durationSeconds() == 0.01, "Incorrect duration");
        for (std::size_t channel = 0; channel < 2; ++channel)
            for (std::size_t frame = 0; frame < 480; ++frame)
                require(buffer.channelData(channel)[frame] == 0.0f, "Buffer not zero-initialised");
        buffer.channelData(1)[0] = 0.75f;
        require(buffer.channelData(0)[0] == 0.0f, "Channels alias each other");
        require(buffer.channelData(1)[0] == 0.75f, "Write did not persist");
        requireThrows([] { drizel::SampleBuffer b(0, 4, 48000); }, "Accepted zero channels");
        requireThrows([] { drizel::SampleBuffer b(3, 4, 48000); }, "Accepted surround channels");
        requireThrows([] { drizel::SampleBuffer b(1, 4, 0); }, "Accepted zero rate");
        requireThrows([] { drizel::SampleBuffer b(1, 4, -1); }, "Accepted negative rate");
        requireThrows([] { drizel::SampleBuffer b(1, 4,
            std::numeric_limits<double>::quiet_NaN()); }, "Accepted NaN rate");
        requireThrows([] { drizel::SampleBuffer b(2,
            std::numeric_limits<std::size_t>::max(), 48000); }, "Size overflow not rejected");
    });

    // 2 channel layouts * 3 encodings * 2 rates = 12 independent combinations.
    // First check a raw fixture, then save and reload the decoded sample data.
    for (unsigned channels : { 1u, 2u })
        for (unsigned bits : { 16u, 24u, 32u })
            for (unsigned rate : { 44100u, 48000u })
            {
                const auto name = std::to_string(channels) + "ch-" + std::to_string(bits)
                    + "bit-" + std::to_string(rate);
                test(name + " decode and exact float round trip", [&]
                {
                    const auto input = temp.path / (name + ".wav");
                    const auto output = temp.path / (name + "-copy.wav");
                    const auto expected = fixture(input, channels, bits, bits == 32, 19, rate);
                    const auto buffer = drizel::loadWav(input);
                    compare(buffer, expected, rate);
                    drizel::writeWav(output, buffer);
                    compare(drizel::loadWav(output), expected, rate);
                });
            }

    // One frame catches short-buffer assumptions. 10,001 frames spans two full
    // 4,096-frame transfers plus a partial final transfer, exercising offsets.
    for (unsigned frames : { 1u, 10001u })
        test(std::to_string(frames) + " frames including chunk boundaries", [&]
        {
            const auto input = temp.path / ("frames-" + std::to_string(frames) + ".wav");
            const auto output = temp.path / ("frames-copy-" + std::to_string(frames) + ".wav");
            const auto expected = fixture(input, 2, 24, false, frames);
            const auto buffer = drizel::loadWav(input);
            compare(buffer, expected, 48000);
            drizel::writeWav(output, buffer);
            compare(drizel::loadWav(output), expected, 48000);
        });

    // Distinguish no file, no bytes, invalid bytes, and a valid zero-frame header.
    test("missing, empty, invalid, and zero-frame files", [&]
    {
        requireThrows([&] { drizel::loadWav(temp.path / "missing.wav"); }, "Missing file accepted");
        const auto path = temp.path / "invalid.wav";
        { std::ofstream out(path, std::ios::binary); }
        requireThrows([&] { drizel::loadWav(path); }, "Empty file accepted");
        { std::ofstream out(path, std::ios::binary); out << "This is not audio"; }
        requireThrows([&] { drizel::loadWav(path); }, "Invalid header accepted");
        fixture(path, 1, 16, false, 0);
        requireThrows([&] { drizel::loadWav(path); }, "Zero-frame WAV accepted");
    });

    // JUCE can recognise more formats than this milestone promises; the public
    // adapter must enforce its own narrower channel/encoding/rate contract.
    test("unsupported encodings and channel counts", [&]
    {
        const auto path = temp.path / "unsupported.wav";
        fixture(path, 1, 8, false);
        requireThrows([&] { drizel::loadWav(path); }, "PCM8 accepted");
        fixture(path, 1, 32, false);
        requireThrows([&] { drizel::loadWav(path); }, "PCM32 accepted");
        fixture(path, 3, 16, false);
        requireThrows([&] { drizel::loadWav(path); }, "Surround WAV accepted");
        fixture(path, 1, 16, false, 19, 0);
        requireThrows([&] { drizel::loadWav(path); }, "Zero sample rate accepted");
    });

    // Keep the declared length while removing real payload bytes. A decoder
    // that silently pads the missing samples would incorrectly pass the load.
    test("truncated payload rejected instead of padded with silence", [&]
    {
        const auto path = temp.path / "truncated.wav";
        fixture(path, 2, 24, false, 10001);
        std::filesystem::resize_file(path, std::filesystem::file_size(path) - 100);
        requireThrows([&] { drizel::loadWav(path); }, "Truncated audio accepted");
    });

    // Use a tiny configurable limit to test the exact boundary cheaply. The
    // later header mutation models excessive input without a multi-GB fixture.
    test("decoded memory limit checked before allocation", [&]
    {
        const auto path = temp.path / "limited.wav";
        const auto expected = fixture(path, 2, 16, false, 19);
        const auto bytes = 2u * 19u * sizeof(float);
        compare(drizel::loadWav(path, bytes), expected, 48000);
        requireThrows([&] { drizel::loadWav(path, bytes - 1); }, "Exceeded memory cap");
        // A header claiming a huge payload must be rejected without allocating it.
        std::fstream out(path, std::ios::in | std::ios::out | std::ios::binary);
        // Byte 40 is the data-length field in this helper's minimal WAV header.
        out.seekp(40);
        littleEndian(out, 0x7ffffffc, 4);
        out.close();
        requireThrows([&] { drizel::loadWav(path); }, "Huge claimed length accepted");
    });

    // A non-finite value could contaminate later grain mixing. Cover a NaN
    // decoded from a file and infinity introduced through writable buffer access.
    test("non-finite samples rejected on read and write", [&]
    {
        const auto path = temp.path / "nan.wav";
        fixture(path, 1, 32, true);
        std::fstream out(path, std::ios::in | std::ios::out | std::ios::binary);
        // Byte 44 starts audio data in our fixture; 0x7fc00000 is a float32 NaN.
        out.seekp(44);
        littleEndian(out, 0x7fc00000, 4);
        out.close();
        requireThrows([&] { drizel::loadWav(path); }, "NaN input accepted");
        drizel::SampleBuffer buffer(1, 1, 48000);
        buffer.channelData(0)[0] = std::numeric_limits<float>::infinity();
        requireThrows([&] { drizel::writeWav(temp.path / "infinity.wav", buffer); },
                      "Infinite output accepted");
    });

    // Confirm both rejection and preservation of an existing source. The final
    // cases reject invalid output metadata before a destination is created.
    // This tests sequential file protection, not concurrent filesystem races.
    test("invalid output and existing destination leave files intact", [&]
    {
        const auto path = temp.path / "protected.wav";
        const auto expected = fixture(path, 1, 16, false);
        const auto buffer = drizel::loadWav(path);
        requireThrows([&] { drizel::writeWav(path, buffer); }, "Overwrote input");
        compare(drizel::loadWav(path), expected, 48000);
        requireThrows([&] { drizel::writeWav(temp.path, buffer); }, "Wrote to directory");
        requireThrows([&] { drizel::writeWav(temp.path / "missing" / "out.wav", buffer); },
                      "Missing output directory accepted");
        requireThrows([&] { drizel::writeWav(temp.path / "empty.wav",
            drizel::SampleBuffer(1, 0, 48000)); }, "Empty output accepted");
        requireThrows([&] { drizel::writeWav(temp.path / "fractional.wav",
            drizel::SampleBuffer(1, 1, 48000.5)); }, "Fractional WAV rate accepted");
        requireThrows([&] { drizel::writeWav(temp.path / "huge-rate.wav",
            drizel::SampleBuffer(2, 1, 1.0e9)); }, "Overflowing WAV byte rate accepted");
        require(!std::filesystem::exists(temp.path / "empty.wav"), "Failed write left an output");
    });

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1; // CTest treats zero as success for this executable.
}
