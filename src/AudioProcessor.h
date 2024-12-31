#pragma once

#include <vector>
#include <boost/circular_buffer.hpp>
#include <fftw3.h>
#include <gnuplot-iostream.h>
#include "Filters.h"
#include "SpectrumProcessor.h"

void  process_volume(const Signal& data, boost::circular_buffer<float>& vrms, boost::circular_buffer<float>& vpk);

class AudioProcessor
{
public:
    friend class AudioProcessorTest;
    AudioProcessor(size_t display_w=1920, size_t display_h=480, size_t window_size=16834);
    ~AudioProcessor();

    void process(const Signal& data);
    void setup_plot();

private:
    size_t disp_w;
    size_t disp_h;
    float  history;

    Gnuplot                         gnuplot;
    SpectrumProcessor               process_spectrum;
    boost::circular_buffer<float>   vpk;
    boost::circular_buffer<float>   vrms;
    Spectrum                        log2_fft;
};
