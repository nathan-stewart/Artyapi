#include "AudioSource.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <sndfile.h>

using namespace std;

// Define the callback function
int paCallback(const void *inputBuffer, void *,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo*,
                      PaStreamCallbackFlags statusFlags,
                      void *userData)
{
    // Cast userData to your custom data type
    AudioCapture *audioCapture = static_cast<AudioCapture*>(userData);

    // Cast inputBuffer to the correct type
    const float *in = static_cast<const float*>(inputBuffer);
    if ((statusFlags & paInputOverflow) == 0)
    {
        // Lock the mutex to protect the buffer
        std::lock_guard<std::mutex> lock(audioCapture->bufferMutex);

        // Copy the input data to the buffer
        audioCapture->buffer.insert(audioCapture->buffer.end(), in, in + framesPerBuffer);
    }
    // else skip the buffer on overflow so we can catch up
    return paContinue;
}

PaDeviceIndex AudioCapture::find_device(string device_name)
{
    PaDeviceIndex numDevices = Pa_GetDeviceCount();
    if (numDevices < 0)
    {
        throw std::runtime_error("Failed to get number of devices");
    }

    PaDeviceIndex deviceIndex = paNoDevice;

    // Numeric device index parameter
    if (std::all_of(device_name.begin(), device_name.end(), ::isdigit))
    {
        deviceIndex = stoi(device_name);
        if (deviceIndex >= 0 && deviceIndex < numDevices)
        {
            return deviceIndex;
        }
        throw std::runtime_error("Invalid device index: " + device_name);
    }
    else  // List devices
    {
        for (PaDeviceIndex i = 0; i < numDevices; i++)
        {
            const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
            cout << "Device " << i << ": " << deviceInfo->name << endl;
        }
    }

    if (deviceIndex == paNoDevice)
    {
        Pa_Terminate();
        throw std::runtime_error("Failed to find specified PortAudio device");
    }
    return deviceIndex;
}

AudioCapture::AudioCapture(std::string device_name)
: AudioSource(device_name)
, sample_rate(48e3f)
, buffer(1 << 16)
{
    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        throw std::runtime_error("Failed to initialize PortAudio");
    }

    // Get the device from the path given
    PaDeviceIndex deviceIndex = find_device(device_name);
    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
    cout << "Selected device index " << deviceIndex << ": " << deviceInfo->name << endl;

    // getting channel_count > maxchans - not sure why

    PaStreamParameters input_params = {deviceIndex,
                                       1,
                                       paFloat32,
                                       Pa_GetDeviceInfo(deviceIndex)->defaultLowInputLatency,
                                       nullptr};
    err = Pa_OpenStream(&stream,
                        &input_params,
                        nullptr, // no output parameters
                        sample_rate,
                        256, // frames per buffer
                        paClipOff, // no clipping
                        paCallback, // callback function
                        this); // user data

    if (err != paNoError)
    {
        Pa_Terminate();
        throw std::runtime_error("Failed to open PortAudio stream");
    }

    err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        Pa_CloseStream(stream);
        Pa_Terminate();
        throw std::runtime_error("Failed to start PortAudio stream");
    }
}


AudioCapture::~AudioCapture()
{
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}

Signal AudioCapture::read()
{
    Signal signal(buffer.size());
    cout << "Reading " << buffer.size() << " samples from audio capture\n";
    std::lock_guard<std::mutex> lock(bufferMutex);
    std::copy(buffer.begin(), buffer.end(), signal.begin());
    buffer.clear();
    return signal;
}


// Helper function to convert a string to lowercase
std::string to_lowercase(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
    return lower_str;
}


AudioFileHandler::AudioFileHandler(Filepath path)
: AudioSource(path)
, current(nullptr)
{
    if (std::filesystem::is_directory(path))
    {
        folder = path;
        wav_files = get_wav_in_dir();
        if (wav_files.empty())
        {
            throw std::runtime_error("No wav files found in directory: " + path.string());
        }
    } else if (std::filesystem::is_regular_file(path) && path.extension() == ".wav")
    {
        folder = "";
        current = std::make_unique<WavFile>(path);
    } else
    {
        throw std::runtime_error("Invalid file or directory: " + path.string());
    }
    last_read = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}


AudioFileHandler::~AudioFileHandler()
{
}


std::vector<Filepath> AudioFileHandler::get_wav_in_dir() const
{
    std::vector<Filepath> wav_files;
    for (const auto& entry : std::filesystem::directory_iterator(folder))
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
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    auto elapsed = now - last_read;
    sf_count_t frames_to_read = static_cast<sf_count_t>(max(static_cast<sf_count_t>(sample_rate) * elapsed / 1000000, 500UL));
    Signal signal(frames_to_read);

    last_read = now;
    if (!current)
    {
        current = std::make_unique<WavFile>(wav_files.front());
    }

    signal = current->read(frames_to_read);

    if (signal.size() < static_cast<size_t>(frames_to_read))
    {
        if (!folder.empty())
        {
            wav_files = get_wav_in_dir();
            current = nullptr;
        } else if (current)
        {
            current->rewind();
        }
    }
    return signal;
}


WavFile::WavFile(std::string path)
: filepath(path)
, infile(nullptr)
, sample_rate (48e3f)
{
    // Using Headerless PCM 24bit 48khz format
    SF_INFO info;
    info.samplerate = static_cast<int>(sample_rate);
    info.channels = 1; // mono
    info.format = SF_FORMAT_RAW | SF_FORMAT_PCM_24;
    infile = sf_open(filepath.c_str(), SFM_READ, &info);
    if (!infile)
    {
        throw std::runtime_error("Error opening file: " + filepath.string());
    }
    total_frames = info.frames;
    current_position = 0;
}

WavFile::~WavFile()
{
    if (infile) {
        sf_close(infile);
    }
}

Signal WavFile::read(size_t frames_to_read)
{
    sf_count_t frames_remaining = total_frames - current_position;
    frames_to_read = min(frames_remaining, static_cast<sf_count_t>(frames_to_read));
    cout << "Reading " << frames_to_read << " frames from " << filepath << endl;
    Signal signal(frames_to_read);

    sf_seek(infile, current_position, SEEK_SET);
    auto frames_read = static_cast<size_t>(sf_readf_float(infile, signal, frames_to_read));
    current_position += frames_read;
    if (frames_read < frames_to_read)
    {
        std::cout << "Resizing\n";
        signal.resize(frames_read);
    }

    return signal;
}


std::unique_ptr<AudioSource> AudioSourceFactory::createAudioSource(const std::string& source)
{
    std::unique_ptr<AudioSource> audio_source;
    if (std::filesystem::is_directory(source) ||
       (std::filesystem::is_regular_file(source) && std::filesystem::path(source).extension() == ".wav"))
    {
        audio_source = std::make_unique<AudioFileHandler>(source);
    }
    else
    {
        audio_source = std::make_unique<AudioCapture>(source);
    }
    if (!audio_source)
    {
        throw std::runtime_error("Failed to create audio source");
    }
    return audio_source;
}
