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
    Signal zeros(samples, 0.0f);
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
    Signal ones(samples, 1.0f);
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
    Signal sine_440 = sine_wave(440, float(sample_rate), samples);
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
    Spectrum linear_buffer(1<<14);
    ASSERT_NEAR(bin_to_freq_linear(linear_buffer, static_cast<float>(0), f0, f1), f0, 0.1f);
    ASSERT_NEAR(bin_to_freq_linear(linear_buffer, static_cast<float>(linear_buffer.size()), f0, f1), f1, 0.1f);

    Spectrum log2buffer(1920);
    ASSERT_NEAR(bin_to_freq_log2(log2buffer, 0, f0, f1), f0, 0.1f);
    ASSERT_NEAR(bin_to_freq_log2(log2buffer, static_cast<float>(log2buffer.size()), f0, f1), f1, 0.1f);
}

TEST(AudioProcessorTest, BinMapping)
{
    float f0 = 40.0f;
    float f1 = 20000.0f;
    Spectrum source((1<<14) / 2 + 1);
    Spectrum destination(1920);
    Spectrum mapping = precompute_bin_mapping(source, destination, f0, f1);
    ASSERT_EQ(mapping.size(), source.size());
    ASSERT_NEAR(mapping[mapping.size() - 1], 1920, 0.1f);

    // pick a linear bin that lies between two output bins
    size_t b = static_cast<size_t>(bin_to_freq_log2(destination, 1000, f0, f1));
    float test_freq = (bin_to_freq_log2(destination, static_cast<float>(b), f0, f1) +
                     bin_to_freq_log2(destination, static_cast<float>(b)+1, f0, f1)) / 2.0f;
    float test_bin_lin = freq_to_lin_fractional_bin(source, test_freq, f0, f1);
    float test_bin_log = freq_to_log_fractional_bin(destination, test_freq, f0, f1);

    source[static_cast<size_t>(test_bin_lin)] = 1.0f;
    map_bins(mapping, source, destination);
    ASSERT_EQ(static_cast<size_t>(test_bin_log), b);
    auto just_before = destination.begin() + b - 1;
    ASSERT_NEAR(std::accumulate(just_before, just_before + 4, 0.0f), 1.0f, 0.1f);
}


TEST(AudioProcessorTest, SineSpectrumLinear)
{
    size_t samples = 1<<14;
    float f0 = 40.0f;
    float f1 = 20000.0f;
    SpectrumProcessor sp(1920, 480, samples);

    Signal sine_440 = sine_wave(440, 48000, samples);
    sp(sine_440);
    Spectrum spectrum = sp.get_linear_fft();

    // Nearly all bins should be empty
    size_t non_zero = std::count_if(spectrum.begin(), spectrum.end(), [](float v) { return v > 0.1f; });
    EXPECT_GE(non_zero, 1); // At least one bin should be nonzero
    EXPECT_LE(non_zero, 3); // one peak but allow some leakage

    // Bin Zero (DC) should always be empty - not sure why its not
    EXPECT_LT(spectrum[0], 0.1f);

    // check that the peak is at the right frequency in linear space - look on either side too
    auto peak = std::max_element(spectrum.begin(), spectrum.end());
    float sum = std::accumulate(peak - 1, peak + 1, 0.0f);
    EXPECT_NEAR(sum, 1.0f, 0.2f);

    // check that the peak is at the right frequency - look on either side too
    size_t bin = std::distance(spectrum.begin(), peak);
    EXPECT_LE(bin_to_freq_linear(spectrum, static_cast<float>(bin - 1), f0, f1), 440.0f);
    EXPECT_GE(bin_to_freq_linear(spectrum, static_cast<float>(bin + 1), f0, f1), 440.0f);
}

TEST(AudioProcessorTest, SineSpectrumLog)
{
    size_t samples = 1<<24;
    float f0 = 40.0f;
    float f1 = 20000.0f;

    Signal sine_440 = sine_wave(440, 48000, samples);
    SpectrumProcessor sp(1920, 480, 16834);
    sp(sine_440);
    Spectrum spectrum = sp.get_log2_fft();

    // Nearly all bins should be empty
    size_t almost_zero = std::count_if(spectrum.begin(), spectrum.end(), [](float v) { return v < 0.1f; });
    float virtually_all = static_cast<float>(spectrum.size()) * 0.95f;
    EXPECT_GE(almost_zero, virtually_all);

    // Bin Zero (DC) should always be empty - not sure why its not
    // EXPECT_LT(spectrum[0], 0.0f);

    // At least one bin should be nonzero
    size_t non_zero = std::count_if(spectrum.begin(), spectrum.end(), [](float v) { return v > 0.1f; });
    EXPECT_GT(non_zero, 0);

    // Only one peak - but tolerate some leakage
    EXPECT_LE(non_zero, 3);

    // check that the peak is at the right frequency in linear space - look on either side too
    auto peak = std::max_element(spectrum.begin(), spectrum.end());
    float sum = std::accumulate(peak - 1, peak + 1, 0.0f);
    EXPECT_NEAR(sum, 1.0f, 0.2f);

    // check that the peak is at the right frequency - look on either side too
    size_t bin = std::distance(spectrum.begin(), peak);
    EXPECT_LE(bin_to_freq_log2(spectrum, static_cast<float>(bin - 1), f0, f1), 440.0f);
    EXPECT_GE(bin_to_freq_log2(spectrum, static_cast<float>(bin + 1), f0, f1), 440.0f);
}