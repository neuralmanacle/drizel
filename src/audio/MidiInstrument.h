#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace drizel
{
// One shared cloud, channel 1, last-note priority. Fixed storage; audio-owned.
// The processor dispatches events BEFORE rendering their sample offset.
class MidiInstrument
{
public:
    void prepare(double rate) noexcept { rate_ = rate; reset(); }
    void reset() noexcept
    {
        count_ = 0; remaining_ = 0; envelope_ = target_ = step_ = 0.0;
        note_ = 60; bend_ = wheel_ = 0.0;
        position_ = density_ = -1.0;
    }
    void setEnvelope(double attack, double release) noexcept
    {
        attack_ = std::clamp(attack, 0.001, 2.0);
        release_ = std::clamp(release, 0.005, 5.0);
    }
    void noteOn(int note, double velocity) noexcept
    {
        if (note < 0 || note > 127) return;
        if (velocity <= 0.0) { noteOff(note); return; }
        remove(note); // Repeated note-on replaces that key, never grows unbounded.
        notes_[count_] = note;
        velocities_[count_++] = std::clamp(velocity, 0.0, 1.0);
        select();
    }
    void noteOff(int note) noexcept
    {
        if (!remove(note)) return;
        if (count_ > 0) select();
        else ramp(0.0, release_);
    }
    void pitchBend(int value) noexcept
    {
        value = std::clamp(value, 0, 16383);
        bend_ = 2.0 * (value - 8192) / (value < 8192 ? 8192.0 : 8191.0);
    }
    void controller(int cc, int value) noexcept
    {
        const double normal = std::clamp(value, 0, 127) / 127.0;
        if (cc == 1) wheel_ = normal;
        else if (cc == 74) position_ = normal;
        else if (cc == 71) density_ = 1.0 + 99.0 * normal;
        else if (cc == 123) { count_ = 0; ramp(0.0, release_); }
        else if (cc == 120) reset(); // All sound off is deliberately immediate.
        else if (cc == 121) { bend_ = wheel_ = 0.0; position_ = density_ = -1.0; }
    }
    double nextAmplitude() noexcept
    {
        if (remaining_ > 0)
        {
            --remaining_;
            envelope_ = remaining_ == 0 ? target_ : envelope_ + step_;
        }
        return envelope_;
    }
    double pitchOffset(int root) const noexcept { return note_ - root + bend_; }
    double wheel() const noexcept { return wheel_; }
    double position(double base) const noexcept { return position_ < 0.0 ? base : position_; }
    double density(double base) const noexcept { return density_ < 0.0 ? base : density_; }
    int activeNote() const noexcept { return count_ > 0 ? note_ : -1; }
private:
    bool remove(int note) noexcept
    {
        for (int i = 0; i < count_; ++i)
            if (notes_[i] == note)
            {
                for (int j = i + 1; j < count_; ++j)
                { notes_[j - 1] = notes_[j]; velocities_[j - 1] = velocities_[j]; }
                --count_; return true;
            }
        return false;
    }
    void select() noexcept
    {
        note_ = notes_[count_ - 1];
        ramp(velocities_[count_ - 1], attack_); // Retrigger from current level, no jump.
    }
    void ramp(double target, double seconds) noexcept
    {
        target_ = target;
        remaining_ = static_cast<int>(std::max(1.0, std::round(seconds * rate_)));
        step_ = (target_ - envelope_) / remaining_;
    }
    std::array<int, 128> notes_ {};
    std::array<double, 128> velocities_ {};
    int count_ = 0, note_ = 60, remaining_ = 0;
    double rate_ = 48000.0, attack_ = 0.01, release_ = 0.2;
    double envelope_ = 0.0, target_ = 0.0, step_ = 0.0;
    double bend_ = 0.0, wheel_ = 0.0, position_ = -1.0, density_ = -1.0;
};
}
