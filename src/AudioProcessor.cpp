#include "AudioProcessor.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>
#include <iostream>

const float LOGMIN = 1e-10f;

void  process_volume(const Signal& data, boost::circular_buffer<float>& vrms, boost::circular_buffer<float>& vpk)
{
    float rms = 0.0f;
    float pk = 0.0f;

    for (auto& sample : data)
    {
        rms += sample * sample;
        pk = std::max(pk, std::abs(sample));
    }

    rms = sqrtf(rms / static_cast<float>(data.size()));
    vrms.push_back(rms);
    vpk.push_back(pk);
}


AudioProcessor::AudioProcessor(size_t display_w, size_t display_h, size_t window_size)
: disp_w(display_w)
, disp_h(display_h)
, display_mode(DisplayMode::Volume)
, process_spectrum(display_w, display_h, window_size)
{
    vpk.set_capacity(display_w);
    vrms.set_capacity(display_w);
}


AudioProcessor::~AudioProcessor()
{
}

void AudioProcessor::setup_plots()
{
    // float dpi = 100.0f;
    // Volume Plot is filled below rms and peak is a line plot
    gnuplot << "set multiplot layout 2,1\n";
    // gnuplot << "set terminal wxt size " << disp_w / dpi << "," << disp_h / dpi << " font 'Arial,10'\n";
    gnuplot << "set size 1,0.5\n";
    gnuplot << "set origin 0,0.5\n";
    gnuplot << "set title 'Volume'\n";
    gnuplot << "set xrange [0:" << disp_w << "]\n";
    gnuplot << "set yrange [0:1]\n";
    gnuplot << "set xlabel 'Time'\n";
    gnuplot << "set ylabel 'Volume'\n";
    gnuplot << "plot '-' with lines title 'RMS', '-' with lines title 'Peak'\n";
    gnuplot.send1d(vrms);
    gnuplot.send1d(vpk);
    gnuplot << "set size 1,0.5\n";
    gnuplot << "set origin 0,0\n";

    // gnuplot << "set title 'Spectrum'\n";
    // Spectral plot is pixel based heat map


}

void AudioProcessor::process(const Signal& data)
{
    if (data.size() == 0)
        return;

    process_volume(data, vrms, vpk);
    process_spectrum(data);
}
