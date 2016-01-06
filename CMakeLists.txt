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

#Add library modules
add_executable(bassoon "${CMAKE_SOURCE_DIR}/bassoon.cpp")

#Import libraries
add_library(dchain SHARED IMPORTED)
add_library(nihdb SHARED IMPORTED)

find_library(dchainloc dchain)
find_library(nihdbloc nihdb)

set_target_properties(dchain PROPERTIES IMPORTED_LOCATION ${dchainloc})
set_target_properties(nihdb PROPERTIES IMPORTED_LOCATION ${nihdbloc})

target_link_libraries(bassoon dchain nihdb ncurses)

#Installation
set(INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")

install(TARGETS bassoon DESTINATION ${INSTALL_DIR})
