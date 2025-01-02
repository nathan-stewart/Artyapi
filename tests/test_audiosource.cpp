#include "../src/AudioSource.h"
#include "test_util.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sndfile.h>
#include <filesystem>
#include <thread>

using namespace std::chrono_literals;

TEST(AudioSource, FilePlayback)
{
    Filepath tempdir = std::filesystem::temp_directory_path();
    float sample_rate = 48e3f;
    size_t samples = 48000;
    Signal   sine_1khz = sine_wave(1000.0f, sample_rate, samples);
    Filepath sine_file = tempdir / "sine_1khz.wav";
    write_wav_file(sine_file, sine_1khz, static_cast<int>(sample_rate));

    Signal signal;
    Signal recovered;

    AudioFileHandler af(sine_file);
    auto start = std::chrono::steady_clock::now();
    for (auto t :  {100ms, 200ms, 300ms, 400ms})
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(t));
        auto now = std::chrono::steady_clock::now();
        float elapsed = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(now - start).count())/1e6f;
        start = now;
        signal = af.read();
        recovered.insert(recovered.end(), signal.begin(), signal.end());

        size_t tolerance = 10;
        size_t expected = static_cast<size_t>(elapsed * static_cast<float>(sample_rate) + 0.5f);
        EXPECT_LE(signal.size(), expected);
        if (recovered.size() < samples)
        {
            // Last read could be short
            EXPECT_GE(signal.size(), expected - tolerance);
        }
        EXPECT_LE(signal.size(), expected + tolerance);
    }

    EXPECT_EQ(recovered.size(), samples);
    EXPECT_LE(average(recovered), 1e-3);
    EXPECT_NEAR(rms(recovered), 0.707f, 1e-3);
    EXPECT_NEAR(peak(recovered), 1.0f, 1e-6);

    float generated_frequency = (static_cast<float>(zero_crossings(recovered)) / 2.0f) * (sample_rate / static_cast<float>(recovered.size()));
    EXPECT_NEAR(generated_frequency, 1000.0f , 1.0f);

    std::filesystem::remove(sine_file);
}