#pragma once
#include <vector>
#include <boost/circular_buffer.hpp>
#include <fftw3.h>
#include <thread>
#include <mutex>
#include "Signal.h"
#include "Filters.h"
#include "SpectrumProcessor.h"

// Spectrum is just a typed vector<float>
struct Spectrum {
    std::vector<float> data;

    // Constructors for convenience
    Spectrum() = default;
    virtual ~Spectrum() = default;
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

float bin_to_freq_linear(size_t num_bins, float bin, float f0, float f1);
float bin_to_freq_log2(size_t num_bins, float bin, float f0, float f1);
float freq_to_lin_fractional_bin(size_t num_bins, float freq, float f0, float f1);
float freq_to_log_fractional_bin(size_t num_bins, float freq, float f0, float f1);

void map_bins(const Spectrum& bin_mapping, const Spectrum& source, Spectrum& destination);
Spectrum precompute_bin_mapping(size_t lin_fft_bins, size_t log_fft_bins, float f0, float f1);

class SpectrumProcessor
{
public:
    SpectrumProcessor(size_t window_size, size_t log_bin_count);
    virtual ~SpectrumProcessor();

    virtual std::vector<Spectrum> operator()(const Signal& data);

private:
    mutable std::mutex              buffer_mutex;
    boost::circular_buffer<float>   raw;
    float              sample_rate;
    float              f0;
    float              f1;
    size_t             lin_fft_bins;
    size_t             log_fft_bins;
    Signal             window;
    FilterCoefficients hpf;
    FilterCoefficients lpf;
    Spectrum           bin_mapping;
    float*             fftw_in;
    float*             fftw_out;
    fftwf_plan         plan;
};
