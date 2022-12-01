/****************************************************************
 *
 * Copyright (C) 2018 Alessandro Pignotti <alessandro@leaningtech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 ***************************************************************/

#include <stddef.h>

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

		switch (len)
		{
			case 0:
				return dst;
			case 1:
			case 2:
			case 3:
				*dst8 = *src8;
				if (len >= 2)
				{
					// Write second and last byte (potentially overlapping)
					*(dst8+1) = *(src8+1);
					*(dst8+len-1) = *(src8+len-1);
				}
				return dst;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				// Copy [0..3] and [len-4..len-1] ranges, potentially overlapping
				*((unsigned int*)dst8) = *((unsigned int*)src8);
				*((unsigned int*)(dst8+len-4)) = *((unsigned int*)(src8+len-4));
				return dst;
			default:
				break;
		}

		// Unroll for 64 bytes at a time
		while(int(src8) <= int(srcEnd - 64))
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
		// Loop 8 bytes at a time (leaving up to 8 bytes left to be done)
		while(int(src8) < int(srcEnd - 8))
		{
			*((unsigned long long*)dst8) = *((unsigned long long*)src8);
			dst8+=8;
			src8+=8;
		}
		// len is greater than 8, so we blindly copy [len-8..len-1] bytes in a 64 bit load + 64 bit store
		*((unsigned long long*)(((unsigned char*)dst) + len-8)) = *((unsigned long long*)(((unsigned char*)src) + len-8));
		return dst;
	}

	void *memset(void *dst, int c, size_t len)
	{
		unsigned char cu = c;
		// -1 / 255 -> 0x0001000100010001, then multiplied for something between 0 and 255 will not overlow to higher 'lanes'
		unsigned int c32 = (((unsigned int)-1)/255) * cu;
		unsigned long long c64 = ((unsigned long long)(c32)<<0) | ((unsigned long long)(c32)<<32);
		unsigned char* dst8 = (unsigned char*)dst;
		unsigned char* dstEnd = dst8 + len;

		switch (len)
		{
			case 0:
				return dst;
			case 1:
			case 2:
			case 3:
				*dst8 = cu;
				if (len >= 2)
				{
					// Write second and last byte (potentially overlapping)
					*(dst8+1) = cu;
					*(dst8+len-1) = cu;
				}
				return dst;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				// Write [0..3] and [len-4..len-1] ranges, potentially overlapping
				*((unsigned int*)dst8) = c32;
				*((unsigned int*)(dst8+len-4)) = c32;
				return dst;
			default:
				break;
		}
		while(int(dst8) < int(dstEnd - 64))
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
		while(int(dst8) <= int(dstEnd - 8))
		{
			*((unsigned long long*)dst8) = c64;
			dst8+=8;
		}
		// len is greater than 8, so we blindly write [len-8..len-1] bytes in a single 64 bit store
		*((unsigned long long*)(((unsigned char*)dst) + len-8)) = c64;
		return dst;
	}
}
