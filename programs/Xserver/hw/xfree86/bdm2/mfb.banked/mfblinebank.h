/* $XConsortium: mfblinebank.h,v 1.2 94/03/31 14:17:26 dpw Exp $ */
/* mfb_bank.h */
/* included from mfb.h if MFB_LINE_BANK is defined */

/* #include "bdm.h" */

/* excerpt from bdm.h */
extern void (* bdmSetBankAFunc)();
extern void (* bdmSetBankBFunc)();

/* variables in bdm.c */
extern unsigned char *bdmBankABottom;
extern unsigned char *bdmBankATop;
extern unsigned char *bdmBankBBottom;
extern unsigned char *bdmBankBTop;
extern int  bdmSegmentMask;
extern int  bdmSegmentShift;
extern int  bdmBankAseg;
extern int  bdmBankBseg;

#if defined(__386BSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
#define BDMBASE 0xFF000000
#else
#define BDMBASE 0xF0000000
#endif

#if defined(__GNUC__)
#define inline __inline__
#else
#define inline /**/
#endif

static inline PixelType *bdmScanlineOffsetFunc(p, offset)
PixelType *p;
int offset;
{
register signed int delta;
register signed int deltabank;
  if ((unsigned int)(p)>=BDMBASE) {
     /* virtual framebuffer address */
     /* p - BDMBASE is not necessarily a valid pointer within the bank */
     /* -> do absolute banking */
     p += offset;
     p = (PixelType *)((unsigned int)(p) - (unsigned int)BDMBASE);
     deltabank = (signed int)p >> bdmSegmentShift;
     bdmBankAseg = deltabank;
     (*bdmSetBankAFunc)(bdmBankAseg);
     p = (PixelType *)( (unsigned int)p + bdmBankABottom
			- (deltabank << bdmSegmentShift) );
     return(p);
  }
  /* At least on Linux, the mapped Area is at 0x40000000, so test >= first */
  if ((unsigned int)(p)>=(unsigned int)bdmBankABottom) {
     if ((unsigned int)(p)<(unsigned int)bdmBankATop) {
        /* within the framebuffer */
        (p) += (offset);
        /* outside of the bank now ? */
        delta = (signed int)p - (signed int)bdmBankABottom;
        deltabank = delta >> bdmSegmentShift;
        if (0!=deltabank) {
                bdmBankAseg += deltabank;
                (*bdmSetBankAFunc)(bdmBankAseg);
                p = (PixelType *)( (unsigned int)p
				   - (deltabank << bdmSegmentShift) );
        }
        return(p);
     }
  }
  /* no framebuffer address */
  return((p)+(offset));
}

static inline PixelType *bdmScanlineOffsetFuncA(p, offset)
PixelType *p;
int offset;
{
register signed int delta;
register signed int deltabank;
  if ((unsigned int)(p)>=BDMBASE) {
     /* virtual framebuffer address */
     /* p - BDMBASE is not necessarily a valid pointer within the bank */
     /* -> do absolute banking */
     p += offset;
     p = (PixelType *)((unsigned int)(p) - (unsigned int)BDMBASE);
     deltabank = (signed int)p >> bdmSegmentShift;
     bdmBankAseg = deltabank;
     (*bdmSetBankAFunc)(bdmBankAseg);
     p = (PixelType *)( (unsigned int)p + bdmBankABottom
			- (deltabank << bdmSegmentShift) );
     return(p);
  }
  /* At least on Linux, the mapped Area is at 0x40000000, so test >= first */
  if ((unsigned int)(p)>=(unsigned int)bdmBankABottom) {
     if ((unsigned int)(p)<(unsigned int)bdmBankATop) {
        /* within the framebuffer */
        (p) += (offset);
        /* outside of the bank now ? */
        delta = (signed int)p - (signed int)bdmBankABottom;
        deltabank = delta >> bdmSegmentShift;
        if (0!=deltabank) {
                bdmBankAseg += deltabank;
                (*bdmSetBankAFunc)(bdmBankAseg);
                p = (PixelType *)( (unsigned int)p
				   - (deltabank << bdmSegmentShift) );
        }
        return(p);
     }
  }
  /* no framebuffer address */
  return((p)+(offset));
}

