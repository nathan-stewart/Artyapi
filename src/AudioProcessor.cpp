#include "AudioProcessor.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>
const float LOGMIN = 1e-10f;

std::vector<float> get_slice(const boost::circular_buffer<float>& buffer, size_t n) 
{
    if (n == 0)
        n = buffer.size();
    std::vector<float> slice(n);
    auto it = buffer.end() - n;
    std::copy(it, buffer.end(), slice.begin());
    return slice;
}


AudioProcessor::AudioProcessor(size_t display_w, size_t display_h, size_t window_size)
: disp_w(display_w)
, disp_h(display_h)
, sample_rate(48000.0f)
, f0(40.0f)
, f1(20000.0f)
{
    raw.set_capacity(window_size);
    vpk.set_capacity(display_w);
    vrms.set_capacity(display_w);

    precompute_bin_mapping();
    hpf = butterworth_filter(4, f0);
    lpf = butterworth_filter(4, f1);

    // out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * (n / 2 + 1));
    // plan = fftwf_plan_dft_r2c_1d(window_size, 
    //                             raw.data(), 
    //                             reinterpret_cast<fftwf_complex*>(out.data()), 
    //                             FFTW_ESTIMATE);
}


AudioProcessor::~AudioProcessor()
{
    // fftwf_destroy_plan(plan);
}


void AudioProcessor::process(const std::vector<float>& data)
{
    process_volume(data);
    process_spectrum(data);
}


void AudioProcessor::process_volume(const std::vector<float>& data)
{
    // Compute RMS and peak
    float rms = 0.0;
    float peak = 0.0;

    for (float v : data) {
        v = fabsf(v);
        rms += v * v;
        if (v > peak)
            peak = v;
    }
    rms = sqrtf(rms / float(data.size()));
    // Update circular buffers with the db values
    vpk.push_back(20 * log10f(peak + LOGMIN ));
    vrms.push_back(20 * log10f(rms + LOGMIN ));
}


void AudioProcessor::process_spectrum(const std::vector<float>& data)
{
    // Append data to the circular buffer
    raw.insert(raw.end(), data.begin(), data.end());

    // if buffer isn't full - fill it up with copies of what we have
    // while (raw.size() < raw.capacity()) {
    //     size_t n = raw.capacity() - raw.size();
    //     raw.insert(raw.end(), raw.begin(), raw.begin() + n);
    // }

    std::vector<float> slice = get_slice(raw);
    // apply_window(window, slice);
    // apply_filter(hpf, slice);
    // apply_filter(lpf, slice);
    // Perform FFT
    // Map FFT bins to log2 bins
    // Compute Decay per bin
}


const std::vector<float> AudioProcessor::Vrms() const
{
    return get_slice(vrms);
}


const std::vector<float> AudioProcessor::Vpeak() const
{
    return get_slice(vpk);
}

void AudioProcessor::precompute_bin_mapping()
{
    size_t num_bins = disp_w; // Number of bins based on display width
    bin_mapping.resize(num_bins);
    log2_fft.resize(num_bins, 0.0f);

    // Precompute the mapping from linear FFT bins to log2-spaced bins
    for (size_t i = 0; i < num_bins; ++i) {
        float log2_bin_index = static_cast<float>(i) / static_cast<float>(num_bins);
        float freq = f0 * std::pow(2.0f, log2_bin_index * std::log2(f1 / f0));

        float log2_bin_index_exact =static_cast<float>(num_bins) * (log2f(freq / f0) / log2f(f1 / f0));
        size_t index_low = static_cast<size_t>(std::floor(log2_bin_index_exact));
        size_t index_high = static_cast<size_t>(std::ceil(log2_bin_index_exact));

        float weight_low = static_cast<float>(index_high) - static_cast<float>(log2_bin_index_exact);
        float weight_high = static_cast<float>(log2_bin_index_exact) - static_cast<float>(index_low);

        bin_mapping[i] = {index_low, index_high, weight_low, weight_high};
    }
}


void AudioProcessor::map_to_log2_bins()
{
    size_t num_bins = disp_w; // Number of bins based on display width
    
    // Use the precomputed mapping to map linear FFT bins to log2-spaced bins
    for (size_t i = 0; i < linear_fft.size(); ++i) {
        const auto& mapping = bin_mapping[i];
        if (mapping.index_low < num_bins) {
            log2_fft[mapping.index_low] += linear_fft[i] * mapping.weight_low;
        }
        if (mapping.index_high < num_bins && mapping.index_high != mapping.index_low) {
            log2_fft[mapping.index_high] += linear_fft[i] * mapping.weight_high;
        }
    }
}
