#include "../src/Filters.h"
#include <gtest/gtest.h>
#include <vector>
#include <boost/circular_buffer.hpp>
#include <cmath>
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

    // Check frequency using zero-crossing method
    int zero_crossings = 0;
    for (size_t i = 1; i < sine.size(); ++i)
    {
        if ((sine[i - 1] < 0 && sine[i] >= 0) || (sine[i - 1] > 0 && sine[i] <= 0))
        {
            zero_crossings++;
        }
    }
    float generated_frequency = (static_cast<float>(zero_crossings) / 2.0f) * (samplerate / static_cast<float>(samples));
    ASSERT_NEAR(generated_frequency, frequency, 1.0f); // Check frequency is close to 1 kHz
}

TEST(FilterTest, Butterworth_Coefficients)
{
    // This is currently a null test since the coefficients are hardcoded
    // int order = 4;
    // float cutoff = 1000.0f;
    // float samplerate = 48000.0f;
    // FilterCoefficients hpf = butterworth_hpf(order, cutoff, samplerate);
    // FilterCoefficients lpf = butterworth_lpf(order, cutoff, samplerate);

    // Generated coefficients for a 4th order 1kHz LPF,HPF generated in octave via:
    //
    /*
    % Generate Butterworth high-pass filter
    function formatted_str = format_coefficients(coeffs)
        formatted_str = '{';
        for i = 1:length(coeffs)
            formatted_str = [formatted_str, sprintf('%.7ff', coeffs(i))];
            if i < length(coeffs)
                formatted_str = [formatted_str, ', '];
            end
        end
        formatted_str = [formatted_str, '}'];
    end
    [b_hpf, a_hpf] = butter(order, cutoff / (samplerate / 2), 'high');
    [b_lpf, a_lpf] = butter(order, cutoff / (samplerate / 2), 'low');

    % Format and print coefficients for C++ initializer list
    formatted_b_hpf = format_coefficients(b_hpf);
    formatted_a_hpf = format_coefficients(a_hpf);
    printf('HPF Coefficients {b,a}: { %s, %s }\n', formatted_b_hpf, formatted_a_hpf);

    formatted_b_lpf = format_coefficients(b_lpf);
    formatted_a_lpf = format_coefficients(a_lpf);
    printf('LPF Coefficients: {b,a}: { %s, %s }\n', formatted_b_lpf, formatted_a_lpf);
    */

    // ASSERT_EQ(hpf.first.size(), 5);
    // ASSERT_EQ(hpf.second.size(), 5);
    // ASSERT_EQ(lpf.first.size(), 5);
    // ASSERT_EQ(lpf.second.size(), 5);

    // FilterCoefficients hpf_truth =
    // {
    //     {0.8426766f, -3.3707065f, 5.0560598f, -3.3707065f, 0.8426766f},
    //     {1.0000000f, -3.6580603f, 5.0314335f, -3.0832283f, 0.7101039f}
    // };

    // FilterCoefficients lpf_truth =
    // {
    //     {0.0000156f, 0.0000622f, 0.0000933f, 0.0000622f, 0.0000156f},
    //     {1.0000000f, -3.6580603f, 5.0314335f, -3.0832283f, 0.7101039f}
    // };

    // for (size_t i = 0; i < 5; ++i)
    // {
    //     ASSERT_NEAR(hpf.first[i], hpf_truth.first[i], 1e-6);
    //     ASSERT_NEAR(hpf.second[i], hpf_truth.second[i], 1e-6);
    //     ASSERT_NEAR(lpf.first[i], lpf_truth.first[i], 1e-6);
    //     ASSERT_NEAR(lpf.second[i], lpf_truth.second[i], 1e-6);
    // }
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
    EXPECT_LT(  db(rms(above)),  -60.0f);       // should be severely attenuuated

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