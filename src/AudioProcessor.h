#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <vector>
#include <boost/circular_buffer.hpp>
#include <fftw3.h>
#include "Filters.h"

std::vector<float> get_slice(const boost::circular_buffer<float>& buffer, size_t n = 0);

float linbin_to_freq(std::vector<float> buffer, size_t bin, float f0, float f1);
float logbin_to_freq(std::vector<float> buffer, size_t bin, float f0, float f1);

class AudioProcessor {
public:
    AudioProcessor(size_t display_w=1920, size_t display_h=480, size_t window_size=16834);
    ~AudioProcessor();

    void process(const std::vector<float>& data);
    void process_volume(const std::vector<float>& data);
    void process_spectrum(const std::vector<float>& data);

    void precompute_bin_mapping();
    void map_to_log2_bins();

    const std::vector<float> Vrms() const;
    const std::vector<float> Vpeak() const;
    const std::vector<float> Spectrum() const;


private:
    size_t disp_w;
    size_t disp_h;
    float sample_rate;
    float f0;
    float f1;
    std::vector<float> window;
    FilterCoefficients hpf;
    FilterCoefficients lpf;

    boost::circular_buffer<float>   vpk;
    boost::circular_buffer<float>   vrms;
    boost::circular_buffer<float>   raw;
    std::vector<float>              current_slice;
    std::vector<float>              linear_fft;
    float* fftw_in;
    float* fftw_out;;

    fftwf_plan                      plan;
    std::vector<float>              log2_fft;

    struct BinMapping {
        size_t index_low;
        size_t index_high;
        float weight_low;
        float weight_high;
    };
    std::vector<BinMapping>         bin_mapping;
    void normalize_fft();

};

#endif // AUDIO_PROCESSOR_H