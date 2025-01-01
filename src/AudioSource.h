#pragma once
#include <sndfile.h>
#include <filesystem>
#include <chrono>
#include <utility>
#include "Signal.h"

using Filepath = std::filesystem::path;

class AudioSource
{
public:
    AudioSource();
    ~AudioSource();
    float sample_rate() const { return static_cast<float>(sr); }
    virtual Signal read() = 0;

protected:
    int sr;

    Signal read() = 0;
};


class AudioCapture : public AudioSource
{
public:
    AudioCapture();
    ~AudioCapture();
    Signal read() override;
};


class AudioFileHandler : public AudioSource 
{
public:
    AudioFileHandler(Filepath path);
    ~AudioFileHandler();
    std::vector<Filepath> get_wav_in_dir() const;
    Signal read() override;
private:
    std::vector<Filepath> wav_files;
    std::unique_ptr<AudioFile> current;
};


class AudioFile {
public:
    AudioFile(Filepath path);
    ~AudioFile();

    std::pair<bool, Signal> read();

private:
    Filepath    filepath;
    SNDFILE     *infile;
    sf_count_t  total_frames;
    sf_count_t  current_position;
    size_t      last_read;
};

class AudioSourceFactory
{
public:
    static std::unique_ptr<AudioSource> createAudioSource(const std::string& source);
};
