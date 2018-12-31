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
		if(len < 16)
		{
			// Small, use a byte loop
			while(src8 != srcEnd)
			{
			*dst8 = *src8;
			dst8++;
			src8++;
			}
			return dst;
		}
		// Align the source operand
		if(((unsigned long)src8)&1)
		{
			*dst8 = *src8;
			dst8++;
			src8++;
		}
		if(((unsigned long)src8)&2)
		{
			*((unsigned short*)dst8) = *((unsigned short*)src8);
			dst8+=2;
			src8+=2;
		}
		// Now the source pointer is aligned, loop 4 bytes at a time
		while(src8 < srcEnd - 4)
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
