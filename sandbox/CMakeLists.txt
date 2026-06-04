add_executable(ember_sandbox
    main.cpp
    scripts/Scripts.cpp        # registers sandbox scripts (pulls yaml via RegisterScript.h)
)

target_include_directories(ember_sandbox PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ember provides the engine; yaml-cpp is needed by scripts/Scripts.cpp because
# script registration instantiates the serializer (yaml is PRIVATE in the
# serialization lib, so it isn't propagated through the aggregate).
target_link_libraries(ember_sandbox PRIVATE ember yaml-cpp::yaml-cpp)

ember_set_compiler_options(ember_sandbox)

# Copy runtime assets (shaders, etc.) next to the executable after each build.
add_custom_command(TARGET ember_sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:ember_sandbox>/assets)

# On Windows, use the Windows subsystem for release (no console window)
if(WIN32)
    set_target_properties(ember_sandbox PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()

# Epic 12 — integrated 2D gameplay slice (physics + tilemap + particles + UI + audio).
add_executable(ember_sandbox_game2d game2d.cpp)
target_include_directories(ember_sandbox_game2d PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ember_sandbox_game2d PRIVATE ember)
ember_set_compiler_options(ember_sandbox_game2d)
add_custom_command(TARGET ember_sandbox_game2d POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:ember_sandbox_game2d>/assets)
if(WIN32)
    set_target_properties(ember_sandbox_game2d PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()
