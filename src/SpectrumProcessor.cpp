#include "SpectrumProcessor.h"
#include <cmath>


float bin_to_freq_linear(std::vector<float> buffer, size_t bin, float f0, float f1)
{
    int num_bins = static_cast<int>(buffer.size());
    float log2_bin_index = static_cast<float>(bin) / static_cast<float>(num_bins);
    return f0 + log2_bin_index * (f1 - f0);
}

float bin_to_freq_log2(std::vector<float> buffer, size_t bin, float f0, float f1)
{
    int num_bins = static_cast<int>(buffer.size());
    float bin_index = static_cast<float>(bin) / static_cast<float>(num_bins);
    return f0 * std::pow(2.0f, static_cast<float>(log2(f1 / f0)) * bin_index);
}

float freq_to_log_fractional_bin(std::vector<float> buffer, float freq, float f0, float f1)
{
    float num_bins = static_cast<float>(buffer.size());
    float log2_bin_index = std::log2(freq / f0) / std::log2(f1 / f0);
    return log2_bin_index * num_bins;
}

std::vector<BinMapping> precompute_bin_mapping(const std::vector<float> &linear_fft, const std::vector<float> &log_fft, float f0, float f1)
{
    size_t linear_size = linear_fft.size();
    std::vector<BinMapping> mapping(linear_size);

    for (size_t i = 0; i < linear_size; ++i) {
        float freq = bin_to_freq_linear(linear_fft, i, f0, f1);
        float log2_bin_index_exact = freq_to_log_fractional_bin(log_fft, freq, f0, f1);
        size_t index  = static_cast<size_t>(std::floor(log2_bin_index_exact));
        float weight  = log2_bin_index_exact - static_cast<float>(index);

        if (index < log_fft.size())
        {
            mapping[i].index = index;
            mapping[i].weight = weight;
        }
    }
    return mapping;
}

SpectrumProcessor::SpectrumProcessor(size_t display_w, [[maybe_unused]] size_t display_h, size_t window_size)
: sample_rate(48000.0f)
, f0(40.0f)
, f1(20000.0f)
{
    linear_fft.resize(static_cast<size_t>(window_size / 2 + 1));
    log2_fft.resize(display_w);

    bin_mapping = precompute_bin_mapping(linear_fft, log2_fft, f0, f1);
    hpf = butterworth_hpf(4, f0, sample_rate);
    lpf = butterworth_lpf(4, f1, sample_rate);
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


SpectrumProcessor& SpectrumProcessor::operator()(const std::vector<float>& data)
{
    // Append data to the circular buffer
    raw.insert(raw.end(), data.begin(), data.end());

    // if buffer isn't full - fill it up with copies of what we have
    while (raw.size() < raw.capacity())
    {
        size_t n = raw.capacity() - raw.size();
        raw.insert(raw.end(), raw.begin(), raw.begin() + n);
    }

    std::copy(raw.begin(), raw.end(), current_slice.begin());
    apply_window(window, current_slice);
    apply_filter(hpf, current_slice);
    apply_filter(lpf, current_slice);
    std::copy(current_slice.begin(), current_slice.end(), fftw_in);
    fftwf_execute(plan);
    std::copy(fftw_out, fftw_out + linear_fft.size(), linear_fft.begin());
    normalize_fft();
    //map_to_log2_bins();

    // Compute Decay per bin
    return *this;
}


void SpectrumProcessor::map_to_log2_bins()
{
    // Zero out the log2 FFT
    log2_fft.assign(log2_fft.size(), 0.0f);

    // Use the precomputed mapping to map linear FFT bins to log2-spaced bins
    size_t num_bins = log2_fft.size();
    for (size_t i = 0; i < linear_fft.size() - 1; ++i) {
        const auto& mapping = bin_mapping[i];
        if (mapping.index < num_bins) {
            log2_fft[mapping.index] += linear_fft[i] * mapping.weight;
        }
        if (mapping.index + 1 < num_bins) {
            log2_fft[mapping.index + 1] += linear_fft[i] * (1.0f - mapping.weight);
        }
    }
}


void SpectrumProcessor::normalize_fft()
{
    // normalize FFT
    float norm = 2.0f / static_cast<float>(linear_fft.size());
    std::transform(linear_fft.begin(), linear_fft.end(), linear_fft.begin(),
                   [norm](float v) { return v * norm; });
}

