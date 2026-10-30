// Copyright 2025 Leaning Technologies

#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <string.h>

#include "impl.h"

extern "C" {

extern void* __dl_open(const char* file, int mode);
extern void* __dl_symbol(void* handle, const char* name);
extern int __exc_setjmp(void* buf, int stackTarget, int funcId, int funcOffset);
extern void __exc_longjmp(void* buf, int val);

// Implement dlopen / dlsym at the kernel level
void *dlopen(const char *file, int mode)
{
	// Override dlopen with an implementation that won't link cleanly
	return __dl_open(file, strlen(file));
}

void* WEAK __dlsym_time64(void* handle, const char* name)
{
	return __dl_symbol(handle, name);
}

// We must make sure the syscall itself is contained in the caller, otherwise
// we won't observe the correct return address
__attribute__((always_inline)) int setjmp(jmp_buf buf)
{
	// Allocate an unused stack variable used as a marker during frame resolution
	int stackMarker;
	// HACK: force the stackMarker to not be optimized away. we need a stack
	// frame for the unwinding to work
	buf->__jb[0] = reinterpret_cast<unsigned long>(&stackMarker);
	assert(sizeof(uint32_t) * 3 <= sizeof(jmp_buf));
	int stackPtr = reinterpret_cast<int>(__builtin_cheerp_stack_save());
	// The kernel will populate other data
	return __exc_setjmp(buf, stackPtr, __builtin_cheerp_func_id(), __builtin_cheerp_func_offset());
}
void longjmp(jmp_buf env, int val)
{
	__exc_longjmp(env, val);
	__builtin_unreachable();
}

// TODO: this currently is exactly the same as setjmp
__attribute__((always_inline)) int sigsetjmp(jmp_buf buf, int sigmask)
{
	// Allocate an unused stack variable used as a marker during frame resolution
	int stackMarker;
	// HACK: force the stackMarker to not be optimized away. we need a stack
	// frame for the unwinding to work
	buf->__jb[0] = reinterpret_cast<unsigned long>(&stackMarker);
	assert(sizeof(uint32_t) * 3 <= sizeof(jmp_buf));
	int stackPtr = reinterpret_cast<int>(__builtin_cheerp_stack_save());
	// The kernel will populate other data
	return __exc_setjmp(buf, stackPtr, __builtin_cheerp_func_id(), __builtin_cheerp_func_offset());
}

}
