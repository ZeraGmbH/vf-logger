cmake_minimum_required(VERSION 3.1)

project(vf-logger LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(VfLogger_VERSION_MAJOR "0")
set(VfLogger_VERSION_MINOR "0")
set(VfLogger_VERSION_PATCH "1")
set(PROJECT_VERSION "${VfLogger_VERSION_MAJOR}.${VfLogger_VERSION_MINOR}.${VfLogger_VERSION_PATCH}")

include(FeatureSummary)
include(GNUInstallDirs)
include(GenerateExportHeader)

find_package(Qt5 REQUIRED COMPONENTS Core Qml Sql Quick Test CONFIG)
find_package(VeinFramework REQUIRED)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
