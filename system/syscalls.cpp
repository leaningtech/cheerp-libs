extern "C" {
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
}
#include <limits.h>
#include <errno.h>
#include <ctime>
#include <cstdint>
#include <cassert>
#include <cheerp/client.h>

#define WEAK __attribute__((weak))
extern "C" {


// HACK: The value of this variables will be rewritten to the correct heap start
// and end by the compiler backend
__attribute__((cheerp_asmjs)) char* volatile _heapStart = (char*)0xdeadbeef;
__attribute__((cheerp_asmjs)) char* volatile _heapEnd = (char*)0xdeadbeef;

__attribute__((cheerp_asmjs)) char* _heapCur = 0;

static void set_errno(int v)
{
	errno = v;
}

long WEAK __syscall_open(const char* pathname, int flags, ...)
{
	errno = EACCES;
	return -1;
}

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
#define ALIGN(x) ((typeof (x))((((uintptr_t)x) + WASM_PAGE-1) & ~(WASM_PAGE-1)))
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

namespace {

class [[cheerp::genericjs]] CheerpStringBuilder
{
private:
	client::String* out;
	void outputCodepoint(unsigned int codepoint);
public:
	// Keep codepoint and remaining updated while encountering char ch
	// Potentially calls (either 0 or 1 times) outputCodepoint to populate out
	void processChar(unsigned int& codepoint, unsigned int& remaining, unsigned char ch);
	client::String* getString()
	{
		return out;
	}
	void setString(client::String* a)
	{
		out = a;
	}
};

void CheerpStringBuilder::outputCodepoint(unsigned int codepoint)
{
	if (codepoint <= 0xffff)
	{
		if (codepoint)
			out = out->concat(client::String::fromCharCode(codepoint));
	}
	else
	{
		// surrogate pair
		codepoint -= 0x10000;
		unsigned int highSurrogate = (codepoint >> 10) + 0xd800;
		unsigned int lowSurrogate = (codepoint & 0x3ff) + 0xdc00;
		out = out->concat(client::String::fromCharCode(highSurrogate));
		out = out->concat(client::String::fromCharCode(lowSurrogate));
	}
}

void CheerpStringBuilder::processChar(unsigned int& codepoint, unsigned int& remaining, unsigned char ch)
{
	if (ch < 192)
	{
		// Continuation bytes
		if (remaining > 0)
		{
#ifndef NDEBUG
			assert((ch & 192) == 128);
			assert(remaining);
#endif
			codepoint = codepoint << 6 | (ch & 0x3f);
			remaining--;
			// If there are more, wait
			if (remaining > 0)
				return;
			// Otherwise, output the codepoint
		}
		// We should be in ASCII range
		else
		{
#ifndef NDEBUG
			assert(ch < 128u);
#endif
			codepoint = ch;
		}

		// Output the current codepoint
		outputCodepoint(codepoint);
	}
	else
	{
#ifndef NDEBUG
		assert(remaining == 0);
#endif
		unsigned int mask;
		// Start of 2-bytes sequence
		if (ch <= 0xdf)
		{
			remaining = 1;
			mask = 0x1f;
		}
		// Start of 3-bytes sequence
		else if (ch <= 0xef)
		{
			remaining = 2;
			mask = 0x0f;
		}
		// Start of 4-bytes sequence
		else
		{
			remaining = 3;
			mask = 0x07;
		}

		codepoint = ch & mask;
	}
}

} //unnamed namespace

[[cheerp::genericjs]]
static long do_syscall_writev(const iovec* ios, long len)
{
	static client::String* curr = new client::String();
	static unsigned int codepoint = 0;
	static unsigned int remaining = 0;

	CheerpStringBuilder builder;
	builder.setString(curr);

	long __ret = 0;
	for (int i=0; i < len; i++)
	{
		if (ios[i].iov_len == 0)
			continue;

		int curr_len = ios[i].iov_len;
		__ret += curr_len;
		unsigned char* begin = (unsigned char*)ios[i].iov_base;
		for (int j=0; j<curr_len; j++)
		{
			builder.processChar(codepoint, remaining, begin[j]);
		}
	}

	client::TArray<client::String>* arr = builder.getString()->split("\n");
	const int L = arr->get_length();
	for (int i=0; i+1<L; i++)
	{
		client::console.log((*arr)[i]);
	}

	// Last, potentially empty, segment left for next iteration
	curr = (*arr)[L-1];
	return __ret;
}

long WEAK __syscall_writev(long fd, const iovec* ios, long len)
{
	return do_syscall_writev(ios, len);
}

[[cheerp::genericjs]]
double offsetInMilliseconds()
{
	return (new client::Date())->getTimezoneOffset() * -60.0 * 1000.0;
}
[[cheerp::genericjs]]
double performanceNow()
{
	return client::performance.now();
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
			double now = cheerp::date_now();
			now += offsetInMilliseconds();
			tp->tv_sec = now / 1000;
			tp->tv_nsec = (now-(tp->tv_sec*1000.0))*1000.0*1000.0;
			break;
		}
		case CLOCK_MONOTONIC:
		{
			// 'now' is in milliseconds
			double now = cheerp::date_now();
			tp->tv_sec = now / 1000;
			tp->tv_nsec = (now-(tp->tv_sec*1000.0))*1000.0*1000.0;
			break;
		}
		case CLOCK_PROCESS_CPUTIME_ID:
		{
			// 'now' is in milliseconds
			double now = performanceNow();
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

long WEAK __syscall_futex(int* uaddr, int futex_op, uint32_t val, struct timespec *timeout, uint32_t *uaddr2, uint32_t val3)
{
	return 0;
}

int WEAK __syscall_close(int fd)
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

[[cheerp::genericjs]] [[noreturn]] void raiseSignal()
{
	__asm__("throw new Error('Cheerp: Signal raised')");
	__builtin_unreachable();
}

long __syscall_tkill(long a1, ...)
{
	raiseSignal();
	return 0;
}

}

