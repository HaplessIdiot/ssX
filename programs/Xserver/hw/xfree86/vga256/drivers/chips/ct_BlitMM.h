/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/chips/ct_BlitMM.h,v 3.0 1996/08/11 13:02:34 dawes Exp $ */

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
  {HW_DEBUG(ct0x93D0); \
   while(*(volatile unsigned int *)(ctMMIOBase + ct0x93D0) & \
    (ctisHiQV32 ? 0x80000000 : 0x00100000)){};}

#define ctSETROP(op) \
  {HW_DEBUG(ct0x93D0); *(unsigned int *)(ctMMIOBase + ct0x93D0) = op;}

#define ctSETSRCADDR(srcAddr) \
  {HW_DEBUG(ct0x97D0); \
   *(unsigned int *)(ctMMIOBase + ct0x97D0) = srcAddr&0x7FFFFFL;}

#define ctSETDSTADDR(dstAddr) \
{HW_DEBUG(ct0x9BD0); \
  *(unsigned int *)(ctMMIOBase + ct0x9BD0) = dstAddr&0x7FFFFFL;}

#define ctSETPITCH(srcPitch,dstPitch) \
{HW_DEBUG(ct0x83D0); \
  *(unsigned int *)(ctMMIOBase + ct0x83D0) = ((dstPitch&0xFFFF)<<16)| \
      (srcPitch&0xFFFF);}

#define ctSETHEIGHTWIDTHGO(Height,Width)\
{HW_DEBUG(ct0x9FD0); \
  *(unsigned int *)(ctMMIOBase + ct0x9FD0) = ((Height&0xFFFF)<<16)| \
      (Width&0xFFFF);}

#define ctSETPATSRCADDR(srcAddr)\
{HW_DEBUG(ct0x87D0); \
  *(unsigned int *)(ctMMIOBase + ct0x87D0) = srcAddr&0x1FFFFFL;}

#define ctSETBGCOLOR8(bgColor)\
{HW_DEBUG(ct0x8BD0); \
  *(unsigned int *)(ctMMIOBase + ct0x8BD0) = \
           (((((bgColor&0xFF)<<8)|(bgColor&0xFF))<<16) | \
	   (((bgColor&0xFF)<<8)|(bgColor&0xFF)));}

#define ctSETBGCOLOR16(bgColor)\
{HW_DEBUG(ct0x8BD0); \
  *(unsigned int *)(ctMMIOBase + ct0x8BD0) = \
           (((bgColor&0xFFFF)<<16)|(bgColor&0xFFFF));}

/* As the 6554x doesn't support 24bpp colour expansion this doesn't work,
 * It is here only for later use with the 65550 */
#define ctSETBGCOLOR24(bgColor)\
{HW_DEBUG(ct0x8BD0); \
  *(unsigned int *)(ctMMIOBase + ct0x8BD0) = \
           (bgColor&0xFFFFFF);}

#define ctSETFGCOLOR8(fgColor)\
{HW_DEBUG(ct0x8FD0); \
  *(unsigned int *)(ctMMIOBase + ct0x8FD0) = \
           (((((fgColor&0xFF)<<8)|(fgColor&0xFF))<<16) | \
	   (((fgColor&0xFF)<<8)|(fgColor&0xFF)));}

#define ctSETFGCOLOR16(fgColor)\
{HW_DEBUG(ct0x8FD0); \
  *(unsigned int *)(ctMMIOBase + ct0x8FD0) = \
           (((fgColor&0xFFFF)<<16)|(fgColor&0xFFFF));}

/* As the 6554x doesn't support 24bpp colour expansion this doesn't work,
 * It is here only for later use with the 65550 */
#define ctSETFGCOLOR24(fgColor)\
{HW_DEBUG(ct0x8FD0); \
  *(unsigned int *)(ctMMIOBase + ct0x8FD0) = \
           (fgColor&0xFFFFFF);}

#ifdef HWCUR_MMIO
#define ctGETHWCUR(status) \
{HW_DEBUG(0xA3D0); status = MMIOmeml(0xA3D0);}
#define ctPUTHWCUR(status) \
{HW_DEBUG(0xA3D0); MMIOmemw(0xA3D0) = status;}
#else
#define ctGETHWCUR(status) \
{HW_DEBUG(0x8); status = inl(DR(0x8));}
#define ctPUTHWCUR(status) \
{HW_DEBUG(0x8); outw(DR(0x8),status);}
#endif
