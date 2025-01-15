#include "AudioProcessor.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>
#include <iostream>
#include <future>


const float LOGMIN = 1e-10f;
using namespace std;

pair<float,float> process_volume(const Signal& data)
{
    float rms = 0.0f;
    float pk = 0.0f;

    for (auto& sample : data)
    {
        rms += sample * sample;
        pk = max(pk, abs(sample));
    }

    rms = sqrtf(rms / static_cast<float>(data.size()));
    return make_pair(20 * log10(rms + LOGMIN), 20 * log10(pk + LOGMIN));
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
    size_t pool = std::thread::hardware_concurrency(); // Use the number of available hardware threads
    size_t chunk_size = bins / pool;
    SpectralHistory transposed(bins, std::vector<float>(slices));

    auto transpose_chunk = [&](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i)
        {
            for (size_t j = 0; j < slices; ++j)
            {
                transposed[i][j] = history[j][i];
            }
        }
    };

    std::vector<std::future<void>> futures;
    for (size_t p = 0; p < pool; ++p)
    {
        size_t start = p * chunk_size;
        size_t end = (p == pool - 1) ? bins : start + chunk_size;
        futures.push_back(std::async(std::launch::async, transpose_chunk, start, end));
    }

    for (auto& future : futures)
    {
        future.get();
    }

    return transposed;
}


Spectrum AudioProcessor::calculate_decay_rates()
{
    SpectralHistory transposed = transpose(history);
    Spectrum decay_rates(transposed.size());
    static Spectrum ema_decay_rates(transposed.size());
    const float alpha = 0.1f; // Smoothing factor for EMA

    size_t pool = std::thread::hardware_concurrency(); // Use the number of available hardware threads
    size_t chunk_size = transposed.size() / pool;

    auto calculate_chunk = [&](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i)
        {
            if (transposed[i].size() > 1)
            {
                float decay_rate = transposed[i][0] - transposed[i][1];
                ema_decay_rates[i] = alpha * decay_rate + (1 - alpha) * ema_decay_rates[i];
                decay_rates[i] = ema_decay_rates[i];
            }
        }
    };

    std::vector<std::future<void>> futures;
    for (size_t p = 0; p < pool; ++p)
    {
        size_t start = p * chunk_size;
        size_t end = (p == pool - 1) ? transposed.size() : start + chunk_size;
        futures.push_back(std::async(std::launch::async, calculate_chunk, start, end));
    }

    for (auto& future : futures)
    {
        future.get();
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
    vector<Spectrum> ffts = spectrum(data);
    {
        lock_guard<mutex> lock(historyMutex);
        history.insert(history.end(), ffts.begin(), ffts.end());
        calculate_decay_rates();
    }
}

