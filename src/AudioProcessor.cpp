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
: spectrum(window_size, fft_bins)
, plotter(fft_bins, fft_history, Plotter::PlotMode::Spectrum, false)
, vrms(-96.0f)
, vpk(-96.0f)
, history(fft_history)
{
}

AudioProcessor::~AudioProcessor()
{
}


SpectralHistory transpose(const boost::circular_buffer<Spectrum>& history) 
{
    if (history.empty()) return {};

    size_t bins = history[0].size();
    size_t slices = history.size();
    SpectralHistory transposed(bins, std::vector<float>(slices));
    for (size_t i = 0; i < bins; ++i)
    {
        for (size_t j = 0; j < slices; ++j)
        {
            transposed[i][j] = history[j][i];
        }
    }
    return transposed;
}

Spectrum AudioProcessor::calculate_decay_rates()
{
    SpectralHistory transposed = transpose(history);
    Spectrum decay_rates;
    for (auto& bin : transposed)
    {
    }
    return decay_rates;
}

void AudioProcessor::process(const Signal& data)
{
    if (data.size() == 0)
        return;

    auto [vrms_val, vpk_val] = process_volume(data);
    vrms = vrms_val;
    vpk = vpk_val;
    history.push_front(spectrum(data));
    calculate_decay_rates();
}

