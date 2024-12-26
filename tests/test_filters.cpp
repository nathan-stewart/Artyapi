#include "../src/Filters.h"
#include <gtest/gtest.h>
#include <vector>
#include <boost/circular_buffer.hpp>
#include <cmath>
#include <map>
#include <string>
#include <algorithm>
#include <random>
#include <cstddef>
#include "test_util.h"
#include <iostream>

TEST(SineWaveGenerator, SineWaveGenerator)
{
    float samplerate = 48000.0f;
    size_t samples = 1 << 16;
    float frequency = 1000.0f; // 1 kHz

    std::vector<float> sine = sine_wave(frequency, samplerate, samples);

    // Calculate the frequency of the generated sine wave
    float max_val = *std::max_element(sine.begin(), sine.end());
    float min_val = *std::min_element(sine.begin(), sine.end());
    float amplitude = (max_val - min_val) / 2.0f;

    ASSERT_NEAR(amplitude, 1.0f, 0.01f);     // Check amplitude is close to 1.0
    ASSERT_NEAR(average(sine), 0.0f, 0.01f); // Check average is close to 0.0
    ASSERT_NEAR(rms(sine), 0.707f, 0.01f);   // Check RMS is close to 0.707
    ASSERT_NEAR(peak(sine), 1.0f, 0.01f);    // Check peak is close to 1.0

    float generated_frequency = (static_cast<float>(zero_crossings(sine)) / 2.0f) * (samplerate / static_cast<float>(samples));
    ASSERT_NEAR(generated_frequency, frequency, 1.0f); // Check frequency is close to 1 kHz
}


TEST(NoiseGenerator, Noise)
{
    size_t samples = 1 << 16;
    std::vector<float> noise = white_noise(samples);
    ASSERT_EQ(noise.size(), samples);
    ASSERT_GT(*std::max_element(noise.begin(), noise.end()), -1.0f);
    ASSERT_LE(*std::max_element(noise.begin(), noise.end()),  1.0f);
    float avg = average(noise);
    ASSERT_NEAR(avg, 0.0f, 0.1f);
}


TEST(FilterTest, Butterworth_Coefficients)
{
    // Generated coefficients for a 4th order 40Hz, 1kHz, and 20kHz LPF/HPF generated in octave
    int order = 4;
    float samplerate = 48000.0f;
    std::vector<std::tuple<std::string, float, FilterCoefficients>> TruthTable = {
            {"HPF", 40.0f, { {0.9931822f, -3.9727288f, 5.9590931f, -3.9727288f, 0.9931822f}, {1.0000000f, -3.9863177f, 5.9590467f, -3.9591398f, 0.9864109f}}},
            {"HPF",  1e3f, { {0.8426766f, -3.3707065f, 5.0560598f, -3.3707065f, 0.8426766f}, {1.0000000f, -3.6580603f, 5.0314335f, -3.0832283f, 0.7101039f}}},
            {"LPF",  1e3f, { {0.0000156f,  0.0000622f, 0.0000933f,  0.0000622f, 0.0000156f}, {1.0000000f, -3.6580603f, 5.0314335f, -3.0832283f, 0.7101039f}}},
            {"LPF", 20e3f, { {0.4998150f,  1.9992600f, 2.9988900f,  1.9992600f, 0.4998150f}, {1.0000000f,  2.6386277f, 2.7693098f,  1.3392808f, 0.2498217f}}}
        };

    for (const auto& [name, freq, truth] : TruthTable)
    {
        FilterCoefficients computed;
        if (name == "HPF")
        {
            computed = butterworth(order, freq, samplerate, true);
        }
        else
        {
            computed = butterworth(order, freq, samplerate, false);
        }
        std::cout << "Computed: " << name << " " << freq << "Hz" << std::endl;

        ASSERT_EQ(computed.first.size(), order + 1);
        ASSERT_EQ(computed.second.size(), order + 1);
        for (int i = 0; i < order + 1; ++i)
        {
            // close but not super tight tolerance
            ASSERT_NEAR(computed.first[i], std::get<0>(truth)[i], 1e-5f);
            ASSERT_NEAR(computed.second[i], std::get<1>(truth)[i], 1e-5f);
        }
    }
}


