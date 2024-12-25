#include "../src/AudioProcessor.h"
#include "../src/SpectrumProcessor.h"
#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include <boost/circular_buffer.hpp>
#include "test_util.h"

class AudioProcessorTest : public ::testing::Test
{
protected:
    AudioProcessorTest() {}
    ~AudioProcessorTest() override {}
    void SetUp() override {}
    void TearDown() override {}
};


TEST(VolumeProcessorTest, VolumeZeros)
{
    size_t samples = 1<<24;
    std::vector<float> zeros(samples, 1.0f);
    boost::circular_buffer<float> vrms;
    boost::circular_buffer<float> vpk;
    process_volume(zeros, vrms, vpk);
    ASSERT_EQ(vrms.size(), 1);
    ASSERT_NEAR(vrms.back(),  0.0f, 0.01f);

    ASSERT_EQ(vpk.size(), 1);
    ASSERT_NEAR(vpk.back(), 0.0f, 0.01f);
}


TEST(AudioProcessorTest, VolumeOnes)
{
    AudioProcessor ap;
    size_t samples = 1<<24;
    std::vector<float> ones(samples, 1.0f);
    boost::circular_buffer<float> vrms;
    boost::circular_buffer<float> vpk;

    process_volume(ones, vrms, vpk);
    ASSERT_EQ(vrms.size(), 1);
    ASSERT_NEAR(vrms.back(),  1.0f, 0.01f);

    ASSERT_EQ(vpk.size(), 1);
    ASSERT_NEAR(vpk.back(), 1.0f, 0.01f);
}


TEST(AudioProcessorTest, VolumeSine)
{
    size_t sample_rate = 48000;
    size_t samples = 1<<16;

    std::vector<float> sine_440 = sine_wave(440, float(sample_rate), samples);
    boost::circular_buffer<float> vrms;
    boost::circular_buffer<float> vpk;

    process_volume(sine_440, vrms, vpk);
    EXPECT_EQ(vrms.size(), 1);
    EXPECT_EQ(vpk.size(), 1);
    ASSERT_NEAR(vrms.back(),  -3.0f, 0.1f);
    ASSERT_NEAR(vpk.back(),  0.0f, 0.1f);
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
    SpectrumProcessor sp(1920, 480, 16834);
    sp(sine_440);
    std::vector<float> spectrum = sp.get_linear_fft();

    // Nearly all bins should be empty
    size_t almost_zero = std::count_if(spectrum.begin(), spectrum.end(), [](float v) { return v < 0.1f; });
    float virtually_all = static_cast<float>(spectrum.size()) * 0.95f;
    EXPECT_GE(almost_zero, virtually_all);

    // check that the peak is at the right frequency in linear space
    size_t peak_bin = std::distance(spectrum.begin(), std::max_element(spectrum.begin(), spectrum.end()));
    float peak_freq = bin_to_freq_linear(spectrum, peak_bin, f0, f1);
    EXPECT_NEAR(peak_freq, 440.0f, 6.0f); // 2 x 3Hz bin size in the linear space

    // check that the peak is at the right frequency in log2 space
    spectrum = sp.get_log2_fft();

    // Nearly all bins should be empty
    almost_zero = std::count_if(spectrum.begin(), spectrum.end(), [](float v) { return v < 0.1f; });
    virtually_all = static_cast<float>(spectrum.size()) * 0.95f;
    EXPECT_GE(almost_zero, virtually_all);

    // check that the peak is at the right frequency
    peak_bin = std::distance(spectrum.begin(), std::max_element(spectrum.begin(), spectrum.end()));
    peak_freq = bin_to_freq_log2(spectrum, peak_bin, f0, f1);
    EXPECT_NEAR(peak_freq, 440.0f, 6.0f);
}