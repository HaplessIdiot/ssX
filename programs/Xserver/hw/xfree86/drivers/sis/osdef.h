/* $XFree86$ */

/* OS depending defines */

/* Copyright (C) 2001-2004 by Thomas Winischhofer, Vienna, Austria
 *
 * If distributed outside the scope of XFree86 (such as but only exclusively the
 * Linux kernel), the following license terms apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License as published by
 * * the Free Software Foundation; either version 2 of the License, or
 * * any later version.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * As a part of XFree86 code, the following terms apply:
 *
 * * Permission to use, copy, modify, distribute, and sell this software and its
 * * documentation for any purpose is hereby granted without fee, provided that
 * * the above copyright notice appears in all copies and that both that copyright
 * * notice and this permission notice appear in supporting documentation, and
 * * and that the name of the copyright holder not be used in advertising
 * * or publicity pertaining to distribution of the software without specific,
 * * written prior permission. The copyright holder makes no representations
 * * about the suitability of this software for any purpose.  It is provided
 * * "as is" without expressed or implied warranty.
 * *
 * * THE COPYRIGHT HOLDER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * * EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: 	Thomas Winischhofer <thomas@winischhofer.net>
 *
 */
 
/* The choices are: */
/* #define LINUX_KERNEL	 */  	/* Kernel framebuffer */
#define LINUX_XF86    		/* XFree86 */

/**********************************************************************/
#ifdef LINUX_KERNEL /* ----------------------------*/
#include <linux/config.h>

#ifdef CONFIG_FB_SIS_300
#define SIS300
#endif

#ifdef CONFIG_FB_SIS_315
#define SIS315H
#endif

#if 1
#define SISFBACCEL	/* Include 2D acceleration */
#endif

#endif

#ifdef LINUX_XF86 /* ----------------------------- */
#define SIS300
#define SIS315H
#endif

/**********************************************************************/
#ifdef LINUX_KERNEL
#define SiS_SetMemory(MemoryAddress,MemorySize,value) memset(MemoryAddress, value, MemorySize)
#define SiS_MemoryCopy(Destination,Soruce,Length) memcpy(Destination,Soruce,Length)
#endif

#ifdef LINUX_XF86
#define SiS_SetMemory(MemoryAddress,MemorySize,value) memset(MemoryAddress, value, MemorySize)
#define SiS_MemoryCopy(Destination,Soruce,Length) memcpy(Destination,Soruce,Length)
#endif

/**********************************************************************/

#ifdef OutPortByte
#undef OutPortByte
#endif /* OutPortByte */

#ifdef OutPortWord
#undef OutPortWord
#endif /* OutPortWord */

#ifdef OutPortLong
#undef OutPortLong
#endif /* OutPortLong */

#ifdef InPortByte
#undef InPortByte
#endif /* InPortByte */

#ifdef InPortWord
#undef InPortWord
#endif /* InPortWord */

#ifdef InPortLong
#undef InPortLong
#endif /* InPortLong */

/**********************************************************************/
/*  LINUX KERNEL                                                      */
/**********************************************************************/

#ifdef LINUX_KERNEL
#define OutPortByte(p,v) outb((u8)(v),(u16)(p))
#define OutPortWord(p,v) outw((u16)(v),(u16)(p))
#define OutPortLong(p,v) outl((u32)(v),(u16)(p))
#define InPortByte(p)    inb((u16)(p))
#define InPortWord(p)    inw((u16)(p))
#define InPortLong(p)    inl((u16)(p))
#endif

/**********************************************************************/
/*  LINUX XF86                                                        */
/**********************************************************************/

#ifdef LINUX_XF86
#define OutPortByte(p,v) outb((IOADDRESS)(p),(CARD8)(v))
#define OutPortWord(p,v) outw((IOADDRESS)(p),(CARD16)(v))
#define OutPortLong(p,v) outl((IOADDRESS)(p),(CARD32)(v))
#define InPortByte(p)    inb((IOADDRESS)(p))
#define InPortWord(p)    inw((IOADDRESS)(p))
#define InPortLong(p)    inl((IOADDRESS)(p))
#endif

