#include "../src/AudioProcessor.h"
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

TEST(AudioProcessorTest, Decay)
{
    // Test calculate_decay_rates - takes a spectral history and calculates decay rates

    boost::circular_buffer<Spectrum> history;
     calculate decay rates
Spectrum calculate_decay_rates(const SpectralHistory& spectral_history, size_t fft_bins, size_t fft_history) {
    Spectrum decay_rates(fft_bins, 0.0f);

    for (size_t bin = 0; bin < fft_bins; ++bin) {
        float sum = 0.0f;
        for (size_t history = 0; history < fft_history; ++history) {
            sum += spectral_history[history][bin];
        }
        decay_rates[bin] = sum / static_cast<float>(fft_history);
    }

    return decay_rates;
}

void AudioProcessor::process(const Signal& data)
{
    SpectralHistory hist_copy;
    vector<Spectrum> ffts = spectrum(data);
    {
        lock_guard<mutex> lock(historyMutex);
        history.insert(history.end(), ffts.begin(), ffts.end());
        current_rates = calculate_decay_rates(hist_copy, Spectrum& decay);
    }
}
