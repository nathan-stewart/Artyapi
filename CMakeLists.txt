cmake_minimum_required(VERSION 3.10)
project(rta)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)
include_directories(${SNDFILE_INCLUDE_DIRS})
link_directories(${SNDFILE_LIBRARY_DIRS})

pkg_check_modules(FFTW3 REQUIRED fftw3f)
include_directories(${FFTW3_INCLUDE_DIRS})
link_directories(${FFTW3_LIBRARY_DIRS})

find_package(Boost REQUIRED COMPONENTS iostreams system filesystem program_options)
include_directories(${Boost_INCLUDE_DIRS})

link_directories(/usr/lib/x86_64-linux-gnu)


# Add global compiler flags
add_compile_options(
  -Wall
  -Werror
  -Wextra
  -Wconversion
  -Wfloat-conversion
  -Wdouble-promotion
)

# Add the executable
add_executable(rta
  src/main.cpp
  src/AudioSource.cpp
  src/AudioProcessor.cpp
  src/SpectrumProcessor.cpp
  src/Filters.cpp
)

# Link the necessary libraries
target_link_libraries(rta
  ${Boost_IOSTREAMS_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${SNDFILE_LIBRARIES}
  ${FFTW3_LIBRARIES})

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
)

# Link the FFTW3 and Google Test libraries to the test executable
target_link_libraries(TestMain
  ${Boost_IOSTREAMS_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${SNDFILE_LIBRARIES}
  ${FFTW3_LIBRARIES}
  gtest_main)

# Add the test to CTest
add_test(NAME TestMain COMMAND TestMain)

# Add a custom target to run the tests

add_custom_target(tests 
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure  DEPENDS TestMain
)


add_custom_target(run COMMAND rta DEPENDS rta)

# Make the default target the run_tests target
add_custom_target(default_target ALL DEPENDS tests)