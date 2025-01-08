#pragma once

#include <vector>
#include <fftw3.h>
#include "Filters.h"
#include <utility>
#include "SpectrumProcessor.h"
#include "Signal.h"
#include "Plotter.h"

std::pair<float,float>  process_volume(const Signal& data);

class AudioProcessor
{
public:
    friend class AudioProcessorTest;
    AudioProcessor(size_t fft_bins=1920, size_t fft_history=480, size_t window_size=16834);
    ~AudioProcessor();

    void process(const Signal& data);

private:
    SpectrumProcessor            spectrum;
    Plotter                      plotter;

    float                        vrms;
    float                        vpk;
    std::vector<Spectrum>        history;
};
