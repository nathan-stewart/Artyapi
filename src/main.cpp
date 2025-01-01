#include "AudioProcessor.h"
#include "AudioSource.h"
#include <boost/program_options.hpp>
#include <iostream>

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
    std::unique_ptr<AudioSource> source = nullptr
    namespace po = boost::program_options;
    po::options_description desc("rta - a Real Time Analyzer / SPL meter\nAllowed options");
    desc.add_options()
        ("help", "produce help message")
        ("source", po::value<std::string>(), "data source - a device, file, or directory of wav files");

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
        std::unique_ptr<AudioSource> source = AudioSourceFactory::createAudioSource(vm["source"].as<std::string>());
    }

    AudioProcessor ap;
    while (true)
    {
        Signal data = source->read();
        ap.process(data);
        ap.update_plot();
    }        
    return 0;
}