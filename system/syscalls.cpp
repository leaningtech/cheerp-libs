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
#include <vector>
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

struct
[[cheerp::wasm]]
Page {
	size_t size;
	Page* next;
	Page* prev;

	void init(size_t s)
	{
		size = s;
		next = nullptr;
		prev = nullptr;
	}
	void clear()
	{
		uint64_t* addr = reinterpret_cast<uint64_t*>(this);
		char* a = reinterpret_cast<char*>(addr);
		uint64_t* end = reinterpret_cast<uint64_t*>(a+size);
		for(;addr < end; addr++)
		{
			*addr = 0;
		}
	}
	Page* split(size_t amount)
	{
		assert(amount <= size && "requesting amount greater than size");
		Page* newthis = nullptr;
		if (amount < size)
		{
			newthis = reinterpret_cast<Page*>(reinterpret_cast<char*>(this)+amount);
			newthis->init(size-amount);
		}
		size = amount;
		return newthis;
	}
};

struct
[[cheerp::wasm]]
PageList {
	Page _end{0, &_end, &_end};

	Page* end()
	{
		return &_end;
	}
	Page* head()
	{
		return end()->next;
	}
	Page* lower_bound(size_t size)
	{
		for(Page* p = head(); p != end(); p = p->next)
		{
			if (p->size >= size)
				return p;
		}
		return end();
	}
	void remove(Page* p)
	{
		p->prev->next = p->next;
		p->next->prev = p->prev;
	}
	bool try_merge(Page* toMerge)
	{
		for(Page* p = head(); p != end(); p = p->next)
		{
			char* pStart = reinterpret_cast<char*>(p);
			char* pEnd = pStart + p->size;
			char* toMergeStart = reinterpret_cast<char*>(toMerge);
			char* toMergeEnd = toMergeStart + toMerge->size;
			if (toMergeEnd == pStart || toMergeStart == pEnd)
			{
				remove(p);
				Page* merged = reinterpret_cast<Page*>(pStart < toMergeStart ? pStart : toMergeStart);
				merged->init(p->size + toMerge->size);
				do_insert(merged);
				return true;
			}
		}
		return false;
	}
	void do_insert(Page* p)
	{
		Page* insertPt = lower_bound(p->size);
		p->next = insertPt;
		p->prev = insertPt->prev;
		p->prev->next= p;
		insertPt->prev = p;
	}
	void insert(Page* p)
	{
		if (try_merge(p))
			return;
		do_insert(p);
	}
};

[[cheerp::wasm]]
static char* mmapStart = 0;
[[cheerp::wasm]]
static char* mmapEnd = 0;
[[cheerp::wasm]]
static PageList freePages;

#define DO_ALIGN(x) ((typeof (x))((((uintptr_t)x) + PAGE_SIZE-1) & ~(PAGE_SIZE-1)))
#define ASSERT_ALIGNED(x) (assert(x==DO_ALIGN(x)&&"address is not aligned to wasm page size"))
#define ADDR_TO_PAGE(x) ((uintptr_t)x / PAGE_SIZE);
#define PAGE_TO_ADDR(x) ((char*)(x * PAGE_SIZE));
[[cheerp::wasm]]
static long mmap_new(long length)
{
	// Initialize data structures
	if (mmapStart == 0)
	{
		// Align to page size
		mmapStart = DO_ALIGN(_heapStart);
		// This is already aligned
		mmapEnd = _heapEnd;

		if (mmapStart != mmapEnd)
		{
			Page* p = reinterpret_cast<Page*>(mmapStart);
			p->init(mmapEnd - mmapStart);
			freePages.insert(p);
		}
	}

	Page* p = freePages.lower_bound(length);
	if (p != freePages.end())
	{
		freePages.remove(p);
		Page* rest = p->split(length);
		if (rest)
			freePages.insert(rest);
		return reinterpret_cast<long>(p);
	}

	int res = __builtin_cheerp_grow_memory(length);
	if (res == -1)
	{
		set_errno(ENOMEM);
		return -1;
	}
	char* ret = mmapEnd;
	mmapEnd += length;

	return reinterpret_cast<long>(ret);
}

long WEAK __syscall_mmap2(long addr, long length, long prot, long flags, long fd, long offset)
{
	ASSERT_ALIGNED(addr);
	ASSERT_ALIGNED(length);
	assert(fd == -1 && "mmapping files is unsupported");

	// TODO handle flags
	return mmap_new(length);
}

