cmake_minimum_required(VERSION 3.22)
project(TapeStopPro VERSION 1.0.0)

if(NOT DEFINED JUCE_PATH)
    set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
endif()

add_subdirectory(${JUCE_PATH} JUCE)

juce_add_plugin(TapeStopPro
    PLUGIN_MANUFACTURER_CODE  Ttsp
    PLUGIN_CODE               Tstp
    FORMATS                   VST3
    PRODUCT_NAME              "Tape Stop Pro"
    COMPANY_NAME              "YourName"
    BUNDLE_ID                 com.yourname.tapestoppro
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
)

juce_generate_juce_header(TapeStopPro)

target_sources(TapeStopPro PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h
)

target_compile_definitions(TapeStopPro PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(TapeStopPro PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_plugin_client
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
