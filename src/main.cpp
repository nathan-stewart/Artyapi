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

void check_fps(size_t &frame_count)
{
    static auto previous = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    float  elapsed = static_cast<float>(now - previous)/1e6f;
    frame_count++;
    if (elapsed > 5.0f)
    {
        previous = now;
        float fps = static_cast<float>(frame_count) / elapsed;
        std::cout << "FPS: " << std::fixed << std::setprecision(2) << fps << std::endl;
        frame_count = 0;
    }
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
    
    size_t frame_count = 0;
    AudioProcessor ap(1920, 480, 16384);
    while (true)
    {
        Signal data = source->read();
        ap.process(data);
        ap.update_plot();
        check_fps(++frame_count);
    }
    return 0;
}