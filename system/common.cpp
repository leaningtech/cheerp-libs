extern "C" {
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sysexits.h>
}
#include <errno.h>
#include <sched.h>
#include <cassert>
#include <ctime>
#include <cstdint>
#include <cstdlib>
#include <cstdarg>
#include <cheerpintrin.h>
#include <cheerp/clientlib.h>

#  define LEAN_CXX_LIB
#  include <client/cheerp/types.h>

#include "impl.h"

[[cheerp::genericjs]] client::TArray<client::String*>* __builtin_cheerp_environ();
[[cheerp::genericjs]] client::TArray<client::String*>* __builtin_cheerp_argv();

_Thread_local int tid = 1;
_Thread_local int *clear_child_tid = nullptr;
// The dso_handle is used internally to call the __cxa_atexit function for global destructors
// But __cxa_atexit ignores it, so we can just set it to NULL.
void* __dso_handle = NULL;

extern "C" {


#ifdef __ASMJS__
// HACK: The value of this variables will be rewritten to the correct heap start
// and end by the compiler backend
char* volatile _heapStart = (char*)0xdeadbeef;
char* volatile _heapEnd = (char*)0xdeadbeef;
#endif


long WEAK __syscall_ioctl(long fd, long req, void* arg)
{
	switch(req)
	{
		case TIOCGWINSZ:
		{
			winsize* ws = static_cast<winsize*>(arg);
			ws->ws_row = 25;
			ws->ws_col = 80;
			ws->ws_xpixel = 0;
			ws->ws_ypixel = 0;
			return 0;
		}
		default:
		{
			return -EINVAL;
		}
	}
	return -EINVAL;
}

#define WASM_PAGE (64*1024)
#define ALIGN(x) ((decltype (x))((((uintptr_t)x) + WASM_PAGE-1) & ~(WASM_PAGE-1)))
long WEAK __syscall_brk(void* newaddr)
{
#ifndef __ASMJS__
	return 0;
#else
	static char* brkEnd = nullptr;
	if (!brkEnd)
	{
		brkEnd = ALIGN(_heapStart);
	}
	char* a1 = reinterpret_cast<char*>(newaddr);
	if (a1 <= brkEnd)
	{
		return reinterpret_cast<long>(brkEnd);
	}
	unsigned int length = (char*)newaddr - brkEnd;
	length = ALIGN(length);
	if (brkEnd < _heapEnd)
	{
		unsigned int avail = _heapEnd - brkEnd;
		if (avail >= length)
		{
			brkEnd += length;
			return reinterpret_cast<long>(brkEnd);
		}
		length -= avail;
		brkEnd += avail;
	}
	int res = __builtin_cheerp_grow_memory(length>>16);
	if (res == -1)
	{
		return reinterpret_cast<long>(brkEnd);
	}
	brkEnd += length;
	return reinterpret_cast<long>(brkEnd);
#endif
}

long WEAK __syscall_clock_gettime64(int clock_id, struct timespec* tp)
{
	if (!tp)
		return 0;
	switch (clock_id)
	{
		case CLOCK_REALTIME:
		{
			// 'now' is in milliseconds
			double now = sys_internal::real_time_now();
			now += sys_internal::timezone_offset();
			tp->tv_sec = now / 1000;
			tp->tv_nsec = (now-(tp->tv_sec*1000.0))*1000.0*1000.0;
			break;
		}
		case CLOCK_MONOTONIC:
		{
			// 'now' is in milliseconds
			double now = sys_internal::monotonic_time_now();
			tp->tv_sec = now / 1000;
			tp->tv_nsec = (now-(tp->tv_sec*1000.0))*1000.0*1000.0;
			break;
		}
		case CLOCK_PROCESS_CPUTIME_ID:
		{
			// 'now' is in milliseconds
			double now = sys_internal::cpu_time_now();
			tp->tv_sec = now / 1000;
			tp->tv_nsec = (now-(tp->tv_sec*1000.0))*1000.0*1000.0;
			break;
		}
		default:
		{
			return -EINVAL;
		}
	}
	return 0;
}

long __syscall_exit(long);

long WEAK __syscall_tkill(pid_t tid, int sig)
{
	return sys_internal::tkill(tid, sig);
}

long WEAK __syscall_exit_group(long code,...)
{
	return sys_internal::exit_group(code);
}

long WEAK __syscall_gettid(void)
{
	return tid;
}

long WEAK __syscall_futex(uint32_t* uaddr, int futex_op, ...)
{
	va_list args;
	va_start(args, futex_op);
	int ret = sys_internal::futex(uaddr, futex_op, args);
	va_end(args);
	return ret;
}

int WEAK __syscall_mprotect(long addr, size_t len, int prot)
{
	return 0;
}

long WEAK __syscall_rt_sigprocmask(long a1, ...)
{
	return 0;
}

long WEAK __syscall_futex_time64(long a1,...)
{
	return -ENOSYS;
}

long WEAK __syscall_clock_gettime32(long a1,...)
{
	return -ENOSYS;
}

long WEAK __syscall_gettimeofday_time32(long a1,...)
{
	return -ENOSYS;
}

long WEAK __syscall_set_robust_list(long a1, ...)
{
	return -ENOSYS;
}

long WEAK __syscall_fcntl64(long a1, ...)
{
	return -ENOSYS;
}

long WEAK __syscall_fstat64(long a1,...)
{
	return -ENOSYS;
}

long WEAK __syscall_rt_sigaction(long a1,...)
{
	return -ENOSYS;
}

long WEAK __syscall_mmap2(long a1, long a2, long a3, long a4, long a5, long a6)
{
	return -ENOSYS;
}

long WEAK __syscall_munmap(long a1, long length)
{
	return -ENOSYS;
}

[[cheerp::genericjs]] static size_t client_to_utf8(char *dest, size_t dlen, const client::String *str) {
	constexpr uint32_t REPLACEMENT_CHARACTER = 0xFFFD;
	constexpr uint32_t MAX_CODEPOINT = 0x10FFFF;
	constexpr uint32_t INVALID_CODEPOINT = -1;

	const size_t slen = str->get_length();
	size_t j = 0;
	for (size_t i = 0; i < slen; ++i) {
		uint32_t cp = str->charCodeAt(i);

		if (cp >= 0xD800 && cp <= 0xDFFF) {
			if (i + 1 < slen) {
				uint32_t trail = str->charCodeAt(++i);
				cp = 0x10000 + ((cp & 0x3FF) | (trail & 0x3FF));
			} else {
				// Missing lower surrogate
				cp = INVALID_CODEPOINT;
			}
		}

		if (cp > MAX_CODEPOINT)
			cp = INVALID_CODEPOINT;

		if (cp <= 0x7F) {
			if (j < dlen)
				*dest++ = static_cast<char>(cp);

			j += 1;
		} else if (cp <= 0x7FF) {
			if (j + 1 < dlen) {
				*dest++ = 0xC0 | (cp >> 6);
				*dest++ = 0x80 | (cp & 63);
			}

			j += 2;
		} else if (cp <= 0xFFFF) {
			if (j + 2 < dlen) {
				*dest++ = 0xE0 | (cp >> 12);
				*dest++ = 0x80 | ((cp >> 6) & 63);
				*dest++ = 0x80 | (cp & 63);
			}

			j += 3;
		} else {
			if (j + 3 < dlen) {
				*dest++ = 0xF0 | (cp >> 18);
				*dest++ = 0x80 | ((cp >> 12) & 63);
				*dest++ = 0x80 | ((cp >> 6) & 63);
				*dest++ = 0x80 | (cp & 63);
			}

			j += 4;
		}
	}
	return j;
}

#define MAX_ENTRIES 64
static size_t buf_size = 0;
static char argv_environ_buf[MAX_ENTRIES * 1024];

[[cheerp::genericjs]] static size_t read_to_buf(char* dest, size_t n, const client::TArray<client::String*> *arr, size_t idx)
{
	if (idx >= arr->get_length())
		return 0;

	size_t len = client_to_utf8(dest, n, (*arr)[idx]);
	if (len < n)
		dest[len] = '\0';
	return len + 1;
}

[[cheerp::genericjs]] static client::TArray<client::String*>* read_nodejs_args(const client::String *opt_name) {
	client::TArray<client::String*> *result = new client::TArray<client::String*>();

	const client::TArray<client::String*> *argv = nullptr;
        __asm__("(typeof process == 'undefined' ? [] : process.argv) || []" : "=r"(argv));

	for (size_t i = 0; i < argv->get_length(); ++i) {
		const client::String *arg = (*argv)[i];
		if (arg->startsWith(opt_name))
			result->push(arg->substr(opt_name->get_length()));
	}
	return result;
}

[[cheerp::genericjs]] static size_t read_arg(char *dest, size_t n, size_t idx)
{
	static client::TArray<client::String*> *client_argv = __builtin_cheerp_argv()
		? __builtin_cheerp_argv()
		: read_nodejs_args(new client::String("--cheerp-arg="));
	return read_to_buf(dest, n, client_argv, idx);
}

void WEAK __syscall_main_args(int* argc_p, char*** argv_p)
{
	static char* argv[MAX_ENTRIES];

	size_t i = 0;
	while (true) {
		if (i > sizeof(argv)/sizeof(argv[0]))
			abort();

		const size_t rem = sizeof argv_environ_buf - buf_size;
		size_t len = read_arg(&argv_environ_buf[buf_size], rem, i);

		if (!len)
			break;
		if (len > rem)
			abort();

		argv[i++] = &argv_environ_buf[buf_size];
		buf_size += len;
	}

	*argc_p = i;
	*argv_p = argv;
}

[[cheerp::genericjs]] static size_t read_env(char *dest, size_t n, size_t idx)
{
	static client::TArray<client::String*> *client_environ = __builtin_cheerp_environ()
		? __builtin_cheerp_environ()
		: read_nodejs_args(new client::String("--cheerp-env="));
	return read_to_buf(dest, n, client_environ, idx);
}


extern "C" char **environ;
void WEAK __syscall_main_environ() {
	static char* cheerp_environ[MAX_ENTRIES];

	size_t i = 0;
	while (true) {
		if (i >= sizeof(cheerp_environ)/sizeof(cheerp_environ[0]))
			abort();

		const size_t rem = sizeof argv_environ_buf - buf_size;
		size_t len = read_env(&argv_environ_buf[buf_size], rem, i);

		if (!len)
			break;
		if (len > rem)
			abort();

		cheerp_environ[i++] = &argv_environ_buf[buf_size];
		buf_size += len;
	}

	cheerp_environ[i] = 0;
	environ = cheerp_environ;
}

long WEAK __syscall_membarrier(int cmd, unsigned int flags)
{
	return 0;
}

long WEAK __syscall_sched_setscheduler(pid_t pid, int policy, const struct sched_param *param)
{
	return 0;
}

long WEAK __syscall_sched_yield()
{
	return 0;
}

long WEAK __syscall_set_tid_address(int *tidptr)
{
	clear_child_tid = tidptr;
	return tid;
}

long WEAK __syscall_sched_getaffinity(pid_t pid, int cpusetsize, unsigned long* mask)
{
	return sys_internal::sched_getaffinity(pid, cpusetsize, mask);
}

}

namespace sys_internal {

long WEAK tkill(pid_t tid, int sig)
{
	__syscall_exit(EX_OSERR);
	return 0;
}

long WEAK exit_group(long code)
{
	__syscall_exit(code);
	return 0;
}

long WEAK sched_getaffinity(pid_t pid, int cpusetsize, unsigned long* mask)
{
	// Only a pid of 0 (the current process) is supported.
	assert(pid == 0);

	unsigned char* set = reinterpret_cast<unsigned char*>(mask);
	set[0] = 1;
	for (int i = 1; i < cpusetsize; i++)
		set[i] = 0;
	return cpusetsize;
}

long WEAK futex(uint32_t* uaddr, int futex_op, va_list args)
{
	return -ENOSYS;
}

} // namespace sys_internal
