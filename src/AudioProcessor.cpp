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
    gnuplot << "set terminal x11 size 1920,480\n";    
    gnuplot << "set xrange [0:" << disp_w << "]\n";
    gnuplot << "set yrange [-96:12]\n";
    gnuplot << "set ytics 12\n";
    gnuplot << "set mytics 4\n";
    gnuplot << "set tics scale 2,1.2\n";
    gnuplot << "set border linewidth 1 lc rgb 'white'\n";
    gnuplot << "set tics textcolor rgb 'white'\n";
    gnuplot << "set ylabel textcolor rgb 'white'\n";
    gnuplot << "set object 1 rectangle from screen 0,0 to screen 1,1 fillcolor rgb 'black' behind\n";
    gnuplot << "set lmargin at screen 0.02\n";
    gnuplot << "set rmargin at screen 0.99\n";
    gnuplot << "set bmargin at screen 0.02\n";
    gnuplot << "set tmargin at screen 0.97\n";
    gnuplot << "unset xtics\n";
    gnuplot << "plot '-' with lines title 'RMS' lc rgb 'white', '-' with lines title 'Peak' lc rgb 'white'\n";

    gnuplot.send1d(vrms);
    gnuplot.send1d(vpk);
    gnuplot << "e\n"; // End of data
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
        // gnuplot.send1d(vrms);
        // gnuplot.send1d(vpk);
        // gnuplot << "e\n"; // End of data
        break;
    case DisplayMode::Spectrum:
        break;
    }

}
