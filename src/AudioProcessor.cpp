
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

MultiSpectra::MultiSpectra(const Spectrum& s)
: spectrum(s.size()) 
{
    for (const float& v : s)
    {
        spectrum.push_back(Bin(v));
    }
}


AudioProcessor::AudioProcessor(size_t fft_bins, size_t fft_history, size_t window_size)
: spectrum_processor(window_size, fft_bins)
, plotter(fft_bins, fft_history, Plotter::PlotMode::Spectrum, false)
, vrms(-96.0f)
, vpk(-96.0f)
, plot_history(fft_history)
, ema(Spectrum(fft_bins, 0.0f))
{
}

AudioProcessor::~AudioProcessor()
{
}


void AudioProcessor::process(const Signal& data)
{
    if (data.size() == 0)
        return;

    auto [vrms_val, vpk_val] = process_volume(data);
    vrms = vrms_val;
    vpk = vpk_val;
    vector<Spectrum> ffts = spectrum_processor(data);
    for (auto& fft : ffts)
    {
        MultiSpectra spectra(fft);
        for (size_t i = 0; i < fft.size(); ++i)
        {
            ema[i] = alpha * ema[i] + (1.0f - alpha) * fft[i];
            spectra.spectrum[i].decay = ema[i];
        }
    }
    lock_guard<mutex> lock(historyMutex);
    plot_history.insert(plot_history.end(), ffts.begin(), ffts.end());
}
