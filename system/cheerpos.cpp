// Copyright 2025 Leaning Technologies

#include <string.h>

extern "C" {

extern void* __dl_open(const char* file, int mode);
extern void* __dl_symbol(void* handle, const char* name);

// Implement dlopen / dlsym at the kernel level
void *dlopen(const char *file, int mode)
{
	// Override dlopen with an implementation that won't link cleanly
	return __dl_open(file, strlen(file));
}

void* __dlsym_time64(void* handle, const char* name)
{
	return __dl_symbol(handle, name);
}

}
