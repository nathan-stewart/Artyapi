#include "test_util.h"
#include <gtest/gtest.h>
#include <vector>
#include <random>

std::vector<float> white_noise(size_t samples)
{
    std::vector<float> white_noise(samples);
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(-1.0, 1.0);

    for (auto& sample : white_noise) {
        sample = distribution(generator);
    }

    return white_noise;
}

float average(const std::vector<float>& data)
{
    return std::accumulate(data.begin(), data.end(), 0.0f) / float(data.size());
}

std::vector<float> sine_wave(float frequency, float sample_rate, size_t samples)
{
    std::vector<float> sine_wave(samples);
    for (size_t i = 0; i < samples; i++)
        sine_wave[i] = sinf(float(2 * i) * M_PIf * frequency  / sample_rate);
    return sine_wave;
}
