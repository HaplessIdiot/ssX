/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/os2/os2_video.c,v 3.0 1995/03/11 14:15:28 dawes Exp $ */
/*
 * (c) Copyright 1994 by Holger Veit
 *			<Holger.Veit@gmd.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * HOLGER VEIT  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Holger Veit shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Holger Veit.
 *
 */

#include "X.h"
#include "input.h"
#include "scrnintstr.h"

#define I_NEED_OS2_H
#define INCL_DOSFILEMGR
#include "xf86.h"
#include "xf86Priv.h"

/***************************************************************************/
/* Video Memory Mapping helper functions                                   */
/***************************************************************************/

/* This section uses the xf86sup.sys driver developed for xfree86.
 * The driver allows mapping of physical memory
 * You must install it with a line DEVICE=path\xf86sup.sys in config.sys.
 */

static HFILE mapdev = -1;
static char* mappath = "\\DEV\\PMAP$";
static HFILE open_mmap() 
{
	APIRET rc;
	ULONG action;

	if (mapdev != -1)
		return mapdev;

	rc = DosOpen((PSZ)mappath, (PHFILE)&mapdev, (PULONG)&action,
	   (ULONG)0, FILE_SYSTEM, FILE_OPEN,
	   OPEN_SHARE_DENYNONE|OPEN_FLAGS_NOINHERIT|OPEN_ACCESS_READONLY,
	   (ULONG)0);
	if (rc!=0)
		mapdev = -1;
	return mapdev;
}

static void close_mmap()
{
	if (mapdev != -1)
		DosClose(mapdev);
	mapdev = -1;
}

/* this structure is used as a parameter packet for the direct access
 * ioctl of pmap$
 */

/* Changed here for structure of driver PMAP$ */

typedef struct{
	ULONG addr;
	ULONG size;
	} DIOParPkt;

/* This is the data packet for the mapping function */

typedef struct {
	ULONG addr;
	USHORT sel;
	} DIODtaPkt;

/***************************************************************************/
/* Video Memory Mapping section                                            */
/***************************************************************************/

/* ARGSUSED */
pointer xf86MapVidMem(ScreenNum, Region, Base, Size)
int ScreenNum;
int Region;
pointer Base;
unsigned long Size;
{
	DIOParPkt	par;
	ULONG		plen;
	DIODtaPkt	dta;
	ULONG		dlen;

	par.addr	= (ULONG)Base;
	par.size	= (ULONG)Size;
	plen 		= sizeof(par);
	dlen		= sizeof(dta);

	open_mmap();
	if (mapdev == -1)
		FatalError("xf86MapVidMem: install DEVICE=path\\XF86SUP.SYS!");

	if (DosDevIOCtl(mapdev, (ULONG)0x76, (ULONG)0x44,
	      (PVOID)&par, (ULONG)plen, (PULONG)&plen,
	      (PVOID)&dta, (ULONG)dlen, (PULONG)&dlen) == 0) {
		ErrorF("xf86MapVidMem: (ScreenNum= %d, Base= %p, Size= 0x%x\n",
		ScreenNum, Base, Size);
		if (dlen==sizeof(dta))
			return (pointer)dta.addr;
		/*else fail*/
	}

	/* fail */
	ErrorF("xf86MapVidMem FAILED!!: (ScreenNum= %d, Base= %p, Size= 0x%x return len %d\n",
		ScreenNum, Base, Size,dlen);
	return (pointer)0;
}

/* ARGSUSED */
void xf86UnMapVidMem(ScreenNum, Region, Base, Size)
int ScreenNum;
int Region;
pointer Base;
unsigned long Size;
{
	DIOParPkt	par;
	ULONG		plen;

/* We need here the VIRTADDR for unmapping, not the physical address      */
/* This should be taken care of either here by keeping track of allocated */
/* pointers, but this is also already done in the driver... Thus it would */
/* be a waste to do this tracking twice. Can this be changed when the fn. */
/* is called? This would require tracking this function in all servers,   */
/* and changing it appropriately to call this with the virtual adress	  */
/* If the above mapping function is only called once, then we can store   */
/* the virtual adress and use it here.... 				  */
	
	par.addr	= (ULONG)Base;
	par.size	= Size;
	plen 		= sizeof(par);

	if (mapdev != -1)
	    DosDevIOCtl(mapdev, (ULONG)0x76, (ULONG)0x45,
	      (PVOID)&par, (ULONG)plen, (PULONG)&plen,
	      NULL, 0, NULL);

/* Now if more than one region has been allocated and we close the driver, *
 * the other pointers will immediately become invalid. We avoid closing    *
 * driver for now, but this should be fixed                                */
 
	/*close_mmap();*/
}

Bool xf86LinearVidMem()
{
	/* setting it to true needs further testing */
	return(FALSE);
}

/***************************************************************************/
/* Interrupt Handling section                                              */
/***************************************************************************/

Bool xf86DisableInterrupts()
{
	/* no interrupt disabling allowed */
	return(FALSE);
}

void xf86EnableInterrupts()
{
	/*nothing*/
	return;
}
