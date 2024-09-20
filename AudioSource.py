#!/usr/bin/env python3
import soundfile as sf
import pyaudio
import time
import numpy as np
import threading
import os 


def RealTimeAudioSource(chunksize=16384, readbufsize=4096, samplerate=44100):
    def _capture_audio():
        nonlocal buffer, write_index

        while not stop_flag:
            # Capture new audio data
            data = stream.read(readbufsize)
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

    # Initialize audio capture
    p = pyaudio.PyAudio()
    stream = p.open(format=pyaudio.paInt16, channels=1, rate=samplerate, input=True, frames_per_buffer=readbufsize)

    # Initialize circular buffer and threading
    buffer = np.zeros(chunksize, dtype=np.int16)
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
            if write_index == chunksize:
                chunk = buffer.copy()  # No wrapping needed
            else:
                # Return the last `chunksize` samples in proper order (handle wrap-around)
                chunk = np.concatenate((buffer[write_index:], buffer[:write_index]))

        yield chunk

def FileAudioSource(testdir, chunksize):
    files = os.listdir(testdir)
    while True:
        for f in files:
            fullpath = os.path.join(testdir, f)
            if not (fullpath and fullpath.lower().endswith('.wav')):
                print(fullpath)
                raise ValueError('Only .wav files are supported')

            audio_data, samplerate = sf.read(fullpath)

            # Convert to mono if necessary
            if len(audio_data.shape) == 2:
                audio_data = audio_data.mean(axis=1)

            audio_data = audio_data.astype(np.float32)  # Convert to float
            audio_data /= np.max(np.abs(audio_data))  # Normalize to range [-1, 1]
            n_samples = len(audio_data)
            position = 0
            t0 = time.time()
            while True:
                running_time = time.time() - t0
                position = int(running_time * samplerate)
                if position + chunksize > n_samples:
                    chunk = audio_data[position:position + chunksize]
                    if len(chunk) < chunksize:
                        chunk = np.pad(chunk, (0, chunksize - len(chunk)), mode='constant')
                    yield chunk
                    break
                else:
                    chunk = audio_data[position:position + chunksize]
                    yield chunk
        files = os.listdir(testdir)
    
    