#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <sched.h>
#include <atomic>

#include <cheerpintrin.h>
#define LEAN_CXX_LIB
#include <cheerp/clientlib.h>
#include <cheerp/client.h>

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

[[cheerp::genericjs]] client::Worker* utilityWorker = nullptr;
MessageQueue<ThreadSpawnInfo> threadMessagingQueue;

extern "C" {

void _start();

std::atomic<uint32_t*> mainThreadWaitAddress = 0;

long __syscall_futex(uint32_t* uaddr, int futex_op, ...)
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

			uint32_t threadsWokenUp = 0;
			// If the main thread is waiting on this address, handle this case specially.
			if (uaddr == mainThreadWaitAddress)
			{
				// Notify the main thread that it can wake up.
				mainThreadWaitAddress.store(0);
				val -= 1;
				threadsWokenUp = 1;
				// If there are no other threads to wake up, return here.
				if (val <= 0)
					return threadsWokenUp;
			}
			return threadsWokenUp + __builtin_cheerp_atomic_notify(uaddr, val);
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

			// If this is the main thread, it's illegal to do a futex wait operation.
			// Instead, we busy wait while checking a special value.
			if (tid == 1) // TODO: improve to use actual browser main thread here.
			{
				// Manually test the value at uaddr against val. If they do not match, return EAGAIN.
				if (*uaddr != val)
					return EAGAIN;
				mainThreadWaitAddress.store(uaddr);
				if (*uaddr != val)
				{
					mainThreadWaitAddress.store(0);
					return EAGAIN;
				}
				while (mainThreadWaitAddress.load() != 0)
				{
				}
				// TODO : timeout calculation, if elapsed, ETIMEDOUT.
			}
			else
			{
				uint32_t ret = __builtin_cheerp_atomic_wait(uaddr, val, timeout);
				if (ret == 1)
					return EAGAIN; // Value at uaddr did not match val.
				else if (ret == 2)
					return ETIMEDOUT;
			}
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
	// If this is the main thread, only spawn utility thread.
	// Else, use utility thread to spawn the new thread.
	if (utilityWorker == nullptr)
	{
		client::ThreadingObject* threadingObject = __builtin_cheerp_get_threading_object();
		client::Blob* blob = __builtin_cheerp_get_threading_blob();
		threadingObject->set_func(fp);
		threadingObject->set_args(args);
		threadingObject->set_tls(tls);
		threadingObject->set_tid(newThreadId);
		threadingObject->set_stack(stack);
		threadingObject->set_ctid(ctid);
		client::WorkerOptions* opts = new client::WorkerOptions();
		opts->set_name("Utility");
		utilityWorker = new client::Worker(client::URL.createObjectURL(blob), opts);
		utilityWorker->postMessage(threadingObject);
		return;
	}
	struct ThreadSpawnInfo spawnInfo;
	spawnInfo.func = fp;
	spawnInfo.args = args;
	spawnInfo.tls = tls;
	spawnInfo.tid = newThreadId;
	spawnInfo.stack = stack;
	spawnInfo.ctid = ctid;

	threadMessagingQueue.send(spawnInfo);
}

[[cheerp::genericjs]]
void spawnThreadFromUtility()
{
	ThreadSpawnInfo spawnInfo = threadMessagingQueue.receive();
	client::ThreadingObject* threadingObject = __builtin_cheerp_get_threading_object();
	client::Blob* blob = __builtin_cheerp_get_threading_blob();
	threadingObject->set_func(spawnInfo.func);
	threadingObject->set_args(spawnInfo.args);
	threadingObject->set_tls(spawnInfo.tls);
	threadingObject->set_tid(spawnInfo.tid);
	threadingObject->set_stack(spawnInfo.stack);
	threadingObject->set_ctid(spawnInfo.ctid);
	client::WorkerOptions* opts = new client::WorkerOptions();
	opts->set_name((new client::String("Thread "))->concat(spawnInfo.tid));
	client::Worker* worker = new client::Worker(client::URL.createObjectURL(blob), opts);
	worker->set_onmessage(spawnThreadFromUtility);
	worker->postMessage(threadingObject);
}

[[cheerp::genericjs]]
void leak_thread()
{
	client::String* throwObj = new client::String("LeakUtilityThread");
	__builtin_cheerp_throw(throwObj);
}

[[cheerp::genericjs]]
void reschedule()
{
	client::setTimeout(spawnThreadFromUtility, 0);
}

[[cheerp::wasm]]
void *utilityRoutine(void *arg)
{
	reschedule();
	leak_thread();
	return NULL;
}

[[cheerp::wasm]]
void spawnUtilityThread()
{
	pthread_t utilityTid;

	pthread_create(&utilityTid, NULL, &utilityRoutine, NULL);
}

[[cheerp::genericjs]]
void callStart()
{
	_start();
}

[[cheerp::genericjs]]
void spawnUtility()
{
	spawnUtilityThread();
	// When the utility workers sends a message, continue execution in main thread with the _start function.
	utilityWorker->addEventListener("message", callStart);
}

long __syscall_gettid(void)
{
	return tid;
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

long __syscall_membarrier(int cmd, unsigned int flags)
{
	return 0;
}

}

namespace sys_internal {

bool exit_thread()
{
	if (tid != 1 && clear_child_tid != nullptr)
	{
		// If clear_child_tid is set, write 0 to the address it points to,
		// and do a FUTEX_WAKE on the address.
		uint32_t *wake_address = reinterpret_cast<uint32_t*>(clear_child_tid);
		*clear_child_tid = 0;
		__syscall_futex(wake_address, FUTEX_WAKE, 1, nullptr, nullptr, 0);
		return true;
	}
	return false;
}

}
