#include "SpectrumProcessor.h"
#include <cmath>
#include <iostream>
#include <algorithm>

using namespace std;

float bin_to_freq_linear(size_t num_bins, float bin, float f0, float f1)
{
    float range_fraction = bin / static_cast<float>(num_bins);
    return f0 + range_fraction * (f1 - f0);
}


float bin_to_freq_log2(size_t num_bins, float bin, float f0, float f1)
{
    float range_fraction = bin / static_cast<float>(num_bins);
    return f0 * std::pow(2.0f, range_fraction * static_cast<float>(log2(f1 / f0)));
}


float freq_to_lin_fractional_bin(size_t num_bins, float freq, float f0, float f1)
{
    float bin_fraction = (freq - f0) / (f1 - f0);
    return bin_fraction * static_cast<float>(num_bins);
}


float freq_to_log_fractional_bin(size_t num_bins, float freq, float f0, float f1)
{
    float log2_bin_index = std::log2(freq / f0) / std::log2(f1 / f0);
    return log2_bin_index * static_cast<float>(num_bins);
}


void map_bins(const Spectrum& mapping, const Spectrum& source, Spectrum& destination)
{
    if (mapping.size() != source.size())
        throw std::runtime_error("Mapping size mismatch");

    // zero out the destination since we'll be adding multiple source bins to each destination bin
    std::fill(destination.begin(), destination.end(), 0.0f);
    size_t dest_size = destination.size();
    for (size_t t = 0; t < mapping.size(); ++t)
    {
        size_t dest_index = static_cast<size_t>(mapping[t]);
        float frac = fmodf(mapping[t], 1.0f);
        destination[dest_index] += (1.0f - frac) * source[t];
        if (dest_index + 1 < dest_size)
            destination[dest_index + 1] += frac * source[t];
    }
}

Spectrum precompute_bin_mapping(size_t lin_fft_bins, size_t log_fft_bins, float f0, float f1)
{
    Spectrum bin_mapping(lin_fft_bins);
    for (size_t i = 0; i < lin_fft_bins; ++i) {
        float freq = bin_to_freq_linear(lin_fft_bins, static_cast<float>(i), f0, f1);
        bin_mapping[i] = freq_to_log_fractional_bin(log_fft_bins, freq, f0, f1);
    }
    return bin_mapping;
}

SpectrumProcessor::SpectrumProcessor(size_t window_size, size_t log_bin_count)
: raw(window_size * 2) // allow sliding windows
, sample_rate(48000.0f)
, f0(40.0f)
, f1(20000.0f)
, lin_fft_bins(static_cast<size_t>(window_size / 2 + 1))
, log_fft_bins(log_bin_count)
{
    std::lock_guard<std::mutex> lock(buffer_mutex);
    bin_mapping = precompute_bin_mapping(lin_fft_bins, log_fft_bins, f0, f1);

    // 2nd order butterworth 40Hz HPF - 4th order is unstable
    hpf = {{0.9963044f, -1.9926089f, 0.9963044f}, {1.0000000f, -1.9925952f, 0.9926225f}};

    // 4th order butterowrth 20khz LPF
    lpf = { {0.4998150f,  1.9992600f, 2.9988900f,  1.9992600f, 0.4998150f}, {1.0000000f,  2.6386277f, 2.7693098f,  1.3392808f, 0.2498217f}};

    window = hanning_window(window_size);

    fftwf_init_threads();

    fftw_in = fftwf_alloc_real(window_size);
    fftw_out = fftwf_alloc_real(window_size);
    if (fftw_in == nullptr || fftw_out == nullptr)
        throw std::bad_alloc();

    plan = fftwf_plan_r2r_1d(
            static_cast<int>(window_size),
            fftw_in,
            fftw_out,
            FFTW_R2HC,
            FFTW_ESTIMATE);
    if (plan == nullptr)
        throw std::runtime_error("Failed to create FFTW plan");
}


SpectrumProcessor::~SpectrumProcessor()
{
    fftwf_cleanup_threads();
    fftwf_destroy_plan(plan);
    fftwf_free(fftw_in);
    fftwf_free(fftw_out);
    fftw_in = nullptr;
    fftw_out = nullptr;
    plan = nullptr;
}

void nan_check(const Signal& data, string message)
{
    if ( any_of(data.begin(), data.end(), [](float sample) { return std::isnan(sample); }) )
        throw std::runtime_error(message);
}

vector<Spectrum> SpectrumProcessor::operator()(const Signal& data)
{
    vector<Spectrum> output;
    if (data.size() == 0)
    {
        return output;
    }

    {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        // Append data to the circular buffer
        raw.insert(raw.end(), data.begin(), data.end());

        // first time through if buffer isn't full
        // fill it up with copies of what we have
        while (raw.size() < raw.capacity())
        {
            raw.insert(raw.end(), data.begin(), data.begin() + data.size());
        }
    }

    Spectrum linear_fft(lin_fft_bins);
    Signal slice(window.size());
    size_t step_size = 64; // 64 samples per step
    for (size_t i = 0; i < data.size(); i += step_size)
    {
        Spectrum log2_fft(log_fft_bins);
        // get a copy of the circular buffer for the window
        {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            copy(raw.begin(), raw.begin() + window.size(), slice.begin());
        }

        apply_window(window, slice);
        slice = filter(hpf, raw);
        slice = filter(lpf, raw);
        nan_check(raw, "NaN after LPF");

        std::copy(slice.begin(), slice.end(), fftw_in);
        fftwf_execute(plan);
        std::copy(fftw_out, fftw_out + linear_fft.size(), linear_fft.begin());

        // normalize FFT
        float norm = 2.0f / static_cast<float>(linear_fft.size());
        std::transform(linear_fft.begin(), linear_fft.end(), linear_fft.begin(),
                       [norm](float v) { return v * norm; });

        map_bins(bin_mapping, linear_fft, log2_fft);
        output.push_back(log2_fft);
    }
    return output;
}


