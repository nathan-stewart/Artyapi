#pragma once

#include <vector>
#include <fftw3.h>
#include "Filters.h"
#include <utility>
#include "SpectrumProcessor.h"
#include "Signal.h"

std::pair<float,float>  process_volume(const Signal& data, boost::circular_buffer<float>& vrms, boost::circular_buffer<float>& vpk);

class AudioProcessor
{
public:
    friend class AudioProcessorTest;
    AudioProcessor(size_t fft_bins=1920, size_t fft_history=480, size_t window_size=16834);
    ~AudioProcessor();

    void process(const Signal& data);

private:
    float                        vrms;
    float                        vpk;
    SpectrumProcessor             spectrum;
    std::vector<Spectrum>         history;
};
