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


void AudioProcessor::create_volume_plot()
{
    // float dpi = 100.0f;
    // Volume Plot is filled below rms and peak is a line plot
    // gnuplot << "set terminal fbdev\n";
    gnuplot << "set terminal x11\n";
    gnuplot << "set title 'Volume'\n";
    gnuplot << "set xrange [0:" << disp_w << "]\n";
    gnuplot << "set yrange [-96:12]\n";
    gnuplot << "set ylabel 'SPL'\n";
    gnuplot << "plot '-' with lines title 'RMS', '-' with lines title 'Peak'\n";
}


void AudioProcessor::process(const Signal& data)
{
    if (data.size() == 0)
        return;

    process_volume(data, vrms, vpk);
    process_spectrum(data);
}

void AudioProcessor::update_plot()
{
    switch (display_mode)
    {
    case DisplayMode::Volume:
        gnuplot.send1d(vrms);
        // gnuplot.send1d(vpk);
        gnuplot << "e\n"; // End of data
        break;
    case DisplayMode::Spectrum:
        break;
    }

}
