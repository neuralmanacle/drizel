#include "audio/Modulation.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

static void check(bool ok, const char* message)
{
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}

int main()
{
    drizel::Lfo sine;
    sine.prepare(100.0);
    sine.setRate(1.0);
    check(std::abs(sine.next()) < 1e-12, "sine starts at zero");
    for (int i = 1; i < 25; ++i) sine.next();
    check(std::abs(sine.next() - 1.0) < 1e-12, "quarter-cycle sine peak");
    for (int i = 26; i < 100; ++i) sine.next();
    check(std::abs(sine.next()) < 1e-11, "one-cycle return");

    drizel::Lfo triangle;
    triangle.prepare(100.0);
    triangle.setRate(1.0);
    triangle.setShape(drizel::LfoShape::triangle);
    check(std::abs(triangle.next() + 1.0) < 1e-12, "triangle starts at -1");
    for (int i = 1; i < 50; ++i) triangle.next();
    check(std::abs(triangle.next() - 1.0) < 1e-11, "triangle midpoint peak");

    drizel::Lfo a, b;
    a.prepare(48000.0); b.prepare(48000.0);
    a.setRate(3.3); b.setRate(3.3);
    for (int block : {1, 17, 64, 511, 4096})
        for (int i = 0; i < block; ++i)
            check(a.next() == b.next(), "block partition independent");

    drizel::ModulationRouter router;
    router.set(drizel::ModDestination::position, 0.0);
    auto c = router.apply(0.5, 2.0, 1.0);
    check(c.position == 0.5 && c.pitch == 2.0, "zero depth is identity");
    router.set(drizel::ModDestination::position, 1.0);
    c = router.apply(0.9, 2.0, 1.0);
    check(c.position == 1.0 && c.pitch == 2.0, "position clamped");
    router.set(drizel::ModDestination::pitch, 1.0);
    c = router.apply(0.5, 20.0, 1.0);
    check(c.pitch == 24.0 && c.position == 0.5, "pitch clamped");
    router.set(drizel::ModDestination::off, 1.0);
    c = router.apply(0.5, 2.0, 1.0);
    check(c.position == 0.5 && c.pitch == 2.0, "off leaves target unchanged");

    std::cout << "Step 7 LFO/modulation tests passed\n";
}
