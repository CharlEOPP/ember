# Only build tests if Catch2 was fetched (EMBER_FETCH_DEPS=ON)
if(NOT TARGET Catch2::Catch2WithMain)
    message(STATUS "Ember tests: Catch2 not available -- skipping test target")
    return()
endif()

include(CTest)
include(Catch2/extras/Catch.cmake)

add_executable(ember_tests
    core/TestUUID.cpp
    core/TestStringHash.cpp
    core/TestArena.cpp
    core/TestPool.cpp
    core/TestLog.cpp
    platform/TestTimer.cpp
    platform/TestFileSystem.cpp
)

target_link_libraries(ember_tests
    PRIVATE
        ember
        Catch2::Catch2WithMain
)

ember_set_compiler_options(ember_tests)
catch_discover_tests(ember_tests)
