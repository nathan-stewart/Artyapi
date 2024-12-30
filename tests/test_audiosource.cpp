#include "../src/AudioSource.h"
#include "test_util.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sndfile.h>
#include <filesystem>

TEST(AudioSource, FileSource)
{
    Filepath tempdir = std::filesystem::temp_directory_path();

    std::cout << "Creating AudioFile object" << std::endl;
    float sample_rate = 48e3f;
    size_t samples = 48000;
    Signal   sine_1khz = sine_wave(1000.0f, sample_rate, samples);
    Filepath sine_file = tempdir / "sine_1khz.wav";
    write_wav_file(sine_file, sine_1khz, static_cast<int>(sample_rate));
    std::cout << "Wrote sine wave to " << sine_file << std::endl;

    // Check if file exists
    ASSERT_TRUE(std::filesystem::exists(sine_file));
    // Check file permissions
    ASSERT_NE((std::filesystem::status(sine_file).permissions() & std::filesystem::perms::owner_read), std::filesystem::perms::none);


    std::cout << "Creating AudioFile object" << std::endl;
    AudioFile af(sine_file);

    bool done;
    Signal signal;
    size_t count = 0;
    do
    {
        std::tie(done, signal) = af.read();
        std::cout << "Samples read: " << signal.size() << " of " << samples << std::endl;
        count += signal.size();
    } while (!done);
    EXPECT_EQ(count, samples);
}