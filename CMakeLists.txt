# Copyright 2018 Leaning Technologies Ltd. All Rights Reserved.

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
# Rename as appropiate
PROJECT (CHEERP_LIBS_ADDONS)

SET(POSIXCOMPAT_SOURCES cheerpfs.cpp cheerpposix.cpp)

ADD_LIBRARY(posixcompat STATIC ${POSIXCOMPAT_SOURCES})

IF(NOT DEFINED CHEERP_MODE)
MESSAGE(FATAL_ERROR "-DCHEERP_MODE is not specified")
ENDIF()

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -cheerp-mode=${CHEERP_MODE}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -cheerp-mode=${CHEERP_MODE}")

INSTALL(TARGETS posixcompat emscompat DESTINATION lib/${CHEERP_MODE}/)
INSTALL(FILES cheerpfs.h emscripten.h DESTINATION include/)
INSTALL(FILES emscripten/html5.h DESTINATION include/emscripten/)
