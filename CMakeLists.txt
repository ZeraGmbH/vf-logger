cmake_minimum_required(VERSION 3.1)

project(vf-logger LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "0")
set(PROJECT_VERSION_PATCH "1")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

include(FeatureSummary)
include(GNUInstallDirs)
include(GenerateExportHeader)

find_package(Qt5 REQUIRED COMPONENTS Core Sql Concurrent Test CONFIG)
find_package(VeinFramework REQUIRED)
find_package(zera-json-params REQUIRED)

enable_testing()

add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(testlib)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
