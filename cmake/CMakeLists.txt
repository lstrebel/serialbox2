##===------------------------------------------------------------------------------*- CMake -*-===##
##
##                                   S E R I A L B O X
##
## This file is distributed under terms of BSD license. 
## See LICENSE.txt for more information.
##
##===------------------------------------------------------------------------------------------===##

cmake_minimum_required(VERSION 3.1)

## Install FindSerialbox.cmake
install(FILES ${CMAKE_SOURCE_DIR}/cmake/FindSerialbox.cmake DESTINATION cmake/)
install(FILES ${CMAKE_SOURCE_DIR}/cmake/SerialboxTooling.cmake DESTINATION cmake/)

