/* $XConsortium: cir_blitter.h,v 1.1 94/03/28 21:48:17 dpw Exp $ */
/* $XFree86$ */

/* Definitions for BitBLT engine communication. */

/* BitBLT modes. */

#define FORWARDS		0x00
#define BACKWARDS		0x01
#define SYSTEMDEST		0x02
#define SYSTEMSRC		0x04
#define TRANSPARENCYCOMPARE	0x08
#define PIXELWIDTH16		0x10
#define PIXELWIDTH32		0x30	/* 543x only. */
#define PATTERNCOPY		0x40
#define COLOREXPAND		0x80


/* Address: the 5426 adresses 2MBytes, the 5434 can address 4MB. */

#define SETDESTADDR(dstAddr) \
  outw (0x3CE, (((dstAddr) & 0x000000FF) << 8) | 0x28); \
  outw (0x3CE, (((dstAddr) & 0x0000FF00)) | 0x29); \
  outw (0x3CE, (((dstAddr) & 0x003F0000) >> 8) | 0x2A);

#define SETSRCADDR(srcAddr) \
  outw (0x3CE, (((srcAddr) & 0x000000FF) << 8) | 0x2C); \
  outw (0x3CE, (((srcAddr) & 0x0000FF00)) | 0x2D); \
  outw (0x3CE, (((srcAddr) & 0x003F0000) >> 8) | 0x2E);

/* Pitch: the 5426 goes up to 4095, the 5434 can do 8191. */

#define SETDESTPITCH(dstPitch) \
  outw (0x3CE, (((dstPitch) & 0x000000FF) << 8) | 0x24); \
  outw (0x3CE, (((dstPitch) & 0x00001F00)) | 0x25);

#define SETSRCPITCH(srcPitch) \
  outw (0x3CE, (((srcPitch) & 0x000000FF) << 8) | 0x26); \
  outw (0x3CE, (((srcPitch) & 0x00001F00)) | 0x27);

/* Width: the 5426 goes up to 2048, the 5434 can do 8192. */

#define SETWIDTH(fillWidth) \
  outw (0x3CE, ((((fillWidth) - 1) & 0x000000FF) << 8) | 0x20); \
  outw (0x3CE, ((((fillWidth) - 1) & 0x00001F00)) | 0x21);

/* Height: the 5426 goes up to 1024, the 5434 can do 2048. */

#define SETHEIGHT(fillHeight) \
  outw (0x3CE, ((((fillHeight) - 1) & 0x000000FF) << 8) | 0x22); \
  outw (0x3CE, (((fillHeight) - 1) & 0x00000700) | 0x23);

#define SETBLTMODE(m) outw(0x3CE, ((m) << 8) | 0x30);

#define SETTRANSPARENCYCOLOR(c) outw(0x3ce, ((c) << 8) | 0x34);

#define SETROP(rop) \
  outw (0x3CE, ((rop) << 8) | 0x32);

#define STARTBLT() \
  outw (0x3CE, (0x02 << 8) | 0x31);

#define WAITUNTILFINISHED() CirrusBLTWaitUntilFinished()  
