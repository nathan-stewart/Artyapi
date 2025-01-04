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
    vrms.push_front(20 * std::log10(rms + LOGMIN));
    vpk.push_front (20 * std::log10(pk  + LOGMIN));
}


AudioProcessor::AudioProcessor(size_t display_w, size_t display_h, size_t window_size)
: disp_w(display_w)
, disp_h(display_h)
, margin_left(0.02f)
, margin_right(0.97f)
, margin_top(0.02f)
, margin_bottom(0.97f)
, display_mode(DisplayMode::Volume)
, process_spectrum(display_w, display_h, window_size)
{
    size_t plottable_width = static_cast<size_t>(static_cast<float>(disp_w) * (margin_right - margin_left));
    // size_t plottable_height = static_cast<size_t>(static_cast<float>(disp_w) * (margin_bottom - margin_top));
    vpk.set_capacity(plottable_width);
    vpk.assign(plottable_width, -96.0f);
    vrms.set_capacity(plottable_width);
    vrms.assign(plottable_width, -96.0f);
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
    gnuplot << "set xrange [1824:0]\n";
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
        std::cout << "Vrms: " << vrms.size() << " entries" << std::endl;
        std::cout << "Vrms: " << vrms.back() << " db" << std::endl;
        gnuplot << "set xrange [" << std::to_string(disp_w) << ":0]" << std::endl;
        if (gnuplot.fail()) {
            std::cerr << "Gnuplot failed to plot" << std::endl;
            throw std::runtime_error("Gnuplot failed to plot");
        }
        // gnuplot << "plot '-' with lines title 'RMS' lc rgb 'red', '-' with lines title 'Peak' lc rgb 'white'\n";
        gnuplot << "plot '-' with lines title 'RMS' lc rgb 'red'"  << std::endl;
        if (gnuplot.fail()) {
            std::cerr << "Gnuplot failed to plot" << std::endl;
            throw std::runtime_error("Gnuplot failed to plot");
        }
        gnuplot.send1d(vrms);
        if (gnuplot.fail()) {
            std::cerr << "Gnuplot failed to plot" << std::endl;
            throw std::runtime_error("Gnuplot failed to plot");
        }
        // gnuplot.send1d(vpk);
        gnuplot << "e"  << std::endl; // End of data
        if (gnuplot.fail()) {
            std::cerr << "Gnuplot failed to plot" << std::endl;
            throw std::runtime_error("Gnuplot failed to plot");
        }
        assert(false);
        //std::cout << "Vrms: " << vrms.back() << " db" << std::endl;
        // std::cout << "Vpeak min: " << *std::min_element(vpk.begin(), vpk.end()) << " Vpeak max: " << *std::max_element(vpk.begin(), vpk.end()) << std::endl;
        break;
    case DisplayMode::Spectrum:
        break;
    }

}
