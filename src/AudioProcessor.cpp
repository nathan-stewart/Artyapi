#include "AudioProcessor.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>
#include <iostream>

const float LOGMIN = 1e-10f;

std::pair<float,float> process_volume(const Signal& data)
{
    float rms = 0.0f;
    float pk = 0.0f;

    for (auto& sample : data)
    {
        rms += sample * sample;
        pk = std::max(pk, std::abs(sample));
    }

    rms = sqrtf(rms / static_cast<float>(data.size()));
    return std::make_pair(20 * std::log10(rms + LOGMIN), 20 * std::log10(pk + LOGMIN));
}


AudioProcessor::AudioProcessor(size_t fft_bins, size_t fft_history, size_t window_size)
: spectrum(fft_bins, fft_history, window_size)
{
}


AudioProcessor::~AudioProcessor()
{
}


void AudioProcessor::process(const Signal& data)
{
    if (data.size() == 0)
        return;

    vrms, vpk = process_volume(data);
    process_spectrum(data);
}

