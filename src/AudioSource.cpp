#include "AudioSource.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <sndfile.h>

using namespace std;

std::unique_ptr<AudioSource> AudioSourceFactory::createAudioSource(const std::string& source)
{
    std::unique_ptr<AudioSource> audio_source;
    if (source == "device")
    {
        audio_source = std::make_unique<AudioCapture>();
    }
    else if (source == "file")
    {
        audio_source = std::make_unique<AudioFile>("/home/username/Downloads/440.wav");
    }
    else if (source == "directory")
    {
        audio_source = std::make_unique<AudioFile>(source);
    }
    else
    {
        throw std::runtime_error("Unknown audio source: " + source);
    }
    return audio_source;
}

AudioSource::AudioSource()
: sr(48000)
{
}


AudioSource:: ~AudioSource()
{
}


AudioCapture::AudioCapture()
{
}


AudioCapture::~AudioCapture()
{
}


std::pair<bool, Signal> AudioCapture::read()
{
    Signal signal(4800);
    return std::make_pair(false, signal);
}


// Helper function to convert a string to lowercase
std::string to_lowercase(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
    return lower_str;
}


AudioFileHandler::AudioFileHandler(Filepath path)
: current(nullptr)
{
    wav_files(get_wav_in_dir());
    if (wav_files.empty())
    {
        throw std::runtime_error("No wav files found in directory: " + path.string());
    }
}


AudioFileHandler::~AudioFileHandler()
{
}


std::vector<Filepath> AudioFileHandler::get_wav_in_dir() const
{
    std::vector<Filepath> wav_files;
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_regular_file() && to_lowercase(entry.path().extension().string()) == ".wav")
        {
            wav_files.push_back(entry.path());
        }
    }
    return wav_files;
}


Signal AudioFileHandler::read()
{
    Signal signal(4800);
    
    return signal;
}


AudioFile::AudioFile(Filepath path)
: filepath(path)
, infile(nullptr)
{
    // Using Headerless PCM 24bit 48khz format
    SF_INFO info;
    info.samplerate = sr;
    info.channels = 1; // mono
    info.format = SF_FORMAT_RAW | SF_FORMAT_PCM_24;
    infile = sf_open(filepath.string().c_str(), SFM_READ, &info);
    if (!infile)
    {
        throw std::runtime_error("Error opening file: " + filepath.string());
    }
    total_frames = info.frames;
    current_position = 0;
    last_read = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

AudioFile::~AudioFile() {
    if (infile) {
        sf_close(infile);
    }
}

std::pair<bool, Signal> AudioFile::read()
{
    bool done = false;
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    auto elapsed = now - last_read;
    sf_count_t frames_to_read = static_cast<sf_count_t>(max(sr * elapsed / 1000000, 500UL));
    sf_count_t frames_remaining = total_frames - current_position;
    frames_to_read = min(frames_remaining, frames_to_read);
    Signal signal(frames_to_read);
    last_read = now;

    sf_seek(infile, current_position, SEEK_SET);
    auto frames_read = sf_readf_float(infile, signal, frames_to_read);
    current_position += frames_read;
    if (frames_read < frames_to_read)
    {
        signal.resize(frames_read);
    }

    if (current_position >= total_frames)
    {
        done = true;
    }
    return std::make_pair(done, signal);
}
