#include "io/WavFile.h"

/**
 * @file main.cpp
 * @brief Offline experiment: load a WAV, inspect it, save it, and verify a reload.
 *
 * This executable does not open an audio device or run the plugin. It proves
 * the sample-data path before any grain scheduling or live playback is added.
 */

#include <exception>
#include <iomanip>
#include <iostream>

/**
 * @param argc Argument count including argv[0], the executable name.
 * @param argv argv[1] is the input WAV; argv[2] is a new output filename.
 * @return 0 after exact numeric sample/property comparison, 1 on usage or I/O
 *         errors. Paths containing spaces should be quoted in the shell.
 *
 * All work happens synchronously on the command-line program's main thread.
 * Exceptions are reported here instead of escaping as an uncaught termination.
 */
int main(int argc, char* argv[])
{
    // Two user arguments plus the executable name are required. Validate the
    // count before indexing argv so missing arguments cannot be dereferenced.
    if (argc != 3)
    {
        std::cerr << "Usage: drizel_wav_roundtrip input.wav output.wav\n"
                  << "Output must be a new filename. Writes 32-bit float WAV.\n";
        return 1;
    }

    try
    {
        // const selects read-only access later and prevents accidental sample
        // edits during this experiment; loading already owns the entire buffer.
        const auto buffer = drizel::loadWav(argv[1]);
        // setprecision(3) with fixed changes only printed duration, not samples.
        std::cout << "Channels: " << buffer.channelCount()
                  << "\nSample rate: " << buffer.sampleRate() << " Hz"
                  << "\nFrames: " << buffer.frameCount()
                  << "\nDuration: " << std::fixed << std::setprecision(3)
                  << buffer.durationSeconds() << " seconds\n";
        drizel::writeWav(argv[2], buffer);
        // Read the actual completed file back, not a second view of the first
        // buffer. Both buffers coexist, using up to twice the per-load cap.
        const auto copy = drizel::loadWav(argv[2]);
        // Validate dimensions before indexing the reloaded channel arrays.
        if (copy.channelCount() != buffer.channelCount()
            || copy.frameCount() != buffer.frameCount()
            || copy.sampleRate() != buffer.sampleRate())
            throw std::runtime_error("Round-trip audio properties changed");

        // Numeric equality is appropriate for a float32 copy with no DSP.
        // This is not a container-byte or metadata comparison. It also treats
        // +0 and -0 as equal. Future arithmetic DSP will usually need tolerance.
        for (std::size_t channel = 0; channel < buffer.channelCount(); ++channel)
            for (std::size_t frame = 0; frame < buffer.frameCount(); ++frame)
                if (copy.channelData(channel)[frame] != buffer.channelData(channel)[frame])
                    throw std::runtime_error("Round-trip sample data changed");

        std::cout << "Verified: decoded samples match exactly.\nSaved: " << argv[2] << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        // A nonzero status lets scripts/CTest recognise failure. If the reload
        // or comparison fails after saving, the completed output is left on disk.
        std::cerr << "WAV round trip failed: " << error.what() << '\n';
        return 1;
    }
}
