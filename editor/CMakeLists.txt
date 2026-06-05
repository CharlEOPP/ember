# ---- Editor core (ImGui-free logic) ----
# Built ALWAYS so the test target can link it even where the GUI is skipped
# (e.g. headless CI without GLFW/ImGui).
add_library(ember_editor_core STATIC
    CommandHistory.cpp
    Commands.cpp
    SceneOps.cpp
    InspectorRegistry.cpp
    Picking.cpp
    EditorSettings.cpp
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
set(IMGUI_SOURCES
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
    ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# ImGuizmo (Epic 08 gizmos) — compile ONLY ImGuizmo.cpp ourselves so it picks up
# imgui's include dir (its own CMakeLists is skipped via SOURCE_SUBDIR). Master
# keeps it at src/ImGuizmo.cpp; older layouts at the root. Degrade gracefully if
# the fetch didn't populate (EMBER_HAS_IMGUIZMO).
set(EMBER_HAS_IMGUIZMO OFF)
set(IMGUIZMO_DIR "")
foreach(cand "${IMGUIZMO_SOURCE_DIR}/src/ImGuizmo.cpp" "${IMGUIZMO_SOURCE_DIR}/ImGuizmo.cpp")
    if(cand AND EXISTS "${cand}")
        get_filename_component(IMGUIZMO_DIR "${cand}" DIRECTORY)
        list(APPEND IMGUI_SOURCES "${cand}")
        set(EMBER_HAS_IMGUIZMO ON)
        break()
    endif()
endforeach()
if(NOT EMBER_HAS_IMGUIZMO)
    message(WARNING "Ember editor: ImGuizmo.cpp not found under '${IMGUIZMO_SOURCE_DIR}' "
                    "-- building without transform gizmos. Delete build/<cfg>/_deps/imguizmo-* "
                    "and reconfigure to fetch it.")
endif()

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui SYSTEM PUBLIC
    ${IMGUI_SOURCE_DIR}
    ${IMGUI_SOURCE_DIR}/backends
)
if(EMBER_HAS_IMGUIZMO)
    target_include_directories(imgui SYSTEM PUBLIC ${IMGUIZMO_DIR})
    target_compile_definitions(imgui PUBLIC EMBER_HAS_IMGUIZMO)
endif()
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
    panels/ProfilerPanel.cpp
    panels/TilemapEditorPanel.cpp
    panels/EditorSettingsPanel.cpp
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
