#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <sched.h>

#include <cheerpintrin.h>
#define LEAN_CXX_LIB
#include <cheerp/clientlib.h>

#include "impl.h"
#include "futex.h"

namespace [[cheerp::genericjs]] client {
	class ThreadingObject : public Object{
	public:
		void set_func(int);
		void set_args(int);
		void set_tls(int);
		void set_tid(int);
		void set_stack(int);
		void set_ctid(int);
	};
}


[[cheerp::genericjs]] client::ThreadingObject* __builtin_cheerp_get_threading_object();
[[cheerp::genericjs]] client::Blob* __builtin_cheerp_get_threading_blob();

extern "C" {

long __syscall_futex(int32_t* uaddr, int futex_op, ...)
{
	bool isPrivate = futex_op & FUTEX_PRIVATE;
	bool isRealTime = futex_op & FUTEX_CLOCK_REALTIME;
	futex_op &= ~FUTEX_PRIVATE;
	futex_op &= ~FUTEX_CLOCK_REALTIME;
	(void)isPrivate;
	(void)isRealTime;

	// These ops are currently not implemented, since musl doesn't use them.
	assert(futex_op != FUTEX_FD);
	assert(futex_op != FUTEX_CMP_REQUEUE);
	assert(futex_op != FUTEX_WAKE_OP);
	assert(futex_op != FUTEX_WAIT_BITSET);
	assert(futex_op != FUTEX_WAKE_BITSET);
	assert(futex_op != FUTEX_LOCK_PI2);
	assert(futex_op != FUTEX_TRYLOCK_PI);
	assert(futex_op != FUTEX_CMP_REQUEUE_PI);
	assert(futex_op != FUTEX_WAIT_REQUEUE_PI);

	switch (futex_op)
	{
		case FUTEX_WAKE:
		{
			va_list ap;
			va_start(ap, futex_op);
			uint32_t val = va_arg(ap, uint32_t);
			va_end(ap);
			return __builtin_cheerp_atomic_notify(uaddr, val);
		}
		case FUTEX_WAIT:
		{
			va_list ap;
			va_start(ap, futex_op);
			uint32_t val = va_arg(ap, uint32_t);
			const struct timespec *ts = va_arg(ap, const struct timespec *);
			va_end(ap);
			int64_t timeout = -1;
			if (ts != nullptr)
				timeout = ts->tv_sec * 1000000000 + ts->tv_nsec;
			uint32_t ret = __builtin_cheerp_atomic_wait(uaddr, val, timeout);
			if (ret == 1)
				return EAGAIN;
			else if (ret == 2)
				return ETIMEDOUT;
			return 0;
		}
		case FUTEX_LOCK_PI:
		{
			// TODO
			assert(false);
		}
		case FUTEX_UNLOCK_PI:
		{
			// TODO
			assert(false);
		}
		case FUTEX_REQUEUE:
		{
			// TODO
			assert(false);
		}
		default:
		{
			// This should be unreachable, all unhandled cases are asserted for at the top.
			assert(false);
		}
	}
}

[[cheerp::wasm]]
long __syscall_set_thread_area(unsigned long tp);

[[cheerp::genericjs]]
void startWorkerFunction(unsigned int fp, unsigned int args, unsigned int tls, int newThreadId, unsigned int stack, unsigned int ctid)
{
	client::ThreadingObject* threadingObject = __builtin_cheerp_get_threading_object();
	client::Blob* blob = __builtin_cheerp_get_threading_blob();
	threadingObject->set_func(fp);
	threadingObject->set_args(args);
	threadingObject->set_tls(tls);
	threadingObject->set_tid(newThreadId);
	threadingObject->set_stack(stack);
	threadingObject->set_ctid(ctid);
	client::Worker* w = new client::Worker(client::URL.createObjectURL(blob));
	w->postMessage(threadingObject);
}

[[cheerp::wasm]]
[[cheerp::jsexport]]
void workerEntry(unsigned long tp, unsigned int func, unsigned int arg, int newThreadId, unsigned int stack, unsigned int ctid)
{
	// This is the setup for a worker thread.
	// Set the thread pointer
	if (tp != 0)
		__syscall_set_thread_area(tp);
	// Set the thread stack pointer
	__builtin_cheerp_stack_restore(reinterpret_cast<void *>(stack));
	// Assign tid
	tid = newThreadId;
	// Set the clear_child_tid if necessary
	if (ctid != 0)
		clear_child_tid = reinterpret_cast<int *>(ctid);
	// Call the function passed to pthread_create with the arguments passed
	void *(*entry)(void *) = reinterpret_cast<void*(*)(void*)>(func);
	void *argument = reinterpret_cast<void *>(arg);
	entry(argument);
}

[[cheerp::wasm]]
long WEAK __syscall_clone4(int (*func)(void *), void *stack, int flags, void *arg, void *ptid, void *tls, void *ctid)
{
	static int uniqueThreadId = 2;
	int newThreadId = uniqueThreadId++;
	int *set_tid = 0;
	void *tlsPointer = 0;

	// Assert that only the flags are set that we expect.
	assert(flags & CLONE_VM);
	flags &= ~CLONE_VM;
	assert(flags & CLONE_FS);
	flags &= ~CLONE_FS;
	assert(flags & CLONE_FILES);
	flags &= ~CLONE_FILES;
	assert(flags & CLONE_SIGHAND);
	flags &= ~CLONE_SIGHAND;
	assert(flags & CLONE_THREAD);
	flags &= ~CLONE_THREAD;
	assert(flags & CLONE_SYSVSEM);
	flags &= ~CLONE_SYSVSEM;
	assert(flags & CLONE_DETACHED);
	flags &= ~CLONE_DETACHED;

	if (flags & CLONE_SETTLS)
	{
		flags &= ~CLONE_SETTLS;
		tlsPointer = tls;
	}
	if (flags & CLONE_PARENT_SETTID)
	{
		flags &= ~CLONE_PARENT_SETTID;
		*(int*)ptid = newThreadId;
	}
	if (flags & CLONE_CHILD_CLEARTID)
	{
		flags &= ~CLONE_CHILD_CLEARTID;
		set_tid = (int*)ctid;
	}

	assert(flags == 0);

	startWorkerFunction((unsigned int)func, (unsigned int)arg, (unsigned int)tlsPointer, newThreadId, (unsigned int)stack, (unsigned int)set_tid);
	return newThreadId;
}

}

namespace sys_internal {

bool exit_thread()
{
	if (tid != 1 && clear_child_tid != nullptr)
	{
		// If clear_child_tid is set, write 0 to the address it points to,
		// and do a FUTEX_WAKE on the address.
		int *wake_address = clear_child_tid;
		*clear_child_tid = 0;
		__syscall_futex(wake_address, FUTEX_WAKE, 1, nullptr, nullptr, 0);
		return true;
	}
	return false;
}

}
