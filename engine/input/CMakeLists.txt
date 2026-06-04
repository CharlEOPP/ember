add_library(ember_input STATIC
    src/InputManager.cpp
)

target_include_directories(ember_input
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# glfw and yaml-cpp are PRIVATE: GLFW never leaks past this module (epic.md
# "Notes & Decisions"), and yaml-cpp stays out of consumers' include paths.
target_link_libraries(ember_input
    PUBLIC  ember_core ember_events glm::glm
    PRIVATE glfw yaml-cpp::yaml-cpp ember_platform
)

target_compile_features(ember_input PUBLIC cxx_std_20)
ember_set_compiler_options(ember_input)
