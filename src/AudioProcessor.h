#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <vector>
#include <boost/circular_buffer.hpp>
#include <fftw3.h>

std::vector<float> get_slice(const boost::circular_buffer<float>& cb);

class AudioProcessor {
public:
    AudioProcessor(size_t display_w=1920, size_t display_h=480, size_t window_size=16834);
    ~AudioProcessor();

    void process_data(const std::vector<float>& data);
        
    const std::vector<float> Vrms() const;
    const std::vector<float> Vpeak() const;

private:
    size_t disp_w;
    size_t disp_h;

    boost::circular_buffer<float> raw;
    boost::circular_buffer<float> vpk;
    boost::circular_buffer<float> vrms;
    fftwf_plan plan;

    
};

#endif // AUDIO_PROCESSOR_H