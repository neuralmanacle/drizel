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
                        double pitch = 0.0, double density = 100.0, double gain = 1.0)
{
    audioThread = true;
    auto frame = engine.next(position, pitch, density, gain);
    audioThread = false;
    check(std::isfinite(frame.left) && std::isfinite(frame.right)
        && std::abs(frame.left) <= 1.0f && std::abs(frame.right) <= 1.0f, "finite bounded output");
    return frame;
}
int main()
{
    for (double rate : {44100.0, 48000.0, 96000.0, 192000.0})
    {
        drizel::RealtimeEngine a, b;
        a.prepare(rate); b.prepare(rate);
        check(tick(a).left == 0.0f, "unloaded silence");
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
