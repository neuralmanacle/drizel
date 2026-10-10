# Offline build entry point. Configure with `cmake -S . -B build`, then build
# with `cmake --build build`. The existing .jucer file remains the plugin build.
cmake_minimum_required(VERSION 3.22)
# JUCE's module helpers require both C and C++ compilers to be enabled even
# though the Drizel-owned implementation files in this target are C++.
project(DrizelOffline VERSION 0.1.0 LANGUAGES C CXX)

# CACHE PATH exposes a user setting: -DDRIZEL_JUCE_PATH=/your/JUCE/checkout.
# The sibling directory is a default, not a dependency downloaded by CMake.
set(DRIZEL_JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" CACHE PATH
    "JUCE source checkout (tested with 8.0.15)")
if(NOT EXISTS "${DRIZEL_JUCE_PATH}/extras/Build/CMake/JUCEModuleSupport.cmake")
    # Fail at configuration with an actionable message instead of a later
    # missing-header/linker error. The value should point to the JUCE root.
    message(FATAL_ERROR "Set DRIZEL_JUCE_PATH to a JUCE source checkout; see README.md")
endif()

# Import the framework's official module support without configuring Projucer
# or GUI build tools. The juce:: prefix is a CMake target namespace, not C++ code.
# Core supplies files/streams, audio_basics supplies sample/audio helpers, and
# audio_formats supplies WAV readers/writers. Module metadata supplies OS links.
include("${DRIZEL_JUCE_PATH}/extras/Build/CMake/JUCEModuleSupport.cmake")
juce_add_modules(ALIAS_NAMESPACE juce
    "${DRIZEL_JUCE_PATH}/modules/juce_core"
    "${DRIZEL_JUCE_PATH}/modules/juce_audio_basics"
    "${DRIZEL_JUCE_PATH}/modules/juce_audio_formats")

set(CMAKE_CXX_EXTENSIONS OFF) # Request standard C++ mode instead of GNU extensions.
# An INTERFACE library has no compiled object file. It packages include paths
# and the minimum language requirement for consumers of the header-only buffer.
add_library(drizel_core INTERFACE)
target_include_directories(drizel_core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(drizel_core INTERFACE cxx_std_17)

# A STATIC library compiles the adapter and required JUCE module sources into
# an archive shared by the experiment and tests. PUBLIC propagates drizel_core's
# usage requirements; PRIVATE keeps JUCE-specific includes out of our public API.
# CMake still handles link dependencies needed to consume the static archive.
add_library(drizel_wav STATIC src/io/WavFile.cpp)
target_link_libraries(drizel_wav PUBLIC drizel_core PRIVATE juce::juce_audio_formats)
# This target includes module headers directly, without generated JuceHeader.h
# or AppConfig.h. Disable optional network/browser and codec dependencies used
# by neither the experiment nor its tests. This is not a full codec allow-list;
# loadWav itself enforces the supported WAV encodings.
target_compile_definitions(drizel_wav PRIVATE
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0
    JUCE_USE_FLAC=0 JUCE_USE_OGGVORBIS=0 JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_USE_WINDOWS_MEDIA_FORMAT=0)

# Compiler flag spellings differ on MSVC and GCC/Clang. Strict warnings apply
# to our sources rather than third-party module sources; /WX and -Werror turn
# warnings into errors. These flags are local target/source settings, not global.
if(MSVC)
    set(DRIZEL_WARNINGS /W4 /WX)
else()
    set(DRIZEL_WARNINGS -Wall -Wextra -Wpedantic -Werror)
endif()
# Set the adapter's options per source, since drizel_wav also compiles JUCE code.
set_source_files_properties(src/io/WavFile.cpp PROPERTIES
    COMPILE_OPTIONS "${DRIZEL_WARNINGS}")

# The command-line executable has its own main(), links our adapter, and gains
# the public core include path and C++17 requirement through that dependency.
add_executable(drizel_wav_roundtrip experiments/wav-roundtrip/main.cpp)
target_link_libraries(drizel_wav_roundtrip PRIVATE drizel_wav)
target_compile_options(drizel_wav_roundtrip PRIVATE ${DRIZEL_WARNINGS})

# CTest enables test discovery and provides BUILD_TESTING (ON by default).
# Configure with -DBUILD_TESTING=OFF to build just the library and experiment.
include(CTest)
if(BUILD_TESTING)
    add_executable(drizel_audio_tests tests/AudioDataTests.cpp)
    target_link_libraries(drizel_audio_tests PRIVATE drizel_wav)
    target_compile_options(drizel_audio_tests PRIVATE ${DRIZEL_WARNINGS})
    # The audio_data entry runs the executable's 21 named internal test cases.
    add_test(NAME audio_data COMMAND drizel_audio_tests)
    # Invoking the CLI without paths should return nonzero. WILL_FAIL inverts
    # the expected exit-code result; it does not verify the exact error message.
    add_test(NAME roundtrip_usage COMMAND drizel_wav_roundtrip)
    set_tests_properties(roundtrip_usage PROPERTIES WILL_FAIL TRUE)
endif()
