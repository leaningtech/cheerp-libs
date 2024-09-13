#ifndef _IMPL_H_
#define _IMPL_H_

#define WEAK __attribute__((weak))

#ifdef __ASMJS__
#define THREAD_LOCAL _Thread_local
#else
#define THREAD_LOCAL
#endif

namespace sys_internal {
	double timezone_offset();
	double real_time_now();
	double monotonic_time_now();
	double cpu_time_now();
}

extern THREAD_LOCAL int tid;
extern THREAD_LOCAL int *clear_child_tid;

extern "C" {
void __syscall_main_args(int* argc, char*** argv);
}

#endif //_IMPL_H_
