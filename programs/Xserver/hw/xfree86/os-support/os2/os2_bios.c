/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/os2/os2_bios.c,v 3.0 1995/03/11 14:15:20 dawes Exp $ */
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
#define INCL_32
#define INCL_DOS
#define INCL_DOSFILEMGR
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Read BIOS via TESTCFG.SYS device driver
 */

#define Bios_Base 0

int xf86ReadBIOS(Base, Offset, Buf, Len)
unsigned long Base;
unsigned long Offset;
unsigned char *Buf;
int Len;
{
	HFILE fd;
	struct {
		ULONG cmd;
		ULONG phys;
		USHORT len;
	} par;
	UCHAR	*dta;
	ULONG	plen,dlen;
	int 	i;
	ULONG	action;

	/* open the special device (default with OS/2) */
	if (DosOpen((PSZ)"\\DEV\\TESTCFG$", (PHFILE)&fd, (PULONG)&action,
	   (ULONG)0, FILE_SYSTEM, FILE_OPEN,
	   OPEN_SHARE_DENYNONE|OPEN_FLAGS_NOINHERIT|OPEN_ACCESS_READONLY,
	   (ULONG)0) != 0) {
		FatalError("xf86ReadBIOS: install DEVICE=path\\TESTCFG.SYS!");
		return -1;
	}

	/* prepare parameter and data packets for ioctl */
	par.cmd 	= 0;
	par.phys 	= (ULONG)Bios_Base+(Offset & 0xffff8000);
	par.len 	= (Offset & 0x7fff) + Len;
	plen 		= sizeof(par);

	dta		= (UCHAR*)xalloc(par.len);
	dlen 		= Len;

	/* issue call to get a readonly copy of BIOS ROM */
	if (DosDevIOCtl(fd, (ULONG)0x80, (ULONG)0x40,
	   (PVOID)&par, (ULONG)plen, (PULONG)&plen,
	   (PVOID)dta, (ULONG)dlen, (PULONG)&dlen)) {
		FatalError("xf86ReadBIOS: BIOS map failed, addr=%lx\n", Bios_Base+Offset);
		DosClose(fd);
		return -1;
	}

	/*
 	 * Sanity check...
 	 */
	if ((Offset & 0x7fff) != 0 && 
		(dta[0] != 0x55 || dta[1] != 0xaa)) {
		FatalError("BIOS sanity check failed, addr=%x\n",
			Bios_Base+Offset);
		DosClose(fd);
		return -1;
	}

	/* copy data to buffer */
	memcpy(Buf,dta + (Offset & 0x7fff), Len);
	xfree(dta);

	/* close device */
	DosClose(fd);

	return(Len);
}
