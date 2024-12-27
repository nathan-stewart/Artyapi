#pragma once

#include <vector>
#include <boost/circular_buffer.hpp>
#include <fftw3.h>
#include "Filters.h"
#include "SpectrumProcessor.h"

// Spectrum is just a typed vector<float>
struct Spectrum {
    std::vector<float> data;

    // Constructors for convenience
    Spectrum() = default;
    Spectrum(size_t size) : data(size) {}
    Spectrum(const std::vector<float>& vec) : data(vec) {}
    Spectrum(std::vector<float>&& vec) : data(std::move(vec)) {}

    // Provide access to the underlying vector
    float& operator[](size_t index) { return data[index]; }
    const float& operator[](size_t index) const { return data[index]; }
    size_t size() const { return data.size(); }
    void resize(size_t newSize) { data.resize(newSize); }
    void fill(float value) { std::fill(data.begin(), data.end(), value); }

    // expose the underlying iterators
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }

};

float bin_to_freq_linear(const Spectrum& spectrum, float bin, float f0, float f1);
float bin_to_freq_log2(const Spectrum& spectrum, float bin, float f0, float f1);
float freq_to_lin_fractional_bin(const Spectrum& spectrum, float freq, float f0, float f1);
float freq_to_log_fractional_bin(const Spectrum& spectrum, float freq, float f0, float f1);

Spectrum precompute_bin_mapping(const Spectrum &linear_fft, const Spectrum &log_fft, float f0, float f1);
void map_bins(const Spectrum& bin_mapping, const Spectrum& source, Spectrum& destination);

class SpectrumProcessor
{
public:
    SpectrumProcessor(size_t display_w, size_t display_h, size_t window_size);
    ~SpectrumProcessor();

    SpectrumProcessor& operator()(const Signal& data);
    void                normalize_fft();
    Spectrum            get_linear_fft() const { return linear_fft; }
    Spectrum            get_log2_fft() const { return log2_fft; }

private:
    boost::circular_buffer<float>   raw;
    Signal             current_slice;
    float              sample_rate;
    float              f0;
    float              f1;
    Signal             window;
    FilterCoefficients hpf;
    FilterCoefficients lpf;
    Spectrum           bin_mapping;
    Spectrum           linear_fft;
    Spectrum           log2_fft;
    float*             fftw_in;
    float*             fftw_out;
    fftwf_plan         plan;
};
