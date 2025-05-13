#ifndef _IMPL_H_
#define _IMPL_H_

#include <pthread.h>
#include <cheerp/client.h>

#define WEAK __attribute__((weak))

namespace sys_internal {
	double timezone_offset();
	double real_time_now();
	double monotonic_time_now();
	double cpu_time_now();
	bool exit_thread();
}

extern _Thread_local int tid;
extern _Thread_local int *clear_child_tid;

class FutexSpinLock
{
private:
	pthread_spinlock_t spinLock;

public:
	FutexSpinLock()
	{
		pthread_spin_init(&spinLock, PTHREAD_PROCESS_PRIVATE);
	}
	~FutexSpinLock()
	{
		pthread_spin_destroy(&spinLock);
	}
	void lock()
	{
		pthread_spin_lock(&spinLock);
	}
	void unlock()
	{
		pthread_spin_unlock(&spinLock);
	}
};

struct ThreadSpawnInfo {
	int func;
	int args;
	int tls;
	int tid;
	int stack;
	int ctid;
};

enum QueueMessageType {
	SPAWN_THREAD = 0,
	KILL_THREAD,
	KILL_ALL_THREADS,
};

struct QueueMessage {
	QueueMessageType type;
	union {
		ThreadSpawnInfo spawnInfo;
		int tid;
	};
};

template <typename T>
class MessageQueue
{
private:
	T messageBuffer;
	pthread_mutex_t mutexQueue;
	pthread_cond_t condQueueEmpty;
	pthread_cond_t condQueueFull;
	bool queueFull = false;

public:
	MessageQueue()
	{
		pthread_mutex_init(&mutexQueue, NULL);
		pthread_cond_init(&condQueueEmpty, NULL);
		pthread_cond_init(&condQueueFull, NULL);
	}
	~MessageQueue()
	{
		pthread_cond_destroy(&condQueueFull);
		pthread_cond_destroy(&condQueueEmpty);
		pthread_mutex_destroy(&mutexQueue);
	}
	void send(T message)
	{
		pthread_mutex_lock(&mutexQueue);
		while (queueFull)
			pthread_cond_wait(&condQueueEmpty, &mutexQueue);

		messageBuffer = cheerp::utility::move(message);
		queueFull = true;
		pthread_cond_signal(&condQueueFull);
		pthread_mutex_unlock(&mutexQueue);
	}
	T receive()
	{
		T copiedBuffer;
		pthread_mutex_lock(&mutexQueue);
		while (!queueFull)
			pthread_cond_wait(&condQueueFull, &mutexQueue);

		copiedBuffer = cheerp::utility::move(messageBuffer);
		queueFull = false;
		pthread_cond_signal(&condQueueEmpty);
		pthread_mutex_unlock(&mutexQueue);

		return copiedBuffer;
	}
};

extern "C" {
void __syscall_main_args(int* argc, char*** argv);
bool testUseAtomicWait();
}

#endif //_IMPL_H_
