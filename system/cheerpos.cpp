// Copyright 2025 Leaning Technologies

#include <stdarg.h>
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

// Forward variadic syscalls to extended versions, we cannot use variadic calls across user/kernel boundary
// NOTE: This is dirty, there might not be extra arguments, in which case
//       we read undefined values, but if actually required it should be there
long __syscall_open_3(const char* pathname, int flags, int mode);

long __syscall_open(const char* pathname, int flags, ...)
{
	va_list args;
	va_start(args, flags);
	int mode = va_arg(args, int);
	va_end(args);
	return __syscall_open_3(pathname, flags, mode);
}

}
