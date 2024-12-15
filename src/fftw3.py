import numpy as np
import ctypes
from ctypes import POINTER, c_double, c_uint, c_void_p

# Load the FFTW3 library
fftw3 = ctypes.CDLL('libfftw3.so')

# Define the FFTW3 functions
fftw_plan_dft_r2c_1d = fftw3.fftw_plan_dft_r2c_1d
fftw_plan_dft_r2c_1d.restype = c_void_p
fftw_plan_dft_r2c_1d.argtypes = [c_uint, POINTER(c_double), POINTER(c_void_p), c_uint]

fftw_execute = fftw3.fftw_execute
fftw_execute.restype = None
fftw_execute.argtypes = [c_void_p]

fftw_destroy_plan = fftw3.fftw_destroy_plan
fftw_destroy_plan.restype = None
fftw_destroy_plan.argtypes = [c_void_p]

# Define a function to perform FFT using FFTW3
def fftw_rfft(data):
    n = len(data)
    output = np.empty(n//2 + 1, dtype=np.complex128)
    plan = fftw_plan_dft_r2c_1d(n, data.ctypes.data_as(POINTER(c_double)), output.ctypes.data_as(POINTER(c_void_p)), 0)
    fftw_execute(plan)
    fftw_destroy_plan(plan)
    return output