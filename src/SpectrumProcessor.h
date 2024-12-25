#ifndef SPECTRUM_PROCESSOR_H
#define SPECTRUM_PROCESSOR_H

#include <vector>
#include <boost/circular_buffer.hpp>
#include <fftw3.h>
#include "Filters.h"
#include "SpectrumProcessor.h"


float bin_to_freq_linear(std::vector<float> buffer, size_t bin, float f0, float f1);
float bin_to_freq_log2(std::vector<float> buffer, size_t bin, float f0, float f1);
float freq_to_log_fractional_bin(std::vector<float> buffer, float freq, float f0, float f1);

struct BinMapping {
    size_t index;
    float weight;
};
std::vector<BinMapping> precompute_bin_mapping(const std::vector<float> &linear_fft, const std::vector<float> &log_fft, float f0, float f1);

class SpectrumProcessor
{
public:
    SpectrumProcessor(size_t display_w, size_t display_h, size_t window_size);
    ~SpectrumProcessor();

    SpectrumProcessor&      operator()(const std::vector<float>& data);
    void                    normalize_fft();
    void                    map_to_log2_bins();
    std::vector<float>      get_linear_fft() const { return linear_fft; }
    std::vector<float>      get_log2_fft() const { return log2_fft; }

private:
    boost::circular_buffer<float>   raw;
    std::vector<float>      current_slice;
    float                   sample_rate;
    float                   f0;
    float                   f1;
    std::vector<float>      window;
    FilterCoefficients      hpf;
    FilterCoefficients      lpf;
    std::vector<BinMapping> bin_mapping;
    std::vector<float>      linear_fft;
    std::vector<float>      log2_fft;
    float*                  fftw_in;
    float*                  fftw_out;
    fftwf_plan              plan;
};

#endif // SPECTRUM_PROCESSOR_H