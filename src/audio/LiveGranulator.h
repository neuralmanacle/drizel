#pragma once
// Real-time stereo granular effect for DAW input. Audio thread owns all state.
// prepare() allocates; process() is allocation-free, lock-free and does no I/O.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace drizel
{
class LiveGranulator
{
public:
    struct Frame { float left = 0.0f, right = 0.0f; };

    void prepare(double sampleRate)
    {
        rate_ = std::max(1.0, sampleRate);
        capacity_ = static_cast<std::size_t>(rate_ * 2.0);
        for (auto& channel : history_) channel.assign(capacity_, 0.0f);
        grainLength_ = std::max<std::size_t>(16, static_cast<std::size_t>(rate_ * 0.08));
        voices_ = {};
        write_ = filled_ = active_ = 0;
        untilNext_ = 0.0;
    }

    Frame process(float left, float right, double position, double pitch,
                  double density, double gain) noexcept
    {
        if (capacity_ == 0) return {};
        history_[0][write_] = std::isfinite(left) ? left : 0.0f;
        history_[1][write_] = std::isfinite(right) ? right : 0.0f;
        write_ = (write_ + 1) % capacity_;
        filled_ = std::min(capacity_, filled_ + 1);

        const double frequency = std::clamp(std::isfinite(density) ? density : 20.0, 1.0, 100.0);
        if (untilNext_ <= 0.0)
        {
            trigger(position, pitch);
            untilNext_ += rate_ / frequency;
        }
        untilNext_ -= 1.0;

        Frame output;
        for (auto& voice : voices_)
        {
            if (!voice.active) continue;
            constexpr float pi = 3.14159265358979323846f;
            const float phase = static_cast<float>(voice.age) / static_cast<float>(grainLength_ - 1);
            const float envelope = 0.5f - 0.5f * std::cos(2.0f * pi * phase);
            output.left += read(0, voice.position) * envelope;
            output.right += read(1, voice.position) * envelope;
            voice.position += voice.speed;
            if (++voice.age >= grainLength_)
            {
                voice.active = false;
                --active_;
            }
        }
        // Rough constant-power normalization based on expected grain overlap.
        const float scale = static_cast<float>(2.0 / std::max(1.0, frequency * 0.08));
        const float level = static_cast<float>(std::clamp(std::isfinite(gain) ? gain : 0.0, 0.0, 1.0));
        output.left = std::clamp(output.left * scale * level, -1.0f, 1.0f);
        output.right = std::clamp(output.right * scale * level, -1.0f, 1.0f);
        return output;
    }

    std::size_t activeGrains() const noexcept { return active_; }

private:
    struct Voice { double position = 0.0, speed = 1.0; std::size_t age = 0; bool active = false; };
    static constexpr std::size_t maxVoices = 64;

    void trigger(double position, double pitch) noexcept
    {
        // 0.4s of startup history avoids reading unwritten samples.
        if (filled_ < static_cast<std::size_t>(rate_ * 0.4)) return;
        auto voice = std::find_if(voices_.begin(), voices_.end(),
                                  [](const Voice& v) { return !v.active; });
        if (voice == voices_.end()) return;
        const double p = std::clamp(std::isfinite(position) ? position : 0.0, 0.0, 1.0);
        const double semitones = std::clamp(std::isfinite(pitch) ? pitch : 0.0, -24.0, 24.0);
        const double speed = std::exp2(semitones / 12.0);
        // Look behind the write head by 0.4-1.1 seconds, allowing the
        // entire +24 semitone grain (0.32 seconds) to stay in recorded history.
        const double lag = rate_ * (0.4 + p * 0.7);
        voice->position = static_cast<double>(write_) - lag;
        voice->speed = speed;
        voice->age = 0;
        voice->active = true;
        ++active_;
    }

    float read(int channel, double index) const noexcept
    {
        const double length = static_cast<double>(capacity_);
        double offset = std::fmod(index, length);
        if (offset < 0.0) offset += length;
        const auto first = static_cast<std::size_t>(offset);
        const auto next = (first + 1) % capacity_;
        const float fraction = static_cast<float>(offset - static_cast<double>(first));
        return history_[channel][first] + (history_[channel][next] - history_[channel][first]) * fraction;
    }

    std::array<std::vector<float>, 2> history_;
    std::array<Voice, maxVoices> voices_ {};
    double rate_ = 48000.0, untilNext_ = 0.0;
    std::size_t capacity_ = 0, grainLength_ = 0;
    std::size_t write_ = 0, filled_ = 0, active_ = 0;
};
}