static inline PixelType *bdmScanlineOffsetFuncB(p, offset)
PixelType *p;
int offset;
{
register signed int delta;
register signed int deltabank;
  if ((unsigned int)(p)>=BDMBASE) {
     /* virtual framebuffer address */
     /* p - BDMBASE is not necessarily a valid pointer within the bank */
     /* -> do absolute banking */
     p += offset;
     p = (PixelType *)((unsigned int)(p) - (unsigned int)BDMBASE);
     deltabank = (signed int)p >> bdmSegmentShift;
     bdmBankBseg = deltabank;
     (*bdmSetBankBFunc)(bdmBankBseg);
     p = (PixelType *)( (unsigned int)p + bdmBankBBottom
			- (deltabank << bdmSegmentShift) );
     return(p);
  }
  /* At least on Linux, the mapped Area is at 0x40000000, so test >= first */
  if ((unsigned int)(p)>=(unsigned int)bdmBankBBottom) {
     if ((unsigned int)(p)<(unsigned int)bdmBankBTop) {
        /* within the framebuffer */
        (p) += (offset);
        /* outside of the bank now ? */
        delta = (signed int)p - (signed int)bdmBankBBottom;
        deltabank = delta >> bdmSegmentShift;
        if (0!=deltabank) {
                bdmBankBseg += deltabank;
                (*bdmSetBankBFunc)(bdmBankBseg);
                p = (PixelType *)( (unsigned int)p
				   - (deltabank << bdmSegmentShift) );
        }
        return(p);
     }
  }
  /* no framebuffer address */
  return((p)+(offset));
}
                
#if 0

extern PixelType *bdmScanlineOffsetFunc(
#if NeedFunctionPrototypes
PixelType * /* pointer */,
int /* offset */
#endif
);

extern PixelType *bdmScanlineOffsetFuncA(
#if NeedFunctionPrototypes
PixelType * /* pointer */,
int /* offset */
#endif
);

extern PixelType *bdmScanlineOffsetFuncB(
#if NeedFunctionPrototypes
PixelType * /* pointer */,
int /* offset */
#endif
);

#endif

#define mfbScanlineOffset(_pointer, offset) \
	bdmScanlineOffsetFunc((_pointer), (offset))

#define mfbScanlineInc(_pointer, offset) \
	(_pointer) = mfbScanlineOffset((_pointer), (offset))

#define mfbScanlineOffsetSrc(_pointer, offset) \
	bdmScanlineOffsetFuncA((_pointer), (offset))

#define mfbScanlineIncSrc(_pointer, offset) \
	(_pointer) = mfbScanlineOffsetSrc((_pointer), (offset))

#define mfbScanlineDeltaSrc(pointer, y, width) \
    mfbScanlineOffsetSrc(pointer, (y) * (width))
    
#define mfbScanlineSrc(pointer, x, y, width) \
    mfbScanlineOffsetSrc(pointer, (y) * (width) + ((x) >> MFB_PWSH))

#define mfbScanlineOffsetDst(_pointer, offset) \
	bdmScanlineOffsetFuncB((_pointer), (offset))

#define mfbScanlineIncDst(_pointer, offset) \
	(_pointer) = mfbScanlineOffsetSrc((_pointer), (offset))

#define mfbScanlineDeltaDst(pointer, y, width) \
    mfbScanlineOffsetSrc(pointer, (y) * (width))
    
#define mfbScanlineDst(pointer, x, y, width) \
    mfbScanlineOffsetSrc(pointer, (y) * (width) + ((x) >> MFB_PWSH))
