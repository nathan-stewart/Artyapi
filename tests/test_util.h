#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <vector>
#include <numeric>
#include <cstddef>

std::vector<float> white_noise(size_t samples);
float average(const std::vector<float>& data);
std::vector<float> sine_wave(float frequency, float sample_rate, size_t samples);
float rms(const std::vector<float>& data);
float peak(const std::vector<float>& data);
float db(float value);
#endif