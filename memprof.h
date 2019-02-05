// Copyright 2018 Leaning Technologies Ltd. All Rights Reserved.
#ifndef CHEERP_MEMPROF
#define CHEERP_MEMPROF

#include <sys/types.h>
typedef int CHEERP_MEMPROF_TAG;

#ifdef __cplusplus
extern "C"
{
#endif
	//Create a named tag that will records the allocations/free that will follow
	__attribute__((cheerp_wasm)) CHEERP_MEMPROF_TAG cheerpMemProfAnnotate(const char *tag_name);

	//Close the tag and report what was create and not yet freed (logging the report to the console)
	__attribute__((cheerp_wasm)) void cheerpMemProfClose(CHEERP_MEMPROF_TAG tag);

	//Log to the console the memory allocated after the tag creation and yet to be freed
	__attribute__((cheerp_wasm)) void cheerpMemProfLive(CHEERP_MEMPROF_TAG tag);

	//Return the amount of memory allocated after the tag creation and yet to be freed
	__attribute__((cheerp_wasm)) size_t cheerpMemProfUsed(CHEERP_MEMPROF_TAG tag);

	//Return the total amount of memory currently allocated by the program
	__attribute__((cheerp_wasm)) size_t cheerpMemProfTotalUsed();
#ifdef __cplusplus
}
#endif

#endif
