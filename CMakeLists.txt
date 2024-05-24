# Copyright 2018 Leaning Technologies Ltd. All Rights Reserved.

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
# Rename as appropiate
PROJECT (CHEERP_MEMPROF)

SET(MEMPROF_SOURCES memprof.cpp memprofui.cpp)

ADD_LIBRARY(memprof STATIC ${MEMPROF_SOURCES})

INSTALL(TARGETS memprof DESTINATION lib/${CHEERP_MODE}/)
INSTALL(FILES memprof.h DESTINATION include/client/cheerp/)
