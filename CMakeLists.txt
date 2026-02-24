cmake_minimum_required(VERSION 3.22)
project(ExciteLifelineModule VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
if(APPLE)
  # Build universal binaries so Apple Silicon (arm64) and Intel (x86_64) hosts
  # can both see and load the plugin.
  set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
endif()
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
include(cmake/cpm.cmake)

CPMAddPackage(
  NAME JUCE
  GIT_TAG 8.0.6
  VERSION 8.0.6
  GITHUB_REPOSITORY juce-framework/JUCE
  SOURCE_DIR ${LIB_DIR}/juce
)

if(MSVC)
  set(CXX_PROJECT_WARNINGS "/W4;/WX;/wd4820;/wd4514")
else()
  set(CXX_PROJECT_WARNINGS "-Wall;-Werror;-Wextra;-Wpedantic")
endif()

add_subdirectory(plugin)
