#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include "../src/Filters.h"
#include <vector>
#include <numeric>
#include <cstddef>

Signal white_noise(size_t samples);
float average(const Signal& data);
Signal sine_wave(float frequency, float sample_rate, size_t samples);
float rms(const Signal& data);
float peak(const Signal& data);
float db(float value);
int zero_crossings(const Signal& data);
#endif