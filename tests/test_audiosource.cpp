#include "../src/AudioSource.h"
#include "test_util.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sndfile.h>
#include <filesystem>
#include <thread>

using namespace std::chrono_literals;

TEST(AudioSource, FileSource)
{
    Filepath tempdir = std::filesystem::temp_directory_path();
    float sample_rate = 48e3f;
    size_t samples = 48000;
    Signal   sine_1khz = sine_wave(1000.0f, sample_rate, samples);
    Filepath sine_file = tempdir / "sine_1khz.wav";
    write_wav_file(sine_file, sine_1khz, static_cast<int>(sample_rate));
    ASSERT_TRUE(std::filesystem::exists(sine_file));
    ASSERT_NE((std::filesystem::status(sine_file).permissions() & std::filesystem::perms::owner_read), std::filesystem::perms::none);


    AudioFile af(sine_file);

    bool done;
    Signal signal;
    Signal recovered;
    size_t count = 0;
    do
    {
        std::tie(done, signal) = af.read();
        count += signal.size();
        recovered.insert(recovered.end(), signal.begin(), signal.end());
    } while (!done);
    EXPECT_EQ(count, samples);
    EXPECT_EQ(recovered.size(), samples);
    EXPECT_LE(average(recovered), 1e-3);
    EXPECT_NEAR(rms(recovered), 0.707f, 1e-3);
    EXPECT_NEAR(peak(recovered), 1.0f, 1e-6);
 
    float generated_frequency = (static_cast<float>(zero_crossings(recovered)) / 2.0f) * (af.sample_rate() / static_cast<float>(recovered.size()));
    EXPECT_NEAR(generated_frequency, 1000.0f , 1.0f);

    std::filesystem::remove(sine_file);
}

TEST(AudioSource, FilePlayback)
{
    Filepath tempdir = std::filesystem::temp_directory_path();
    float sample_rate = 48e3f;
    size_t samples = 48000;
    Signal   sine_1khz = sine_wave(1000.0f, sample_rate, samples);
    Filepath sine_file = tempdir / "sine_1khz.wav";
    write_wav_file(sine_file, sine_1khz, static_cast<int>(sample_rate));

     bool done;
    Signal signal;

    AudioFile af(sine_file);
    auto start = std::chrono::steady_clock::now();
    for (auto t :  {200ms, 200ms, 400ms})
    {
        // sleep for 100ms
        std::this_thread::sleep_for(std::chrono::milliseconds(t));
        auto now = std::chrono::steady_clock::now();
        float elapsed = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(now - start).count())/1e3f;
        start = now;
        std::tie(done, signal) = af.read();
        
        float expected = elapsed * static_cast<float>(sample_rate) / 1e6f;
        if (!done) 
        {
            // done may be short of the expected size
            std::cout << "Sleep for " << elapsed << "ms, expected " << expected << " samples, got " << signal.size() << " samples\n";
            EXPECT_NEAR(static_cast<float>(signal.size()), expected, 10.0f);
        }
        
    } while (!done);
   
    std::filesystem::remove(sine_file);
}