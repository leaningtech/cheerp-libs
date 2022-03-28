CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
PROJECT (system)

FILE(GLOB SOURCES
	*.cpp
	*.c
)

ADD_LIBRARY(system STATIC ${SOURCES})


if (CMAKE_CXX_COMPILER_TARGET STREQUAL "cheerp-wasm")
set(LIB_TARGET "asmjs" CACHE STRING "Target used to build the library")
elseif (CMAKE_CXX_COMPILER_TARGET STREQUAL "cheerp")
set(LIB_TARGET "genericjs" CACHE STRING "Target used to build the library")
else ()
message(FATAL_ERROR "Invalid target for system cheerp: ${CMAKE_CXX_COMPILER_TARGET}")
endif ()

INSTALL(TARGETS system DESTINATION "lib/${LIB_TARGET}")
