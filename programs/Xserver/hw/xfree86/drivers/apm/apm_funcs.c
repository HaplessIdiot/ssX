/* $XFree83: xc/programs/Xserver/hw/xfree86/drivers/apm/apm_funcs.c,v 1.3 1999/07/10 12:17:28 dawes Exp $ */

#undef DEBUG
#define FASTER
#ifdef IOP_ACCESS
#  if defined(PSZ) && (PSZ == 24)
#    define A(s)	Apm##s##24##_IOP
#  else
#    define A(s)	Apm##s##_IOP
#  endif
#  undef	RDXB
#  undef	RDXW
#  undef	RDXL
#  undef	WRXB
#  undef	WRXW
#  undef	WRXL
#  undef	ApmWriteSeq
#  define RDXB	RDXB_IOP
#  define RDXW	RDXW_IOP
#  define RDXL	RDXL_IOP
#  define WRXB	WRXB_IOP
#  define WRXW	WRXW_IOP
#  define WRXL	WRXL_IOP
#  define ApmWriteSeq(i, v)	wrinx(0x3C4, i, v)
#  ifdef DEBUG
#    if defined(PSZ) && (PSZ == 24)
#      define DPRINTNAME(s)	do { ErrorF("Apm" #s "24_IOP\n"); fflush(stderr); sync(); } while (0)
#    else
#      define DPRINTNAME(s)	do { ErrorF("Apm" #s "_IOP\n"); fflush(stderr); sync(); } while (0)
#    endif
#  endif
#else
#  if defined(PSZ) && (PSZ == 24)
#    define A(s)	Apm##s##24
#  else
#    define A(s)	Apm##s
#  endif
#  ifdef DEBUG
#    if defined(PSZ) && (PSZ == 24)
#      define DPRINTNAME(s)	do { ErrorF("Apm" #s "24\n"); fflush(stderr); sync(); } while (0)
#    else
#      define DPRINTNAME(s)	do { ErrorF("Apm" #s "\n"); fflush(stderr); sync(); usleep(100000); } while (0)
#    endif
#  endif
#endif
#ifndef DEBUG
#  define DPRINTNAME(s)	/**/
#endif

/* Defines */
#define MAXLOOP 1000000


/* Local functions */
static void A(Sync)(ScrnInfoPtr pScrn);
static void A(SetupForSolidFill)(ScrnInfoPtr pScrn, int color, int rop,
					unsigned int planemask);
static void A(SubsequentSolidFillRect)(ScrnInfoPtr pScrn, int x, int y, int w, int h);
static void A(SetupForScreenToScreenCopy)(ScrnInfoPtr pScrn, int xdir, int ydir, int rop, unsigned int planemask,
                                          int transparency_color);
static void A(SubsequentScreenToScreenCopy)(ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2, int w, int h);
#if !defined(PSZ) || PSZ != 24
static void A(SetupForCPUToScreenColorExpandFill)(ScrnInfoPtr pScrn, int bg, int fg, int rop, unsigned int planemask);
static void A(SubsequentCPUToScreenColorExpandFill)(ScrnInfoPtr pScrn, int x, int y, int w, int h, int skipleft);
static void A(SetupForScreenToScreenColorExpandFill)(ScrnInfoPtr pScrn,
						     int fg, int bg, int rop,
						     unsigned int planemask);
static void A(SetupForImageWrite)(ScrnInfoPtr pScrn, int rop,
				  unsigned int planemask, int trans_color,
				  int bpp, int depth);
static void A(SubsequentImageWriteRect)(ScrnInfoPtr pScrn, int x, int y,
					int w, int h, int skipleft);
static void A(SubsequentScreenToScreenColorExpandFill)(ScrnInfoPtr pScrn,
						       int x, int y,
						       int w, int h,
						       int srcx, int srcy,
						       int offset);
static void A(SubsequentSolidBresenhamLine)(ScrnInfoPtr pScrn, int x1, int y1, int octant, int err, int e1, int e2, int length);
static void A(SetClippingRectangle)(ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2);
static void A(WritePixmap)(ScrnInfoPtr pScrn, int x, int y, int w, int h,
			   unsigned char *src, int srcwidth, int rop,
			   unsigned int planemask, int trans, int bpp,
			   int depth);
static void A(SetupForMono8x8PatternFill)(ScrnInfoPtr pScrn, int patx, int paty,
				          int fg, int bg, int rop,
				          unsigned int planemask);
static void A(SubsequentMono8x8PatternFillRect)(ScrnInfoPtr pScrn, int patx,
					        int paty, int x, int y,
					        int w, int h);
static void A(SetupForColor8x8PatternFill)(ScrnInfoPtr pScrn,int patx,int paty,
				           int rop, unsigned int planemask,
					   int transparency_color);
static void A(SubsequentColor8x8PatternFillRect)(ScrnInfoPtr pScrn, int patx,
					         int paty, int x, int y,
					         int w, int h);
#endif

