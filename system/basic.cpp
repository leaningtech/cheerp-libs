// Copyright 2023-2025 Leaning Technologies

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
long __syscall_set_thread_area(unsigned long tp)
{
#if defined(__CHEERP__) && defined(__ASMJS__)
	__builtin_cheerp_set_thread_pointer(tp);
#endif
	return 0;
}

}
