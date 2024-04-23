file(GLOB SOURCES 
    *.cpp
    )

file(GLOB HEADERS
    *.h
    )

# Exclude psql files
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/vl_postgresdatabase.cpp")
list(REMOVE_ITEM HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/vl_postgresdatabase.h")

file(GLOB RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.qrc
    )

add_library(VfLogger SHARED
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
    )

target_link_libraries(VfLogger
    PUBLIC
    Zera::VfEvent
    Zera::VfComponent
    Zera::VfStorageHash
    Zera::VfCpp
    Zera::ModmanBase
    Zera::VfConvenientCode
    PRIVATE
    zera-json-params
    Qt5::Sql
    Qt5::Concurrent
    )

set_target_properties(VfLogger PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(VfLogger PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

generate_export_header(VfLogger)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/vflogger_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/VfLogger
    )

target_include_directories(VfLogger
    PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/VfLogger>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

install(TARGETS VfLogger
    EXPORT VfLoggerExport
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    VfLoggerConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY SameMajorVersion
    )

configure_file(VfLoggerConfig.cmake.in VfLoggerConfig.cmake @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/VfLoggerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/VfLoggerConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VfLogger
    )

install(EXPORT VfLoggerExport
    FILE VfLoggerTargets.cmake
    NAMESPACE VeinMeta::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VfLogger
    )

install(
    FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/VfLogger
    )