TEST(FilterTest, Butterworth_Sine_HPF)
{
    float samplerate = 48000.0f;
    size_t samples = 1 << 16;
    float f0 = 1000.0f * powf(2.0f, -4.0f); // 62.5 Hz
    float f1 = 1000.0f;                     // 1 kHz
    float f2 = 1000.0f * powf(2.0f,  4.0f); // 16 kHz

    FilterCoefficients hpf = { {0.8426766f, -3.3707065f, 5.0560598f, -3.3707065f, 0.8426766f}, {1.0000000f, -3.6580603f, 5.0314335f, -3.0832283f, 0.7101039f} };

    // above and below are 4 octaves either side of cutoff  or
    std::vector<float> below = sine_wave(f0, samplerate, samples);
    std::vector<float> cutoff = sine_wave(f1, samplerate, samples);
    std::vector<float> above = sine_wave(f2, samplerate, samples);
    apply_filter(hpf, above);
    apply_filter(hpf, cutoff);
    apply_filter(hpf, below);
    ASSERT_NEAR(average(below), 0.0f, 0.01f); // Check for DC offset
    EXPECT_LT(  db(rms(below)),  -60.0f);       // should be severely attenuated
    EXPECT_NEAR(db(rms(cutoff)), -6.0f, 0.2f);  // should be  attenuated -3db
    EXPECT_NEAR(db(rms(above)),  -3.0f, 0.2f);  // should not be  attenuated
}

TEST(FilterTest, Butterworth_Sine_LPF)
{
    float samplerate = 48000.0f;
    size_t samples = 1 << 16;
    float f0 = 1000.0f * powf(2.0f, -4.0f); // 62.5 Hz
    float f1 = 1000.0f;                     // 1 kHz
    float f2 = 1000.0f * powf(2.0f,  4.0f); // 16 kHz

    FilterCoefficients lpf ={ {0.0000156f, 0.0000622f, 0.0000933f, 0.0000622f, 0.0000156f}, {1.0000000f, -3.6580603f, 5.0314335f, -3.0832283f, 0.7101039f} };

    // above and below are 4 octaves either side of cutoff  or
    std::vector<float> below = sine_wave(f0, samplerate, samples);
    std::vector<float> cutoff = sine_wave(f1, samplerate, samples);
    std::vector<float> above = sine_wave(f2, samplerate, samples);

    apply_filter(lpf, below);
    apply_filter(lpf, cutoff);
    apply_filter(lpf, above);

    EXPECT_NEAR(db(rms(below)),  -3.0f, 0.2f);  // should not be attentuated
    EXPECT_NEAR(db(rms(cutoff)), -6.0f, 0.2f);  // should be  attenuated -3db
    EXPECT_LT(  db(rms(above)),  -60.0f);       // should be severely attenuated
}

TEST(FilterTest, HPF_LPF)
{
    float samplerate = 48000.0f;
    size_t samples = 1 << 14;
    FilterCoefficients hpf = butterworth(4, 40.0f, samplerate, true);
    FilterCoefficients lpf = butterworth(4, 20000.0f, samplerate, false);
    std::vector<float> sine_1khz = sine_wave(1000.0f, samplerate, samples);

    EXPECT_NEAR(db(rms(sine_1khz)), 0.0f, 0.1f); // 0db in the passband
    apply_filter(hpf, sine_1khz);
    EXPECT_NEAR(db(rms(sine_1khz)), 0.0f, 0.1f); // 0db in the passband
    apply_filter(lpf, sine_1khz);
    EXPECT_NEAR(db(rms(sine_1khz)), 0.0f, 0.1f); // 0db in the passband
    EXPECT_NEAR(average(sine_1khz), 0.0f, 0.01f); // Check for DC offset

    float generated_frequency = (static_cast<float>(zero_crossings(sine_1khz)) / 2.0f) * (samplerate / static_cast<float>(samples));
    ASSERT_NEAR(generated_frequency, 1000.0f, 1.0f); // Check frequency is close to 1 kHz
}


TEST(WindowTest, Hanning)
{
    size_t ws = 1 << 14;
    std::vector<float> window = hanning_window(ws);
    // Verify symmetry

    for (size_t i = 0; i < ws / 2; ++i) {
        ASSERT_NEAR(window[i], window[ws - 1 - i], 1e-6);
    }

    // Verify specific values
    ASSERT_NEAR(window[0], 0.0f, 1e-6);
    ASSERT_NEAR(window[ws / 2], 1.0f, 1e-6);
    ASSERT_NEAR(window[ws - 1], 0.0f, 1e-6);
}