/* Inline functions */
static __inline__ void
A(WaitForFifo)(ApmPtr pApm, int slots)
{
  if (!pApm->UsePCIRetry) {
    volatile int i;

    for(i = 0; i < MAXLOOP; i++) {
      if ((STATUS() & STATUS_FIFO) >= slots)
	break;
    }
    if (i == MAXLOOP) {
      apm_stopit();
      FatalError("Hung in WaitForFifo() (Status = 0x%08X)\n", STATUS());
    }
  }
}

static __inline__ void
A(CheckMMIO_InitFast)(ScrnInfoPtr pScrn)
{
  if (!APMPTR(pScrn)->apmMMIO_Init)
    A(CheckMMIO_Init)(pScrn);
}


static void
A(SetupForSolidFill)(ScrnInfoPtr pScrn, int color, int rop,
			unsigned int planemask)
{
  APMDECL(pScrn);

  DPRINTNAME(SetupForSolidFill);
  A(CheckMMIO_InitFast)(pScrn);
#ifdef FASTER
  A(WaitForFifo)(pApm, 3 + pApm->apmClip);
  SETDEC(DEC_QUICKSTART_ONDIMX | DEC_OP_RECT | DEC_DEST_UPD_TRCORNER |
	  pApm->Setup_DEC);
#else
  A(WaitForFifo)(pApm, 2 + pApm->apmClip);
#endif
  SETFOREGROUNDCOLOR(color);

  if (pApm->apmClip)  {
    SETCLIP_CTRL(0);
    pApm->apmClip = FALSE;
  }

  SETROP(apmROP[rop]);
}

static void
A(SubsequentSolidFillRect)(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
  APMDECL(pScrn);

  DPRINTNAME(SubsequentSolidFillRect);
#if defined(PSZ) && (PSZ == 24)
#  ifndef FASTER
  A(WaitForFifo)(pApm, 4);
#  else
  A(WaitForFifo)(pApm, 3);
#  endif
  /* COLOR */
  SETOFFSET(3*(pApm->displayWidth - w));
#else
#  ifndef FASTER
  A(WaitForFifo)(pApm, 3);
#  else
  A(WaitForFifo)(pApm, 2);
#  endif
#endif
  SETDESTXY(x, y);
  SETWIDTHHEIGHT(w, h);
  UPDATEDEST(x + w + 1, y);
#ifndef FASTER
  SETDEC(DEC_START | DEC_OP_RECT | DEC_DEST_UPD_TRCORNER | pApm->Setup_DEC);
#endif
}

static void
A(SetupForScreenToScreenCopy)(ScrnInfoPtr pScrn, int xdir, int ydir, int rop,
				unsigned int planemask, int transparency_color)
{
  APMDECL(pScrn);

  DPRINTNAME(SetupForScreenToScreenCopy);
  A(CheckMMIO_InitFast)(pScrn);

  if (pApm->apmLock) {
    /*
     * This is just an attempt, because Daryll is tampering with MY registers.
     */
    WRXB(0xDB, (RDXB(0xDB) & 0xF4) |  0x0A);
    ApmWriteSeq(0x1B, 0x20);
    ApmWriteSeq(0x1C, 0x2F);
    pApm->apmLock = FALSE;
  }

  pApm->blitxdir = xdir;
  pApm->blitydir = ydir;

  pApm->apmTransparency = (transparency_color != -1);

#ifdef FASTER
  A(WaitForFifo)(pApm, 2 + pApm->apmClip + pApm->apmTransparency);
  SETDEC(DEC_QUICKSTART_ONDIMX | DEC_OP_BLT | DEC_DEST_UPD_TRCORNER |
	  (pApm->apmTransparency ? DEC_SOURCE_TRANSPARENCY : 0) | pApm->Setup_DEC |
	  ((xdir < 0) ? DEC_DIR_X_NEG : DEC_DIR_X_POS) |
	  ((ydir < 0) ? DEC_DIR_Y_NEG : DEC_DIR_Y_POS));
#else
  A(WaitForFifo)(pApm, 1 + pApm->apmClip + (transparency_color != -1));
#endif

  if (pApm->apmClip)  {
    SETCLIP_CTRL(0);
    pApm->apmClip = FALSE;
  }
  if (transparency_color != -1)
    SETBACKGROUNDCOLOR(transparency_color);

  SETROP(apmROP[rop]);
}

