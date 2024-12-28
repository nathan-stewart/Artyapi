#include "SpectrumProcessor.h"
#include <cmath>
#include <iostream>

float bin_to_freq_linear(const Spectrum& spectrum, float bin, float f0, float f1)
{
    int num_bins = static_cast<int>(spectrum.size());
    float log2_bin_index = bin / static_cast<float>(num_bins);
    return f0 + log2_bin_index * (f1 - f0);
}


float bin_to_freq_log2(const Spectrum& spectrum, float bin, float f0, float f1)
{
    int num_bins = static_cast<int>(spectrum.size());
    float bin_index = bin / static_cast<float>(num_bins);
    return f0 * std::pow(2.0f, static_cast<float>(log2(f1 / f0)) * bin_index);
}


float freq_to_lin_fractional_bin(const Spectrum& spectrum, float freq, float f0, float f1)
{
    float num_bins = static_cast<float>(spectrum.size());
    float bin_fraction = (freq - f0) / (f1 - f0);
    return bin_fraction * num_bins;
}


float freq_to_log_fractional_bin(const Spectrum& spectrum, float freq, float f0, float f1)
{
    float num_bins = static_cast<float>(spectrum.size());
    float log2_bin_index = std::log2(freq / f0) / std::log2(f1 / f0);
    return log2_bin_index * num_bins;
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

Spectrum precompute_bin_mapping(const Spectrum& source, const Spectrum& destination, float f0, float f1)
{
    size_t linear_size = source.size();
    Spectrum mapping(linear_size);
    for (size_t i = 0; i < linear_size; ++i) {
        float freq = bin_to_freq_linear(source, static_cast<float>(i), f0, f1);
        mapping[i] = freq_to_log_fractional_bin(destination, freq, f0, f1);
    }
    return mapping;
}

SpectrumProcessor::SpectrumProcessor(size_t display_w, [[maybe_unused]] size_t display_h, size_t window_size)
: raw(window_size)
, current_slice(window_size)
, sample_rate(48000.0f)
, f0(40.0f)
, f1(20000.0f)

{
    linear_fft.resize(static_cast<size_t>(window_size / 2 + 1));
    log2_fft.resize(display_w);
    bin_mapping = precompute_bin_mapping(linear_fft, log2_fft, f0, f1);

    hpf = butterworth(2, f0, sample_rate, true);
    lpf = butterworth(4, f1, sample_rate, false);
    window = hanning_window(window_size);

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
    fftwf_destroy_plan(plan);
    fftwf_free(fftw_in);
    fftwf_free(fftw_out);
    fftw_in = nullptr;
    fftw_out = nullptr;
    plan = nullptr;
}


SpectrumProcessor& SpectrumProcessor::operator()(const Signal& data)
{
    // Append data to the circular buffer
    raw.insert(raw.end(), data.begin(), data.end());

    // if buffer isn't full - fill it up with copies of what we have
    while (raw.size() < raw.capacity())
    {
        size_t n = raw.capacity() - raw.size();
        raw.insert(raw.end(), data.begin(), data.begin() + n);
    }

    std::copy(raw.begin(), raw.end(), current_slice.begin());
    apply_window(window, current_slice);
    current_slice = filter(hpf, current_slice);
    current_slice = filter(lpf, current_slice);
    std::copy(current_slice.begin(), current_slice.end(), fftw_in);
    fftwf_execute(plan);
    std::copy(fftw_out, fftw_out + linear_fft.size(), linear_fft.begin());
    normalize_fft();
    map_bins(bin_mapping, linear_fft, log2_fft);

    // Compute Decay per bin
    return *this;
}


void SpectrumProcessor::normalize_fft()
{
    // normalize FFT
    float norm = 2.0f / static_cast<float>(linear_fft.size());
    std::transform(linear_fft.begin(), linear_fft.end(), linear_fft.begin(),
                   [norm](float v) { return v * norm; });
}

