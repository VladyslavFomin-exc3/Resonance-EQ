cmake_minimum_required(VERSION 3.22)

project(ResonanceEQ VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(ResonanceEQ
    COMPANY_NAME "YourName"
    BUNDLE_ID com.yourname.resonanceeq
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE

    PLUGIN_MANUFACTURER_CODE RsnE
    PLUGIN_CODE RsEQ

    FORMATS VST3 Standalone
    PRODUCT_NAME "Harmonic Resonance EQ"
    VST3_CAN_REPLACE_VST2 FALSE
)

juce_generate_juce_header(ResonanceEQ)

target_sources(ResonanceEQ
    PRIVATE
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
        Source/Dsp/Utilities.h
        Source/Dsp/Utilities.cpp
        Source/Dsp/EqCurve.h
        Source/Dsp/EqCurve.cpp
        Source/Dsp/ResonanceEngine.h
        Source/Dsp/ResonanceEngine.cpp
        Source/Dsp/SafetyLimiter.h
        Source/Dsp/SafetyLimiter.cpp
)

target_include_directories(ResonanceEQ
    PRIVATE
        Source
        Source/Dsp
)

target_compile_definitions(ResonanceEQ
    PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ResonanceEQ
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