static void
A(SubsequentScreenToScreenCopy)(ScrnInfoPtr pScrn, int x1, int y1,
				    int x2, int y2, int w, int h)
{
  APMDECL(pScrn);
  BoxRec	box;
#ifndef FASTER
  u32		c = pApm->apmTransparency ? DEC_SOURCE_TRANSPARENCY : 0;
#endif
#if defined(PSZ) && (PSZ == 24)
  u16		offset;
#endif
  u32		sx, dx, sy, dy;

  DPRINTNAME(SubsequentScreenToScreenCopy);
  if (pApm->blitxdir < 0)
  {
#ifndef FASTER
    c |= DEC_DIR_X_NEG;
#endif
#if defined(PSZ) && (PSZ == 24)
    offset = 3*(pApm->displayWidth + w);
#endif
    sx = x1+w-1;
    dx = x2+w-1;
  }
  else
  {
#ifndef FASTER
    c |= DEC_DIR_X_POS;
#endif
#if defined(PSZ) && (PSZ == 24)
    offset = 3*(pApm->displayWidth - w);
#endif
    sx = x1;
    dx = x2;
  }

  if (pApm->blitydir < 0)
  {
#ifndef FASTER
    c |= DEC_DIR_Y_NEG | DEC_START | DEC_OP_BLT | DEC_DEST_UPD_TRCORNER |
	    pApm->Setup_DEC;
#endif
    sy = y1+h-1;
    dy = y2+h-1;
  }
  else
  {
#ifndef FASTER
    c |= DEC_DIR_Y_POS | DEC_START | DEC_OP_BLT | DEC_DEST_UPD_TRCORNER |
	    pApm->Setup_DEC;
#endif
    sy = y1;
    dy = y2;
  }

#if defined(PSZ) && (PSZ == 24)
#  ifndef FASTER
  A(WaitForFifo)(pApm, 5);
#  else
  A(WaitForFifo)(pApm, 4);
#  endif
  SETOFFSET(offset);
#else
#  ifndef FASTER
  A(WaitForFifo)(pApm, 4);
#  else
  A(WaitForFifo)(pApm, 3);
#  endif
#endif

  SETSOURCEXY(sx,sy);
  SETDESTXY(dx,dy);
  SETWIDTHHEIGHT(w,h);
  UPDATEDEST(sx + (w + 1)*pApm->blitxdir, sy);

#ifndef FASTER
  SETDEC(c);
#endif
  if (POINT_IN_REGION(pApm->pScreen, &pApm->apmLockedRegion, sx, sy, &box))
      A(Sync)(pScrn);
}

#if !defined(PSZ) || PSZ != 24
static void
A(SetupForCPUToScreenColorExpandFill)(ScrnInfoPtr pScrn, int fg, int bg,
					int rop, unsigned int planemask)
{
  APMDECL(pScrn);

  DPRINTNAME(SetupForCPUToScreenColorExpandFill);
  A(CheckMMIO_InitFast)(pScrn);
  if (bg == -1)
  {
#ifndef FASTER
    pApm->apmTransparency = TRUE;
    A(WaitForFifo)(pApm, 3);
#else
    A(WaitForFifo)(pApm, 4);
    SETDEC(DEC_OP_HOSTBLT_HOST2SCREEN | DEC_SOURCE_LINEAR | DEC_SOURCE_CONTIG |
      DEC_SOURCE_TRANSPARENCY | DEC_SOURCE_MONOCHROME | DEC_QUICKSTART_ONDIMX |
      DEC_DEST_UPD_TRCORNER | pApm->Setup_DEC);
#endif
    SETFOREGROUNDCOLOR(fg);
    SETBACKGROUNDCOLOR(fg+1);
  }
  else
  {
#ifndef FASTER
    pApm->apmTransparency = FALSE;
    A(WaitForFifo)(pApm, 3);
#else
    A(WaitForFifo)(pApm, 4);
    SETDEC(DEC_OP_HOSTBLT_HOST2SCREEN | DEC_SOURCE_LINEAR | DEC_SOURCE_CONTIG |
	   DEC_DEST_UPD_TRCORNER | DEC_SOURCE_MONOCHROME |
	   DEC_QUICKSTART_ONDIMX | pApm->Setup_DEC);
#endif
    SETFOREGROUNDCOLOR(fg);
    SETBACKGROUNDCOLOR(bg);
  }
  SETROP(apmROP[rop]);
}

static void
A(SubsequentCPUToScreenColorExpandFill)(ScrnInfoPtr pScrn, int x, int y,
					int w, int h, int skipleft)
{
  APMDECL(pScrn);
#ifndef FASTER
  u32 c;
#endif

  DPRINTNAME(SubsequentCPUToScreenColorExpandFill);
#ifndef FASTER
  c = DEC_OP_HOSTBLT_HOST2SCREEN | DEC_SOURCE_LINEAR | DEC_SOURCE_CONTIG |
      DEC_SOURCE_MONOCHROME | DEC_START | DEC_DEST_UPD_TRCORNER |
      pApm->Setup_DEC;

  if (pApm->apmTransparency)
    c |= DEC_SOURCE_TRANSPARENCY;

  A(WaitForFifo)(pApm, 7);
#else
  A(WaitForFifo)(pApm, 6);
#endif

  SETCLIP_LEFTTOP(x+skipleft, y);
  SETCLIP_RIGHTBOT(x+w-1, y+h-1);
  SETCLIP_CTRL(0x01);
  pApm->apmClip = TRUE;
  SETSOURCEX(0); /* According to manual, it just has to be zero */
  SETDESTXY(x, y);
  SETWIDTHHEIGHT((w + 31) & ~31, h);
  UPDATEDEST(x + ((w + 31) & ~31), y);

#ifndef FASTER
  SETDEC(c);
#endif
}

