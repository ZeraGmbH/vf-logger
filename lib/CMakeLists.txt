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

# As long as vf-script is in we need QML/Quick and they must be private
# to not get GUI in... (in vf-script they are private either)
target_link_libraries(VfLogger
    PUBLIC
    VeinMeta::VfScript
    PRIVATE
    zera-json-params
    Zera::VfEvent
    Zera::VfComponent
    Zera::VfStorageHash
    Zera::VfNet2
    Zera::VfTcp
    Zera::VfQml
    Zera::VfCpp
    Zera::ModmanBase
    Qt5::Core
    Qt5::Sql
    Qt5::Concurrent
    Qt5::Qml
    Qt5::Quick
    )

set_target_properties(VfLogger PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(VfLogger PROPERTIES SOVERSION ${VfLogger_VERSION_MAJOR})

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
