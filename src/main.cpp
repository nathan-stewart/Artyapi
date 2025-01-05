#include <boost/program_options.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include "AudioSource.h"
#include "AudioProcessor.h"
#include "Plotter.h"

const float target_period =  1.0f/30.0f;
Signal sine_wave(float frequency, float sample_rate, size_t samples)
{
    Signal sine_wave(samples);
    float amplitude = 1.0f;
    float phase = 0.0f;
    float increment = 2.0f * M_PIf * frequency / sample_rate;
    for (size_t i = 0; i < samples; ++i) {
        sine_wave[i] = amplitude * std::sin(phase);
        phase += increment;
    }
    return sine_wave;
}


int main(int argc, char** argv)
{
    std::unique_ptr<AudioSource> source = nullptr;
    namespace po = boost::program_options;
    po::options_description desc("rta - a Real Time Analyzer / SPL meter\nAllowed options");
    desc.add_options()
        ("help", "produce help message")
        ("source", po::value<std::string>()->default_value("0"), "data source - a device, file, or directory of wav files");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }

    if (vm.count("source"))
    {
        source = AudioSourceFactory::createAudioSource(vm["source"].as<std::string>());
    }
    if (!source)
    {
        throw std::runtime_error("No source found");
    }

    AudioProcessor ap(1920, 480, 16384);
    auto previous = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    while (true)
    {
        // Don't need to run faster than 30fps or so
        auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        float  elapsed = static_cast<float>(now - previous)/1e6f;
        if (elapsed < target_period)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>((target_period - elapsed) * 1e6f)));
        }
        previous = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        Signal data = source->read();
        ap.process(data);
    }
    return 0;
}