#include "audio/MidiInstrument.h"
#include <cstdlib>
#include <iostream>

static void check(bool ok, const char* message)
{
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
static bool near(double a, double b) { return std::abs(a - b) < 1e-9; }
int main()
{
    drizel::MidiInstrument m;
    m.prepare(1000);
    m.setEnvelope(0.01, 0.02);
    check(m.nextAmplitude() == 0, "silent before first note");
    m.noteOn(60, 0.8);
    check(m.pitchOffset(60) == 0, "root is source pitch");
    for (int i = 0; i < 10; ++i) m.nextAmplitude();
    check(near(m.nextAmplitude(), 0.8), "attack reaches velocity");
    m.noteOn(72, 1.0);
    check(m.pitchOffset(60) == 12, "octave maps to 12 semitones");
    m.noteOff(72);
    check(m.activeNote() == 60, "last-note release returns to held key");
    m.noteOff(45);
    check(m.activeNote() == 60, "unmatched off ignored");
    m.noteOn(60, 0);
    for (int i = 0; i < 21; ++i) m.nextAmplitude();
    check(m.nextAmplitude() == 0, "velocity-zero note-on releases to silence");
    m.pitchBend(0); check(m.pitchOffset(60) == -2, "negative bend endpoint");
    m.pitchBend(16383); check(m.pitchOffset(60) == 2, "positive bend endpoint");
    m.pitchBend(8192); check(m.pitchOffset(60) == 0, "centred bend");
    m.controller(1, 127); m.controller(74, 127); m.controller(71, 0);
    check(m.wheel() == 1 && m.position(0.4) == 1 && m.density(50) == 1, "CC mapping");
    m.controller(121, 0);
    check(m.wheel() == 0 && m.position(0.4) == 0.4 && m.density(50) == 50, "reset controllers");
    for (int i = 0; i < 128; ++i) m.noteOn(i, 1);
    for (int i = 0; i < 500; ++i) m.noteOn(127, 1);
    m.noteOff(127); check(m.activeNote() == 126, "bounded repeated note stack");
    m.controller(123, 0); check(m.activeNote() == -1, "all notes off clears keys");
    m.noteOn(60, 1); m.nextAmplitude(); m.controller(120, 0);
    check(m.nextAmplitude() == 0, "all sound off is immediate");
    // Same event timeline, different host block partitions.
    auto render = [](int block)
    {
        drizel::MidiInstrument voice; voice.prepare(1000); voice.setEnvelope(0.01, 0.02);
        std::array<double, 100> out {};
        for (int start = 0; start < 100; start += block)
            for (int i = start; i < std::min(100, start + block); ++i)
            {
                if (i == 17) voice.noteOn(60, 1);
                if (i == 41) voice.noteOff(60);
                out[i] = voice.nextAmplitude();
            }
        return out;
    };
    check(render(1) == render(64), "event timing independent of block partition");
    check(render(64)[16] == 0 && render(64)[17] > 0 && render(64)[62] == 0, "sample-offset gate");
    std::cout << "MIDI instrument tests passed\n";
}
