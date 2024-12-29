#pragma once

#include "../src/Filters.h"
#include <vector>
#include <numeric>
#include <cstddef>
#include <string>

Signal white_noise(size_t samples);
float average(const Signal& data);
Signal sine_wave(float frequency, float sample_rate, size_t samples);
float rms(const Signal& data);
float peak(const Signal& data);
float db(float value);
int zero_crossings(const Signal& data);
void write_wav_file(const std::string& filename, const Signal& signal, int sample_rate, int channels);
