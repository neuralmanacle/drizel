#pragma once // One definition per translation unit.

#include "Grain.h" // Reuse the tested single-grain state and interpolation.
#include "Window.h" // Allocate Hann tables during preparation only.
#include <algorithm> // min, max and clamp express parameter bounds.
#include <array> // Fixed-capacity storage never grows during playback.
#include <cstdint> // uint32_t gives the random generator exact wraparound.
#include <stdexcept> // Invalid preparation settings are reported offline.

namespace drizel
{
struct CloudSettings
{
    double outputRate = 48000.0; // Output frames per second.
    double density = 20.0; // Grains per second; interval = outputRate / density.
    std::size_t length = 4800; // Centre grain length in output frames.
    std::size_t lengthSpread = 0; // Choose length-spread, length, or length+spread.
    double position = 0.0; // Centre start position in SOURCE frames.
    double positionSpread = 0.0; // Uniform +/- source-frame displacement.
    double semitones = 0.0; // Centre pitch relative to the source.
    double pitchSpread = 0.0; // Uniform +/- semitone displacement.
    float pan = 0.0f; // Centre pan, from -1 left to +1 right.
    float panSpread = 0.0f; // Uniform +/- variation, clamped at the edges.
    double jitter = 0.0; // Fractional +/- interval variation, in [0, 0.95].
    float gain = 0.8f; // Conservative upper peak target, in [0,1].
    std::uint32_t seed = 1; // Zero is also a valid deterministic seed.
    InterpolationMode mode = InterpolationMode::linear; // Existing reader choice.
};

// Owns windows and borrows the source. Prepare only outside audio processing.
// Source must remain alive and unchanged through the last next() call.
// Copy/move is forbidden: grains borrow addresses in this object's windows.
// No thread safety is implied; preparation and playback must not overlap.
template <std::size_t Capacity = 64>
class GrainCloud
{
    static_assert(Capacity > 0, "A cloud needs at least one grain slot");
public:
    GrainCloud() = default; // Arrays begin inactive; vectors begin empty.
    GrainCloud(const GrainCloud&) = delete; // Prevent invalid borrowed pointers.
    GrainCloud& operator=(const GrainCloud&) = delete; // Same lifetime rule.
    GrainCloud(GrainCloud&&) = delete; // Keep window ownership stationary.
    GrainCloud& operator=(GrainCloud&&) = delete; // No moving during playback.

    void prepare(const SampleBuffer& source, const CloudSettings& settings)
    {
        source_ = nullptr; // Failed preparation leaves a silent, stopped cloud.
        grains_ = {}; // Release every old borrow before replacing window tables.
        started_ = dropped_ = maximumActive_ = 0; // Reset diagnostic counters.
        countdown_ = 0.0; // The first trigger is on output frame zero.
        const auto& s = settings; // Short read-only alias, without a copy.
        if (source.frameCount() == 0 || !std::isfinite(s.outputRate)
            || s.outputRate < 1 || s.outputRate > 384000
            || !std::isfinite(s.density) || s.density <= 0 || s.density > s.outputRate
            || s.length < 3 || s.length > 2000000 || s.lengthSpread > s.length - 3
            || s.lengthSpread > 2000000 - s.length
            || !std::isfinite(s.position) || s.position < 0
            || s.position > static_cast<double>(source.frameCount() - 1)
            || !std::isfinite(s.positionSpread) || s.positionSpread < 0
            || s.positionSpread > static_cast<double>(source.frameCount())
            || !std::isfinite(s.semitones) || !std::isfinite(s.pitchSpread)
            || s.pitchSpread < 0 || std::abs(s.semitones) + s.pitchSpread > 48
            || !std::isfinite(s.pan) || std::abs(s.pan) > 1
            || !std::isfinite(s.panSpread) || s.panSpread < 0 || s.panSpread > 1
            || !std::isfinite(s.jitter) || s.jitter < 0 || s.jitter > 0.95
            || !std::isfinite(s.gain) || s.gain < 0 || s.gain > 1
            || (s.mode != InterpolationMode::linear && s.mode != InterpolationMode::hermite))
            throw std::invalid_argument("Invalid cloud settings"); // Offline error.
        const double maxRate = std::pow(2.0, (s.semitones + s.pitchSpread) / 12.0)
                               * source.sampleRate() / s.outputRate;
        if (!std::isfinite(maxRate)) // Avoid a later invalid grain trigger.
            throw std::invalid_argument("Unrepresentable playback rate");
        double peak = 0.0; // Scan once, outside playback, to support float WAV headroom.
        for (std::size_t channel = 0; channel < source.channelCount(); ++channel)
            for (std::size_t frame = 0; frame < source.frameCount(); ++frame)
            {
                const double value = source.channelData(channel)[frame]; // Read only.
                if (!std::isfinite(value)) // Reject NaN/Inf before interpolation.
                    throw std::invalid_argument("Source must contain finite samples");
                peak = std::max(peak, std::abs(value)); // Maximum absolute amplitude.
            }
        // Float interpolation arithmetic can overflow on extreme finite inputs.
        if (peak > 1.0e30)
            throw std::invalid_argument("Source amplitude exceeds supported range");
        settings_ = s; // Snapshot parameters; no concurrent live control yet.
        random_ = s.seed; // Re-preparing with the same seed restarts the sequence.
        interval_ = s.outputRate / s.density; // Fractional frames are preserved.
        if (!std::isfinite(interval_))
            throw std::invalid_argument("Unrepresentable trigger interval");
        windows_[0] = makeHannWindow(s.length - s.lengthSpread); // Short choice.
        windows_[1] = makeHannWindow(s.length); // Centre choice.
        windows_[2] = makeHannWindow(s.length + s.lengthSpread); // Long choice.
        const double minimumInterval = std::max(1.0, interval_ * (1.0 - s.jitter));
        const double overlap = std::min(static_cast<double>(Capacity),
            std::ceil(static_cast<double>(windows_[2].size()) / minimumInterval));
        // Hermite's four coefficient magnitudes sum to at most 1.25; linear's to 1.
        const double readerBound = s.mode == InterpolationMode::hermite ? 1.25 : 1.0;
        grainGain_ = static_cast<float>(s.gain / (overlap * std::max(1.0, peak) * readerBound));
        source_ = &source; // Publish readiness only after all preparation succeeds.
    }

