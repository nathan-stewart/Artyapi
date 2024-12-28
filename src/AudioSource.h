#pragma once
#include <filesystem>
#include "Signal.h"

class AudioSource 
{
public:
    AudioSource();
    ~AudioSource();

    virtual Signal read() = 0;
};

class AudioCapture : public AudioSource
{
public:
    AudioCapture();
    ~AudioCapture();
    virtual Signal read();
};

class AudioFile : public AudioSource
{
public:
    AudioFile(std::filesystem::path path);
    ~AudioFile();
    virtual Signal read();
};
