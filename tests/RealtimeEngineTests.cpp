#include "audio/RealtimeEngine.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
#include <thread>

// This guard catches allocation AND reclamation on the callback thread.
thread_local bool audioThread = false;
// Keep replacement allocation functions out of line for GCC allocation analysis.
#if defined(__GNUC__)
__attribute__((noinline))
#endif
void* operator new(std::size_t size)
{
    if (audioThread) std::abort();
    if (auto* memory = std::malloc(size == 0 ? 1 : size)) return memory;
    throw std::bad_alloc();
}
#if defined(__GNUC__)
__attribute__((noinline))
#endif
void operator delete(void* memory) noexcept
{
    if (audioThread) std::abort();
    std::free(memory);
}
void operator delete(void* memory, std::size_t) noexcept { ::operator delete(memory); }
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void* memory) noexcept { ::operator delete(memory); }
void operator delete[](void* memory, std::size_t) noexcept { ::operator delete(memory); }

void check(bool result, const char* message)
{
    if (!result) { std::cerr << message << '\n'; std::exit(1); }
}
std::unique_ptr<drizel::PreparedSample> packet(double rate, float value = 1.0f)
{
    drizel::SampleBuffer source(2, 48000, 48000);
    for (std::size_t i = 0; i < source.frameCount(); ++i)
    {
        source.channelData(0)[i] = value;
        source.channelData(1)[i] = value * 0.5f;
    }
    return std::make_unique<drizel::PreparedSample>(std::move(source), rate);
}
drizel::GrainFrame tick(drizel::RealtimeEngine& engine, double position = 0.0,
                        double pitch = 0.0, double density = 100.0, double gain = 1.0,
                        drizel::GrainSelection mode = drizel::GrainSelection::manual)
{
    audioThread = true;
    auto frame = engine.next(position, pitch, density, gain, mode);
    const auto count = engine.activeGrains();
    audioThread = false;
    check(count <= 64, "activity telemetry stays within voice capacity");
    check(std::isfinite(frame.left) && std::isfinite(frame.right)
        && std::abs(frame.left) <= 1.0f && std::abs(frame.right) <= 1.0f, "finite bounded output");
    return frame;
}
void checkFeatureParity()
{
    drizel::SampleBuffer stereo(2, 8192, 48000);
    for (std::size_t i = 3072; i < 7168; ++i)
    {
        stereo.channelData(0)[i] = (i % 2 == 0 ? 0.5f : -0.5f);
        stereo.channelData(1)[i] = -stereo.channelData(0)[i];
    }
    // Golden frames from Step 10 analyze(stereo, 48000, 2048, 512).
    // Null centroid and first-frame flux are represented by zero in C++.
    constexpr std::array<drizel::SampleFeatures::Frame, 16> reference {{
        {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0},
        {0.25, 0.09741192955057806, 23844.13134380339},
        {0.3535533905932738, 0.26669570782361224, 23942.325153715596},
        {0.4330127018922193, 0.18422748352534438, 23984.665289524808},
        {0.5, 0.036987720150996384, 23992.1875},
        {0.5, 0.0, 23992.1875},
        {0.5, 0.0, 23992.1875},
        {0.5, 0.0, 23992.1875},
        {0.5, 0.0, 23992.1875},
        {0.4330127018922193, 0.07828887796144118, 23984.62019789382},
        {0.3535533905932738, 0.09713748248491876, 23942.205320348494},
        {0.28867513459481287, 0.0, 23843.80315147976}
    }};
    drizel::SampleFeatures features(stereo);
    for (std::size_t i = 0; i < reference.size(); ++i)
    {
        const auto frame = features.next(i * drizel::SampleFeatures::hop);
        check(std::abs(frame.rms - reference[i].rms) <= 1e-12, "Step 10 RMS parity");
        check(std::abs(frame.onset - reference[i].onset) <= 1e-12, "Step 10 onset parity");
        check(std::abs(frame.centroid - reference[i].centroid) <= 1e-8, "Step 10 centroid parity");
    }
}

