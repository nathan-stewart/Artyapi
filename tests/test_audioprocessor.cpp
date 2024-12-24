#include "../src/AudioProcessor.h"
#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

#include "test_util.h"

class AudioProcessorTest : public ::testing::Test {
protected:
    AudioProcessorTest() {}
    ~AudioProcessorTest() override {}
    void SetUp() override {}
    void TearDown() override {}
};


TEST(AudioProcessorTest, VolumeZeros)
{
    AudioProcessor ap;
    size_t samples = 1<<24;
    std::vector<float> zeros(samples, 1.0f);
    ap.process(zeros);
    ASSERT_EQ(get_slice(ap.vrms).size(), 1);
    ASSERT_NEAR(ap.vrms.back(),  0.0f, 0.01f);

    ASSERT_EQ(get_slice(ap.vpk).size(), 1);
    ASSERT_NEAR(ap.vpk.back(), 0.0f, 0.01f);
}


TEST(AudioProcessorTest, VolumeOnes)
{
    AudioProcessor ap;
    size_t samples = 1<<24;
    std::vector<float> ones(samples, 1.0f);
    ap.process(ones);
    ASSERT_EQ(get_slice(ap.vrms).size(), 1);
    ASSERT_NEAR(ap.vrms.back(),  1.0f, 0.01f);

    ASSERT_EQ(get_slice(ap.vpk).size(), 1);
    ASSERT_NEAR(ap.vpk.back(), 1.0f, 0.01f);
}


TEST(AudioProcessorTest, VolumeSine)
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

    std::vector<float> rms = get_slice(ap.vrms);
    std::vector<float> peak = get_slice(ap.vpk);
    EXPECT_EQ(rms.size(), 1);
    EXPECT_EQ(peak.size(), 1);
    ASSERT_NEAR(rms.back(),  -3.0f, 0.1f);
    ASSERT_NEAR(peak.back(),  0.0f, 0.1f);
}


TEST(AudioProcessorTest, BinToFrequency)
{
    float f0 = 40.0f;
    float f1 = 20000.0f;
    std::vector<float> linear_buffer(1<<14);
    ASSERT_NEAR(bin_to_freq_linear(linear_buffer, 0, f0, f1), f0, 0.1f);
    ASSERT_NEAR(bin_to_freq_linear(linear_buffer, linear_buffer.size(), f0, f1), f1, 0.1f);
    
    std::vector<float> log2buffer(1920);
    ASSERT_NEAR(bin_to_freq_log2(log2buffer, 0, f0, f1), f0, 0.1f);
    ASSERT_NEAR(bin_to_freq_log2(log2buffer, log2buffer.size(), f0, f1), f1, 0.1f);
}


TEST(AudioProcessorTest, SpectrumSine)
{
    size_t samples = 1<<24;
    float f0 = 40.0f;
    float f1 = 20000.0f;

    std::vector<float> sine_440 = sine_wave(440, 48000, samples);
    AudioProcessor ap;
    ap.process(sine_440);
    std::vector<float> spectrum = ap.linear_fft;
    
    // Nearly all bins should be empty
    size_t almost_zero = std::count_if(spectrum.begin(), spectrum.end(), [](float v) { return v < 0.1f; });
    float virtually_all = static_cast<float>(spectrum.size()) * 0.95f;
    EXPECT_GE(almost_zero, virtually_all);

    // check that the peak is at the right frequency in linear space
    size_t peak_bin = std::distance(spectrum.begin(), std::max_element(spectrum.begin(), spectrum.end()));
    float peak_freq = bin_to_freq_linear(spectrum, peak_bin, f0, f1);
    EXPECT_NEAR(peak_freq, 440.0f, 6.0f); // 2 x 3Hz bin size in the linear space

    // check that the peak is at the right frequency in log2 space
    spectrum = ap.linear_fft;
    
    // Nearly all bins should be empty
    almost_zero = std::count_if(spectrum.begin(), spectrum.end(), [](float v) { return v < 0.1f; });
    virtually_all = static_cast<float>(spectrum.size()) * 0.95f;
    EXPECT_GE(almost_zero, virtually_all);

    // check that the peak is at the right frequency
    peak_bin = std::distance(spectrum.begin(), std::max_element(spectrum.begin(), spectrum.end()));
    peak_freq = bin_to_freq_log2(spectrum, peak_bin, f0, f1);
    EXPECT_NEAR(peak_freq, 440.0f, 6.0f);
}