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
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/vf-logger-testlib>
    )

target_link_libraries(vf-logger-testlib
    PUBLIC
    ${QTX}::Test
    VfLogger
    Zera::zera-timers-testlib
    Zera::VfFrameworkTest
)

set_target_properties(vf-logger-testlib PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(vf-logger-testlib PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

export(TARGETS vf-logger-testlib NAMESPACE VeinMeta:: FILE VfLoggerTargets.cmake)
install(TARGETS vf-logger-testlib
    EXPORT VfLoggerExport
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

install(
    FILES ${HEADER}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vf-logger-testlib
    )
