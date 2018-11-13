# Copyright 2018 Leaning Technologies Ltd. All Rights Reserved.

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
# Rename as appropiate
PROJECT (CHEERP_LIBS_ADDONS)

SET(POSIXCOMPAT_SOURCES cheerpfs.cpp cheerpposix.cpp)

ADD_LIBRARY(posixcompat STATIC ${POSIXCOMPAT_SOURCES})

TARGET_COMPILE_OPTIONS(posixcompat PUBLIC -cheerp-mode=${CHEERP_MODE})

SET(EMSCOMPAT_SOURCES emscompat.cpp)

ADD_LIBRARY(emscompat STATIC ${EMSCOMPAT_SOURCES})

TARGET_COMPILE_OPTIONS(emscompat PUBLIC -cheerp-mode=${CHEERP_MODE})

INSTALL(TARGETS posixcompat emscompat DESTINATION lib/${CHEERP_MODE}/)
INSTALL(FILES cheerpfs.h emscripten.h DESTINATION include/)
INSTALL(FILES emscripten/html5.h DESTINATION include/emscripten/)

IF(NOT DEFINED CHEERP_MODE)
MESSAGE(FATAL_ERROR "-DCHEERP_MODE is not specified")
ELSEIF(${CHEERP_MODE} STREQUAL "asmjs")

SET(MEMPROF_SOURCES memprof.cpp)

ADD_LIBRARY(memprof STATIC ${MEMPROF_SOURCES})

TARGET_COMPILE_OPTIONS(memprof PUBLIC -cheerp-mode=genericjs)

INSTALL(TARGETS memprof DESTINATION lib/${CHEERP_MODE}/)
INSTALL(FILES memprof.h DESTINATION include/client/cheerp/)

SET(MEMPROFUI_SOURCES memprofui.cpp)

ADD_EXECUTABLE(memprofui ${MEMPROFUI_SOURCES})

TARGET_COMPILE_OPTIONS(memprofui PUBLIC -cheerp-mode=genericjs)

ENDIF()
