add_library(ember_core STATIC
    src/Log.cpp
    src/Assert.cpp
    src/UUID.cpp
    src/Arena.cpp
)

target_include_directories(ember_core
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(ember_core PUBLIC cxx_std_20)
ember_set_compiler_options(ember_core)

# ---- spdlog: fetched > system package > vendored stub ----
if(TARGET spdlog::spdlog)
    message(STATUS "Ember: spdlog via FetchContent")
    target_link_libraries(ember_core PUBLIC spdlog::spdlog)
else()
    find_package(spdlog QUIET)
    if(spdlog_FOUND)
        message(STATUS "Ember: spdlog via find_package")
        target_link_libraries(ember_core PUBLIC spdlog::spdlog)
    else()
        message(STATUS "Ember: spdlog not found -- using bundled stub (offline/CI build)")
        add_library(spdlog_stub INTERFACE)
        target_include_directories(spdlog_stub SYSTEM INTERFACE
            ${CMAKE_SOURCE_DIR}/third_party/spdlog_stub
        )
        target_link_libraries(ember_core PUBLIC spdlog_stub)
    endif()
endif()

# ---- std::expected: C++23 -- GCC 12+, Clang 16+, MSVC 19.36+ ----
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_FLAGS "-std=c++20")
check_cxx_source_compiles(
    "#include <expected>\nint main() { std::expected<int,int> e = 1; return 0; }"
    EMBER_HAS_STD_EXPECTED
)

if(NOT EMBER_HAS_STD_EXPECTED)
    message(STATUS "Ember: std::expected unavailable -- using vendored tl::expected")
    add_library(tl_expected INTERFACE)
    target_include_directories(tl_expected INTERFACE
        ${CMAKE_SOURCE_DIR}/third_party/tl
    )
    target_link_libraries(ember_core PUBLIC tl_expected)
    target_compile_definitions(ember_core PUBLIC EMBER_USE_TL_EXPECTED)
else()
    message(STATUS "Ember: using std::expected")
endif()