[[cheerp::wasm]]
static long do_munmap(long a, long length)
{
	void* addr = reinterpret_cast<void*>(a);
	ASSERT_ALIGNED(addr);
	ASSERT_ALIGNED(length);
	assert(addr >= mmapStart && "unmapping address below mmapped range");
	assert(addr < mmapEnd && "unmapping address above mmapped range");
#if !defined(NDEBUG)
	bool already_unmapped = false;
	for(Page* p = freePages.head(); p != freePages.end(); p = p->next)
	{
		if (addr >= p && addr < (char*)p + p->size)
		{
			already_unmapped = true;
			break;
		}
	}
	assert(!already_unmapped && "address already unmapped");
#endif
	Page* p = reinterpret_cast<Page*>(addr);
	p->init(length);
	freePages.insert(p);
	return 0;
}

long WEAK __syscall_munmap(long a, long length)
{
	return do_munmap(a, length);
}

[[cheerp::wasm]]
static long do_mremap(long a, long old_len, long new_len)
{
	char* addr = reinterpret_cast<char*>(a);
	ASSERT_ALIGNED(addr);
	ASSERT_ALIGNED(old_len);
	ASSERT_ALIGNED(new_len);
	assert(addr >= mmapStart && "remapping address below mmapped range");
	assert(addr < mmapEnd && "remapping address above mmapped range");

	Page* candidate = nullptr;
	for(Page* p = freePages.head(); p != freePages.end(); p = p->next)
	{
		if (addr + old_len == (char*)p)
		{
			candidate = p;
			break;
		}
	}
	if (candidate && candidate->size + old_len >= new_len)
	{
		freePages.remove(candidate);
		Page* rest = candidate->split(new_len-old_len);
		if (rest)
			freePages.insert(rest);
		return a;
	}
	// TODO: if MREMAP_MAYMOVE is passed as a flag, we should move the mapping.
	// this is not necessary for musl's malloc but it may be for other stuff
	set_errno(ENOMEM);
	return -1;
}

long WEAK __syscall_mremap(long old_addr, long old_len, long new_len, long flags, long new_addr)
{
	return do_mremap(old_addr, old_len, new_len);
}

[[cheerp::wasm]]
long WEAK __syscall_madvise(long a, long length, long advice)
{
	ASSERT_ALIGNED(a);
	ASSERT_ALIGNED(length);
	assert((void*)a >= mmapStart && "madvise address below mmapped range");
	assert((void*)a < mmapEnd && "madvise address above mmapped range");
	if (advice != MADV_DONTNEED)
	{
		set_errno(EINVAL);
		return -1;
	}
	uint64_t* addr = reinterpret_cast<uint64_t*>(a);
	uint64_t* end = reinterpret_cast<uint64_t*>(a+length);
	for(;addr < end; addr++)
	{
		*addr = 0;
	}
	return 0;
}

[[cheerp::wasm]]
long WEAK __syscall_brk(void* newaddr)
{
	static char* brkEnd = nullptr;
	if (!brkEnd)
	{
		brkEnd = DO_ALIGN(_heapStart);
	}
	char* a1 = reinterpret_cast<char*>(newaddr);
	if (a1 < _heapStart)
	{
		return reinterpret_cast<long>(_heapStart);
	}
	if (a1 >= brkEnd)
		return -1;
	return reinterpret_cast<long>(newaddr);
}

[[cheerp::genericjs]]
void print(const char* buf, size_t len)
{
	auto* toWrite = client::String::fromUtf8(buf, len);
	uint32_t l = toWrite->get_length();
	if (toWrite->charCodeAt(l-1) == 10)
		toWrite = toWrite->substr(0, l -1);
	client::console.log(toWrite);
}

static const char* get_base(const iovec* io)
{
	return static_cast<const char*>(io->iov_base);
}

long WEAK __syscall_writev(long fd, const iovec* ios, long len)
{
	long __ret = 0;
	std::vector<char> buf;
	for (int i = 0; i < len; i++)
	{
		if (ios[i].iov_len == 0)
			continue;
		char* begin = (char*)ios[i].iov_base;
		char* end = begin + ios[i].iov_len;
		buf.insert(buf.end(), begin, end);
		__ret += ios[i].iov_len;
	}
	print(buf.data(), buf.size());
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

}

