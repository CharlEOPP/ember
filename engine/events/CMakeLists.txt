add_library(ember_events STATIC
    src/EventBus.cpp
)

target_include_directories(ember_events
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ember_events
    PUBLIC ember_core EnTT::EnTT
)

target_compile_features(ember_events PUBLIC cxx_std_20)
ember_set_compiler_options(ember_events)
