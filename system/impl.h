#ifndef _IMPL_H_
#define _IMPL_H_

#define WEAK __attribute__((weak))

namespace sys_internal {
	double timezone_offset();
	double real_time_now();
	double monotonic_time_now();
	double cpu_time_now();
}

extern "C" {
void __syscall_main_args(int* argc, char*** argv);
}

#endif //_IMPL_H_
