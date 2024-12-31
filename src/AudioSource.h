#pragma once
#include <sndfile.h>
#include <filesystem>
#include <chrono>
#include <utility>
#include "Signal.h"

class AudioSource
{
public:
    AudioSource();
    ~AudioSource();
    float sample_rate() const { return static_cast<float>(sr); }

protected:
    int sr;

    virtual std::pair<bool, Signal> read() = 0;
};

using Filepath = std::filesystem::path;
using Timestamp = std::chrono::steady_clock::time_point;
class AudioCapture : public AudioSource
{
public:
    AudioCapture();
    ~AudioCapture();
    virtual std::pair<bool, Signal> read() override;
};

class AudioFile : public AudioSource {
public:
    AudioFile(std::filesystem::path path);
    ~AudioFile();

    virtual std::pair<bool, Signal> read() override;
    void                            get_wav_in_dir() const;

private:
    bool open_next_file();

    Filepath    filepath;
    SNDFILE     *infile;
    sf_count_t  total_frames;
    sf_count_t  current_position;
    size_t      last_read;
};