#pragma once
#include <vector>
#include <fftw3.h>
#include <boost/circular_buffer.hpp>
#include <utility>
#include <thread>
#include <mutex>
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

std::pair<float,float>  process_volume(const Signal& data);

struct Bin
{
    Bin(float i = 0.0f) : intensity(i), decay(0.0f) {}
    float intensity;
    float decay;
};

struct MultiSpectra
{
    MultiSpectra(const Spectrum& s);
    std::vector<Bin> spectrum;
};
using SpectralHistory = boost::circular_buffer<MultiSpectra>;

class AudioProcessor
{
public:
    friend class AudioProcessorTest;
    AudioProcessor(size_t fft_bins=1920, size_t fft_history=480, size_t window_size=16834);
    ~AudioProcessor();

    void process(const Signal& data);

private:
    mutable std::mutex historyMutex;
    SpectrumProcessor  spectrum_processor;
    Plotter            plotter;
    float              vrms = -96.0f;
    float              vpk  = -96.0f;
    SpectralHistory    plot_history;
    float              alpha = 0.9f;
    Spectrum           decay_rate;
    float              frame_duration_ms = 1.33f; // Frame duration in milliseconds
};