    // Audio-thread-only update: existing grains keep their captured parameters.
    // Density may only decrease from the prepared ceiling, preserving the
    // original worst-case overlap gain bound even while old grains are alive.
    void setLiveControls(double position, double semitones, double density) noexcept
    {
        if (source_ == nullptr)
            return; // No source bounds exist until preparation succeeds.
        if (std::isfinite(position))
            settings_.position = std::clamp(position, 0.0,
                static_cast<double>(source_->frameCount() - 1));
        if (std::isfinite(semitones))
            settings_.semitones = std::clamp(semitones,
                -48.0 + settings_.pitchSpread, 48.0 - settings_.pitchSpread);
        if (std::isfinite(density))
        {
            const double nextInterval = settings_.outputRate
                / std::clamp(density, std::min(1.0, settings_.density), settings_.density);
            countdown_ *= nextInterval / interval_; // Preserve scheduler phase.
            interval_ = nextInterval; // No window preparation or source scan.
        }
    }

    GrainFrame next(bool trigger = true) noexcept // One stereo output frame.
    {
        if (source_ == nullptr)
            return {}; // Safe before prepare or after a failed prepare.
        if (trigger && countdown_ <= 0.0) // At most one trigger per output frame.
        {
            launch(); // Try a free slot; a full pool drops the new grain.
            countdown_ += std::max(1.0, interval_ * (1.0 + settings_.jitter * bipolar()));
        } // Addition keeps fractional timing error instead of rounding each hop.
        if (trigger)
            countdown_ -= 1.0; // A paused scheduler retains its fractional phase.
        double left = 0.0, right = 0.0; // Wider accumulation reduces summation error.
        for (auto& grain : grains_) // Bounded work, independent of render duration.
        {
            const auto frame = grain.next(); // Inactive slots return silence.
            left += frame.left; // Overlap-add: every voice contributes at this time.
            right += frame.right; // Keep stereo channels separate.
        }
        return {static_cast<float>(left), static_cast<float>(right)}; // Stereo value.
    }

    std::size_t activeCount() const noexcept // Count currently live voices.
    {
        std::size_t count = 0; // Stack-only counter.
        for (const auto& grain : grains_)
            count += grain.active() ? 1u : 0u; // No mutation of the grain.
        return count;
    }
    std::size_t started() const noexcept { return started_; } // Accepted triggers.
    std::size_t dropped() const noexcept { return dropped_; } // Full-pool triggers.
    std::size_t maximumActive() const noexcept { return maximumActive_; } // High water.

private:
    double unit() noexcept // Explicit LCG, avoiding platform-specific distributions.
    {
        random_ = random_ * 1664525u + 1013904223u; // Unsigned modulo 2^32.
        return static_cast<double>(random_) / 4294967296.0; // Uniform grid in [0,1).
    }
    double bipolar() noexcept { return 2.0 * unit() - 1.0; } // Map to [-1,1).
    void launch() noexcept
    {
        const auto choice = static_cast<std::size_t>(unit() * 3.0); // Three lengths.
        const double position = std::clamp(settings_.position + settings_.positionSpread * bipolar(),
            0.0, static_cast<double>(source_->frameCount() - 1)); // Clamp starts only.
        const double pitch = settings_.semitones + settings_.pitchSpread * bipolar();
        const float pan = static_cast<float>(std::clamp(settings_.pan + settings_.panSpread * bipolar(), -1.0, 1.0));
        const double rate = std::pow(2.0, pitch / 12.0) * source_->sampleRate() / settings_.outputRate;
        // Draw all parameters even on overflow, keeping the random timeline stable.
        for (auto& grain : grains_)
            if (!grain.active()) // Reuse the first finished voice without allocation.
            {
                const auto& window = windows_[choice]; // Borrow a prepared table.
                grain.start(*source_, window.data(), window.size(), position, rate,
                            grainGain_, pan, settings_.mode); // Settings were validated.
                ++started_; // Account for this accepted trigger.
                maximumActive_ = std::max(maximumActive_, activeCount()); // Include age zero.
                return; // Exactly one voice per scheduler event.
            }
        ++dropped_; // Drop-new preserves existing envelopes and avoids forced cuts.
    }
    std::array<Grain, Capacity> grains_ {}; // All voice state is allocated inline.
    std::array<std::vector<float>, 3> windows_; // Owned, prepared Hann tables.
    const SampleBuffer* source_ = nullptr; // Non-owning; caller preserves lifetime.
    CloudSettings settings_; // Immutable snapshot during playback.
    double interval_ = 1.0; // Nominal frames between triggers.
    double countdown_ = 0.0; // Fractional time until the next trigger.
    float grainGain_ = 0.0f; // Fixed gain avoids active-count-driven pumping.
    std::uint32_t random_ = 1; // Reproducible random state.
    std::size_t started_ = 0, dropped_ = 0, maximumActive_ = 0; // Offline diagnostics.
};
} // namespace drizel
