/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cirBlitMM.h,v 1.1 1997/03/06 23:15:12 hohndel Exp $ */

/* Definitions for BitBLT engine communication. */
/* Using Memory-Mapped I/O. */

/* $XConsortium: cirBlitMM.h /main/5 1996/10/25 10:30:29 kaleb $ */

#if !defined(__STDC__) && !defined(__GNUC__)

/* If we don't have volatile, MMIO isn't used, but we compile anyway. */

#ifdef volatile
#undef volatile
#endif
#define volatile /**/

#endif

/* BitBLT modes. */

#define FORWARDS		0x00
#define BACKWARDS		0x01
#define SYSTEMDEST		0x02
#define SYSTEMSRC		0x04
#define TRANSPARENCYCOMPARE	0x08
#define PIXELWIDTH16		0x10
#define PIXELWIDTH24		0x20	/* 5436/46 only. */
#define PIXELWIDTH32		0x30	/* 5434/36/46 only. */
#define PATTERNCOPY		0x40
#define COLOREXPAND		0x80

/* Extended BitBLT modes (36/46 only). */

#define SOURCEDWORDGRANULARITY	0x01
#define INVERTSOURCESENSE	0x02
#define SOLIDCOLORFILL		0x04

/* MMIO addresses (offset from 0xb8000). */

#define MMIOBACKGROUNDCOLOR	0x00
#define MMIOFOREGROUNDCOLOR	0x04
#define MMIOWIDTH		0x08
#define MMIOHEIGHT		0x0a
#define MMIODESTPITCH		0x0c
#define MMIOSRCPITCH		0x0e
#define MMIODESTADDR		0x10
#define MMIOSRCADDR		0x14
#define MMIOBLTWRITEMASK	0x17
#define MMIOBLTMODE		0x18
#define MMIOROP			0x1a
#define MMIOBLTMODEEXT		0x1b
#define MMIOTRANSPARENCYCOLOR   0x1c
#define MMIOTRANSPARENCYCOLORMASK 0x20
#define MMIOBLTSTATUS		0x40

/* Address: the 5426 adresses 2MBytes, the 5434 can address 4MB. */

#define SETDESTADDR(dstAddr) 	CIR_MMIO_WRITE32(MMIODESTADDR, dstAddr)

#define SETSRCADDR(srcAddr)  	CIR_MMIO_WRITE32(MMIOSRCADDR, srcAddr)

#define SETSRCADDRUNMODIFIED SETSRCADDR

/* Pitch: the 5426 goes up to 4095, the 5434 can do 8191. */

#define SETDESTPITCH(dstPitch)	CIR_MMIO_WRITE16(MMIODESTPITCH, dstPitch)

#define SETSRCPITCH(srcPitch)	CIR_MMIO_WRITE16(MMIOSRCPITCH, srcPitch)

/* Width: the 5426 goes up to 2048, the 5434 can do 8192. */

#define SETWIDTH(fillWidth)	CIR_MMIO_WRITE16(MMIOWIDTH, fillWidth - 1)

/* Height: the 5426 goes up to 1024, the 5434 can do 2048. */

#define SETHEIGHT(fillHeight)	CIR_MMIO_WRITE16(MMIOHEIGHT, fillHeight - 1)

#define SETBLTMODE(m)	CIR_MMIO_WRITE8(MMIOBLTMODE, m)

#define SETBLTMODEEXT(m)	 CIR_MMIO_WRITE8(MMIOBLTMODEEXT, m)

#define SETBLTWRITEMASK(m) 	CIR_MMIO_WRITE8(MMIOBLTWRITEMASK, m)

#define SETROP(rop) 	CIR_MMIO_WRITE8(MMIOROP, rop)

#define STARTBLT() 	CIR_MMIO_WRITE8(MMIOBLTSTATUS, 0x02)

#define SETBLTSTATUS(v) CIR_MMIO_WRITE8(MMIOBLTSTATUS, v)

#define BLTBUSY(s)	(s) = CIR_MMIO_READ8(MMIOBLTSTATUS); (s) &= 1

