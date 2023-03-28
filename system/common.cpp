extern "C" {
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sysexits.h>
}
#include <limits.h>
#include <errno.h>
#include <ctime>
#include <cstdint>
#include <cassert>
#include <cheerpintrin.h>

#include "impl.h"

extern "C" {


// HACK: The value of this variables will be rewritten to the correct heap start
// and end by the compiler backend
__attribute__((cheerp_asmjs)) char* volatile _heapStart = (char*)0xdeadbeef;
__attribute__((cheerp_asmjs)) char* volatile _heapEnd = (char*)0xdeadbeef;

__attribute__((cheerp_asmjs)) char* _heapCur = 0;

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
			return -1;
		}
	}
	return -1;
}

#define WASM_PAGE (64*1024)
#define ALIGN(x) ((decltype (x))((((uintptr_t)x) + WASM_PAGE-1) & ~(WASM_PAGE-1)))
[[cheerp::wasm]]
long WEAK __syscall_brk(void* newaddr)
{
	static char* brkEnd = nullptr;
	if (!brkEnd)
	{
		brkEnd = ALIGN(_heapStart);
	}
	char* a1 = reinterpret_cast<char*>(newaddr);
	if (a1 < brkEnd)
	{
		return reinterpret_cast<long>(brkEnd);
	}
	int length = (char*)newaddr - brkEnd;
	if (length <= 0)
	{
		return reinterpret_cast<long>(brkEnd);
	}
	length = ALIGN(length);
	if (brkEnd < _heapEnd)
	{
		int avail = _heapEnd - brkEnd;
		if (avail >= length)
		{
			brkEnd += length;
			return reinterpret_cast<long>(brkEnd);
		}
		length -= avail;
		brkEnd += avail;
	}
	int res = __builtin_cheerp_grow_memory(length);
	if (res == -1)
	{
		return reinterpret_cast<long>(brkEnd);
	}
	brkEnd += res;
	return reinterpret_cast<long>(brkEnd);
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
			return -1;
		}
	}
	return 0;
}

long __syscall_exit(long);

long WEAK __syscall_tkill(long a1, ...)
{
	__syscall_exit(EX_OSERR);
	return 0;
}

long WEAK __syscall_exit_group(long code,...)
{
	__syscall_exit(code);
	return 0;
}


long WEAK __syscall_futex(int* uaddr, int futex_op, ...)
{
	return 0;
}

[[cheerp::wasm]]
int WEAK __syscall_mprotect(long addr, size_t len, int prot)
{
	return 0;
}

long __syscall_rt_sigprocmask(long a1, ...)
{
	return 0;
}

int __syscall_access(const char *pathname, int mode)
{
	return -1;
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

int pthread_cancel(struct __pthread* t)
{
	return __syscall_exit(EX_SOFTWARE);
}

}

