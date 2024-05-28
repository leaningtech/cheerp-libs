# Copyright 2018 Leaning Technologies Ltd. All Rights Reserved.

cmake_minimum_required (VERSION 3.20)
# Rename as appropiate
project (CHEERP_MEMPROF)

add_library(memprof STATIC memprof.cpp memprofui.cpp)
target_include_directories(memprof PUBLIC include)
set_target_properties(memprof PROPERTIES PUBLIC_HEADER "include/cheerp/memprof.h")

install(TARGETS memprof
	ARCHIVE DESTINATION lib/asmjs
	PUBLIC_HEADER DESTINATION include/client/cheerp
)

macro(def_test pathVar)
	set(path "${pathVar}")
	cmake_path(GET path STEM testName)
	add_executable(${testName} EXCLUDE_FROM_ALL tests/${path})
	target_link_libraries(${testName} PUBLIC memprof)
	target_link_options(${testName} PRIVATE -cheerp-make-module=es6 -cheerp-pretty-code)
endmacro()

def_test(test1.cpp)
def_test(test2.cpp)
def_test(test3.c)
def_test(memoryHoarder.cpp)
