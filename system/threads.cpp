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
		int get_tid();
		void set_stack(int);
		void set_ctid(int);
	};
}

enum atomicWaitStatus {
	UNINITIALIZED = 0,
	YES,
	NO,
};

[[cheerp::genericjs]] client::ThreadingObject* __builtin_cheerp_get_threading_object();
[[cheerp::genericjs]] client::Blob* __builtin_cheerp_get_threading_blob();

[[cheerp::genericjs]] client::Worker* utilityWorker = nullptr;
[[cheerp::genericjs]] client::Map<int, client::Worker*>* workerList = nullptr;
FutexSpinLock futexSpinLock;
MessageQueue<QueueMessage> threadMessagingQueue;

extern "C" {

void _start();

std::atomic<uint32_t*> mainThreadWaitAddress = 0;

bool isBrowserMainThread()
{
	static _Thread_local atomicWaitStatus canUseAtomicWait = UNINITIALIZED;
	if (canUseAtomicWait == UNINITIALIZED)
	{
		if (testUseAtomicWait())
			canUseAtomicWait = YES;
		else
			canUseAtomicWait = NO;
	}

	return (canUseAtomicWait == NO);
}

uint32_t wakeThreadsFutex(uint32_t* uaddr, uint32_t amount)
{
	uint32_t threadsWokenUp = 0;
	// If the main thread is waiting on this address, handle this case specially.
	futexSpinLock.lock();
	if (uaddr == mainThreadWaitAddress.load())
	{
		// Notify the main thread that it can wake up.
		mainThreadWaitAddress.store(0);
		amount -= 1;
		threadsWokenUp = 1;
	}
	futexSpinLock.unlock();
	if (amount == 0)
		return threadsWokenUp;
	return threadsWokenUp + __builtin_cheerp_atomic_notify(uaddr, amount);

}

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
	assert(futex_op != FUTEX_WAKE_OP);
	assert(futex_op != FUTEX_WAIT_BITSET);
	assert(futex_op != FUTEX_WAKE_BITSET);
	assert(futex_op != FUTEX_LOCK_PI2);
	assert(futex_op != FUTEX_TRYLOCK_PI);
	assert(futex_op != FUTEX_CMP_REQUEUE_PI);
	assert(futex_op != FUTEX_WAIT_REQUEUE_PI);

	bool isBrowserMain = isBrowserMainThread();

	switch (futex_op)
	{
		case FUTEX_WAKE:
		{
			va_list ap;
			va_start(ap, futex_op);
			uint32_t val = va_arg(ap, uint32_t);
			va_end(ap);

			return wakeThreadsFutex(uaddr, val);
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
			if (isBrowserMain)
			{
				// Manually test the value at uaddr against val. If they do not match, return EAGAIN.
				futexSpinLock.lock();
				if (*uaddr != val)
				{
					futexSpinLock.unlock();
					return -EAGAIN;
				}
				mainThreadWaitAddress.store(uaddr);
				if (*uaddr != val)
				{
					mainThreadWaitAddress.store(0);
					futexSpinLock.unlock();
					return -EAGAIN;
				}
				futexSpinLock.unlock();
				int64_t startTime = 0;
				if (timeout != -1)
				{
					struct timespec startTimeStruct;
					clock_gettime(CLOCK_MONOTONIC, &startTimeStruct);
					startTime = startTimeStruct.tv_sec * 1000000000 + startTimeStruct.tv_nsec;
				}
				while (mainThreadWaitAddress.load() != 0)
				{
					if (timeout != -1)
					{
						struct timespec timeNowStruct;
						clock_gettime(CLOCK_MONOTONIC, &timeNowStruct);
						int64_t timeNow = timeNowStruct.tv_sec * 1000000000 + timeNowStruct.tv_nsec;
						if (timeNow - startTime >= timeout)
							return -ETIMEDOUT;
					}
				}
			}
			else
			{
				uint32_t ret = __builtin_cheerp_atomic_wait(uaddr, val, timeout);
				if (ret == 1)
					return -EAGAIN; // Value at uaddr did not match val.
				else if (ret == 2)
					return -ETIMEDOUT;
			}
			return 0;
		}
		case FUTEX_REQUEUE:
		case FUTEX_CMP_REQUEUE:
		{
			va_list ap;
			va_start(ap, futex_op);
			uint32_t val = va_arg(ap, uint32_t);
			uint32_t val2 = va_arg(ap, unsigned long);
			uint32_t* uaddr2 = va_arg(ap, uint32_t*);

			if (futex_op == FUTEX_CMP_REQUEUE)
			{
				uint32_t val3 = va_arg(ap, uint32_t);
				if (*uaddr != val3)
				{
					va_end(ap);
					return -EAGAIN;
				}
			}
			va_end(ap);

			// We wake up val + val2 threads. The requeued threads are spurious wake-ups.
			uint32_t threadsWokenUp = wakeThreadsFutex(uaddr, val + val2);

			if (futex_op == FUTEX_REQUEUE && threadsWokenUp >= val)
				return val;
			return threadsWokenUp;
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
	QueueMessage message;
	message.type = QueueMessageType::SPAWN_THREAD;
	message.spawnInfo.func = fp;
	message.spawnInfo.args = args;
	message.spawnInfo.tls = tls;
	message.spawnInfo.tid = newThreadId;
	message.spawnInfo.stack = stack;
	message.spawnInfo.ctid = ctid;

	threadMessagingQueue.send(message);
}

[[cheerp::genericjs]]
void waitForMessage();

[[cheerp::genericjs]]
void spawnThreadFromUtility(ThreadSpawnInfo spawnInfo)
{
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
	worker->set_onmessage(waitForMessage);
	worker->postMessage(threadingObject);
	workerList->set(spawnInfo.tid, worker);
}

[[cheerp::genericjs]]
void waitForMessage()
{
	while (true)
	{
		QueueMessage message = threadMessagingQueue.receive();
		if (message.type == QueueMessageType::SPAWN_THREAD)
		{
			spawnThreadFromUtility(message.spawnInfo);
			break;
		}
		else if (message.type == QueueMessageType::KILL_THREAD)
		{
			client::String* tidString = new client::String(message.tid);
			client::Worker* worker = workerList->get(message.tid);
			worker->terminate();
			workerList->delete_(message.tid);
		}
		else if (message.type == QueueMessageType::KILL_ALL_THREADS)
		{
			auto terminateWorker = [](client::Worker* w) {
				w->terminate();
			};
			workerList->forEach(terminateWorker);
			workerList->clear();
			client::self.close();
			break;
		}
	}
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
	workerList = new client::Map<int, client::Worker*>();
	client::setTimeout(waitForMessage, 0);
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
void startWrapper()
{
	_start();
}

[[cheerp::genericjs]]
void callStart()
{
	// We wrap the entrypoint into a try/catch for the exit exception.
	// This code is a duplication of compileEntryPoint in the CheerpWriter.
	client::EventListener* jsStart = cheerp::Callback(startWrapper);
	__asm__("try{%0()}catch(e){if(!(e instanceof CheerpException&&e.isExit&&e.code==0))throw(e);}" :: "r"(jsStart));
	__builtin_cheerp_thread_setup_resolve();
}

[[cheerp::genericjs]]
void spawnUtility()
{
	spawnUtilityThread();
	// When the utility workers sends a message, continue execution in main thread with the _start function.
	utilityWorker->addEventListener("message", callStart);
}

[[cheerp::genericjs]] [[noreturn]]
void worker_close()
{
	QueueMessage message;
	message.type = QueueMessageType::KILL_THREAD;
	client::ThreadingObject* threadingObject = __builtin_cheerp_get_threading_object();
	message.tid = threadingObject->get_tid();
	threadMessagingQueue.send(message);
	client::self.close();
	client::String* throwObj = new client::String("ThreadExit");
	__builtin_cheerp_throw(throwObj);
}

long WEAK __syscall_gettid(void)
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

[[cheerp::genericjs]]
int getHardwareConcurrency()
{
	return client::navigator.get_hardwareConcurrency();
}

long __syscall_sched_getaffinity(pid_t pid, int cpusetsize, unsigned long* mask)
{
	// Only a pid of 0 (the current process) is supported.
	assert(pid == 0);

	int amountCores = getHardwareConcurrency();
	unsigned char* set = reinterpret_cast<unsigned char*>(mask);
	for (int i = 0; i < cpusetsize; i++)
	{
		if (amountCores == 0)
			set[i] = 0;
		else if (amountCores >= 8)
		{
			set[i] = 255;
			amountCores -= 8;
		}
		else
		{
			set[i] = (1 << amountCores) - 1;
			amountCores = 0;
		}
	}
	return cpusetsize;
}

[[cheerp::genericjs]]
void killAllThreads()
{
	QueueMessage message;
	message.type = QueueMessageType::KILL_ALL_THREADS;
	threadMessagingQueue.send(message);
	utilityWorker = nullptr;
}

long __syscall_exit(long);

long __syscall_exit_group(long code)
{
	killAllThreads();
	__syscall_exit(code);
	return 0;
}

}

namespace sys_internal {

[[cheerp::genericjs]] [[noreturn]]
void closeMainThreadAsWorker()
{
	client::self.close();
	client::String* throwObj = new client::String("ExitMainThreadWorker");
	__builtin_cheerp_throw(throwObj);
}

bool exit_thread()
{
	if (!isBrowserMainThread())
	{
		// If the main thread runs in a Worker, close it.
		if (tid == 1)
			closeMainThreadAsWorker();
		else if (clear_child_tid != nullptr)
		{
			// If clear_child_tid is set, write 0 to the address it points to,
			// and do a FUTEX_WAKE on the address.
			uint32_t *wake_address = reinterpret_cast<uint32_t*>(clear_child_tid);
			*clear_child_tid = 0;
			__syscall_futex(wake_address, FUTEX_WAKE, 1, nullptr, nullptr, 0);
		}
		return true;
	}
	return false;
}

}
