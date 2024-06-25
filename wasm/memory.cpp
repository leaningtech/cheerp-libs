//===-- memory.cpp - Wasm-optimized memory functions --------------===//
//
//                     Cheerp: The C++ compiler for the Web
//
// This file is distributed under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE.TXT for details.
//
// Copyright 2018-2023 Leaning Technologies
//
//===----------------------------------------------------------------------===//

#include <stddef.h>
#include <stdint.h>

extern "C"
{
	/** Implement a faster variant of memcpy, specialized for wasm
	 *  At link time this will replace the 'weak' symbol for memcpy
	 *  provided in the lib C
	 */
	void* memcpy(void* const dst, void* const src, size_t len)
	{
		// Wasm supports unaligned load/stores, take advantage of this for a faster memcpy
		unsigned char* src8 = (unsigned char*)src;
		unsigned char* dst8 = (unsigned char*)dst;

		unsigned char* srcEnd = src8+len;

		while(((unsigned long)dst8)&3)
		{
			if(src8 == srcEnd)
				return dst;
			*dst8 = *src8;
			dst8++;
			src8++;
		}
		// Loop conditions below will underflow for too small
		// lengths and NULL pointers
		if(len >= 64)
		{
			// Now the dest pointer is aligned
			// Unroll for 64 bytes at a time
			while(uintptr_t(src8) <= uintptr_t(srcEnd - 64))
			{
				*((unsigned long long*)dst8) = *((unsigned long long*)src8);
				*((unsigned long long*)(dst8+8)) = *((unsigned long long*)(src8+8));
				*((unsigned long long*)(dst8+16)) = *((unsigned long long*)(src8+16));
				*((unsigned long long*)(dst8+24)) = *((unsigned long long*)(src8+24));
				*((unsigned long long*)(dst8+32)) = *((unsigned long long*)(src8+32));
				*((unsigned long long*)(dst8+40)) = *((unsigned long long*)(src8+40));
				*((unsigned long long*)(dst8+48)) = *((unsigned long long*)(src8+48));
				*((unsigned long long*)(dst8+56)) = *((unsigned long long*)(src8+56));
				dst8+=64;
				src8+=64;
			}
			// Loop 8 bytes at a time
			while(uintptr_t(src8) <= uintptr_t(srcEnd - 8))
			{
				*((unsigned long long*)dst8) = *((unsigned long long*)src8);
				dst8+=8;
				src8+=8;
			}
		}
		// Byte loop to finish the copy
		while(src8 != srcEnd)
		{
			*dst8 = *src8;
			dst8++;
			src8++;
		}
		return dst;
	}

	void *memset(void *dst, int c, size_t len)
	{
		unsigned char cu = c;
		unsigned int c32 = (cu<<0) | (cu<<8) | (cu<<16) | (cu<<24);
		unsigned long long c64 = ((unsigned long long)(c32)<<0) | ((unsigned long long)(c32)<<32);
		unsigned char* dst8 = (unsigned char*)dst;
		unsigned char* dstEnd = dst8 + len;
		if(len >= 64)
		{
			while(uintptr_t(dst8) < uintptr_t(dstEnd - 64))
			{
				*((unsigned long long*)dst8) = c64;
				*((unsigned long long*)(dst8+8)) = c64;
				*((unsigned long long*)(dst8+16)) = c64;
				*((unsigned long long*)(dst8+24)) = c64;
				*((unsigned long long*)(dst8+32)) = c64;
				*((unsigned long long*)(dst8+40)) = c64;
				*((unsigned long long*)(dst8+48)) = c64;
				*((unsigned long long*)(dst8+56)) = c64;
				dst8+=64;
			}
			// Loop 8 bytes at a time
			while(uintptr_t(dst8) <= uintptr_t(dstEnd - 8))
			{
				*((unsigned long long*)dst8) = c64;
				dst8+=8;
			}
		}
		// Byte loop to finish the copy
		while(dst8 != dstEnd)
		{
			*dst8 = cu;
			dst8++;
		}
		return dst;
	}
}
