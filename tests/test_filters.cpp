#include "../src/Filters.h"
#include <gtest/gtest.h>
#include <vector>
#include <boost/circular_buffer.hpp>
#include <cmath>
#include <algorithm>
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

TEST(FilterTest, ImpulseResponse)
{
    // Define the filter coefficients for a simple test case
    FilterCoefficients coeffs = butterworth_filter(4, 0.1f);

    // Create an impulse signal
    std::vector<float> impulse(100, 0.0f);
    impulse[0] = 1.0f;

    // Apply the filter
    apply_filter(coeffs, impulse);

    // Check the impulse response (this is just an example, you should replace it with the expected response)
    std::vector<float> expected_response = { /* expected impulse response values */ };
    for (size_t i = 0; i < expected_response.size(); ++i) {
        EXPECT_NEAR(impulse[i], expected_response[i], 1e-5);
    }
}

TEST(FilterTest, ButterworthStepResponse)
{
    auto filter = butterworth_filter(4, 0.1f);
    std::vector<float> step(1000, 0.0f);
    std::fill(step.begin(), step.begin() + 500, 1.0f);
    apply_filter(filter, step);
    float avg = average(step);
    ASSERT_NEAR(avg, 1.0f, 1e-5);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}