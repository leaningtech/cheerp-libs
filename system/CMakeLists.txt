CMAKE_MINIMUM_REQUIRED (VERSION 3.10)
PROJECT (system)

if (CMAKE_CXX_COMPILER_TARGET STREQUAL "cheerp-wasm")
set(LIB_TARGET "asmjs" CACHE STRING "Target used to build the library")
elseif (CMAKE_CXX_COMPILER_TARGET STREQUAL "cheerp")
set(LIB_TARGET "genericjs" CACHE STRING "Target used to build the library")
else ()
message(FATAL_ERROR "Invalid target for system cheerp: ${CMAKE_CXX_COMPILER_TARGET}")
endif ()

SET(CMAKE_CXX_STANDARD 17)

if (LIB_TARGET STREQUAL "asmjs")
	ADD_LIBRARY(wasi STATIC
		common.cpp
		basic.cpp
		wasi.cpp
		wasi_main.cpp
		wasi_shim.cpp
	)
	ADD_LIBRARY(cheerpos STATIC
		basic.cpp
		cheerpos.cpp
		wasi_main.cpp
		wasi_shim.cpp
	)
	ADD_LIBRARY(threads STATIC
		threads.cpp
	)
endif ()

ADD_LIBRARY(system STATIC
	common.cpp
	basic.cpp
	browser.cpp
	divti3.c
)



INSTALL(TARGETS system DESTINATION "lib/${LIB_TARGET}")

if (LIB_TARGET STREQUAL "asmjs")
	INSTALL(TARGETS cheerpos DESTINATION "lib/${LIB_TARGET}")
	INSTALL(TARGETS wasi DESTINATION "lib/${LIB_TARGET}")
	INSTALL(TARGETS threads DESTINATION "lib/${LIB_TARGET}")
endif()
