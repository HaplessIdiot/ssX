/* $XFree86$ */

/* Definitions for the Chips and Technology BitBLT engine communication. */
/* These are done using Memory Mapped IO, of the registers */
/* BitBLT modes for register 93D0. */

#define ctPATCOPY               0xF0
#define ctTOP2BOTTOM            0x100
#define ctLEFT2RIGHT            0x200
#define ctSRCFG                 0x400
#define ctSRCMONO               0x800
#define ctPATMONO               0x1000
#define ctBGTRANSPARENT         0x2000
#define ctSRCSYSTEM             0x4000
#define ctPATSOLID              0x80000L
#define ctPATSTART0             0x00000L
#define ctPATSTART1             0x10000L
#define ctPATSTART2             0x20000L
#define ctPATSTART3             0x30000L
#define ctPATSTART4             0x40000L
#define ctPATSTART5             0x50000L
#define ctPATSTART6             0x60000L
#define ctPATSTART7             0x70000L

/* The offsets in MMIO space to the memory mapped registers */
#define ct0x83D0                0x83D0
#define ct0x87D0                0x87D0
#define ct0x8BD0                0x8BD0
#define ct0x8FD0                0x8FD0
#define ct0x93D0                0x93D0
#define ct0x97D0                0x97D0
#define ct0x9BD0                0x9BD0
#define ct0x9FD0                0x9FD0

/* Macros to do useful things with the C&T BitBLT engine */
#define ctBLTWAIT \
  while(*(volatile unsigned int *)(ctMMIOBase + ct0x93D0) & \
	(ctisHiQV32 ? 0x80000000 : 0x00100000)){}

#define ctSETROP(op) \
  *(unsigned int *)(ctMMIOBase + ct0x93D0) = op

#define ctSETSRCADDR(srcAddr) \
  *(unsigned int *)(ctMMIOBase + ct0x97D0) = srcAddr&0x7FFFFFL

#define ctSETDSTADDR(dstAddr) \
  *(unsigned int *)(ctMMIOBase + ct0x9BD0) = dstAddr&0x7FFFFFL

#define ctSETPITCH(srcPitch,dstPitch) \
  *(unsigned int *)(ctMMIOBase + ct0x83D0) = ((dstPitch&0xFFFF)<<16)| \
      (srcPitch&0xFFFF)

#define ctSETHEIGHTWIDTHGO(Height,Width)\
  *(unsigned int *)(ctMMIOBase + ct0x9FD0) = ((Height&0xFFFF)<<16)| \
      (Width&0xFFFF)

#define ctSETPATSRCADDR(srcAddr)\
  *(unsigned int *)(ctMMIOBase + ct0x87D0) = srcAddr&0x1FFFFFL

#define ctSETBGCOLOR8(bgColor)\
  *(unsigned int *)(ctMMIOBase + ct0x8BD0) = \
           (((((bgColor&0xFF)<<8)|(bgColor&0xFF))<<16) | \
	   (((bgColor&0xFF)<<8)|(bgColor&0xFF)))

#define ctSETBGCOLOR16(bgColor)\
  *(unsigned int *)(ctMMIOBase + ct0x8BD0) = \
           (((bgColor&0xFFFF)<<16)|(bgColor&0xFFFF))

/* As the 6554x doesn't support 24bpp colour expansion this doesn't work,
 * It is here only for later use with the 65550 */
#define ctSETBGCOLOR24(bgColor)\
  *(unsigned int *)(ctMMIOBase + ct0x8BD0) = \
           (bgColor&0xFFFFFF)

#define ctSETFGCOLOR8(fgColor)\
  *(unsigned int *)(ctMMIOBase + ct0x8FD0) = \
           (((((fgColor&0xFF)<<8)|(fgColor&0xFF))<<16) | \
	   (((fgColor&0xFF)<<8)|(fgColor&0xFF)))

#define ctSETFGCOLOR16(fgColor)\
  *(unsigned int *)(ctMMIOBase + ct0x8FD0) = \
           (((fgColor&0xFFFF)<<16)|(fgColor&0xFFFF))

/* As the 6554x doesn't support 24bpp colour expansion this doesn't work,
 * It is here only for later use with the 65550 */
#define ctSETFGCOLOR24(fgColor)\
  *(unsigned int *)(ctMMIOBase + ct0x8FD0) = \
           (fgColor&0xFFFFFF)
