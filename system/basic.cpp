// Copyright 2023-2025 Leaning Technologies

#include <cstdarg>
#include <errno.h>
#include "impl.h"

_Thread_local int tid = 1;
_Thread_local int *clear_child_tid = nullptr;

extern "C" {

#ifdef __ASMJS__
// The value of this variables will be rewritten to the correct heap start
// and end by the compiler backend
char* volatile _heapStart = (char*)0xdeadbeef;
char* volatile _heapEnd = (char*)0xdeadbeef;
#endif

#ifdef __ASMJS__
[[cheerp::wasm]]
#endif
long WEAK __syscall_set_thread_area(unsigned long tp)
{
#if defined(__CHEERP__) && defined(__ASMJS__)
	__builtin_cheerp_set_thread_pointer(tp);
#endif
	return 0;
}

long WEAK __syscall_futex(uint32_t* uaddr, int futex_op, ...)
{
	va_list args;
	va_start(args, futex_op);
	int ret = sys_internal::futex_wrapper(uaddr, futex_op, args);
	va_end(args);
	return ret;
}

}

namespace sys_internal {

long WEAK futex(uint32_t* uaddr, int futex_op, bool canUseAtomics, va_list args)
{
	return -ENOSYS;
}

} // namespace sys_internal
