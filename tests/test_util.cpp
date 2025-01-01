#include "test_util.h"
#include <gtest/gtest.h>
#include <vector>
#include <random>
#include <sndfile.h>

Signal white_noise(size_t samples)
{
    std::vector<float> white_noise(samples);
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(-1.0, 1.0);

    for (auto& sample : white_noise) {
        sample = distribution(generator);
    }

    return white_noise;
}

float average(const Signal& data)
{
    return std::accumulate(data.begin(), data.end(), 0.0f) / float(data.size());
}

Signal sine_wave(float frequency, float sample_rate, size_t samples)
{
    Signal sine_wave(samples);
    float amplitude = 1.0f;
    float phase = 0.0f;
    float increment = 2.0f * M_PIf * frequency / sample_rate;
    for (size_t i = 0; i < samples; ++i) {
        sine_wave[i] = amplitude * std::sin(phase);
        phase += increment;
    }
    return sine_wave;
}

float rms(const Signal& data)
{
    float sum = 0.0f;
    for (float v : data) {
        sum += v * v;
    }
    return sqrtf(sum / float(data.size()));
}

float peak(const Signal& data)
{
    float peak = 0.0f;
    for (float v : data) {
        v = fabsf(v);
        if (v > peak)
            peak = v;
    }
    return peak;
}

float db(float value)
{
    return 20.0f * log10f(abs(value) + 1e-7f);
}

int zero_crossings(const Signal& data)
{
    int crossings = 0;
    for (size_t i = 1; i < data.size(); ++i)
    {
        if ((data[i - 1] < 0 && data[i] >= 0) || (data[i - 1] > 0 && data[i] <= 0))
        {
            crossings++;
        }
    }
    return crossings;
}


void write_wav_file(const std::string& filename, const Signal& signal, int sample_rate) 
{
    SF_INFO sfinfo;
    sfinfo.frames = signal.size();
    sfinfo.samplerate = sample_rate;
    sfinfo.channels = 1; // mono
    sfinfo.format = SF_FORMAT_RAW | SF_FORMAT_PCM_24;

    SNDFILE* outfile = sf_open(filename.c_str(), SFM_WRITE, &sfinfo);
    if (!outfile) {
        throw std::runtime_error("Failed to open WAV file for writing: " + filename);
    }

    sf_write_float(outfile, signal, signal.size());
    sf_close(outfile);
}
