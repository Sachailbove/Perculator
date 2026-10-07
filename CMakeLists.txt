cmake_minimum_required(VERSION 3.22)
project(PERCULATOR VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.12)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(PERCULATOR
    COMPANY_NAME "Bovina Audio"
    BUNDLE_ID "audio.bovina.perculator"
    PLUGIN_MANUFACTURER_CODE Bovn
    PLUGIN_CODE Prcl
    FORMATS VST3 Standalone
    PRODUCT_NAME "PERCULATOR"
    VST3_CATEGORIES Fx Distortion
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(PERCULATOR)

target_sources(PERCULATOR PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_features(PERCULATOR PRIVATE cxx_std_17)
target_compile_definitions(PERCULATOR PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(PERCULATOR PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