static void
A(SetupForImageWrite)(ScrnInfoPtr pScrn, int rop, unsigned int planemask,
		      int trans_color, int bpp, int depth)
{
  APMDECL(pScrn);

  DPRINTNAME(SetupForImageWriteRect);
  A(CheckMMIO_InitFast)(pScrn);
  if (trans_color != -1)
  {
#ifndef FASTER
    pApm->apmTransparency = TRUE;
    A(WaitForFifo)(pApm, 3);
#else
    A(WaitForFifo)(pApm, 4);
    SETDEC(DEC_OP_HOSTBLT_HOST2SCREEN | DEC_SOURCE_LINEAR | DEC_SOURCE_CONTIG |
	  DEC_SOURCE_TRANSPARENCY | DEC_QUICKSTART_ONDIMX | pApm->Setup_DEC);
#endif
    SETBACKGROUNDCOLOR(trans_color);
  }
  else {
#ifndef FASTER
    pApm->apmTransparency = FALSE;
    A(WaitForFifo)(pApm, 2);
#else
    A(WaitForFifo)(pApm, 3);
    SETDEC(DEC_OP_HOSTBLT_HOST2SCREEN | DEC_SOURCE_LINEAR | DEC_SOURCE_CONTIG |
	  DEC_QUICKSTART_ONDIMX | pApm->Setup_DEC);
#endif
  }

  SETROP(apmROP[rop]);
}

static void
A(SubsequentImageWriteRect)(ScrnInfoPtr pScrn, int x, int y, int w, int h,
			    int skipleft)
{
  APMDECL(pScrn);
#ifndef FASTER
  u32 c;
#endif

  DPRINTNAME(SubsequentImageWriteRect);
#ifndef FASTER
  c = DEC_OP_HOSTBLT_HOST2SCREEN | DEC_SOURCE_LINEAR | DEC_SOURCE_CONTIG |
      DEC_START | pApm->Setup_DEC;

  if (pApm->apmTransparency)
    c |= DEC_SOURCE_TRANSPARENCY;

  A(WaitForFifo)(pApm, 7);
#else
  A(WaitForFifo)(pApm, 6);
#endif

  SETCLIP_LEFTTOP(x+skipleft, y);
  SETCLIP_RIGHTBOT(x+w-1, y+h-1);
  SETCLIP_CTRL(0x01);
  pApm->apmClip = TRUE;
  SETSOURCEX(0); /* According to manual, it just has to be zero */
  SETDESTXY(x, y);
  SETWIDTHHEIGHT((w + 3) & ~3, h);

#ifndef FASTER
  SETDEC(c);
#endif
}


static void
A(SetupForScreenToScreenColorExpandFill)(ScrnInfoPtr pScrn, int fg, int bg,
					 int rop, unsigned int planemask)
{
  APMDECL(pScrn);

  DPRINTNAME(SetupForScreenToScreenColorExpandFill);
  A(CheckMMIO_InitFast)(pScrn);
  A(WaitForFifo)(pApm, 3 + pApm->apmClip);
  if (bg == -1)
  {
    SETFOREGROUNDCOLOR(fg);
    SETBACKGROUNDCOLOR(fg+1);
    pApm->apmTransparency = TRUE;
  }
  else
  {
    SETFOREGROUNDCOLOR(fg);
    SETBACKGROUNDCOLOR(bg);
    pApm->apmTransparency = FALSE;
  }

  SETROP(apmROP[rop]);
}

