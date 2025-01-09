#pragma once
#include <vector>
#include <fftw3.h>
#include <boost/circular_buffer.hpp>
#include <utility>
#include <thread>
#include "Filters.h"
#include "SpectrumProcessor.h"
#include "Signal.h"
#include "Plotter.h"

// AudioProcessor is a class that processes audio data and calculates decay rates
// from the audio data. It uses a SpectrumProcessor to calculate the spectrum of
// the audio data and a Plotter to display the results.
// The AudioProcessor class:
//      Takes a Signal object as input and processes the audio data. 
//      Calculates Peak and RMS volume levels from the audio data.
//      Calculates the  FFT 
//      Calculates the decay rates from the FFT data.
//      Displays the results using a Plotter object.
//
// The FFT can run on a separate thread to improve performance, testing indicates it can 
// run around 36kfps on a 1.4Ghz AMD Ryzen 5 5500U

std::pair<float,float>  process_volume(const Signal& data);
using SpectralHistory = std::vector<std::vector<float>>;
SpectralHistory transpose(const std::vector<std::vector<float>>& history);

class AudioProcessor
{
public:
    friend class AudioProcessorTest;
    AudioProcessor(size_t fft_bins=1920, size_t fft_history=480, size_t window_size=16834);
    ~AudioProcessor();

    void process(const Signal& data);
    Spectrum calculate_decay_rates();

private:
    SpectrumProcessor                spectrum;
    Plotter                          plotter;

    float                            vrms;
    float                            vpk;
    boost::circular_buffer<Spectrum> history;
};
