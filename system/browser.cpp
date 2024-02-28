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

#include "impl.h"

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

[[cheerp::genericjs]]
long do_syscall_writev(const iovec* ios, long len)
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

	client::TArray<client::String*>* arr = builder.getString()->split("\n");
	const int L = arr->get_length();
	for (int i=0; i+1<L; i++)
	{
		client::console.log((*arr)[i]);
	}

	// Last, potentially empty, segment left for next iteration
	curr = (*arr)[L-1];
	return __ret;
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

[[cheerp::genericjs]] [[noreturn]] void raiseSignal()
{
	__asm__("throw new Error('Cheerp: Signal raised')");
	__builtin_unreachable();
}

} //unnamed namespace


namespace sys_internal {

double timezone_offset()
{
	return offsetInMilliseconds();
}
double real_time_now()
{
	return cheerp::date_now();
}
double monotonic_time_now()
{
	return cheerp::date_now();
}
double cpu_time_now()
{
	return performanceNow();
}

} //namespace sys_internal

extern "C" {

long WEAK __syscall_exit(long code)
{
	raiseSignal();
	return 0;
}

long WEAK __syscall_writev(long fd, const iovec* ios, long len)
{
	return do_syscall_writev(ios, len);
}

long WEAK __syscall_readv(long fd, const iovec* ios, long len)
{
	return -ENOSYS;
}

int WEAK __syscall_read(int fd, void* buf, int count)
{
	return -ENOSYS;
}

long WEAK __syscall_open(const char* pathname, int flags, ...)
{
	return -ENOSYS;

}
long WEAK __syscall_close(int fd)
{
	return 0;
}

size_t WEAK __syscall__llseek(unsigned int fd, unsigned long offset_high, unsigned long offset_low,
	unsigned long long* result, unsigned int whence)
{
	return -ENOSYS;
}

int WEAK __syscall_rename(const char *oldpath, const char *newpath)
{
	return -ENOSYS;
}

int WEAK __syscall_access(const char *pathname, int mode)
{
	return -ENOSYS;
}


} // extern "C"
