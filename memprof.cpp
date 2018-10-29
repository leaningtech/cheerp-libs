// Copyright 2018 Leaning Technologies Ltd. All Rights Reserved.

#include <cheerp/client.h>

extern "C"
{
	// We redefine internal symbols here
	__attribute__((cheerp_wasm)) void* _malloc_r(void* reent, size_t size);
	__attribute__((cheerp_wasm)) void* _realloc_r(void* reent, void* ptr, size_t size);
	__attribute__((cheerp_wasm)) void* _calloc_r(void* reent, size_t nmemb, size_t size);
	void __attribute__((cheerp_wasm)) _free_r(void* reent, void* ptr);
	extern __attribute__((cheerp_wasm)) void* mALLOc(void* reent, size_t size);
	extern __attribute__((cheerp_wasm)) void* rEALLOc(void* reent, void* ptr, size_t size);
	extern __attribute__((cheerp_wasm)) void* cALLOc(void* reent, size_t nmemb, size_t size);
	extern void __attribute__((cheerp_wasm)) fREe(void* reent, void* ptr);
}

void registerAlloc(uintptr_t ptr, uintptr_t size)
{
	client::Error* e = new client::Error();
	client::String* s = e->get_stack();
	client::console.log("ALLOC", ptr, size, "STACK", s);
}

void registerFree(uintptr_t ptr)
{
	client::console.log("FREE", ptr);
}

__attribute__((cheerp_wasm)) void* _malloc_r(void* reent, size_t size)
{
	void* ret = mALLOc(reent, size);
	registerAlloc(reinterpret_cast<uintptr_t>(ret), size);
	return ret;
}

__attribute__((cheerp_wasm)) void* _realloc_r(void* reent, void* ptr, size_t size)
{
	registerFree(reinterpret_cast<uintptr_t>(ptr));
	void* ret = rEALLOc(reent, ptr, size);
	registerAlloc(reinterpret_cast<uintptr_t>(ret), size);
	return ret;
}

__attribute__((cheerp_wasm)) void* _calloc_r(void* reent, size_t nmemb, size_t size)
{
	void* ret = cALLOc(reent, nmemb, size);
	registerAlloc(reinterpret_cast<uintptr_t>(ret), nmemb*size);
	return ret;
}

__attribute__((cheerp_wasm)) void _free_r(void* reent, void* ptr)
{
	fREe(reent, ptr);
	registerFree(reinterpret_cast<uintptr_t>(ptr));
}
