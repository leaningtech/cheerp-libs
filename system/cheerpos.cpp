// Copyright 2025 Leaning Technologies

#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <string.h>

extern "C" {

extern void* __dl_open(const char* file, int mode);
extern void* __dl_symbol(void* handle, const char* name);
extern int __exc_setjmp(void* buf);
extern void __exc_longjmp(void* buf, int val);

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

// We must make sure the syscall itself is contained in the caller, otherwise
// we won't observe the correct return address
__attribute__((always_inline)) int setjmp(jmp_buf buf)
{
	// Allocate an unused stack variable used as a marker during frame resolution
	int stackMarker;
	// Unsure if there is a convention about the meaning of the slots
	buf->__jb[0] = reinterpret_cast<unsigned long>(&stackMarker);
	// The kernel will populate other data
	return __exc_setjmp(buf);
}

void longjmp(jmp_buf env, int val)
{
	__exc_longjmp(env, val);
	__builtin_unreachable();
}

long __syscall_statx(long a1,...)
{
	// Make this fail to reach the 64-bit versions
	return -ENOSYS;
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

long __syscall_openat_4(int dirfd, const char *pathname, int flags, int mode);

long __syscall_openat(int dirfd, const char *pathname, int flags, ...)
{
	va_list args;
	va_start(args, flags);
	int mode = va_arg(args, int);
	va_end(args);
	return __syscall_openat_4(dirfd, pathname, flags, mode);
}

long __syscall_fcntl64_3(int fd, int op, int arg);

long __syscall_fcntl64(int fd, int op, ...)
{
	va_list args;
	va_start(args, op);
	int arg = va_arg(args, int);
	va_end(args);
	return __syscall_fcntl64_3(fd, op, arg);
}

}
