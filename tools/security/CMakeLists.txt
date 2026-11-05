# Adversarial-input regression harness (not part of the plugin build). See lab.md.
# Feeds NaN/Inf presets, junk host state and crafted WAV headers to the processor
# under AddressSanitizer + UndefinedBehaviorSanitizer. Exits non-zero on any finding.
#   cmake -S tools/security -B build-security && cmake --build build-security
#   build-security/GrainBrainSecurity_artefacts/Debug/GrainBrainSecurity
# Reuse an existing JUCE checkout with -DJUCE_DIR=/path/to/JUCE to skip the download.
cmake_minimum_required(VERSION 3.22)
project(GrainBrainSecurity VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
if(NOT CMAKE_BUILD_TYPE)
 set(CMAKE_BUILD_TYPE Debug)
endif()
if(JUCE_DIR)
 add_subdirectory(${JUCE_DIR} juce)
else()
 include(FetchContent)
 FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
 FetchContent_MakeAvailable(JUCE)
endif()
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../Source)
juce_add_console_app(GrainBrainSecurity PRODUCT_NAME GrainBrainSecurity)
juce_generate_juce_header(GrainBrainSecurity)
target_sources(GrainBrainSecurity PRIVATE main.cpp ${SRC}/PluginProcessor.cpp ${SRC}/PluginEditor.cpp)
target_include_directories(GrainBrainSecurity PRIVATE ${SRC})
target_compile_definitions(GrainBrainSecurity PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_JACK=0 JucePlugin_VersionString="${PROJECT_VERSION}")
if(NOT MSVC)
 target_compile_options(GrainBrainSecurity PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer -g -O1)
 target_link_options(GrainBrainSecurity PRIVATE -fsanitize=address,undefined)
endif()
target_link_libraries(GrainBrainSecurity PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_gui_extra)
