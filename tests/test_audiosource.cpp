#include "../src/AudioSource.h"
#include <gtest/gtest.h>
#include <iostream>

TEST(AudioSource, AudioCapture)
{
    AudioCapture audioCapture;
    Signal signal = audioCapture.read();
    EXPECT_EQ(signal.size(), 0);
}

TEST(AudioSource, AudioFile)
{
    AudioFile audioFile;
    Signal signal = audioFile.read();
    EXPECT_EQ(signal.size(), 0);
}