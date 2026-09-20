# Golden gate for the shared core. Builds and runs with any C++20 compiler and
# the committed golden references; no JUCE or Emscripten required.

add_executable(golden_test golden_test.cpp)
target_link_libraries(golden_test PRIVATE earth_core)

add_test(NAME golden
         COMMAND golden_test ${CMAKE_CURRENT_SOURCE_DIR}/golden)

# Optional: regenerate the golden from the production Apollo Dattorro reference.
set(_apollo_dsp "${CMAKE_CURRENT_SOURCE_DIR}/../../Apollo/Source/DSP")
if(EXISTS "${_apollo_dsp}/Dattorro/Dattorro.cpp")
    add_executable(make_golden
        make_golden.cpp
        "${_apollo_dsp}/Dattorro/Dattorro.cpp"
        "${_apollo_dsp}/Dattorro/dsp/delays/InterpDelay.cpp"
        "${_apollo_dsp}/Dattorro/dsp/filters/OnePoleFilters.cpp"
        "${_apollo_dsp}/DaisySP/overdrive.cpp"
    )
    target_include_directories(make_golden PRIVATE
        "${_apollo_dsp}"
        "${_apollo_dsp}/Dattorro"
        "${_apollo_dsp}/Util"
        "${_apollo_dsp}/DaisySP"
        "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()
