#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <vector>
#include <boost/circular_buffer.hpp>
#include <fftw3.h>

std::vector<float> get_slice(const boost::circular_buffer<float>& cb);

class AudioProcessor {
public:
    AudioProcessor(size_t display_w, size_t display_h);
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
};

#endif // AUDIO_PROCESSOR_H