static void
A(SubsequentScreenToScreenColorExpandFill)(ScrnInfoPtr pScrn, int x, int y,
					   int w, int h, int srcx, int srcy,
					   int offset)
{
  APMDECL(pScrn);
  u32 c;

  DPRINTNAME(SubsequentScreenToScreenColorExpandFill);
#ifdef FASTER
  c = DEC_OP_BLT | DEC_DIR_X_POS | DEC_DIR_Y_POS | DEC_SOURCE_MONOCHROME |
      DEC_QUICKSTART_ONDIMX | DEC_DEST_UPD_TRCORNER | pApm->Setup_DEC;
#else
  c = DEC_OP_BLT | DEC_DIR_X_POS | DEC_DIR_Y_POS | DEC_SOURCE_MONOCHROME |
      DEC_START | DEC_DEST_UPD_TRCORNER | pApm->Setup_DEC;
#endif

  if (pApm->apmTransparency)
    c |= DEC_SOURCE_TRANSPARENCY;

  if (srcy >= pApm->Scanlines) {
      struct ApmStippleCacheRec *pCache;
      CARD32	dist;

      /*
       * Offscreen linear stipple
       */
      pCache = &pApm->apmCache[srcy / pApm->Scanlines - 1];
      if (w != pCache->apmStippleCache.w * pApm->bitsPerPixel) {
	  A(WaitForFifo)(pApm, 3);
	  SETCLIP_LEFTTOP(x, y);
	  SETCLIP_RIGHTBOT(x + w - 1, y + h - 1);
	  SETCLIP_CTRL(0x01);
	  pApm->apmClip = TRUE;
	  w = pCache->apmStippleCache.w * pApm->bitsPerPixel;
	  x -= srcx - pCache->apmStippleCache.x;
	  srcx = pCache->apmStippleCache.x;
      }
      else if (pApm->apmClip) {
	  A(WaitForFifo)(pApm, 1);
	  SETCLIP_CTRL(0x00);
	  pApm->apmClip = FALSE;
      }
      srcx += (srcy - pCache->apmStippleCache.y) * pCache->apmStippleCache.w;
      srcy = pCache->apmStippleCache.y % pApm->Scanlines;
      dist = srcx + srcy * pApm->displayWidth;
      srcx = dist & 0xFFF;
      srcy = dist >> 12;
      c |= DEC_SOURCE_CONTIG | DEC_SOURCE_LINEAR;
  }
  else if (offset) {
      A(WaitForFifo)(pApm, 3);
      SETCLIP_LEFTTOP(x, y);
      SETCLIP_RIGHTBOT(x + w, y + h);
      SETCLIP_CTRL(0x01);
      pApm->apmClip = TRUE;
      w += offset;
      x -= offset;
  }
  else if (pApm->apmClip) {
      A(WaitForFifo)(pApm, 1);
      SETCLIP_CTRL(0x00);
      pApm->apmClip = FALSE;
  }

  A(WaitForFifo)(pApm, 4);

  SETSOURCEXY(srcx, srcy);
  SETDESTXY(x, y);

#ifdef FASTER
  SETDEC(c);
  SETWIDTHHEIGHT(w, h);
#else
  SETWIDTHHEIGHT(w, h);
  SETDEC(c);
#endif
  UPDATEDEST(x + w + 1, h);
}

static void
A(SubsequentSolidBresenhamLine)(ScrnInfoPtr pScrn, int x1, int y1, int e1,
				int e2, int err, int length, int octant)
{
  APMDECL(pScrn);
#ifdef FASTER
  u32 c = DEC_QUICKSTART_ONDIMX | DEC_OP_VECT_ENDP | DEC_DEST_UPD_LASTPIX |
	  pApm->Setup_DEC;
#else
  u32 c = DEC_START | DEC_OP_VECT_ENDP | DEC_DEST_UPD_LASTPIX | pApm->Setup_DEC;
#endif
  int	tmp;

  DPRINTNAME(SubsequentSolidBresenhamLine);

  A(WaitForFifo)(pApm, 5);
  SETDESTXY(x1,y1);
  SETDDA_ERRORTERM(err);
  SETDDA_ADSTEP(e1, e2);

  if (octant & YMAJOR) {
    c |= DEC_MAJORAXIS_Y;
    tmp = e1; e1 = e2; e2 = tmp;
  }
  else
    c |= DEC_MAJORAXIS_X;

  if (octant & XDECREASING) {
    c |= DEC_DIR_X_NEG;
    e1 = -e1;
  }
  else
    c |= DEC_DIR_X_POS;

  if (octant & YDECREASING) {
    c |= DEC_DIR_Y_NEG;
    e2 = -e2;
  }
  else
    c |= DEC_DIR_Y_POS;

#ifdef FASTER
  SETDEC(c);
  SETWIDTH(length);
#else
  SETWIDTH(length);
  SETDEC(c);
#endif

  if (octant & YMAJOR)
    UPDATEDEST(x1 + e1 / 2, y1 + e2 / 2);
  else
    UPDATEDEST(x1 + e2 / 2, y1 + e1 / 2);
  if (pApm->apmClip)
  {
    pApm->apmClip = FALSE;
    A(WaitForFifo)(pApm, 1);
    SETCLIP_CTRL(0);
  }
}

static void
A(SetClippingRectangle)(ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2)
{
  APMDECL(pScrn);

  DPRINTNAME(SetClippingRectangle);
  A(WaitForFifo)(pApm, 3);
  SETCLIP_LEFTTOP(x1,y1);
  SETCLIP_RIGHTBOT(x2,y2);
  SETCLIP_CTRL(0x01);
  pApm->apmClip = TRUE;
}

