add_executable(ember_sandbox main.cpp)

target_link_libraries(ember_sandbox PRIVATE ember)

ember_set_compiler_options(ember_sandbox)

# On Windows, use the Windows subsystem for release (no console window)
if(WIN32)
    set_target_properties(ember_sandbox PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()
