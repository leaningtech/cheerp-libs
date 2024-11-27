#ifndef _IMPL_H_
#define _IMPL_H_

#include <pthread.h>
#include <utility>
#include <iostream>
#include <cheerp/client.h>

#define WEAK __attribute__((weak))

namespace sys_internal {
	double timezone_offset();
	double real_time_now();
	double monotonic_time_now();
	double cpu_time_now();
	bool exit_thread();
}

struct ThreadSpawnInfo {
	int func;
	int args;
	int tls;
	int tid;
	int stack;
	int ctid;
};

extern "C" [[cheerp::genericjs]] void pushDebug(int nr);

template <typename T>
class MessageQueue
{
private:
	T messageBuffer;
	pthread_mutex_t mutexQueue;
	pthread_cond_t condQueueEmpty;
	pthread_cond_t condQueueFull;
	bool queueFull = false;
	int value;

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
	int getValue()
	{
		return value;
	}
	int setValue(int val)
	{
		value = val;
	}
	void send(T message)
	{
		pushDebug(10);
		pthread_mutex_lock(&mutexQueue);
		for (int i = 0; i < 800000000; i++)
			value *= value;
		pushDebug(11);
		while (queueFull)
		{
			pushDebug(12);
			pthread_cond_wait(&condQueueEmpty, &mutexQueue);
			pushDebug(13);
		}
		pushDebug(14);

		messageBuffer = std::move(message);
		queueFull = true;
		pthread_cond_signal(&condQueueFull);
		pthread_mutex_unlock(&mutexQueue);
		pushDebug(15);
	}
	T receive()
	{
		pushDebug(0);
		T copiedBuffer;
		pthread_mutex_lock(&mutexQueue);
		pushDebug(1);
		while (!queueFull)
		{
			pushDebug(2);
			pthread_cond_wait(&condQueueFull, &mutexQueue);
			pushDebug(3);
		}
		pushDebug(4);

		copiedBuffer = std::move(messageBuffer);
		queueFull = false;
		pthread_cond_signal(&condQueueEmpty);
		pthread_mutex_unlock(&mutexQueue);
		pushDebug(5);

		return copiedBuffer;
	}
};

extern "C" {
void __syscall_main_args(int* argc, char*** argv);
}

#endif //_IMPL_H_
