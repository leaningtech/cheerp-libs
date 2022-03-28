extern "C" {
#include <sys/uio.h>
#include <sys/ioctl.h>
}
#include <ctime>
#include <cheerp/client.h>

extern "C" {


// HACK: The value of this variables will be rewritten to the correct heap start
// and end by the compiler backend
__attribute__((cheerp_asmjs)) char* volatile _heapStart = (char*)0xdeadbeef;
__attribute__((cheerp_asmjs)) char* volatile _heapEnd = (char*)0xdeadbeef;

__attribute__((cheerp_asmjs)) char* _heapCur = 0;

long __syscall_ioctl(long fd, long req, void* arg)
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
long __syscall_mmap2(long a1, long a2, long a3, long a4, long a5, long a6)
{
	return -1;
}

long __syscall_brk(void* newaddr)
{
	char* a1 = reinterpret_cast<char*>(newaddr);
	if (a1 < _heapStart)
	{
		return reinterpret_cast<long>(_heapStart);
	}
	if (a1 <= _heapEnd)
	{
		_heapCur = a1;
	}
	else
	{
		int res = __builtin_cheerp_grow_memory(a1-_heapEnd);
		if (res != -1)
		{
			_heapEnd += res;
			_heapCur = a1 < _heapEnd ? a1 : _heapEnd;
		}
	}
	return reinterpret_cast<long>(_heapCur);
}

[[cheerp::genericjs]]
void print_stream(const char* buf, size_t len, bool stream)
{
	static client::TextDecoder* td = new client::TextDecoder("utf-8");
	static client::String* toWrite = new client::String();

	client::Uint8Array* arr = cheerp::MakeTypedArray<client::Uint8Array>(buf, len);
	client::TextDecodeOptions* opts = new client::TextDecodeOptions();
	opts->set_stream(stream);
	client::String* s = td->decode(arr, opts);
	toWrite = toWrite->concat(s);
	if (!stream)
	{
		uint32_t l = toWrite->get_length();
		if (toWrite->charCodeAt(l-1) == 10)
			toWrite = toWrite->substr(0, l -1);
		client::console.log(toWrite);
		toWrite = new client::String();
	}
}

static const char* get_base(const iovec* io)
{
	return static_cast<const char*>(io->iov_base);
}

long __syscall_writev(long fd, const iovec* ios, long len)
{
	long __ret = 0;
	for (int i = 0; i < len; i++)
	{
		print_stream(get_base(&ios[i]), ios[i].iov_len, i < len - 1);
		__ret += ios[i].iov_len;
	}
	return __ret;
}

[[cheerp::genericjs]]
double offsetInMilliseconds()
{
	return (new client::Date())->getTimezoneOffset() * -60.0 * 1000.0;
}
long __syscall_clock_gettime64(int clock_id, struct timespec* tp)
{
	if (tp)
	{
		// 'now' is in milliseconds
		double now = cheerp::date_now();
		if (clock_id == CLOCK_REALTIME)
			now += offsetInMilliseconds();
		tp->tv_sec = now / 1000;
		tp->tv_nsec = (now-(tp->tv_sec*1000.0))*1000.0*1000.0;
	}
	return 0;
}


}

