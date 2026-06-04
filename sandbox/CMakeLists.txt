add_executable(ember_sandbox main.cpp)

target_link_libraries(ember_sandbox PRIVATE ember)

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