void checkGuidedPlayback()
{
    using drizel::GrainSelection;
    drizel::SampleBuffer source(1, 8192, 48000);
    for (std::size_t i = 3072; i < 7168; ++i)
        source.channelData(0)[i] = (i % 2 == 0 ? 0.5f : -0.5f);
    auto prepared = std::make_unique<drizel::PreparedSample>(std::move(source), 48000);
    auto& map = prepared->selection;
    check(!map.prepare(prepared->source, [] { return true; }), "superseded analysis cancels");
    check(map.start(GrainSelection::audible, 0.0, 0.5) == 0.0, "cancelled map falls back");
    check(map.prepare(prepared->source, [] { return false; }), "analysis completes");
    check(map.start(GrainSelection::manual, 0.5, 0.5) == 0.5, "manual start stays fractional");
    check(map.start(GrainSelection::attack, 0.0, 0.5) < 3072.0, "attack includes pre-roll");
    check(map.start(GrainSelection::sustain, 0.0, 0.5) >= 3072.0, "sustain finds stable audio");
    check(map.start(GrainSelection::bright, 0.0, 0.5) >= 2048.0, "bright finds high frequencies");

    drizel::SampleBuffer localSource(1, 512 * 80, 48000);
    for (std::size_t i = 512 * 48; i < localSource.frameCount(); ++i)
        localSource.channelData(0)[i] = 0.5f;
    drizel::SampleSelection localMap;
    localMap.prepare(localSource, [] { return false; });
    check(localMap.start(GrainSelection::audible, 23.5, 0.5) == 23.5,
          "silent region falls back locally without jumping to another region");
    check(localMap.start(GrainSelection::audible, 512 * 40, 0.5) >= 512 * 40,
          "second region uses its own candidates");

    drizel::RealtimeEngine engine;
    engine.prepare(48000);
    engine.expectRevision(2);
    engine.publish(packet(48000)); // Same rate, but obsolete revision.
    for (int i = 0; i < 1000; ++i)
        check(tick(engine).left == 0.0f, "reject superseded same-rate packet");
    engine.collect();
    prepared->revision = 2;
    engine.publish(std::move(prepared));
    float peak = 0.0f;
    for (int i = 0; i < 10000; ++i)
        peak = std::max(peak, std::abs(tick(engine, 0.0, 0.0, 100.0, 1.0, GrainSelection::bright).left));
    check(peak > 0.001f, "guided selection renders audio through the guarded callback");
}

int main()
{
    checkFeatureParity();
    checkGuidedPlayback();
    for (double rate : {44100.0, 48000.0, 96000.0, 192000.0})
    {
        drizel::RealtimeEngine a, b;
        a.prepare(rate); b.prepare(rate);
        check(tick(a).left == 0.0f, "unloaded silence");
        check(a.activeGrains() == 0, "unloaded activity is zero");
        // Reset both so the smoother histories match.
        a.prepare(rate);
        a.publish(packet(rate)); b.publish(packet(rate));
        float peak = 0.0f;
        // Simulate irregular callbacks, including one-sample and large blocks.
        for (int block : {0, 1, 7, 64, 511, 4096, 17000})
            for (int i = 0; i < block; ++i)
            {
                const auto x = tick(a), y = tick(b);
                check(x.left == y.left && x.right == y.right, "partition-independent stream");
                peak = std::max(peak, std::abs(x.left));
            }
        check(peak > 0.001f, "loaded engine produces audio");
        check(a.activeGrains() > 0, "playing source reports active grains");
        for (int i = 0; i < 50000; ++i) tick(a, 0.0, 24.0, 1.0, 0.0);
        check(std::abs(tick(a, 0.0, 24.0, 1.0, 0.0).left) < 1.0e-5f, "smoothed mute");
        a.collect();
        a.publish(packet(rate, -1.0f)); // Active voices are faded before retirement.
        float previous = 0.0f;
        for (int i = 0; i < 10000; ++i)
        {
            const float value = tick(a).left;
            check(std::abs(value - previous) < 0.03f, "sample swap continuity");
            previous = value;
        }
        a.collect();
        a.prepare(rate); // Device restart clears old playback outside callback.
        check(a.activeGrains() == 0, "restart clears activity");
        a.publish(packet(rate == 48000 ? 44100 : 48000));
        for (int i = 0; i < 1000; ++i) check(tick(a).left == 0.0f, "reject old-rate packet");
        a.collect();
    }
    drizel::RealtimeEngine engine;
    engine.prepare(48000);
    std::atomic<bool> done {false};
    std::thread worker([&]
    {
        for (int i = 0; i < 150; ++i)
        {
            engine.collect();
            engine.publish(packet(48000, (i % 2 == 0) ? 1.0f : -1.0f));
        }
        done.store(true);
    });
    while (!done.load()) tick(engine, 0.25, -12.0);
    worker.join();
    for (int i = 0; i < 2000; ++i) tick(engine);
    engine.collect(); // Drain the return slot so the final pending packet can swap.
    for (int i = 0; i < 2000; ++i) tick(engine);
    engine.collect();
    tick(engine, NAN, INFINITY, NAN, NAN); // Invalid controls retain prior values.
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 480000; ++i) tick(engine, 0.0, 0.0, 100.0);
    const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "Realtime tests passed; 10 seconds at 48 kHz / 100 grains/sec rendered in "
              << seconds << " seconds (aggregate, not a callback deadline guarantee).\n";
}
