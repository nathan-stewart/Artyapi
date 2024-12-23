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

float rms(const std::vector<float>& data)
{
    float sum = 0.0f;
    for (float v : data) {
        sum += v * v;
    }
    return sqrtf(sum / float(data.size()));
}

float peak(const std::vector<float>& data)
{
    float peak = 0.0f;
    for (float v : data) {
        v = fabsf(v);
        if (v > peak)
            peak = v;
    }
    return peak;
}
