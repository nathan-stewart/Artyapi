#include "../src/AudioProcessor.h"
#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

#include "test_util.h"

TEST(AudioProcessor, VolumeNoise)
{
    size_t samples = 1<<24;

    std::vector<float> zeros = std::vector<float>(samples, 0.0f);
    {
        AudioProcessor ap;
        ap.process(zeros);
        std::vector<float> rms = ap.Vrms();
        std::vector<float> peak = ap.Vpeak();
        ASSERT_EQ(rms.size(), 1);
        ASSERT_LT(rms[0], -90.0f);

        ASSERT_EQ(peak.size(), 1);
        ASSERT_LT(peak[0], -90.0f);
    }

    std::vector<float> ones = std::vector<float>(samples, 1.0f);
    {
        AudioProcessor ap;
        ap.process(ones);
        std::vector<float> rms = ap.Vrms();
        std::vector<float> peak = ap.Vpeak();
        ASSERT_EQ(rms.size(), 1);
        ASSERT_NEAR(rms[0],  0.0f, 0.01f);
        ASSERT_NEAR(peak[0], 0.0f, 0.01f);
    }

    std::vector<float> noise = white_noise(samples);
    ASSERT_EQ(noise.size(), samples);
    ASSERT_GT(*std::max_element(noise.begin(), noise.end()), -1.0f);
    ASSERT_LE(*std::max_element(noise.begin(), noise.end()),  1.0f);
    float avg = average(noise);
    ASSERT_NEAR(avg, 0.0f, 0.1f);
}

TEST(AudioProcessor, VolumeSine)
{
    size_t sample_rate = 48000;
    size_t samples = 1<<16;

    AudioProcessor ap;

    std::vector<float> sine_440 = sine_wave(440, float(sample_rate), samples);
    ASSERT_GE(*std::min_element(sine_440.begin(), sine_440.end()), -1.0);
    ASSERT_LE(*std::max_element(sine_440.begin(), sine_440.end()),  1.0);
    float avg = average(sine_440);
    ASSERT_NEAR(avg, 0.0f, 0.1f);
    ap.process(sine_440);

    std::vector<float> rms = ap.Vrms();
    std::vector<float> peak = ap.Vpeak();
    EXPECT_EQ(rms.size(), 1);
    EXPECT_EQ(peak.size(), 1);
    ASSERT_NEAR(ap.Vrms()[0], -3.0f, 0.1f);
    ASSERT_NEAR(ap.Vpeak()[0],  0.0f, 0.1f);
}


TEST(AudioProcessor, BinToFrequency)
{
    float f0 = 40.0f;
    float f1 = 20000.0f;
    std::vector<float> linear_buffer(1<<14);
    ASSERT_NEAR(linbin_to_freq(linear_buffer, 0, f0, f1), f0, 0.1f);
    ASSERT_NEAR(linbin_to_freq(linear_buffer, linear_buffer.size(), f0, f1), f1, 0.1f);
    
    std::vector<float> log2buffer(1920);
    ASSERT_NEAR(logbin_to_freq(log2buffer, 0, f0, f1), f0, 0.1f);
    ASSERT_NEAR(logbin_to_freq(log2buffer, log2buffer.size(), f0, f1), f1, 0.1f);
}


TEST(AudioProcessor, SpectrumSine)
{
    size_t samples = 1<<24;
    float f0 = 40.0f;
    float f1 = 20000.0f;

    std::vector<float> sine_440 = sine_wave(440, 48000, samples);
    AudioProcessor ap;
    ap.process(sine_440);
    std::vector<float> spectrum = ap.Spectrum();
    
    // Nearly all bins should be empty
    size_t almost_zero = std::count_if(spectrum.begin(), spectrum.end(), [](float v) { return v < 0.1f; });
    float virtually_all = static_cast<float>(spectrum.size()) * 0.95f;
    EXPECT_GE(almost_zero, virtually_all);

    // check that the peak is at the right frequency
    size_t peak_bin = std::distance(spectrum.begin(), std::max_element(spectrum.begin(), spectrum.end()));
    float peak_freq = logbin_to_freq(spectrum, peak_bin, f0, f1);
    EXPECT_NEAR(peak_freq, 440.0f, 1.0f);
}