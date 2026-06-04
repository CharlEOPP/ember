# ---- Editor core (ImGui-free logic) ----
# Built ALWAYS so the test target can link it even where the GUI is skipped
# (e.g. headless CI without GLFW/ImGui).
add_library(ember_editor_core STATIC
    CommandHistory.cpp
    Commands.cpp
    SceneOps.cpp
    InspectorRegistry.cpp
    Picking.cpp
)
target_include_directories(ember_editor_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ember_editor_core PUBLIC ember)
ember_set_compiler_options(ember_editor_core)

# The editor application needs a real GLFW (not the headless stub) and the fetched
# ImGui sources. Skip just the executable otherwise; ember_editor_core still built.
if(NOT TARGET glfw OR NOT IMGUI_SOURCE_DIR)
    message(STATUS "Ember editor: GLFW/ImGui not available -- skipping editor executable")
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

# ---- Editor application (ImGui TUs) ----
add_executable(ember_editor
    main.cpp
    EditorApplication.cpp
    ImGuiLayer.cpp
    EditorInspectors.cpp
    FileBrowser.cpp
    PlayModeController.cpp
    panels/ViewportPanel.cpp
    panels/SceneHierarchyPanel.cpp
    panels/InspectorPanel.cpp
    panels/AssetBrowserPanel.cpp
)
target_include_directories(ember_editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ember_editor PRIVATE ember_editor_core imgui)
ember_set_compiler_options(ember_editor)

# Copy runtime assets (shaders, fonts) next to the executable after each build.
add_custom_command(TARGET ember_editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:ember_editor>/assets)

if(WIN32)
    set_target_properties(ember_editor PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>)
endif()
