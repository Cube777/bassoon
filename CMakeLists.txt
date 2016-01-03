cmake_minimum_required(VERSION 2.8)
project(bassoon)

#Set build mode to release if no build-type was specified
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
endif()

#Debug postfix
set(CMAKE_DEBUG_POSTFIX "d")

#Add C++11 support
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#Add include paths
include_directories(${CMAKE_SOURCE_DIR}/include)

#Add library modules
add_executable(bassoon "src/bassoon.cpp")

#Installation
set(INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")

install(TARGETS bassoon DESTINATION ${INSTALL_DIR})
