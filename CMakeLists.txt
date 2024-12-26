cmake_minimum_required(VERSION 3.10)

# Set the project name
project(rta)

find_package(SDL2 REQUIRED)
find_package(pybind11 REQUIRED)

# Find the FFTW3 library
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    find_library(FFTW3_LIB fftw3f HINTS /usr/lib/x86_64-linux-gnu)
    find_path(FFTW3_INCLUDE_DIR fftw3.h HINTS /usr/include)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
    find_library(FFTW3_LIB fftw3f HINTS /usr/lib/arm-linux-gnueabihf)
    find_path(FFTW3_INCLUDE_DIR fftw3.h HINTS /usr/include)
else()
    message(FATAL_ERROR "Unsupported architecture")
endif()

# Add the include directories
include_directories(${FFTW3_INCLUDE_DIR})

# Add global compiler flags
add_compile_options(-Wall -Wextra -Wconversion -Wfloat-conversion -Wdouble-promotion -Werror)

# Add the executable
add_executable(rta src/sdlmain.cpp src/AudioProcessor.cpp src/SpectrumProcessor.cpp src/Filters.cpp)

# Link the SDL2 and FFTW3 libraries
target_link_libraries(rta SDL2::SDL2 pybind11::module ${FFTW3_LIB})

# Include Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  URL   https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
  URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Add the test executable
add_executable(TestMain tests/test_util.cpp tests/test_audioprocessor.cpp tests/test_filters.cpp  tests/test_circularbuffer.cpp src/AudioProcessor.cpp src/Filters.cpp src/SpectrumProcessor.cpp)

# Link the FFTW3 and Google Test libraries to the test executable
target_link_libraries(TestMain ${FFTW3_LIB} pybind11::module gtest_main)

# Add the test to CTest
add_test(NAME TestMain COMMAND TestMain)

# Add a custom target to run the tests
add_custom_target(run_tests COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure DEPENDS TestMain)

# Make the default target the run_tests target
add_custom_target(default_target ALL DEPENDS run_tests)