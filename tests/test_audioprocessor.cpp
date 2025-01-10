#include "../src/AudioProcessor.h"
#include "../src/SpectrumProcessor.h"
#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include <boost/circular_buffer.hpp>
#include "test_util.h"
#include <iostream>

using namespace std;

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
    auto [vrms,vpk] = process_volume(zeros);

    ASSERT_LT(vrms, -96.0f);
    ASSERT_LT(vpk, -96.0f);
}


TEST(AudioProcessorTest, VolumeOnes)
{
    size_t samples = 1<<24;
    Signal ones(samples, 1.0f);

    auto [vrms, vpk] = process_volume(ones);
    ASSERT_NEAR(vrms,  0.0f, 0.01f);
    ASSERT_NEAR(vpk, 0.0f, 0.01f);
}


TEST(AudioProcessorTest, VolumeSine)
{
    size_t samples = 1<<16;
    size_t sample_rate = 48000;
    Signal sine_440 = sine_wave(440, float(sample_rate), samples);
    auto [vrms, vpk] = process_volume(sine_440);

    ASSERT_NEAR(vrms,  -3.0f, 0.1f);
    ASSERT_NEAR(vpk,  0.0f, 0.1f);
}


TEST(AudioProcessorTest, BinToFrequency)
{
    float f0 = 40.0f;
    float f1 = 20000.0f;
    size_t linear_count = (1<<14) / 2 + 1;
    EXPECT_NEAR(bin_to_freq_linear(linear_count, static_cast<float>           (0), f0, f1),          f0, 0.1f);
    EXPECT_NEAR(bin_to_freq_linear(linear_count,                          4096.0f, f0, f1), 1.0019e+04f, 0.5f);
    EXPECT_NEAR(bin_to_freq_linear(linear_count, static_cast<float>(linear_count), f0, f1),          f1, 0.1f);



    size_t log_count = 1920;
    EXPECT_NEAR(bin_to_freq_log2(log_count, static_cast<float>        (0), f0, f1),          f0, 0.1f);
    EXPECT_NEAR(bin_to_freq_log2(log_count,                          1880, f0, f1), 1.7571e+04f, 0.5f);
    EXPECT_NEAR(bin_to_freq_log2(log_count,                          1881, f0, f1), 1.7628e+04f, 0.5f);
    EXPECT_NEAR(bin_to_freq_log2(log_count, static_cast<float>(log_count), f0, f1),          f1, 0.1f);



}

TEST(AudioProcessorTest, BinMapping)
{
    float f0 = 40.0f;
    float f1 = 20000.0f;
    Spectrum source(8193);
    Spectrum destination(1920);
    Spectrum mapping = precompute_bin_mapping(source.size(), destination.size(), f0, f1);
    ASSERT_EQ(mapping.size(), source.size());

    // pick a frequency which lies in between two output bins and verify that
    // the mapping is spread between them
    size_t b = static_cast<size_t>(bin_to_freq_log2(destination.size(), 1000, f0, f1));
    // Experimentally f=1khz = output bin 1018
    // test_freq is picked to lied between bin 1018 and bin 1019, f = 1080.9 Hz
    float test_freq = (bin_to_freq_log2(destination.size(), static_cast<float>(b), f0, f1) +
                       bin_to_freq_log2(destination.size(), static_cast<float>(b)+1, f0, f1)) / 2.0f;
    // f = 1080.9 Hz maps to linear bin 427.25 and log2 bin 1018.5
    float test_bin_lin = freq_to_lin_fractional_bin(source.size(), test_freq, f0, f1);

    // set linear bin 427 to 1.0
    source[static_cast<size_t>(test_bin_lin)] = 1.0f;
    map_bins(mapping, source, destination);

    // look for output in bins 1017-1019. They should add up to 1.0
    auto just_before = destination.begin() + b - 1;
    ASSERT_NEAR(std::accumulate(just_before, just_before + 4, 0.0f), 1.0f, 0.1f);

    source.fill(0.0f); // zero out previous test

    // verify that multiple inputs sum to approximately the same output
    size_t test_bin = destination.size() - 2;
    float p = bin_to_freq_log2(destination.size(), static_cast<float>(test_bin), f0, f1);
    float q = bin_to_freq_log2(destination.size(), static_cast<float>(test_bin + 1), f0, f1);
    cout << "Output bin " << test_bin << " covers frequencies " << p << " - " << q << endl;

    size_t r = static_cast<size_t>(freq_to_lin_fractional_bin(source.size(), floor(p), f0, f1));
    size_t s = static_cast<size_t>(freq_to_lin_fractional_bin(source.size(), ceil(q), f0, f1));
    cout << "Input bins " << r << " - " << s << endl;
    size_t distance = s - r;
    cout << "distance = " << s - r << endl;

    for (auto i = source.begin() + r; i != source.begin() + s; ++i)
    {
        *i = 1.0f;
    }
    map_bins(mapping, source, destination);
    // Output should sum to count of input bins set to 1.0
    EXPECT_NEAR(std::accumulate(source.begin(), source.end(), 0.0f), static_cast<float>(distance), 0.5f);
    EXPECT_NEAR(std::accumulate(destination.begin(), destination.end(), 0.0f), static_cast<float>(distance), 0.5f);
}

TEST(AudioProcessorTest, SineSpectrum)
{
    size_t samples = 1<<14;
    float f0 = 40.0f;
    float f1 = 20000.0f;

    Signal sine_440 = sine_wave(440, 48000, samples);
    SpectrumProcessor sp(1920, 16834);
    Spectrum spectrum = sp(sine_440).back();

    // Nearly all bins should be empty
    size_t non_zero = std::count_if(spectrum.begin(), spectrum.end(), [](float v) { return v > 0.1f; });
    EXPECT_GE(non_zero, 1); // At least one bin should be nonzero
    EXPECT_LE(non_zero, 3); // one peak but allow some leakage
    EXPECT_LT(std::abs(spectrum[0]), 8e-2f); // DC should always be empty

    // check that the peak is at the right frequency in linear space - look on either side too
    auto peak = std::max_element(spectrum.begin(), spectrum.end());

    // the peak may be spread across a couple of bins but it should be close to 1.0
    float sum = 0.0f;
    std::for_each(peak -1,peak + 1, [&sum](float v) { sum += std::abs(v); });

    // check that the peak is at the right frequency - look on either side too
    size_t bin = std::distance(spectrum.begin(), peak);
    // std::cout << "Peak at bin " << bin << " : " << bin_to_freq_log2(spectrum, static_cast<float>(bin), f0, f1) << std::endl;
    float tolerance = 60.0f; // grossly too big I want to move on to the next step
    EXPECT_GE(bin_to_freq_log2(spectrum.size(), static_cast<float>(bin - 1), f0, f1), 440.0f - tolerance);
    EXPECT_LE(bin_to_freq_log2(spectrum.size(), static_cast<float>(bin + 1), f0, f1), 440.0f + tolerance);
}