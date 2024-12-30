#include "AudioSource.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <sndfile.h>

using namespace std;

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


AudioFile::AudioFile(Filepath path)
: filepath(path)
, infile(nullptr)
, channels(1)
, total_frames(0)
, current_position(0)
{
    std::cout << "Opening file: " << filepath << std::endl;

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
    auto now = std::chrono::steady_clock::now();
    last_read = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    std::cout << "Opened file: " << filepath << " with " << channels << " channels, " << sr << " Hz sample rate, and " << total_frames << " frames." << std::endl  << std::flush;
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

    sf_count_t frames_to_read = static_cast<sf_count_t>(sr * elapsed / 1000000);
    std::cout << "Elapsed time: " << static_cast<float>(elapsed)/1e6f << " seconds, reading " << frames_to_read << " frames." << std::endl << std::flush;
    Signal signal(frames_to_read); // only returning one channel

    sf_seek(infile, current_position, SEEK_SET);
    sf_count_t frames_read = sf_readf_float(infile, signal, frames_to_read);
    current_position += frames_read;

    // Resize the signal if we read fewer frames than requested (e.g., end of file)
    if (frames_read < frames_to_read)
    {
        done = true;
        signal.resize(frames_read);
    }

    return std::make_pair(done, signal);
}