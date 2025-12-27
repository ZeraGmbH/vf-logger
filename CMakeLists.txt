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

set(QT_MAJOR_VER "5" CACHE STRING "Qt major version")
set(QTX "Qt${QT_MAJOR_VER}")

find_package(${QTX} REQUIRED COMPONENTS Core Sql Gui Test CONFIG)
find_package(VeinFramework REQUIRED)
find_package(zera-json-helpers REQUIRED)
find_package(VfQmlLibs REQUIRED)

enable_testing()

add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(testlib)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
