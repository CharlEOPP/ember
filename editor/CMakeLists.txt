# The editor needs a real GLFW (not the headless stub) and the fetched ImGui
# sources. Skip the target otherwise (e.g. CI without GL), so the rest of the
# build still configures.
if(NOT TARGET glfw OR NOT IMGUI_SOURCE_DIR)
    message(STATUS "Ember editor: GLFW/ImGui not available -- skipping editor target")
    return()
endif()

# ---- Dear ImGui (docking) static lib: core + GLFW/OpenGL3 backends ----
# imgui_impl_opengl3 ships its own GL loader, so no loader define is needed.
add_library(imgui STATIC
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
    ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui SYSTEM PUBLIC
    ${IMGUI_SOURCE_DIR}
    ${IMGUI_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)
# Third-party: do NOT apply the engine's -Werror options.

# ---- Editor application ----
add_executable(ember_editor
    main.cpp
    EditorApplication.cpp
    ImGuiLayer.cpp
    SceneOps.cpp
    InspectorRegistry.cpp
    EditorInspectors.cpp
    CommandHistory.cpp
    Commands.cpp
    FileBrowser.cpp
    PlayModeController.cpp
    panels/ViewportPanel.cpp
    panels/SceneHierarchyPanel.cpp
    panels/InspectorPanel.cpp
)
target_include_directories(ember_editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ember_editor PRIVATE ember imgui)
ember_set_compiler_options(ember_editor)

# Copy runtime assets (shaders, fonts) next to the executable after each build.
add_custom_command(TARGET ember_editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:ember_editor>/assets)

if(WIN32)
    set_target_properties(ember_editor PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>)
endif()
