cmake_minimum_required(VERSION 3.15)

project(EarthDSPCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Portable, dependency-free DSP core. No JUCE, Emscripten, WebAudio or Daisy.
add_library(earth_core STATIC
    EarthDSPCore.cpp
    Dattorro/Dattorro.cpp
    Dattorro/dsp/delays/InterpDelay.cpp
    Dattorro/dsp/filters/OnePoleFilters.cpp
    Effects/Overdrive.cpp
)
target_include_directories(earth_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()
add_subdirectory(tests)
