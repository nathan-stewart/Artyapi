#include "AudioProcessor.h"

std::vector<float> get_slice(const boost::circular_buffer<float>& cb) {
    std::vector<float> slice(cb.size());
    std::copy(cb.begin(), cb.end(), slice.begin());
    return slice;
}

AudioProcessor::AudioProcessor(int fft_window, size_t display_w, size_t display_h)
: window_size(fft_window)
, disp_w(display_w)
, disp_h(display_h)
{
    int window_size;
    size_t disp_w;
    size_t disp_h;

    boost::circular_buffer<float> raw;
    boost::circular_buffer<float> vpk;
    boost::circular_buffer<float> vrms;
    fftw_plan fft_plan;
}


AudioProcessor::~AudioProcessor()
{

}


void AudioProcessor::fetch()
{
}


void AudioProcessor::draw()
{

}

const std::vector<float> AudioProcessor::Vrms() const
{
    return get_slice(this->vrms);
}

const std::vector<float> AudioProcessor::Vpeak() const
{
    return get_slice(this->vrms);
}

