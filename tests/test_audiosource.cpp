#include "../src/AudioSource.h"
#include "test_util.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sndfile.h>
#include <filesystem>

TEST(AudioSource, AudioFile)
{
    float sample_rate = 48e3f;
    size_t samples = 48000;
    Signal   sine_1khz = sine_wave(1000.0f, sample_rate, samples);
    Filepath sine_file = "sine_1khz.wav";
    write_wav_file(sine_file, sine_1khz, static_cast<int>(sample_rate), 1);

    AudioFile af(sine_file);
    bool done;
    Signal signal;
    size_t count = 0;
    do
    {
        std::tie(done, signal) = af.read();
        count += signal.size();
        std::cout << "Samples read: " << signal.size() << " of " << samples << std::endl;
    } while (!done);
    EXPECT_EQ(count, samples);
}