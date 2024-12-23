#include "../src/Filters.h"
#include <gtest/gtest.h>
#include <vector>
#include <boost/circular_buffer.hpp>
#include <cmath>
#include <algorithm>
#include <random>
#include <cstddef>
#include "test_util.h"
#include <iostream>

TEST(SineWaveGenerator, SineWaveGenerator)
{
    float samplerate = 48000.0f;
    size_t samples = 1 << 16;
    float frequency = 1000.0f; // 1 kHz

    std::vector<float> sine = sine_wave(frequency, samplerate, samples);

    // Calculate the frequency of the generated sine wave
    float max_val = *std::max_element(sine.begin(), sine.end());
    float min_val = *std::min_element(sine.begin(), sine.end());
    float amplitude = (max_val - min_val) / 2.0f;

    ASSERT_NEAR(amplitude, 1.0f, 0.01f); // Check amplitude is close to 1.0
    ASSERT_NEAR(average(sine), 0.0f, 0.01f); // Check average is close to 0.0
    ASSERT_NEAR(rms(sine), 0.707f, 0.01f); // Check RMS is close to 0.707
    ASSERT_NEAR(peak(sine), 1.0f, 0.01f); // Check peak is close to 1.0

    // Check frequency using zero-crossing method
    int zero_crossings = 0;
    for (size_t i = 1; i < sine.size(); ++i) {
        if ((sine[i - 1] < 0 && sine[i] >= 0) || (sine[i - 1] > 0 && sine[i] <= 0)) {
            zero_crossings++;
        }
    }
    float generated_frequency = (static_cast<float>(zero_crossings) / 2.0f) * (samplerate / static_cast<float>(samples));
    ASSERT_NEAR(generated_frequency, frequency, 1.0f); // Check frequency is close to 1 kHz
}

TEST(FilterTest, Butterworth_Sine)
{
    int order = 4; // 24 db/octave
    float cutoff = 1e3f;
    float f0 = cutoff * powf(2.0f, -4.0f);
    float f1 = cutoff * powf(2.0f,  4.0f);

    float samplerate = 48000.0f;
    size_t samples = 1 << 16;
    auto lpf = butterworth_lpf(order, cutoff, samplerate);
    auto hpf = butterworth_hpf(order, cutoff, samplerate);

    // above and below are 4 octaves either side of cutoff  or
    std::vector<float> below = sine_wave(f0, samplerate, samples);
    std::vector<float> above = sine_wave(f1, samplerate, samples);
    ASSERT_GT(peak(above), 0.99f); // unmodified 62.5 Hz signal
    ASSERT_GT(peak(below), 0.99f); // unmodified 16 kHz signal

    apply_filter(hpf, above); // should not be  attenuated
    apply_filter(hpf, below); // should be severely attenuated
    EXPECT_GT(peak(above), 0.95f);
    EXPECT_LT(peak(below), 0.05f);

    // filter modifies the buffer in place- make a new one
    below = sine_wave(f0, samplerate, samples);
    above = sine_wave(f1, samplerate, samples);
    ASSERT_GT(peak(above), 0.99f); // unmodified 62.5 Hz signal
    ASSERT_GT(peak(below), 0.99f); // unmodified 16 kHz signal

    apply_filter(lpf, below); // should not be attentuated
    apply_filter(lpf, above); // should be severely attenuuated
    EXPECT_LT(peak(above), 0.05f);
    EXPECT_GT(peak(below), 0.95f);
}
