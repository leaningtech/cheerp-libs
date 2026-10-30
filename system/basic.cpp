// Copyright 2023-2025 Leaning Technologies

#include <errno.h>
#include "impl.h"

_Thread_local int tid = 1;
_Thread_local int *clear_child_tid = nullptr;

// The dso_handle is used internally to call the __cxa_atexit function for global destructors
// But __cxa_atexit ignores it, so we can just set it to NULL.
void* __dso_handle = NULL;

extern "C" {

#ifdef __ASMJS__
// The value of this variables will be rewritten to the correct heap start
// and end by the compiler backend
char* volatile _heapStart = (char*)0xdeadbeef;
char* volatile _heapEnd = (char*)0xdeadbeef;
#endif

}

namespace sys_internal {

long WEAK futex(uint32_t* uaddr, int futex_op, uint32_t val, const struct timespec* timeout, uint32_t* uaddr2, uint32_t val3, bool canUseAtomics)
{
	return -ENOSYS;
}

} // namespace sys_internal
