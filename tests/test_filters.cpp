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

TEST(FilterTest, Butterworth_Sine)
{
    float samplerate = 48000.0f;
    size_t samples = 1 << 16;
    float f0 = 1000.0f * powf(2.0f, -4.0f); // 62.5 Hz
    float f1 = 1000.0f;                     // 1 kHz
    float f2 = 1000.0f * powf(2.0f,  4.0f); // 16 kHz

    // Generated coefficients for a 4th order 1kHz LPF,HPF generated in octave via:
    // octave:15> [b, a] = butter(4, 1000/24000, 'high'); disp(b); disp(a)
    // b =  0.8427  -3.3707   5.0561  -3.3707   0.8427
    // a =  1.0000  -3.6581   5.0314  -3.0832   0.7101
    // octave:16> [b, a] = butter(4, 1000/24000, 'low'); disp(b); disp(a)
    // b = 1.5552e-05   6.2207e-05   9.3310e-05   6.2207e-05   1.5552e-05
    // a = 1.0000  -3.6581   5.0314  -3.0832   0.7101

    FilterCoefficients hpf = {{0.8427f, -3.3707f, 5.0561f, -3.3707f, 0.8427f},
                              {1.0000f, -3.6581f, 5.0314f, -3.0832f, 0.7101f}};

    FilterCoefficients lpf = {{1.5552e-05f, 6.2207e-05f, 9.3310e-05f, 6.2207e-05f, 1.5552e-05f},
                          {1.0000f, -3.6581f, 5.0314f, -3.0832f, 0.7101f}};

    // above and below are 4 octaves either side of cutoff  or
    std::vector<float> below = sine_wave(f0, samplerate, samples);
    std::vector<float> cutoff = sine_wave(f1, samplerate, samples);
    std::vector<float> above = sine_wave(f2, samplerate, samples);
    apply_filter(hpf, above);
    apply_filter(hpf, cutoff);
    apply_filter(hpf, below);

    // Debug output
    std::cout << "HPF - rms above: " << db(rms(above)) << "dB" << std::endl;
    std::cout << "HPF - rms cutoff: " << db(rms(cutoff)) << "dB" << std::endl;
    std::cout << "HPF - rms below: " << db(rms(below)) << "dB" << std::endl;

    EXPECT_GT(  db(rms(above)),  -0.5f);        // should not be  attenuated
    EXPECT_NEAR(db(rms(cutoff)), -3.0f, 0.1f);  // should be  attenuated -3db
    EXPECT_LT(  db(rms(below)),  -70.0f);       // should be severely attenuated

    // filter modifies the buffer in place- make new copies
    below = sine_wave(f0, samplerate, samples);
    cutoff = sine_wave(f1, samplerate, samples);
    above = sine_wave(f2, samplerate, samples);

    apply_filter(lpf, below);
    apply_filter(lpf, cutoff);
    apply_filter(lpf, above);
     // Debug output
    std::cout << "LPF - rms above: "  << db(rms(above)) << "dB" << std::endl;
    std::cout << "LPF - rms cutoff: " << db(rms(above)) << "dB" << std::endl;
    std::cout << "LPF - rms below: "  << db(rms(below)) << "dB" << std::endl;

    EXPECT_LT(  db(rms(above)),  -70.0f);       // should be severely attenuuated
    EXPECT_NEAR(db(rms(cutoff)), -3.0f, 0.1f);  // should be  attenuated -3db
    EXPECT_GT(  db(rms(below)),  -0.5f);        // should not be attentuated
}
