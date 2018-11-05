// Copyright 2018 Leaning Technologies Ltd. All Rights Reserved.

#include <sys/types.h>
typedef int CHEERP_MEMPROF_TAG;

#ifdef __cplusplus
extern "C"
{
#endif
	__attribute__((cheerp_wasm)) CHEERP_MEMPROF_TAG cheerpMemProfAnnotate(const char *tag_name);
	__attribute__((cheerp_wasm)) void cheerpMemProfClose(CHEERP_MEMPROF_TAG tag);
	__attribute__((cheerp_wasm)) void cheerpMemProfLive(CHEERP_MEMPROF_TAG tag);
	__attribute__((cheerp_wasm)) size_t cheerpMemProfUsed(CHEERP_MEMPROF_TAG tag);
	__attribute__((cheerp_wasm)) size_t cheerpMemProfTotalUsed();
#ifdef __cplusplus
}
#endif
