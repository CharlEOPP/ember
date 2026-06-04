# The audio loader (miniaudio) is deferred — see plan Phase 4 note.
add_library(ember_assets STATIC
    src/AssetManager.cpp
    src/AssetDatabase.cpp
    src/FileWatcher.cpp
    src/HotReload.cpp
    loaders/Texture2DLoader.cpp
    loaders/FontAsset.cpp
    loaders/FontLoader.cpp
    loaders/ShaderLoader.cpp
    loaders/DefaultLoaders.cpp
    loaders/stb_impl.cpp
)

target_include_directories(ember_assets
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${STB_INCLUDE_DIR}            # stb_image.h / stb_truetype.h (never leaks)
)

# renderer_api (ITexture2D/IShader/RHI) is PUBLIC because Texture2D.h/Shader.h/
# FontAsset.h expose those types. yaml-cpp stays PRIVATE (NFR-04).
find_package(Threads REQUIRED)
target_link_libraries(ember_assets
    PUBLIC  ember_core ember_platform ember_events ember_renderer_api glm::glm Threads::Threads
    PRIVATE yaml-cpp::yaml-cpp
)

# Hot reload is a dev-only feature (Phase 6) gated behind this flag.
target_compile_definitions(ember_assets PRIVATE $<$<CONFIG:Debug>:EMBER_HOT_RELOAD>)

target_compile_features(ember_assets PUBLIC cxx_std_20)
ember_set_compiler_options(ember_assets)

# stb is third-party: compile its single implementation TU with warnings off so
# the engine's -Werror doesn't choke on stb's known GCC false-positives.
if(MSVC)
    set_source_files_properties(loaders/stb_impl.cpp PROPERTIES COMPILE_OPTIONS "/w")
else()
    set_source_files_properties(loaders/stb_impl.cpp PROPERTIES COMPILE_OPTIONS "-w")
endif()
