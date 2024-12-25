#ifndef SPECTRUM_PROCESSOR_H
#define SPECTRUM_PROCESSOR_H

#include <vector>
#include <boost/circular_buffer.hpp>
#include <fftw3.h>
#include "Filters.h"
#include "SpectrumProcessor.h"

struct BinMapping {
    size_t index_low;
    size_t index_high;
    float weight_low;
    float weight_high;
};

class SpectrumProcessor
{
public:
    SpectrumProcessor(size_t display_w, size_t display_h, size_t window_size);
    ~SpectrumProcessor();

    SpectrumProcessor&      operator()(const std::vector<float>& data);
    std::vector<BinMapping> precompute_bin_mapping(size_t num_bins);
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