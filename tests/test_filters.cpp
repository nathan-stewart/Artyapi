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
    auto filter = butterworth_lpf(4, 0.1f);
    std::vector<float> impulse(1000, 0.0f);
    impulse[0] = 1.0f;
    apply_filter(filter, impulse);
    float avg = average(impulse);
    ASSERT_NEAR(avg, 0.0f, 1e-5);
}

TEST(FilterTest, Butterworth_LPF_Sine)
{
    auto filter = butterworth_lpf(4, 0.1f);
    std::vector<float> sine = sine_wave(0.1f, 1.0f, 1000);
    apply_filter(filter, sine);
    float avg = average(sine);
    ASSERT_NEAR(avg, 0.0f, 1e-5);
}

TEST(FilterTest, Butterworth_LPF_Step)
{
    auto filter = butterworth_lpf(4, 0.1f);
    std::vector<float> step(1000, 0.0f);
    std::fill(step.begin(), step.begin() + 100, 1.0f);
    apply_filter(filter, step);
    float avg = average(step);
    ASSERT_NEAR(avg, 1.0f, 1e-5);
}

TEST(FilterTest, Butterworth_HPF_Impulse)
{
    auto filter = butterworth_hpf(4, 0.1f);
    std::vector<float> impulse(1000, 0.0f);
    impulse[0] = 1.0f;
    apply_filter(filter, impulse);
    float avg = average(impulse);
    ASSERT_NEAR(avg, 0.0f, 1e-5);
}

TEST(FilterTest, Butterworth_HPF_Sine)
{
    auto filter = butterworth_hpf(4, 0.1f);
    std::vector<float> sine = sine_wave(0.1f, 1.0f, 1000);
    apply_filter(filter, sine);
    float avg = average(sine);
    ASSERT_NEAR(avg, 0.0f, 1e-5);
}

TEST(FilterTest, Butterworth_HPF_Step)
{
    auto filter = butterworth_hpf(4, 0.1f);
    std::vector<float> step(1000, 0.0f);
    std::fill(step.begin(), step.begin() + 100, 1.0f);
    apply_filter(filter, step);
    float avg = average(step);
    ASSERT_NEAR(avg, 0.0f, 1e-5);
}