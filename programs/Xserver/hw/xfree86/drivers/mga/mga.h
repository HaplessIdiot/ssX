/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga.h,v 1.9 1998/01/24 16:58:06 hohndel Exp $ */
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

/*
 * Macros for accessing MMIO space
 */
#if defined(__alpha__)
#define INREG8(addr) xf86ReadSparse8(MGAMMIOBase, (addr))
#define INREG16(addr) xf86ReadSparse16(MGAMMIOBase, (addr))
#define INREG(addr) xf86ReadSparse32(MGAMMIOBase, (addr))
#define OUTREG8(addr,val) do { xf86WriteSparse8((val),MGAMMIOBase,(addr)); \
				mem_barrier();} while(0)
#define OUTREG16(addr,val) do { xf86WriteSparse16((val),MGAMMIOBase,(addr)); \
				mem_barrier();} while(0)
#define OUTREG(addr, val) do { xf86WriteSparse32((val),MGAMMIOBase,(addr)); \
				mem_barrier();} while(0)

#elif defined(__powerpc__)
/* Regs must be swapped.  Could use PowerPC compat flag, but   */
/* certain regs need to be accessed as 8/16 bit values meaning */
/* the addresses need adjusting.  It's just simpler to swap    */
/* during the access using the the PPC special load/store      */
/* byte reversed instructions                                  */

#define INREG8(addr)       *(volatile CARD8 *)(MGAMMIOBase + (addr))
#define INREG16(addr)      ldw_brx(MGAMMIOBase, (addr))
#define INREG(addr)        ldl_brx(MGAMMIOBase, (addr))
#define OUTREG8(addr,val)  do { *((volatile CARD8 *)(MGAMMIOBase + (addr))) = (val); mem_barrier(); } while (0)
#define OUTREG16(addr,val) do { stw_brx((CARD16)(val),MGAMMIOBase,(addr)); mem_barrier(); } while (0)
#define OUTREG(addr,val)   do { stl_brx((CARD32)(val),MGAMMIOBase,(addr)); mem_barrier(); } while (0)

#else

#define INREG8(addr) *(volatile CARD8 *)(MGAMMIOBase + (addr))
#define INREG16(addr) *(volatile CARD16 *)(MGAMMIOBase + (addr))
#define INREG(addr) *(volatile CARD32 *)(MGAMMIOBase + (addr))
#define OUTREG8(addr, val) *(volatile CARD8 *)(MGAMMIOBase + (addr)) = (val)
#define OUTREG16(addr, val) *(volatile CARD16 *)(MGAMMIOBase + (addr)) = (val)
#define OUTREG(addr, val) *(volatile CARD32 *)(MGAMMIOBase + (addr)) = (val)

#endif /* __alpha__ */

#define VGAOUTREG8(addr,val)  OUTREG8((0x1c00+addr),val)
#define VGAOUTREG16(addr,val) OUTREG16((0x1c00+addr),val)
#define VGAINREG8(addr)       INREG8((0x1c00+addr))
#define VGAINREG16(addr)      INREG16((0x1c00+addr))

#define MGAISBUSY() (INREG8(MGAREG_Status + 2) & 0x01)
#define MGAWAITFIFO() while(INREG16(MGAREG_FIFOSTATUS) & 0x0100)
#define MGAWAITFREE() while(MGAISBUSY())
#define MGAWAITFIFOSLOTS(slots) while (((INREG16(MGAREG_FIFOSTATUS) & 0x3f) - (slots)) < 0)

typedef struct {
    Bool	isHwCursor;
    int		CursorMaxWidth;
    int 	CursorMaxHeight;
    int		CursorFlags;
    Bool	(*UseHWCursor)();
    void	(*LoadCursorImage)();
    void	(*ShowCursor)();
    void	(*HideCursor)();
    void	(*SetCursorPosition)();
    void	(*SetCursorColors)();
    long	maxPixelClock;
    long	MemoryClock;
} MGARamdacRec;

extern MGARamdacRec MGAdac;
extern PCITAG MGAPciTag;
extern int MGAchipset;
extern int MGArev;
extern int MGAinterleave;
extern int MGABppShft;
extern int MGAusefbitblt;
extern int MGAydstorg;
extern unsigned char *MGAMMIOBase;
#ifdef __alpha__
extern unsigned char *MGAMMIOBaseDENSE;
#endif


extern void Mga8AccelInit();
extern void Mga16AccelInit();
extern void Mga24AccelInit();
extern void Mga32AccelInit();
extern void MGAStormAccelInit();
extern void MGAStormSync();
extern void MGAStormEngineInit();
extern void MGA3026RamdacInit();
extern void MGA1064RamdacInit();

/*
 * definitions for the new acceleration interface
 */
#define WAITUNTILFINISHED()	MGAWAITFREE()
#define SETBACKGROUNDCOLOR(col)	OUTREG(MGAREG_BCOL, (col))
#define SETFOREGROUNDCOLOR(col)	OUTREG(MGAREG_FCOL, (col))
#define SETRASTEROP(rop)	mga_cmd |= MGARop[rop]
#define SETWRITEPLANEMASK(pm)	OUTREG(MGAREG_PLNWT, (pm))
#define SETBLTXYDIR(x,y)	OUTREG(MGAREG_SGN, ((-x+1)>>1)+4*((-y+1)>>1))

#endif
