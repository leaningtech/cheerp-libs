// Copyright 2023-2025 Leaning Technologies

extern "C" {

#if defined(__CHEERP__) && defined(__ASMJS__)
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