#define SPIN_WHILE_BLTBUSY(s) \
			do { BLTBUSY(s) ; } while (s);

#define BLTEMPTY(s)	(s) = CIR_MMIO_READ8(MMIOBLTSTATUS); (s) = (((s) & 0x10) == 0)

/* BitBLT reset: temporarily set bit 2 of GR31 */
#define BLTRESET() { \
  unsigned char tmp8 = CIR_MMIO_READ8(MMIOBLTSTATUS); \
  tmp8 ^= 0x04; \
  CIR_MMIO_WRITE8(MMIOBLTSTATUS, tmp8); \
  tmp8 ^= 0x04; \
  CIR_MMIO_WRITE8(MMIOBLTSTATUS, tmp8); \
}

#define WAITUNTILFINISHED() CirrusBLTWaitUntilFinished()  

/* The macros for setting the background/foreground color are already
   defined using OUTs in cir_driver.h. */

/* To keep consistency with non-MMIO shadow variables, we also update the
 * shadow variables. Note that this is not done for the BitBLT registers;
 * functions that use them with port I/O while MMIO is also enabled must
 * invalidate the shadow variables.
 * Also note that the upper bytes of the potentially 32-bit color must
 * be preserved (they are used even in 8bpp on the 543x). */

#undef SETBACKGROUNDCOLOR
#define SETBACKGROUNDCOLOR(c) { \
  CIR_MMIO_WRITE8(MMIOBACKGROUNDCOLOR, c); \
  *(unsigned char *)(&cirrusBackgroundColorShadow) = c; \
}

#undef SETFOREGROUNDCOLOR
#define SETFOREGROUNDCOLOR(c) { \
  CIR_MMIO_WRITE8(MMIOFOREGROUNDCOLOR, c); \
  *(unsigned char *)(&cirrusForegroundColorShadow) = c; \
}

#undef SETBACKGROUNDCOLOR16
#define SETBACKGROUNDCOLOR16(c) { \
  CIR_MMIO_WRITE16(MMIOBACKGROUNDCOLOR, c); \
  *(unsigned short *)(&cirrusBackgroundColorShadow) = c; \
}

#undef SETFOREGROUNDCOLOR16
#define SETFOREGROUNDCOLOR16(c) { \
  CIR_MMIO_WRITE16(MMIOFOREGROUNDCOLOR, c); \
  *(unsigned short *)(&cirrusForegroundColorShadow) = c; \
}

/* 32-bit colors are exclusive to the BitBLT engine. */

#define SETBACKGROUNDCOLOR32(c) { \
  CIR_MMIO_WRITE32(MMIOBACKGROUNDCOLOR, c); \
  cirrusBackgroundColorShadow = c; \
}

#define SETFOREGROUNDCOLOR32(c) { \
  CIR_MMIO_WRITE32(MMIOFOREGROUNDCOLOR, c); \
  cirrusForegroundColorShadow = c; \
}

/*
 * Extra definitions added for XAA driver.
 */

#define SETBLTMODEANDROP(m, mext, rop)	CIR_MMIO_WRITE32(MMIOBLTMODE, (m) | ((rop)<<16) | ((mext) << 24))

#define SETWIDTHANDHEIGHT(w, h)		CIR_MMIO_WRITE32(MMIOWIDTH, ((w) - 1) | (((h) - 1) << 16))

#define SETTRANSPARENCYCOLOR(c)		CIR_MMIO_WRITE16(MMIOTRANSPARENCYCOLOR, c)

#define SETTRANSPARENCYCOLOR16(c)	CIR_MMIO_WRITE16(MMIOTRANSPARENCYCOLOR, c)

#define SETTRANSPARENCYCOLORMASK(c)	CIR_MMIO_WRITE16(MMIOTRANSPARENCYCOLORMASK, c)

#define SETTRANSPARENCYCOLORMASK16(c)	CIR_MMIO_WRITE16(MMIOTRANSPARENCYCOLORMASK, c)
