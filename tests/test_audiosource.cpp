#include "../src/AudioSource.h"
#include "test_util.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sndfile.h>
#include <filesystem>

TEST(AudioSource, AudioFile)
{
    float sample_rate = 48e3f;
    Signal sine_1khz = sine_wave(1000.0f, sample_rate, 48000);
    std::filesystem::path sine_file = "sine_1khz.wav";
    write_wav_file(sine_file, sine_1khz, static_cast<int>(sample_rate), 1);

    AudioFile af(sine_file);
    Signal signal = af.read();
    EXPECT_EQ(signal.size(), 0);
}