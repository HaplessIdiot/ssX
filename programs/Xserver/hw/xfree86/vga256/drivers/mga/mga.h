/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/mga/mga.h,v 3.0 1996/09/29 13:40:22 dawes Exp $ */

/*
 * MGA Millennium (MGA2064W) functions
 *
 * Copyright 1996 The XFree86 Project, Inc.
 *
 * Authors
 *		Dirk Hohndel
 *			hohndel@XFree86.Org
 *		David Dawes
 *			dawes@XFree86.Org
 */

#ifndef MGA_H
#define MGA_H

#define MGAREG8(addr) *(volatile CARD8 *)(MGAMMIOBase + (addr))
#define MGAREG16(addr) *(volatile CARD16 *)(MGAMMIOBase + (addr))
#define MGAREG32(addr) *(volatile CARD32 *)(MGAMMIOBase + (addr))
#define OUTREG(addr, val) MGAREG32(addr) = (val)

extern unsigned char *MGAMMIOBase;
extern int MGAScrnWidth;

void
mgaLine (
#ifdef NeedFunctionPrototypes
	 DrawablePtr pDrawable, 
	 GCPtr pGC, 
	 int mode, 
	 int npt, 
	 DDXPointPtr pptInit
#endif
);

int
MGAWaitForBlitter(
#ifdef NeedFunctionPrototypes
	void
#endif
);
#endif
