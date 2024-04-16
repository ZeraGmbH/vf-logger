file(GLOB SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    )

file(GLOB HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    )

add_library(vf-logger-testlib STATIC
    ${SOURCES}
    ${HEADER}
)

target_include_directories(vf-logger-testlib
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

target_link_libraries(vf-logger-testlib
    PUBLIC
    VfLogger
    Zera::zera-timers-testlib
)

set_target_properties(vf-logger-testlib PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(vf-logger-testlib PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

