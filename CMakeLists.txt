cmake_minimum_required(VERSION 3.10)
project(rta)
# Set the build type to Debug
set(
   Debug)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)
include_directories(${SNDFILE_INCLUDE_DIRS})
link_directories(${SNDFILE_LIBRARY_DIRS})

find_package(SDL2 REQUIRED)

pkg_check_modules(FFTW3 REQUIRED fftw3f)
include_directories(${FFTW3_INCLUDE_DIRS})
link_directories(${FFTW3_LIBRARY_DIRS})

find_library(FFTW3F_THREADS_LIBRARY NAMES fftw3f_threads)
if (NOT FFTW3F_THREADS_LIBRARY)
    message(FATAL_ERROR "Could not find fftw3f_threads library")
endif()

find_package(Boost REQUIRED COMPONENTS iostreams system filesystem program_options)
include_directories(${Boost_INCLUDE_DIRS})


find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
find_library(PORTAUDIO_LIBRARY NAMES portaudio)

if (PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
    set(PORTAUDIO_FOUND TRUE)
    message(STATUS "Found PortAudio: ${PORTAUDIO_LIBRARY}")
else()
    set(PORTAUDIO_FOUND FALSE)
    message(FATAL_ERROR "Could not find PortAudio")
endif()

include_directories(${PORTAUDIO_INCLUDE_DIR})

link_directories(/usr/lib/x86_64-linux-gnu)


# Add global compiler flags
# -fsanitize=address
add_compile_options(
  -Wall
  -Werror
  -Wextra
  -Wconversion
  -Wfloat-conversion
  -Wdouble-promotion
  -std=c++17
  -g
)


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-O0)
else()
  add_compile_options(-O3)
endif()


# Add the executable
add_executable(rta
  src/main.cpp
  src/AudioSource.cpp
  src/AudioProcessor.cpp
  src/SpectrumProcessor.cpp
  src/Filters.cpp
  src/Plotter.cpp
)

# Link the necessary libraries
target_link_libraries(rta
  ${Boost_IOSTREAMS_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${PORTAUDIO_LIBRARY}
  ${SNDFILE_LIBRARIES}
  ${FFTW3_LIBRARIES}
  ${FFTW3F_THREADS_LIBRARY}
  SDL2::SDL2)

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
add_executable(TestMain
  tests/test_util.cpp
  tests/test_audioprocessor.cpp
  tests/test_filters.cpp
  tests/test_circularbuffer.cpp
  tests/test_audiosource.cpp
  src/AudioProcessor.cpp
  src/Filters.cpp
  src/SpectrumProcessor.cpp
  src/AudioSource.cpp
  src/Plotter.cpp
)

# Link the FFTW3 and Google Test libraries to the test executable
target_link_libraries(TestMain
  ${Boost_IOSTREAMS_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${PORTAUDIO_LIBRARY}
  ${SNDFILE_LIBRARIES}
  ${FFTW3_LIBRARIES}
  ${FFTW3F_THREADS_LIBRARY}
  SDL2::SDL2
  gtest_main)

# Add the test to CTest
add_test(NAME TestMain COMMAND TestMain)

# Add a custom target to run the tests

add_custom_target(tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure  DEPENDS TestMain
)

# Add the executable for automated unit tests
add_executable(plot_test
  src/Plotter.cpp
  tests/test_plotter.cpp
  tests/test_util.cpp
  # Add other unit test files here
)

target_link_libraries(plot_test
  SDL2::SDL2
  ${SNDFILE_LIBRARIES}
  gtest_main)


add_custom_target(run COMMAND rta DEPENDS rta)

# Make the default target the run_tests target
add_custom_target(default_target ALL DEPENDS tests)

add_custom_target(test_plot COMMAND plot_test DEPENDS plot_test)