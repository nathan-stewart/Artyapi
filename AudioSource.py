#!/usr/bin/env python3
import soundfile as sf
import time
import numpy as np
import threading
import os
import logging

# Initialize logging
logging.basicConfig(level=logging.INFO)

samplerate = None
p = None
if not p:
    import pyaudio
    p = pyaudio.PyAudio()

def list_audio_devices():
    global p
    for i in range(p.get_device_count()):
        dev = p.get_device_info_by_index(i)
        if dev['maxInputChannels'] < 1:
            continue
        print(f"Device {dev['name']} ({i})")

def RealTimeAudioSource(source):
    global p, samplerate
    # Initialize audio capture
    os.environ['PA_ALSA_PLUGHW'] = '1'
    os.environ['PYTHONWARNINGS'] = 'ignore'
    bufflen = 2**16    

    def get_audio_device_index(name):
        for i in range(p.get_device_count()):
            dev = p.get_device_info_by_index(i)
            if dev['maxInputChannels'] < 1:
                continue
            if not name:
                print(f"Device {dev['name']} ({i})")
            elif name in dev['name']:
                return i
        return None

    def get_preferred_samplerate(dev_index):
        dev = p.get_device_info_by_index(dev_index)
        for rate in [48000, 44100, 96000, 192000]:
            if dev['defaultSampleRate'] == rate:
                return rate
        raise ValueError('No supported sample rate found')

    def _capture_audio():
        nonlocal buffer, write_index, stop_flag
        while not stop_flag:
            try:
                # Capture new audio data
                data = stream.read(bufflen, exception_on_overflow=False)
                new_data = np.frombuffer(data, dtype=np.int16)

                # Safely update the buffer in a thread-safe manner (circular buffer)
                with lock:
                    # Find the space available and wrap around if necessary
                    end_index = (write_index + len(new_data)) % buffer.size

                    # If no wrap-around, directly copy
                    if write_index < end_index:
                        buffer[write_index:end_index] = new_data
                    else:
                        # If wrapping, copy in two parts
                        buffer[write_index:] = new_data[:buffer.size - write_index]
                        buffer[:end_index] = new_data[buffer.size - write_index:]

                    # Update write_index
                    write_index = end_index
            except OSError as e:
                if e.errno == -9981:
                    logging.error('input overflowed: skipping buffer')
    
    source = get_audio_device_index(source)

    if source is None:
        logging.error('Audio input device found')
        raise RuntimeError('No audio input device found')

    samplerate = get_preferred_samplerate(source)
    stream = p.open(format=pyaudio.paInt16, channels=1, rate=samplerate, input=True, frames_per_buffer=1024, input_device_index=source)

    # Initialize circular buffer and threading
    buffer = np.zeros(bufflen, dtype=np.int16)
    write_index = 0
    lock = threading.Lock()
    stop_flag = False
    capture_thread = threading.Thread(target=_capture_audio)
    capture_thread.start()

    # Generator to yield the latest audio data
    while True:
        t0 = time.time()

        # Safely access the buffer and return the last `chunksize` samples
        with lock:
            # Return a copy of the circular buffer, starting from the correct point
            if write_index == bufflen:
                chunk = buffer.copy()  # No wrapping needed
            else:
                # Return the last `chunksize` samples in proper order (handle wrap-around)
                chunk = np.concatenate((buffer[write_index:], buffer[:write_index]))

        yield chunk

def FileAudioSource(testdir):
    global samplerate
    files = os.listdir(testdir)
    while True:
        for f in files:
            fullpath = os.path.join(testdir, f)
            if not (fullpath and fullpath.lower().endswith('.wav')):
                raise ValueError('Only .wav files are supported')

            with sf.SoundFile(fullpath) as audio_file:
                samplerate = audio_file.samplerate
                n_samples = len(audio_file)
                t0 = time.time()
                p0 = 0
                p1 = 0
                while p1 < n_samples:
                    now = time.time()
                    p0 = p1
                    p1 = int((now - t0) * samplerate)
                    audio_file.seek(p0)
                    chunk = audio_file.read(p1 - p0, dtype='float32')

                    # Convert to mono if necessary
                    if len(chunk.shape) == 2:
                        chunk = chunk.mean(axis=1)

                    yield chunk

        files = os.listdir(testdir)
