#include <vector>
#include "audio/Grain.h"
#include "audio/Window.h"
#include "io/WavFile.h"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 3)
        return 1;
    const auto source = drizel::loadWav(argv[1]);
    constexpr int rate = 48000;
    constexpr int length = rate / 10;
    constexpr int interval = rate / 20;
    const auto window = drizel::makeHannWindow(length);
    std::vector<double> positions;
    double position;
    while (std::cin >> position)
        positions.push_back(position);
    if (positions.empty())
        return 1;
    drizel::SampleBuffer output(2, (positions.size() - 1) * interval + length, rate);
    for (std::size_t i = 0; i < positions.size(); ++i)
    {
        drizel::Grain grain;
        if (!grain.start(source, window.data(), length, positions[i], source.sampleRate() / rate,
                         0.5f, 0.0f, drizel::InterpolationMode::hermite))
            return 1;
        for (int j = 0; j < length; ++j)
        {
            const auto value = grain.next();
            output.channelData(0)[i * interval + j] += value.left;
            output.channelData(1)[i * interval + j] += value.right;
        }
    }
    drizel::writeWav(argv[2], output);
}
