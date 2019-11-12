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
		while(((unsigned long)dst8)&3)
		{
			if(len == 0)
				return dst;
			len--;
			*dst8 = *src8;
			dst8++;
			src8++;
		}
		unsigned char* srcEnd = src8+len;
		// Now the dest pointer is aligned
		// Unroll for 64 bytes at a time
		while(int(src8) <= int(srcEnd - 64))
		{
			*((unsigned int*)dst8) = *((unsigned int*)src8);
			*((unsigned int*)(dst8+4)) = *((unsigned int*)(src8+4));
			*((unsigned int*)(dst8+8)) = *((unsigned int*)(src8+8));
			*((unsigned int*)(dst8+12)) = *((unsigned int*)(src8+12));
			*((unsigned int*)(dst8+16)) = *((unsigned int*)(src8+16));
			*((unsigned int*)(dst8+20)) = *((unsigned int*)(src8+20));
			*((unsigned int*)(dst8+24)) = *((unsigned int*)(src8+24));
			*((unsigned int*)(dst8+28)) = *((unsigned int*)(src8+28));
			*((unsigned int*)(dst8+32)) = *((unsigned int*)(src8+32));
			*((unsigned int*)(dst8+36)) = *((unsigned int*)(src8+36));
			*((unsigned int*)(dst8+40)) = *((unsigned int*)(src8+40));
			*((unsigned int*)(dst8+44)) = *((unsigned int*)(src8+44));
			*((unsigned int*)(dst8+48)) = *((unsigned int*)(src8+48));
			*((unsigned int*)(dst8+52)) = *((unsigned int*)(src8+52));
			*((unsigned int*)(dst8+56)) = *((unsigned int*)(src8+56));
			*((unsigned int*)(dst8+60)) = *((unsigned int*)(src8+60));
			dst8+=64;
			src8+=64;
		}
		// Loop 4 bytes at a time
		while(int(src8) <= int(srcEnd - 4))
		{
			*((unsigned int*)dst8) = *((unsigned int*)src8);
			dst8+=4;
			src8+=4;
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
