#####
# A CMake build file for the ImageAlter service, a service that can
# manipulate images.
# 
# Copyright 2009, Yahoo!
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
# 
#  3. Neither the name of Yahoo! nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# 
# Original Author: Lloyd Hilaiel
#####

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.1)

SET(SERVICE_NAME ImageAlter)
PROJECT(${SERVICE_NAME}Service)

INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/../external/bp-build-templates/cmake/BuildConfigs.cmake)

INCLUDE_DIRECTORIES(
  "${CMAKE_CURRENT_SOURCE_DIR}/../external/dist/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../external/dist/include/GraphicsMagick"
  "${CMAKE_CURRENT_SOURCE_DIR}/../external/bp-service-tools/CppTools/external/bpsdk/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../external/bp-service-tools/CppTools/src"
)

IF ("${CMAKE_BUILD_TYPE}" STREQUAL "CodeCoverage")
  LINK_DIRECTORIES(
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/dist/lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/dist/lib/Debug"
  )
ELSE ()
  LINK_DIRECTORIES(
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/dist/lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/dist/lib/${CMAKE_BUILD_TYPE}"
  )
ENDIF ()

SET(EXT_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/../external/bp-service-tools/CppTools/src/bptypeutil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../external/bp-service-tools/CppTools/src/bpurlutil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../external/bp-service-tools/CppTools/src/bpservicedescription.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../external/bp-service-tools/CppTools/src/bpserviceversion.cpp
)

# add required OS libs here
SET(OS_LIBS)
IF (WIN32)
   FILE(GLOB OS_SRCS "util/*_Windows.cpp")
   # these are separate libraries on winbloze
   SET(OS_LIBS GraphicsMagickCoders_s GraphicsMagickFilters_s)
ELSE ()
   FILE(GLOB OS_SRCS "util/*_Darwin.cpp" "util/*_UNIX.cpp")
ENDIF ()
SET(SRCS service.cpp Transformations.hh ImageProcessor.cpp util/fileutil.cpp ${EXT_SRCS} ${OS_SRCS})
SET(HDRS Transformations.cpp ImageProcessor.hh util/bpsync.hh util/bpthread.hh)
SET(LIBS GraphicsMagick_s png_s jpeg_s zlib_s ${OS_LIBS})

BPAddCppService()

# copy in magic.mgk - an xml lookup table that allows graphics magick
# to quickly identify files based on content, and is required for GM
# to work
SET(PATH_TO_MAGIC_MGK "${CMAKE_CURRENT_SOURCE_DIR}/../external/dist/share/GraphicsMagick-1.3.7/config/magic.mgk")
ADD_CUSTOM_COMMAND(
  TARGET ${SERVICE_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different \"${PATH_TO_MAGIC_MGK}\" \"${OUTPUT_DIR}\")

