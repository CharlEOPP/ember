add_library(ember_platform STATIC
    src/Window.cpp
    src/Timer.cpp
    src/FileSystem.cpp
)

target_include_directories(ember_platform
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---- GLFW: fetched > system > vendored stub ----
if(TARGET glfw)
    message(STATUS "Ember: GLFW via FetchContent")
    target_link_libraries(ember_platform PUBLIC ember_core glfw)
else()
    find_package(glfw3 QUIET)
    if(glfw3_FOUND)
        message(STATUS "Ember: GLFW via find_package")
        target_link_libraries(ember_platform PUBLIC ember_core glfw)
    else()
        message(STATUS "Ember: GLFW not found -- using stub (offline/CI build)")
        add_library(glfw_stub INTERFACE)
        target_include_directories(glfw_stub SYSTEM INTERFACE
            ${CMAKE_SOURCE_DIR}/third_party/glfw_stub
        )
        target_link_libraries(ember_platform PUBLIC ember_core glfw_stub)
    endif()
endif()

ember_set_compiler_options(ember_platform)
