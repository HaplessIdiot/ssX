/* $XFree86$ */
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

/* This section uses a special IBM unsupported device driver function.
 * The driver SMVDD.SYS is supplied for reference in this directory.
 * You must install it with a line DEVICE=path\SMVDD.SYS in config.sys.
 */

static HFILE mapdev = -1;
static char* mappath = "\\DEV\\SMVDD01$";
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
 * ioctl of smvdd.sys
 */
typedef struct {
	ULONG	hstream;
	ULONG 	hid;
	ULONG	flag;
	ULONG	addr;
	ULONG	size;
} DIOParPkt;

/* this structure is used as a return value for the direct access ioctl
 * of smvdd.sys. Attention this structure is unaligned and packed!
 */
typedef struct {
	USHORT	lin[3]; /* this is actually USHORT,ULONG but unpacked */
} DIODtaPkt;

typedef union {
	ULONG l;
	USHORT s[2];
} S2L;

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

	par.hstream	= 0;
	par.hid		= 0;
	par.flag	= 1l;
	par.addr	= (ULONG)Base;
	par.size	= (ULONG)Size;
	plen 		= sizeof(par);

	open_mmap();
	if (mapdev == -1)
		FatalError("xf86MapVidMem: install DEVICE=path\\SMVDD.SYS!");

	if (DosDevIOCtl(mapdev, (ULONG)0x81, (ULONG)0x42,
	      (PVOID)&par, (ULONG)plen, (PULONG)&plen,
	      (PVOID)&dta, (ULONG)6, (PULONG)&dlen) == 0) {
		S2L x;
		x.s[0] = dta.lin[1];
		x.s[1] = dta.lin[2];
		if (dlen==6)
			return (pointer)x.l;
		/*else fail*/
	}

	/* fail */
	ErrorF("xf86MapVidMem: (ScreenNum= %d, Base= %p, Size= 0x%x\n",
		ScreenNum, Base, Size);
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

	par.hstream	= 0l;
	par.hid		= 0l;
	par.flag	= 0l;
	par.addr	= (ULONG)Base;
	par.size	= 0l;
	plen 		= sizeof(par);

	if (mapdev != -1)
	    DosDevIOCtl(mapdev, (ULONG)0x81, (ULONG)0x42,
	      (PVOID)&par, (ULONG)plen, (PULONG)&plen,
	      NULL, 0, NULL);
	close_mmap();
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