static void
A(WritePixmap)(ScrnInfoPtr pScrn, int x, int y, int w, int h,
	       unsigned char *src, int srcwidth, int rop,
	       unsigned int planemask, int trans, int bpp, int depth)
{
    APMDECL(pScrn);
    int dwords, skipleft, Bpp = bpp >> 3; 
    Bool beCareful = FALSE;
    unsigned char *dst = ((unsigned char *)pApm->FbBase) + x * Bpp + y * pApm->bytesPerScanline;
    int PlusOne = 0, mask, count;

    DPRINTNAME(WritePixmap);
    /*
     * First the fast case : source and dest have same alignment. Doc says
     * it's faster to do it here, which may be true since one has to read
     * the chip when CPU to screen-ing.
     */
    if ((skipleft = (long)src & 3L) == ((long)dst & 3L)) {
	int skipright;

	if (skipleft)
	    skipleft = 4 - skipleft;
	dwords = (skipright = (w - skipleft) * Bpp) >> 2;
	skipright %= 4;
	if (!skipleft && !skipright)
	    while (h-- > 0) {
		CARD32 *src2 = (CARD32 *)src;
		CARD32 *dst2 = (CARD32 *)dst;

		for (count = dwords; count-- > 0; )
		    *dst2++ = *src2++;
		src += srcwidth;
		dst += pApm->bytesPerScanline;
	    }
	else if (!skipleft)
	    while (h-- > 0) {
		CARD32 *src2 = (CARD32 *)src;
		CARD32 *dst2 = (CARD32 *)dst;

		for (count = dwords; count-- > 0; )
		    *dst2++ = *src2++;
		for (count = skipright; count-- > 0; )
		    ((char *)dst2)[count] = ((char *)src2)[count];
		src += srcwidth;
		dst += pApm->bytesPerScanline;
	    }
	else if (!skipright)
	    while (h-- > 0) {
		CARD32 *src2 = (CARD32 *)(src + skipleft);
		CARD32 *dst2 = (CARD32 *)(dst + skipleft);

		for (count = skipleft; count-- > 0; )
		    dst[count] = src[count];
		for (count = dwords; count-- > 0; )
		    *dst2++ = *src2++;
		src += srcwidth;
		dst += pApm->bytesPerScanline;
	    }
	else
	    while (h-- > 0) {
		CARD32 *src2 = (CARD32 *)(src + skipleft);
		CARD32 *dst2 = (CARD32 *)(dst + skipleft);

		for (count = skipleft; count-- > 0; )
		    dst[count] = src[count];
		for (count = dwords; count-- > 0; )
		    *dst2++ = *src2++;
		for (count = skipright; count-- > 0; )
		    ((char *)dst2)[count] = ((char *)src2)[count];
		src += srcwidth;
		dst += pApm->bytesPerScanline;
	    }

	return;
    }

    if ((skipleft = (long)src & 0x03L)) {
	if (Bpp == 3)
	   skipleft = 4 - skipleft;
	else
	   skipleft /= Bpp;

	if (x < skipleft) {
	   skipleft = 0;
	   beCareful = TRUE;
	   goto BAD_ALIGNMENT;
	}

	x -= skipleft;	     
	w += skipleft;
	
	if (Bpp == 3)
	   src -= 3 * skipleft;  
	else   /* is this Alpha friendly ? */
	   src = (unsigned char*)((long)src & ~0x03L);     
    }

BAD_ALIGNMENT:

    dwords = ((w * Bpp) + 3) >> 2;
    mask = (pApm->bitsPerPixel / 8) - 1;

    if (dwords & mask) {
	/*
	 * Experimental...
	 * It seems the AT3D needs a padding of scanline to a multiple of
	 * 4 pixels, not only bytes.
	 */
	PlusOne = mask - (dwords & mask) + 1;
    }

    A(SetupForImageWrite)(pScrn, rop, planemask, trans, bpp, depth);
    A(SubsequentImageWriteRect)(pScrn, x, y, w, h, skipleft);

    if (beCareful) {
	/* in cases with bad alignment we have to be careful not
	   to read beyond the end of the source */
	if (((x * Bpp) + (dwords << 2)) > srcwidth) h--;
	else beCareful = FALSE;
    }

    srcwidth -= (dwords << 2);

    while (h--) {
	for (count = dwords; count-- > 0; ) {
	    while (!(STATUS() & STATUS_HOSTBLTBUSY))
		;
	    *(CARD32*)pApm->BltMap = *(CARD32*)src;
	    src += 4;
	}
	src += srcwidth;
	for (count = PlusOne; count-- > 0; ) {
	    int	status;

	    while (!((status = STATUS()) & STATUS_HOSTBLTBUSY))
		if (!(status & STATUS_ENGINEBUSY))
		    break;
	    if (status & STATUS_ENGINEBUSY)
		*(CARD32*)pApm->BltMap = 0x00000000;
	}
    }
    if (beCareful) {
       int shift = ((long)src & 0x03L) << 3;

       if (--dwords) {
	    int	count;

	    for (count = dwords >> 2; count-- > 0; ) {
		while (!(STATUS() & STATUS_HOSTBLTBUSY))
		    ;
		*(CARD32*)pApm->BltMap = *(CARD32*)src;
		src += 4;
	    }
       }
       while (!(STATUS() & STATUS_HOSTBLTBUSY))
	   ;
       *((CARD32*)pApm->BltMap) = *((CARD32*)src) >> shift;
    }

    pApm->apmClip = FALSE;
    A(WaitForFifo)(pApm, 1);
    SETCLIP_CTRL(0);
}

