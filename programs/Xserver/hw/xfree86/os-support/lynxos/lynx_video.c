/*
 * Copyright 1993 by Thomas Mueller
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Mueller not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Mueller makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS MUELLER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS MUELLER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/lynxos/lynx_video.c,v 3.8 1998/08/29 05:43:58 dawes Exp $ */

#include "X.h"
#include "input.h"
#include "scrnintstr.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#if defined(__powerpc__)
#include <machine/absolute.h>
#endif

/***************************************************************************/
/* Video Memory Mapping section                                            */
/***************************************************************************/

typedef struct
{
	char	name[16];
	pointer	Base;
	long	Size;
	char	*ptr;
	int	RefCnt;
}
_SMEMS;

#define MAX_SMEMS	16

static _SMEMS	smems[MAX_SMEMS];


#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

static void
smemCleanup()
{
	int i;

	for (i = 0; i < MAX_SMEMS; i++) {
		if (*smems[i].name && smems[i].ptr) {
			(void)smem_create(NULL, smems[i].ptr, 0, SM_DETACH);
			(void)smem_remove(smems[i].name);
			*smems[i].name = '\0';
			smems[i].ptr = NULL;
			smems[i].Base = NULL;
			smems[i].Size = 0;
			smems[i].RefCnt = 0;
		}
	}
}

pointer
xf86MapVidMem(int ScreenNum, int Flags, pointer Base, unsigned long Size)
{
	static int once;
	int	free_slot = -1;
	int	i;

	if (!once)
	{
		atexit(smemCleanup);
		once = 1;
	}
	for (i = 0; i < MAX_SMEMS; i++)
	{
		if (!*smems[i].name && free_slot == -1)
			free_slot = i;
		if (smems[i].Base == Base && smems[i].Size == Size 
		    && *smems[i].name) {
			smems[i].RefCnt++;
			return smems[i].ptr;
		}
	}
	if (i == MAX_SMEMS && free_slot == -1)
	{
		FatalError("xf86MapVidMem: failed to smem_create Base %x Size %x (out of SMEMS entries)\n",
			Base, Size);
	}

	i = free_slot;
	sprintf(smems[i].name, "Video-%d", i);
	smems[i].Base = Base;
	smems[i].Size = Size;
	
        xf86MsgVerb(X_INFO, 3, "xf86MapVidMem: Base=0x%x Size=0x%x\n",
        	Base, Size);

#if defined(__powerpc__)
	if (((unsigned long)Base & PHYS_IO_MEM_START) != PHYS_IO_MEM_START) {
		Base = (pointer)((unsigned long)Base | PHYS_IO_MEM_START);
	}
#endif

	smems[i].ptr = smem_create(smems[i].name, Base, Size, SM_READ|SM_WRITE);
	smems[i].RefCnt = 1;
	if (smems[i].ptr == NULL)
	{
		/* check if there is a stale segment around */
		if (smem_remove(smems[i].name) == 0) {
	        	xf86Msg(X_INFO,
			    "xf86MapVidMem: removed stale smem_ segment %s\n",
		            smems[i].name);
			smems[i].ptr = smem_create(smems[i].name, 
						Base, Size, SM_READ|SM_WRITE);
		}
	        if (smems[i].ptr == NULL) {
			*smems[i].name = '\0';
			FatalError("xf86MapVidMem: failed to smem_create Base %x Size %x (%s)\n",
				Base, Size, strerror(errno));
		}
	}
        xf86MsgVerb(X_INFO, 3, "xf86MapVidMem: Base=0x%x Size=0x%x Ptr=0x%x\n",
        		 Base, Size, smems[i].ptr);
	return smems[i].ptr;
}

void
xf86UnMapVidMem(int ScreenNum, pointer Base, unsigned long Size)
{
	int	i;

	xf86MsgVerb(X_INFO, 3, "xf86UnMapVidMem: Base/Ptr=0x%x Size=0x%x\n",
		Base, Size);
	for (i = 0; i < MAX_SMEMS; i++)
	{
		if (*smems[i].name && smems[i].ptr == Base 
			&& smems[i].Size == Size)
		{
			if (--smems[i].RefCnt > 0)
				return;

			(void)smem_create(NULL, smems[i].ptr, 0, SM_DETACH);
			xf86MsgVerb(X_INFO, 3,
                           "xf86UnMapVidMem: smem_create(%s, 0x%08x, ... "
                           "SM_DETACH)\n", smems[i].name, smems[i].ptr);
			(void)smem_remove(smems[i].name);
			*smems[i].name = '\0';
			smems[i].RefCnt = 0;
			return;
		}
	}
	xf86Msg(X_WARNING,
		"xf86UnMapVidMem: no SMEM found for Base = %lx Size = %lx\n",
	       	Base, Size);
}

Bool
xf86LinearVidMem()
{
	return(TRUE);
}


/***************************************************************************/
/* Interrupt Handling section                                              */
/***************************************************************************/

Bool
xf86DisableInterrupts()
{
	return(TRUE);
}

void
xf86EnableInterrupts()
{
	return;
}

/***************************************************************************/
/* I/O Permissions section for PowerPC                                     */
/***************************************************************************/

#if defined(__powerpc__)

volatile unsigned char *ioBase = MAP_FAILED;
static int IOEnabled;

static void
removeIOSmem()
{
	smem_create(NULL, (char *) ioBase, 0, SM_DETACH);
	smem_remove("IOBASE");
	ioBase = MAP_FAILED;	
}

void
xf86EnableIO()
{
	if (IOEnabled++ == 0) {
       		ioBase = (unsigned char *) smem_create("IOBASE",
       			(char *)0x80000000, 64*1024, SM_READ|SM_WRITE);
	       	if (ioBase == MAP_FAILED) {
       			--IOEnabled;
			FatalError("xf86EnableIO: Failed to map I/O\n");
       		} else
			atexit(removeIOSmem);
	}        
	return;
}

void
xf86DisableIO()
{
	if (!IOEnabled)
		return;

        if (--IOEnabled == 0) {
        	removeIOSmem();
        }
	return;
}

void
xf86DisableIOPrivs()
{
	return;
}

void *
ppcPciIoMap(int bus)
{
	xf86EnableIO();
}

#endif

