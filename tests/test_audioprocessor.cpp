#include "../src/AudioProcessor.h"
#include <gtest/gtest.h>
#include <vector>
#include <boost/circular_buffer.hpp>
#include <cmath>
#include <algorithm>
#include <random>

std::vector<float> white_noise(size_t samples)
{
    std::vector<float> white_noise(samples);
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(-1.0, 1.0);

    for (auto& sample : white_noise) {
        sample = distribution(generator);
    }

    return white_noise;
}

float average(const std::vector<float>& data)
{
    return std::accumulate(data.begin(), data.end(), 0.0f) / float(data.size());
}

std::vector<float> sine_wave(float frequency, float sample_rate, size_t samples)
{
    std::vector<float> sine_wave(samples);
    for (size_t i = 0; i < samples; i++)
        sine_wave[i] = sinf(float(2 * i) * M_PIf * frequency  / sample_rate);
    return sine_wave;
}


TEST(AudioProcessorTest, GetSlice)
{
    boost::circular_buffer<float> cb(4);
    for (float v : {1.0f, 2.0f, 3.0f, 4.0f})
        cb.push_back(v);

    ASSERT_EQ(get_slice(cb), (std::vector<float>{1.0f, 2.0f, 3.0f, 4.0f}));
    cb.push_back(5.0f);
    ASSERT_EQ(get_slice(cb), (std::vector<float>{2.0f, 3.0f, 4.0f, 5.0f}));
}

TEST(AudioProcessorTest, VolumeNoise)
{
    size_t samples = 2^24;

    std::vector<float> zeros = std::vector<float>(samples, 0.0f);
    {
        AudioProcessor ap(1920, 480);
        ap.process_data(zeros);
        std::vector<float> rms = ap.Vrms();
        std::vector<float> peak = ap.Vpeak();
        ASSERT_EQ(rms.size(), 1);
        ASSERT_LT(rms[0], -90.0f);

        ASSERT_EQ(peak.size(), 1);
        ASSERT_LT(peak[0], -90.0f);

    }

    std::vector<float> ones = std::vector<float>(samples, 1.0f);
    {
        AudioProcessor ap(1920, 480);
        ap.process_data(ones);
        std::vector<float> rms = ap.Vrms();
        std::vector<float> peak = ap.Vpeak();
        ASSERT_EQ(rms.size(), 1);
        ASSERT_GT(rms[0],  -0.1f);
        ASSERT_LT(rms[0],   0.1f);
        ASSERT_GT(peak[0], -0.1f);
        ASSERT_LT(peak[0],  0.1f);
    }

    std::vector<float> noise = white_noise(samples);
    ASSERT_EQ(noise.size(), samples);
    ASSERT_GT(*std::max_element(noise.begin(), noise.end()), -1.0f);
    ASSERT_LE(*std::max_element(noise.begin(), noise.end()),  1.0f);
    float avg = average(noise);
    ASSERT_GE(avg, -0.3f);
    ASSERT_LE(avg,  0.3f);
}

TEST(AudioProcessorTest, VolumeSine)
{

    size_t sample_rate = 48000;
    size_t samples = 65536;

    AudioProcessor ap(1920, 480);

    std::vector<float> sine_440 = sine_wave(440, float(sample_rate), samples); 
    ASSERT_GE(*std::min_element(sine_440.begin(), sine_440.end()), -1.0);
    ASSERT_LE(*std::max_element(sine_440.begin(), sine_440.end()),  1.0);
    float avg = average(sine_440);
    ASSERT_GE(avg, -0.1);
    ASSERT_LE(avg,  0.1);
    ap.process_data(sine_440);

    std::vector<float> rms = ap.Vrms();
    std::vector<float> peak = ap.Vpeak();
    EXPECT_EQ(rms.size(), 1);
    EXPECT_EQ(peak.size(), 1);
    ASSERT_GT(ap.Vrms()[0], -3.2);
    ASSERT_LT(ap.Vrms()[0], -2.9);
    ASSERT_GT(ap.Vpeak()[0], -0.1);
    ASSERT_LT(ap.Vpeak()[0],  0.1);

}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}