cmake_minimum_required(VERSION 3.1)

#set project name and language
project(vf-logger LANGUAGES CXX)

#Automoc and RCC ON. Needed in QT projects
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#C++ 11 Flag
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Set project Version
set(VfLogger_VERSION_MAJOR "0")
set(VfLogger_VERSION_MINOR "0")
set(VfLogger_VERSION_PATCH "1")

set(PROJECT_VERSION "${VfLogger_VERSION_MAJOR}.${VfLogger_VERSION_MINOR}.${VfLogger_VERSION_PATCH}")

#Adding cmake functions like feature_summary()
include(FeatureSummary)
include(GNUInstallDirs)

#Find dependecies
find_package(Qt5 REQUIRED COMPONENTS Core Qml Sql Quick CONFIG  )
find_package(VfHelpers REQUIRED)
find_package(VfEvent REQUIRED)
find_package(VfComponent REQUIRED)
find_package(VfQml REQUIRED)
find_package(VfStorageHash REQUIRED)
find_package(VfCpp REQUIRED)

#sum up project Files 
file(GLOB SOURCES 
    ${PROJECT_SOURCE_DIR}/*.cpp
    )

set(PUBLIC_HEADER
    globalIncludes.h
    jsoncontextloader.h
    vl_abstractloggerdb.h
    vl_databaselogger.h
    vl_datasource.h
    vl_sqlitedb.h
    )

set(PRIVATE_HEADER
    vl_databaseloggerprivate.h
    vl_globallabels.h
    )

file(GLOB RESOURCES 
    ${PROJECT_SOURCE_DIR}/*.qrc
    )


set(zeraContentSetFile "${PROJECT_SOURCE_DIR}/configs/ZeraContext.json")
set(customerContentSetFile "${PROJECT_SOURCE_DIR}/configs/CustomerContext.json")


#Exclude psql files
list(REMOVE_ITEM SOURCES "${PROJECT_SOURCE_DIR}/vl_postgresdatabase.cpp")


#create library 
add_library(VfLogger SHARED
    ${SOURCES}
    ${PUBLIC_HEADER}
    ${PRIVATE_HEADER}
    ${RESOURCES}
    )



#link dependencies
target_link_libraries(VfLogger 
    PRIVATE
    Qt5::Core
    Qt5::Qml
    Qt5::Sql
    Qt5::Quick
    VeinMeta::VfHelpers
    VeinMeta::VfEvent
    VeinMeta::VfComponent
    VeinMeta::VfQml
    VeinMeta::VfStorageHash
    VeinMeta::VfCpp
    )

#set target Version
set_target_properties(VfLogger PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(VfLogger PROPERTIES SOVERSION ${VfLogger_VERSION_MAJOR})


#generate export header
include(GenerateExportHeader)
generate_export_header(VfLogger)

#install export header
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/vflogger_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/VfLogger
    )

# announce headers - target perspective
target_include_directories(VfLogger
    PUBLIC
    $<INSTALL_INTERFACE:include/VfLogger>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

# install library
install(TARGETS VfLogger
    EXPORT VfLoggerExport
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

# configure *ConfigVersion.cmake
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    VfLoggerConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY SameMinorVersion
    )

# configure *Config.cmake
configure_file(VfLoggerConfig.cmake.in VfLoggerConfig.cmake @ONLY)



# install *Config(Version).cmake
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/VfLoggerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/VfLoggerConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VfLogger
    )

# install targets cmake-files
install(EXPORT VfLoggerExport
    FILE VfLoggerTargets.cmake
    NAMESPACE VeinMeta::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VfLogger
    )

# install public headers
install(
    FILES ${PUBLIC_HEADER}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/VfLogger
    )
    
# install ContentSet files
install(
    FILES ${zeraContentSetFile}
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/zera/configs/
    )

install(
    FILES ${customerContentSetFile}
    DESTINATION /home/operator/logger-contentsets/
    )

# spawn out some info on configuration
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)


