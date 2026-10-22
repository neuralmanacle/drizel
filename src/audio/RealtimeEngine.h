#pragma once

#include "GrainCloud.h" // The same renderer used by the offline experiment.
#include <atomic> // Two single-slot ownership mailboxes, never shared_ptr.
#include <memory> // Worker-side construction and exception-safe ownership.

namespace drizel
{
// Construct on the loader thread; keep source and cloud at stable addresses.
struct PreparedSample
{
    SampleBuffer source; // Declared first, so it outlives the borrowing cloud.
    GrainCloud<> cloud; // Windows and voice pool belong to this sample.
    double outputRate; // Reject packets prepared for a previous device rate.

    PreparedSample(SampleBuffer data, double rate)
        : source(std::move(data)), outputRate(rate)
    {
        CloudSettings settings; // Fixed 100 ms grains for the first live milestone.
        settings.outputRate = rate;
        settings.length = static_cast<std::size_t>(rate * 0.1);
        settings.density = 100.0; // Prepare gain for the maximum live density.
        settings.panSpread = 0.8f; // Give the texture a stereo image.
        cloud.prepare(source, settings); // Scan and allocate only on this thread.
    }
};

// One worker publishes/collects; one audio thread calls next(). prepare() and
// destruction require processing to be stopped. The worker must stop before
// destruction. No callback ever destroys a packet or accesses worker mutexes.
class RealtimeEngine
{
public:
    static_assert(std::atomic<PreparedSample*>::is_always_lock_free,
                  "Real-time handoff requires lock-free pointers");
    ~RealtimeEngine()
    {
        delete active_; // Playback and worker have already stopped.
        delete pending_.load();
        delete retired_.load();
    }

    void publish(std::unique_ptr<PreparedSample> sample) noexcept
    {
        delete pending_.exchange(sample.release()); // Worker frees superseded work.
    }
    void collect() noexcept
    {
        delete retired_.exchange(nullptr); // Worker reclaims audio-retired storage.
    }
    void prepare(double rate) noexcept
    {
        delete active_; // Lifecycle callback, never processBlock.
        active_ = nullptr;
        rate_ = rate;
        fade_ = 0.0;
        fadeStep_ = 1.0 / (rate * 0.01); // Ten milliseconds each side of a swap.
        coefficient_ = 1.0 - std::exp(-1.0 / (rate * 0.02)); // 20 ms smoothing.
        position_ = 0.0;
        pitch_ = 0.0;
        density_ = 20.0;
        gain_ = 0.0;
    }
    GrainFrame next(double position, double pitch, double density, double gain) noexcept
    {
        // A packet is only dereferenced after exchange transfers sole ownership.
        const bool waiting = pending_.load() != nullptr;
        fade_ = std::clamp(fade_ + (waiting ? -fadeStep_ : fadeStep_), 0.0, 1.0);
        if (waiting && fade_ == 0.0 && retired_.load() == nullptr)
        {
            auto* incoming = pending_.exchange(nullptr);
            if (incoming != nullptr)
            {
                if (incoming->outputRate == rate_)
                {
                    auto* previous = active_;
                    active_ = incoming; // Old grains are silent before replacement.
                    retired_.store(previous); // Destruction belongs to the worker.
                }
                else
                    retired_.store(incoming); // Stale rate: retain the active source.
            }
        }
        smooth(position_, position, 0.0, 1.0); // Normalised source start position.
        smooth(pitch_, pitch, -24.0, 24.0); // Pitch in semitones.
        smooth(density_, density, 1.0, 100.0); // Grains per second.
        smooth(gain_, gain, 0.0, 1.0); // Play/mute also passes through smoothing.
        if (active_ == nullptr)
            return {}; // Loading and unprepared startup are silent.
        active_->cloud.setLiveControls(position_ * (active_->source.frameCount() - 1),
                                        pitch_, density_);
        const auto frame = active_->cloud.next(); // Fixed-capacity overlap-add.
        return {safe(frame.left), safe(frame.right)};
    }

private:
    void smooth(double& value, double target, double low, double high) noexcept
    {
        if (std::isfinite(target))
            value += coefficient_ * (std::clamp(target, low, high) - value);
    }
    float safe(float value) const noexcept
    {
        const double output = value * fade_ * gain_;
        return std::isfinite(output) ? static_cast<float>(std::clamp(output, -1.0, 1.0)) : 0.0f;
    }
    std::atomic<PreparedSample*> pending_ {nullptr}; // Worker -> audio ownership.
    std::atomic<PreparedSample*> retired_ {nullptr}; // Audio -> worker ownership.
    PreparedSample* active_ = nullptr; // Exclusively audio-owned during processing.
    double rate_ = 48000.0, fade_ = 0.0, fadeStep_ = 1.0 / 480.0;
    double coefficient_ = 0.001, position_ = 0.0, pitch_ = 0.0;
    double density_ = 20.0, gain_ = 0.0; // Audio-thread-only smoother history.
};
} // namespace drizel
