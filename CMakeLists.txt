cmake_minimum_required(VERSION 3.22)
project(VocalChain VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(VocalChain
    COMPANY_NAME "VocalChain"
    PLUGIN_MANUFACTURER_CODE Vchn
    PLUGIN_CODE Vc01
    FORMATS VST3 Standalone
    PRODUCT_NAME "VocalChain"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(VocalChain PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_features(VocalChain PRIVATE cxx_std_17)

target_compile_definitions(VocalChain PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VocalChain
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
