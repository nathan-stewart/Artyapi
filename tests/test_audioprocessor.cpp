#include "../src/AudioProcessor.h"
#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

#include "test_util.h"

TEST(AudioProcessorTest, VolumeNoise)
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

    std::vector<float> rms = ap.Vrms();
    std::vector<float> peak = ap.Vpeak();
    EXPECT_EQ(rms.size(), 1);
    EXPECT_EQ(peak.size(), 1);
    ASSERT_NEAR(ap.Vrms()[0], -3.0f, 0.1f);
    ASSERT_NEAR(ap.Vpeak()[0],  0.0f, 0.1f);
}
