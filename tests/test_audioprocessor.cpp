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
    std::vector<float> zeros(samples, 0.0f);
    boost::circular_buffer<float> vrms(10);
    boost::circular_buffer<float> vpk(10);
    process_volume(zeros, vrms, vpk);
    ASSERT_EQ(vrms.size(), 1);
    ASSERT_NEAR(vrms.back(),  0.0f, 0.01f);

    ASSERT_EQ(vpk.size(), 1);
    ASSERT_NEAR(vpk.back(), 0.0f, 0.01f);
}


TEST(AudioProcessorTest, VolumeOnes)
{
    size_t samples = 1<<24;
    std::vector<float> ones(samples, 1.0f);
    boost::circular_buffer<float> vrms(10);
    boost::circular_buffer<float> vpk(10);

    process_volume(ones, vrms, vpk);
    ASSERT_EQ(vrms.size(), 1);
    ASSERT_NEAR(vrms.back(),  1.0f, 0.01f);

    ASSERT_EQ(vpk.size(), 1);
    ASSERT_NEAR(vpk.back(), 1.0f, 0.01f);
}


TEST(AudioProcessorTest, VolumeSine)
{
    size_t samples = 1<<16;
    size_t sample_rate = 48000;
    boost::circular_buffer<float> vrms(10);
    boost::circular_buffer<float> vpk(10);
    std::vector<float> sine_440 = sine_wave(440, float(sample_rate), samples);
    process_volume(sine_440, vrms, vpk);

    EXPECT_EQ(vrms.size(), 1);
    EXPECT_EQ(vpk.size(), 1);
    ASSERT_NEAR(db(vrms.back()),  -3.0f, 0.1f);
    ASSERT_NEAR(db(vpk.back()),  0.0f, 0.1f);
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

TEST(AudioProcessorTest, BinMapping)
{
    float f0 = 40.0f;
    float f1 = 20000.0f;
    std::vector<float> linear_fft(1<<14);
    std::vector<float> log_fft(1920);
    std::vector<BinMapping> mapping = precompute_bin_mapping(linear_fft, log_fft, f0, f1);
    ASSERT_EQ(mapping.size(), linear_fft.size());
    ASSERT_EQ(mapping[mapping.size() - 1].index, 1919);
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

    // Bin Zero (DC) should always be empty
    EXPECT_LT(spectrum[0], 0.0f);

    // At least one bin should be nonzero
    size_t non_zero = std::count_if(spectrum.begin(), spectrum.end(), [](float v) { return v > 0.1f; });
    EXPECT_GT(non_zero, 0);
    EXPECT_LE(non_zero, 5); // Only one peak - but tolerate some leakage

    // check that the peak is at the right frequency in linear space
    size_t peak_bin = std::distance(spectrum.begin(), std::max_element(spectrum.begin(), spectrum.end()));
    float peak_freq = bin_to_freq_linear(spectrum, peak_bin, f0, f1);
    EXPECT_NEAR(peak_freq, 440.0f, 6.0f); // 2 x 3Hz bin size in the linear space

    // check that the peak is at the right frequency in log2 space
    spectrum = sp.get_log2_fft();

    // check that the peak is at the right frequency
    peak_bin = std::distance(spectrum.begin(), std::max_element(spectrum.begin(), spectrum.end()));

    ASSERT_GT(peak_bin, 0);
    ASSERT_LT(peak_bin, spectrum.size());
    peak_freq = bin_to_freq_log2(spectrum, peak_bin, f0, f1);
    // EXPECT_NEAR(peak_freq, 440.0f, 6.0f);

    std::cout << "==== peak bin " << peak_bin << " of " << spectrum.size() << " =======================" << std::endl;
    for (size_t i = peak_bin - 5; i < peak_bin + 5; ++i)
    {
        if (((i +5) > spectrum.size()) or (i < 5))
        {
            continue;
        }
        float freq = bin_to_freq_log2(spectrum, i, f0, f1);
        std::cout << "bin: " << i << " freq: " << freq << " = " << spectrum[i] << std::endl;
    }
    std::cout << "===========================" << std::endl;

    float next_freq = bin_to_freq_log2(spectrum, peak_bin + 1, f0, f1);
    EXPECT_NEAR(next_freq, 440.0f * powf(2.0f, 1.0f/12.0f), 6.0f); // 1 semitone

}