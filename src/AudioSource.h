#pragma once
#include <sndfile.h>
#include <filesystem>
#include <chrono>
#include <utility>
#include <portaudio.h>
#include <mutex>
#include <boost/circular_buffer.hpp>
#include "Signal.h"

using Filepath = std::filesystem::path;

class AudioSource
{
public:
    AudioSource(std::string) : sample_rate(48000) {}
    ~AudioSource() = default;
    virtual Signal read() = 0;

protected:
    float  sample_rate;
    size_t last_read;
};


class AudioCapture : public AudioSource
{
public:
    AudioCapture(std::string device_name);
    ~AudioCapture();
    Signal read() override;

private:
    PaStream*     stream;
    std::mutex    bufferMutex;
    float         sample_rate;
    boost::circular_buffer<float> buffer;
    PaDeviceIndex find_device(std::string device_name);
    friend int paCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData);
};


class WavFile {
public:
    WavFile(std::string path);
    ~WavFile();

    Signal read(size_t frames_to_read);

private:
    Filepath    filepath;
    SNDFILE    *infile;
    sf_count_t  sample_rate;
    sf_count_t  total_frames;
    sf_count_t  current_position;
};


class AudioFileHandler : public AudioSource
{
public:
    AudioFileHandler(Filepath path);
    ~AudioFileHandler();
    std::vector<Filepath> get_wav_in_dir() const;
    Signal read() override;
private:
    Filepath              folder;
    std::vector<Filepath> wav_files;
    std::unique_ptr<WavFile> current;
};


class AudioSourceFactory
{
public:
    static std::unique_ptr<AudioSource> createAudioSource(const std::string& source);
};
