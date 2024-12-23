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

TEST(FilterTest, Butterworth_LPF_Impulse)
{
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
    apply_filter(hpf, below);
    apply_filter(hpf, above);
    EXPECT_GT(peak(above), 0.8f);
    EXPECT_LT(peak(below), 0.2f);

    // filter modifies the buffer in place- make a new one
    below = sine_wave(f0, samplerate, samples);
    above = sine_wave(f1, samplerate, samples);
    apply_filter(lpf, below);
    apply_filter(lpf, above);
    EXPECT_LT(peak(above), 0.2f);
    EXPECT_GT(peak(below), 0.8f);
}

TEST(FilterTest, Butterworth_LPF_Step)
{
}

TEST(FilterTest, Butterworth_HPF_Impulse)
{
}

TEST(FilterTest, Butterworth_HPF_Sine)
{
}

TEST(FilterTest, Butterworth_HPF_Step)
{
}