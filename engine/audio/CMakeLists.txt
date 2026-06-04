add_library(ember_audio STATIC
    src/AudioEngine.cpp
    src/Audio.cpp
)

target_include_directories(ember_audio
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# core for types/log; assets for AudioClip / AssetManager / AssetHandle.
target_link_libraries(ember_audio
    PUBLIC ember_core ember_assets
)

target_compile_features(ember_audio PUBLIC cxx_std_20)
ember_set_compiler_options(ember_audio)

# Output device uses miniaudio (single header). Only compiled when audio is on;
# warnings are disabled for that TU (third-party C header) like stb.
if(EMBER_ENABLE_AUDIO)
    target_compile_definitions(ember_audio PRIVATE EMBER_ENABLE_AUDIO)
    target_include_directories(ember_audio PRIVATE ${MINIAUDIO_INCLUDE_DIR})
    if(MSVC)
        set_source_files_properties(src/AudioEngine.cpp PROPERTIES COMPILE_OPTIONS "/w")
    else()
        set_source_files_properties(src/AudioEngine.cpp PROPERTIES COMPILE_OPTIONS "-w")
    endif()
endif()
