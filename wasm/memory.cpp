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

		while(((unsigned long)dst8)&3)
		{
			if(src8 == srcEnd)
				return dst;
			*dst8 = *src8;
			dst8++;
			src8++;
		}
		// Now the dest pointer is aligned
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
		// Loop 8 bytes at a time
		while(int(src8) <= int(srcEnd - 8))
		{
			*((unsigned long long*)dst8) = *((unsigned long long*)src8);
			dst8+=8;
			src8+=8;
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
}
