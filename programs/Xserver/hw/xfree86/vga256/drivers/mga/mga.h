/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/mga/mga.h,v 3.5 1996/12/09 11:54:16 dawes Exp $ */
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

#if defined(__alpha__)
#define mb() __asm__ __volatile__("mb": : :"memory")
#define INREG8(addr) xf86ReadSparse8(MGAMMIOBase, (addr))
#define INREG16(addr) xf86ReadSparse16(MGAMMIOBase, (addr))
#define OUTREG8(addr,val) do { xf86WriteSparse8((val),MGAMMIOBase,(addr)); \
				mb();} while(0)
#define OUTREG16(addr,val) do { {xf86WriteSparse16((val),MGAMMIOBase,(addr)); \
				mb();} while(0)
#define OUTREG(addr, val) do { xf86WriteSparse32((val),MGAMMIOBase,(addr)); \
				mb();} while(0)
#else /* __alpha__ */
#define INREG8(addr) *(volatile CARD8 *)(MGAMMIOBase + (addr))
#define INREG16(addr) *(volatile CARD16 *)(MGAMMIOBase + (addr))
#define OUTREG8(addr, val) *(volatile CARD8 *)(MGAMMIOBase + (addr)) = (val)
#define OUTREG16(addr, val) *(volatile CARD16 *)(MGAMMIOBase + (addr)) = (val)
#define OUTREG(addr, val) *(volatile CARD32 *)(MGAMMIOBase + (addr)) = (val)
#endif /* __alpha__ */

extern unsigned char *MGAMMIOBase;
#ifdef __alpha__
extern unsigned char *MGAMMIOBaseDENSE;
#endif

#define MGAWAITFIFO() while(INREG16(MGAREG_FIFOSTATUS) & 0x100);
#define MGAWAITFREE() while(INREG8(MGAREG_Status + 2) & 0x01);

#define MGAWAITFIFOSLOTS(slots) while ( ((INREG16(MGAREG_FIFOSTATUS) & 0x3f) - (slots)) < 0 );

extern int MGAinterleave;
extern int MGAusefbitblt;

/*
 * ROPs
 *
 * for some silly reason, the bits in the ROPs are just the other way round
 */

/*
 * definitions for the new acceleration interface
 */
#define WAITUNTILFINISHED()	MGAWAITFREE()
#define SETBACKGROUNDCOLOR(col)	OUTREG(MGAREG_BCOL, (col))
#define SETFOREGROUNDCOLOR(col)	OUTREG(MGAREG_FCOL, (col))
#define SETRASTEROP(rop)	mga_cmd |= (((rop & 1)==1)*8 | \
					    ((rop & 2)==2)*4 | \
					    ((rop & 4)==4)*2 | \
					    ((rop & 8)==8)) << 16;
#define SETWRITEPLANEMASK(pm)	OUTREG(MGAREG_PLNWT, (pm))
#define SETBLTXYDIR(x,y)	OUTREG(MGAREG_SGN, ((-x+1)>>1)+4*((-y+1)>>1))

/*
 * prototypes
 */
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

#endif