static void A(SetupForMono8x8PatternFill)(ScrnInfoPtr pScrn, int patx, int paty,
				          int fg, int bg, int rop,
				          unsigned int planemask)
{
    APMDECL(pScrn);

    DPRINTNAME(SetupForMono8x8PatternFill);
    if (bg != -1) {
	pApm->apmTransparency = FALSE;
	A(WaitForFifo)(pApm, 3 + pApm->apmClip);
	SETBACKGROUNDCOLOR(bg);
	SETFOREGROUNDCOLOR(fg);
	SETROP(apmROP[rop] & 0xF0);
    }
    else {
	pApm->apmTransparency = TRUE;
	A(WaitForFifo)(pApm, 3);
	SETROP(0xCC);
	SETFOREGROUNDCOLOR(fg);
	SETDESTOFF(pApm->ScratchMem);
	A(WaitForFifo)(pApm, 4 + pApm->apmClip);
#ifdef FASTER
	SETDEC(pApm->Setup_DEC | DEC_OP_STRIP | DEC_DEST_LINEAR |
		DEC_DEST_CONTIG | DEC_QUICKSTART_ONDIMX);
	SETWIDTH(pApm->ScratchMemWidth);
	SETDEC(pApm->Setup_DEC | DEC_OP_BLT | DEC_DEST_XY | DEC_SOURCE_CONTIG |
		DEC_PATTERN_88_1bMONO | DEC_DEST_UPD_BLCORNER | DEC_SOURCE_TRANSPARENCY |
		DEC_QUICKSTART_ONDIMX);
#else
	SETWIDTH(pApm->ScratchMemWidth);
	SETDEC(pApm->Setup_DEC | DEC_OP_STRIP | DEC_DEST_LINEAR |
		DEC_DEST_CONTIG | DEC_START);
#endif
	SETROP((apmROP[rop] & 0xF0) | 0x0A);
    }
    if (pApm->apmClip) {
	SETCLIP_CTRL(0);
	pApm->apmClip = FALSE;
    }
}

static void A(SubsequentMono8x8PatternFillRect)(ScrnInfoPtr pScrn, int patx,
					        int paty, int x, int y,
					        int w, int h)
{
    APMDECL(pScrn);

    DPRINTNAME(SubsequentMono8x8PatternFillRect);
    A(WaitForFifo)(pApm, 6);
    SETPATTERN(patx, paty);
    SETDESTXY(x, y);
    UPDATEDEST(x, y + h + 1);
    if (pApm->apmTransparency) {
	int hoff = pApm->ScratchMemWidth / w;

	SETSOURCEOFF(pApm->ScratchMem);
#ifndef FASTER
	if (h > hoff)
	    SETWIDTHHEIGHT(w, hoff);
#endif
	while (h > hoff) {
#ifdef FASTER
	    A(WaitForFifo)(pApm, 4);
	    SETWIDTHHEIGHT(w, hoff);
	    SETPATTERN(patx, paty);
#else
	    A(WaitForFifo)(pApm, 5);
	    SETDEC(pApm->Setup_DEC | DEC_OP_BLT | DEC_SOURCE_LINEAR |
		    DEC_SOURCE_CONTIG | DEC_DEST_XY | DEC_PATTERN_88_1bMONO |
		    DEC_DEST_UPD_BLCORNER | DEC_START);
	    SETPATTERN(patx, paty);
#endif
	    h -= hoff;
	}
#ifdef FASTER
	SETWIDTHHEIGHT(w, h);
#else
	SETWIDTHHEIGHT(w, h);
	SETDEC(pApm->Setup_DEC | DEC_OP_BLT | DEC_DEST_XY |
		DEC_SOURCE_CONTIG | DEC_PATTERN_88_1bMONO |
		DEC_DEST_UPD_BLCORNER | DEC_START);
#endif
    }
    else {
#ifdef FASTER
	SETDEC(pApm->Setup_DEC | ((h == 1) ? DEC_OP_STRIP : DEC_OP_RECT) | 
		DEC_DEST_XY | DEC_PATTERN_88_1bMONO | DEC_DEST_UPD_TRCORNER |
		DEC_QUICKSTART_ONDIMX);
	SETWIDTHHEIGHT(w, h);
#else
	SETWIDTHHEIGHT(w, h);
	SETDEC(pApm->Setup_DEC | ((h == 1) ? DEC_OP_STRIP : DEC_OP_RECT) | 
		DEC_DEST_XY | DEC_PATTERN_88_1bMONO | DEC_DEST_UPD_TRCORNER |
		DEC_START);
#endif
    }
}

