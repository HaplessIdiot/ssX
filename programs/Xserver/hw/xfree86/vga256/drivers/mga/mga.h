/* $XFree86$ */

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

#define outlPCI(addr, val)	pciWriteLong(MGAPciTag, addr, val)
#define inlPCI(addr)		pciReadLong(MGAPciTag, addr)

#define OUTREG(addr, val) *(volatile CARD32 *)(MGAMMIOBase + (addr)) = (val)

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