static void A(SetupForColor8x8PatternFill)(ScrnInfoPtr pScrn,int patx,int paty,
				           int rop, unsigned int planemask,
					   int transparency_color)
{
    APMDECL(pScrn);

    DPRINTNAME(SetupForColor8x8PatternFillRect);
    if (transparency_color != -1) {
#ifndef FASTER
	pApm->apmTransparency = TRUE;
	A(WaitForFifo)(pApm, 2 + pApm->apmClip);
#else
	A(WaitForFifo)(pApm, 3 + pApm->apmClip);
	SETDEC(pApm->Setup_DEC | DEC_OP_BLT | 
	    DEC_DEST_XY | DEC_PATTERN_88_8bCOLOR | DEC_SOURCE_TRANSPARENCY |
	    DEC_QUICKSTART_ONDIMX);
#endif
	SETBACKGROUNDCOLOR(transparency_color);
    }
    else {
#ifndef FASTER
	pApm->apmTransparency = FALSE;
	A(WaitForFifo)(pApm, 1 + pApm->apmClip);
#else
	A(WaitForFifo)(pApm, 2 + pApm->apmClip);
	SETDEC(pApm->Setup_DEC | DEC_OP_BLT | 
	    DEC_DEST_XY | DEC_PATTERN_88_8bCOLOR | DEC_QUICKSTART_ONDIMX);
#endif
    }
    if (pApm->apmClip) {
	SETCLIP_CTRL(0);
	pApm->apmClip = FALSE;
    }
    SETROP(apmROP[rop]);
}

static void A(SubsequentColor8x8PatternFillRect)(ScrnInfoPtr pScrn, int patx,
					         int paty, int x, int y,
					         int w, int h)
{
    APMDECL(pScrn);

    DPRINTNAME(SubsequentColor8x8PatternFillRect);
#ifndef FASTER
    A(WaitForFifo)(pApm, 5);
#else
    A(WaitForFifo)(pApm, 4);
#endif
    SETSOURCEXY(patx, paty);
    SETDESTXY(x, y);
    SETWIDTHHEIGHT(w, h);
    UPDATEDEST(x + w + 1, y);
#ifndef FASTER
    SETDEC(pApm->Setup_DEC | DEC_OP_BLT | 
	    DEC_DEST_XY | (pApm->apmTransparency * DEC_SOURCE_TRANSPARENCY) |
	    DEC_PATTERN_88_8bCOLOR | DEC_START);
#endif
}
#endif


/* This function is a f*cking kludge since I could not get MMIO to
   work if I initialized in one of the functions in apm_driver.c (like
   preferrably ApmFbInit()... */
void
A(CheckMMIO_Init)(ScrnInfoPtr pScrn)
{
  APMDECL(pScrn);
  volatile int i;

  if (!pApm->apmMMIO_Init)
  {
    pApm->apmMMIO_Init = TRUE;

    A(Sync)(pScrn);

    pApm->Setup_DEC = 0;
    switch(pApm->bitsPerPixel)
    {
      case 8:
           pApm->Setup_DEC |= DEC_BITDEPTH_8;
           break;
      case 16:
           pApm->Setup_DEC |= DEC_BITDEPTH_16;
           break;
      case 24:
           pApm->Setup_DEC |= DEC_BITDEPTH_24;
           break;
      case 32:
           pApm->Setup_DEC |= DEC_BITDEPTH_32;
           break;
      default:
           xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Cannot set up drawing engine control for bpp = %d\n",
		    pScrn->bitsPerPixel);
           break;
    }

    switch(pApm->displayWidth)
    {
      case 640:
           pApm->Setup_DEC |= DEC_WIDTH_640;
           break;
      case 800:
           pApm->Setup_DEC |= DEC_WIDTH_800;
           break;
      case 1024:
           pApm->Setup_DEC |= DEC_WIDTH_1024;
           break;
      case 1152:
           pApm->Setup_DEC |= DEC_WIDTH_1152;
           break;
      case 1280:
           pApm->Setup_DEC |= DEC_WIDTH_1280;
           break;
      case 1600:
           pApm->Setup_DEC |= DEC_WIDTH_1600;
           break;
      default:
           xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "Cannot set up drawing engine control "
		       "for screen width = %d\n", pApm->displayWidth);
           break;
    }

    /* Setup current register values */
    for (i = 0; i < sizeof(pApm->regcurr) / 4; i++)
	((CARD32 *)curr)[i] = RDXL(0x30 + 4*i);

    SETCLIP_CTRL(1);
    SETCLIP_CTRL(0);
    SETBYTEMASK(0x00);
    SETBYTEMASK(0xFF);
    SETROP(ROP_S_xor_D);
    SETROP(ROP_S);

  }
}

static void
A(Sync)(ScrnInfoPtr pScrn)
{
  APMDECL(pScrn);
  volatile u32 i, stat;

  for(i = 0; i < MAXLOOP; i++) {
    stat = STATUS();
    if ((!(stat & (STATUS_HOSTBLTBUSY | STATUS_ENGINEBUSY))) &&
        ((stat & STATUS_FIFO) >= 8))
      break;
  }
  if (i == MAXLOOP) {
    apm_stopit();
    if (!xf86Exiting)
	FatalError("Hung in ApmSync() (Status = 0x%08X)\n", STATUS());
  }
  if (pApm->apmClip) {
    SETCLIP_CTRL(0);
    pApm->apmClip = FALSE;
  }
}


#undef DPRINTNAME
#undef A
#undef DEBUG
#undef DEPTH
