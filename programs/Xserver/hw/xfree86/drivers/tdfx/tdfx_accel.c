/* $XFree86$ */

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that use XAA need this */
#include "xaa.h"
#include "xaalocal.h"
#include "xf86fbman.h"

#include "tdfx.h"

#ifndef PROP_3DFX
#define TDFXWriteLong(p, a, v) TDFXWriteLongMMIO(p, a, v)
#define DECLARE(a)
#define TDFXMakeRoom(p, n) TDFXMakeRoomNoProp(p, n)
#endif

static int TDFXROPCvt[] = {0x00, 0x88, 0x44, 0xCC, 0x22, 0xAA, 0x66, 0xEE,
			   0x11, 0x99, 0x55, 0xDD, 0x33, 0xBB, 0x77, 0xFF,
			   0x00, 0xA0, 0x50, 0xF0, 0x0A, 0xAA, 0x5A, 0xFA,
			   0x05, 0xA5, 0x55, 0xF5, 0x0F, 0xAF, 0x5F, 0xFF};
#define ROP_PATTERN_OFFSET 16

#ifndef PROP_3DFX
static void TDFXMakeRoom(TDFXPtr pTDFX, int size);
#endif
static void TDFXSetClippingRectangle(ScrnInfoPtr pScrn, int left, int top, 
				     int right, int bottom);
static void TDFXDisableClipping(ScrnInfoPtr pScrn);
static void TDFXSetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir, 
					   int ydir, int rop,
					   unsigned int planemask, 
					   int trans_color);
static void TDFXSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int srcX, 
					     int srcY, int dstX, int dstY, 
					     int w, int h);
static void TDFXSetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop, 
				  unsigned int planemask);
static void TDFXSubsequentSolidFillRect(ScrnInfoPtr pScrn, int x, int y, 
					int w, int h);
static void TDFXSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, int patx, 
					   int paty, int fg, int bg, int rop, 
					   unsigned int planemask);
static void TDFXSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, int patx, 
						 int pay, int x, int y, 
						 int w, int h);
static void TDFXSetupForSolidLine(ScrnInfoPtr pScrn, int color, int rop, 
				  unsigned int planemask);
static void TDFXSubsequentSolidTwoPointLine(ScrnInfoPtr pScrn, int srcx, 
					    int srcy, int dstx, int dsty, 
					    int flags);
static void TDFXSubsequentSolidHorVertLine(ScrnInfoPtr pScrn, int x, int y, 
					   int len, int dir);
#if 0
static void TDFXNonTEGlyphRenderer(ScrnInfoPtr pScrn, int x, int y, int n, 
				   NonTEGlyphPtr glyphs, BoxPtr pbox, int fg, 
				   int rop, unsigned int planemask);
#endif

void
TDFXNeedSync(ScrnInfoPtr pScrn) {
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  pTDFX->syncDone=FALSE;
}

static void
TDFXFirstSync(ScrnInfoPtr pScrn) {
  TDFXPtr pTDFX = TDFXPTR(pScrn);

  if (!pTDFX->syncDone) {
    pTDFX->sync(pScrn);
    pTDFX->syncDone=TRUE;
  }
}

void
TDFXCheckSync(ScrnInfoPtr pScrn) {
  TDFXPtr pTDFX = TDFXPTR(pScrn);

  if (pTDFX->syncDone) {
    pTDFX->sync(pScrn);
    pTDFX->syncDone=FALSE;
  }
}

Bool
TDFXAccelInit(ScreenPtr pScreen)
{
  XAAInfoRecPtr infoPtr;
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  CARD32 commonFlags;

  pTDFX->AccelInfoRec = infoPtr = XAACreateInfoRec();
  if (!infoPtr) return FALSE;

  infoPtr->Flags =  PIXMAP_CACHE | 
    OFFSCREEN_PIXMAPS |
    LINEAR_FRAMEBUFFER |
    MICROSOFT_ZERO_LINE_BIAS;

  infoPtr->Sync = pTDFX->sync;
  infoPtr->SetClippingRectangle = TDFXSetClippingRectangle;
  infoPtr->DisableClipping = TDFXDisableClipping;
  infoPtr->ClippingFlags = HARDWARE_CLIP_SCREEN_TO_SCREEN_COLOR_EXPAND |
    HARDWARE_CLIP_SCREEN_TO_SCREEN_COPY |
    HARDWARE_CLIP_MONO_8x8_FILL |
    HARDWARE_CLIP_COLOR_8x8_FILL |
    HARDWARE_CLIP_SOLID_FILL |
    HARDWARE_CLIP_DASHED_LINE |
    HARDWARE_CLIP_SOLID_LINE;

  commonFlags = BIT_ORDER_IN_BYTE_MSBFIRST | NO_PLANEMASK;

  infoPtr->SetupForScreenToScreenCopy = TDFXSetupForScreenToScreenCopy;
  infoPtr->SubsequentScreenToScreenCopy = TDFXSubsequentScreenToScreenCopy;
  infoPtr->ScreenToScreenCopyFlags = commonFlags;

  infoPtr->SetupForSolidFill = TDFXSetupForSolidFill;
  infoPtr->SubsequentSolidFillRect = TDFXSubsequentSolidFillRect;
  infoPtr->SolidFillFlags = commonFlags;

  infoPtr->SetupForMono8x8PatternFill = TDFXSetupForMono8x8PatternFill;
  infoPtr->SubsequentMono8x8PatternFillRect = TDFXSubsequentMono8x8PatternFillRect;
  infoPtr->Mono8x8PatternFillFlags = commonFlags |
    HARDWARE_PATTERN_PROGRAMMED_BITS |
    HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
    HARDWARE_PATTERN_SCREEN_ORIGIN;

  infoPtr->SetupForSolidLine = TDFXSetupForSolidLine;
  infoPtr->SubsequentSolidTwoPointLine = TDFXSubsequentSolidTwoPointLine;
  infoPtr->SubsequentSolidHorVertLine = TDFXSubsequentSolidHorVertLine;
  infoPtr->SolidLineFlags = commonFlags;

#if 0
  infoPtr->NonTEGlyphRenderer = TDFXNonTEGlyphRenderer;
  infoPtr->NonTEGlyphRendererFlags = commonFlags;

  infoPtr->TEGlyphRenderer = TDFXTEGlyphRenderer;
  infoPtr->TEGlyphRendererFlags = commonFlags;
#endif

  infoPtr->CPUToScreenColorExpandFillFlags = commonFlags;

  pTDFX->PciCnt=TDFXReadLongMMIO(pTDFX, 0)&0x1F;
  pTDFX->DrawState=0;

  TDFXWriteLongMMIO(pTDFX, SST_2D_SRCBASEADDR, pTDFX->fbOffset);
  TDFXWriteLongMMIO(pTDFX, SST_2D_DSTBASEADDR, pTDFX->fbOffset);

  /* Fill in acceleration functions */
  return XAAInit(pScreen, infoPtr);
}

static void TDFXMakeRoomNoProp(TDFXPtr pTDFX, int size) {
  int stat;

  pTDFX->PciCnt-=size;
  if (pTDFX->PciCnt<1) {
    do {
      stat=TDFXReadLongMMIO(pTDFX, 0);
      pTDFX->PciCnt=stat&0x1F;
    } while (pTDFX->PciCnt<size);
  }
}

void TDFXSync(ScrnInfoPtr pScrn)
{
  TDFXPtr pTDFX;
  int i;
  int stat;

  TDFXTRACEACCEL("TDFXSync start\n");
  pTDFX=TDFXPTR(pScrn);
  TDFXMakeRoomNoProp(pTDFX, 1);
  TDFXWriteLongMMIO(pTDFX, SST_3D_COMMAND, SST_3D_NOP);
  i=0;
  do {
    stat=TDFXReadLongMMIO(pTDFX, 0);
    if (stat&SST_BUSY) i=0; else i++;
  } while (i<3);
  pTDFX->PciCnt=stat&0x1F;
}

static void
TDFXMatchState(TDFXPtr pTDFX)
{
#ifdef PROP_3DFX
  /* FLUSH_WCB(); !!! Is this really needed? !!! */
#endif
  if (pTDFX->DrawState&DRAW_STATE_CLIPPING)
    pTDFX->Cmd |= BIT(23);
  else
    pTDFX->Cmd &= ~BIT(23);
  if (pTDFX->DrawState&DRAW_STATE_TRANSPARENT) {
    TDFXMakeRoom(pTDFX, 1);
    DECLARE(SSTCP_COMMANDEXTRA);
    TDFXWriteLong(pTDFX, SST_2D_COMMANDEXTRA, 0);
    pTDFX->DrawState&=~DRAW_STATE_TRANSPARENT;
  }
}

static void
TDFXSetClippingRectangle(ScrnInfoPtr pScrn, int left, int top, int right, 
			 int bottom)
{
  TDFXPtr pTDFX;

  TDFXTRACEACCEL("TDFXSetClippingRectangle start\n");
  pTDFX=TDFXPTR(pScrn);
  TDFXMakeRoom(pTDFX, 2);
  DECLARE(SSTCP_CLIP1MIN|SSTCP_CLIP1MAX);
  TDFXWriteLong(pTDFX, SST_2D_CLIP1MIN, (top&0xFFF)<<16 | (left&0xFFF));
  TDFXWriteLong(pTDFX, SST_2D_CLIP1MAX, ((bottom+1)&0xFFF)<<16 | 
		    ((right+1)&0xFFF));
  pTDFX->DrawState|=DRAW_STATE_CLIPPING;
}

static void
TDFXDisableClipping(ScrnInfoPtr pScrn)
{
  TDFXPtr pTDFX;

  TDFXTRACEACCEL("TDFXDisableClippingRectangle start\n");
  pTDFX=TDFXPTR(pScrn);
  pTDFX->DrawState&=~DRAW_STATE_CLIPPING;
}

static void
TDFXSetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir, int ydir, int rop,
			       unsigned int planemask, int trans_color)
{
  TDFXPtr pTDFX;
  int fmt;

  TDFXTRACEACCEL("TDFXSetupForScreenToScreenCopy start\n xdir=%d ydir=%d rop=%d planemask=%d trans_color=%d\n", xdir, ydir, rop, planemask, trans_color);
  pTDFX=TDFXPTR(pScrn);
  TDFXFirstSync(pScrn);
  if (trans_color!=-1) {
    TDFXMakeRoom(pTDFX, 4);
    DECLARE(SSTCP_SRCCOLORKEYMIN|SSTCP_SRCCOLORKEYMAX|
		   SSTCP_ROP|SSTCP_COMMANDEXTRA);
    TDFXWriteLong(pTDFX, SST_2D_SRCCOLORKEYMIN, trans_color);
    TDFXWriteLong(pTDFX, SST_2D_SRCCOLORKEYMAX, trans_color);
    TDFXWriteLong(pTDFX, SST_2D_ROP, TDFXROPCvt[GXinvert]<<8);
    TDFXWriteLong(pTDFX, SST_2D_COMMANDEXTRA, SST_2D_SRC_COLORKEY_EX);
    pTDFX->DrawState|=DRAW_STATE_TRANSPARENT;
  }
  pTDFX->Cmd = (TDFXROPCvt[rop]<<24) | SST_2D_SCRNTOSCRNBLIT;
  if (xdir==-1) pTDFX->Cmd |= SST_2D_X_RIGHT_TO_LEFT;
  if (ydir==-1) pTDFX->Cmd |= SST_2D_Y_BOTTOM_TO_TOP;
  if (pTDFX->cpp==1) fmt=pTDFX->stride|(1<<16); 
  else fmt=pTDFX->stride|((pTDFX->cpp+1)<<16);
  TDFXMakeRoom(pTDFX, 2);
  DECLARE(SSTCP_SRCFORMAT|SSTCP_DSTFORMAT);
  TDFXWriteLong(pTDFX, SST_2D_DSTFORMAT, fmt);
  TDFXWriteLong(pTDFX, SST_2D_SRCFORMAT, fmt);
}  

static void
TDFXSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int srcX, int srcY, 
				 int dstX, int dstY, int w, int h) 
{
  TDFXPtr pTDFX;

  TDFXTRACEACCEL("TDFXSubsequentScreenToScreenCopy start\n srcX=%d srcY=%d dstX=%d dstY=%d w=%d h=%d\n", srcX, srcY, dstX, dstY, w, h);
  pTDFX=TDFXPTR(pScrn);
  if (pTDFX->Cmd&BIT(15)) {
    srcY += h-1;
    dstY += h-1;
  } 
  if (pTDFX->Cmd&BIT(14)) {
    srcX += w-1;
    dstX += w-1;
  }
  pTDFX->Cmd|=SST_2D_GO;
  TDFXMatchState(pTDFX);
  /* Board hangs if you send sequential overlapping blits */
  if (!((srcX+w<pTDFX->prevBlitDest.x1) || (srcX>pTDFX->prevBlitDest.x2) ||
	(srcY+h<pTDFX->prevBlitDest.y1) || (srcY>pTDFX->prevBlitDest.y2))) {
    TDFXMakeRoom(pTDFX, 1);
    DECLARE(SSTCP_COMMAND);
    TDFXWriteLong(pTDFX, SST_2D_COMMAND, SST_2D_NOP|SST_2D_GO);
  }
  TDFXMakeRoom(pTDFX, 4);
  DECLARE(SSTCP_DSTSIZE|SSTCP_DSTXY|SSTCP_SRCXY|SSTCP_COMMAND);
  TDFXWriteLong(pTDFX, SST_2D_SRCXY, (srcX&0x1FFF) | ((srcY&0x1FFF)<<16));
  TDFXWriteLong(pTDFX, SST_2D_DSTSIZE, (w&0x1FFF) | ((h&0x1FFF)<<16));
  TDFXWriteLong(pTDFX, SST_2D_DSTXY, (dstX&0x1FFF) | ((dstY&0x1FFF)<<16));
  TDFXWriteLong(pTDFX, SST_2D_COMMAND, pTDFX->Cmd);
  if (pTDFX->DrawState&DRAW_STATE_TRANSPARENT) {
    TDFXMakeRoom(pTDFX, 1);
    DECLARE(SSTCP_COMMANDEXTRA);
    TDFXWriteLong(pTDFX, SST_2D_COMMANDEXTRA, 0);
    pTDFX->DrawState&=~DRAW_STATE_TRANSPARENT;
  }
  pTDFX->prevBlitDest.x1=dstX;
  pTDFX->prevBlitDest.x2=dstX+w-1;
  pTDFX->prevBlitDest.y1=dstY;
  pTDFX->prevBlitDest.y2=dstY+h-1;
}

static void
TDFXSetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop, 
		      unsigned int planemask)
{
  TDFXPtr pTDFX;
  int fmt;

  TDFXTRACEACCEL("TDFXSetupForSolidFill start color=%d rop=%d planemask=%d\n", color, rop, planemask);
  pTDFX=TDFXPTR(pScrn);
  pTDFX->Cmd=TDFXROPCvt[rop]<<24;
  if (pTDFX->cpp==1) fmt=(1<<16)|pTDFX->stride; 
  else fmt=((pTDFX->cpp+1)<<16)|pTDFX->stride;
  TDFXFirstSync(pScrn);
  TDFXMakeRoom(pTDFX, 3);
  DECLARE(SSTCP_DSTFORMAT|SSTCP_COLORFORE|
		 SSTCP_COLORBACK);
  TDFXWriteLong(pTDFX, SST_2D_DSTFORMAT, fmt);
  TDFXWriteLong(pTDFX, SST_2D_COLORBACK, color);
  TDFXWriteLong(pTDFX, SST_2D_COLORFORE, color);
}

static void
TDFXSubsequentSolidFillRect(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
  TDFXPtr pTDFX;

  TDFXTRACEACCEL("TDFXSubsequentSolidFillRect start x=%d y=%d w=%d h=%d\n", x, y, w, h);
  pTDFX=TDFXPTR(pScrn);
  TDFXMatchState(pTDFX);
  TDFXMakeRoom(pTDFX, 3);
  DECLARE(SSTCP_DSTSIZE|SSTCP_DSTXY|SSTCP_COMMAND);
  TDFXWriteLong(pTDFX, SST_2D_DSTSIZE, ((h&0x1FFF)<<16) | (w&0x1FFF));
  TDFXWriteLong(pTDFX, SST_2D_DSTXY, ((y&0x1FFF)<<16) | (x&0x1FFF));
  TDFXWriteLong(pTDFX, SST_2D_COMMAND, SST_2D_RECTANGLEFILL|pTDFX->Cmd|SST_2D_GO);
}

static void
TDFXSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, int patx, int paty,
			       int fg, int bg, int rop, unsigned int planemask)
{
  TDFXPtr pTDFX;
  int fmt;

  TDFXTRACEACCEL("TDFXSetupForMono8x8PatternFill start patx=%x paty=%x fg=%d bg=%d rop=%d planemask=%d\n", patx, paty, fg, bg, rop, planemask);
  pTDFX=TDFXPTR(pScrn);
  pTDFX->Cmd = (TDFXROPCvt[rop+ROP_PATTERN_OFFSET]<<24) | SST_2D_MONOCHROME_PATTERN;
  if (bg==-1) {
    pTDFX->Cmd |= SST_2D_TRANSPARENT_MONOCHROME;
  }

  if (pTDFX->cpp==1) fmt=(1<<16)|pTDFX->stride; 
  else fmt=((pTDFX->cpp+1)<<16)|pTDFX->stride;
  TDFXFirstSync(pScrn);
  TDFXMakeRoom(pTDFX, 5);
  DECLARE(SSTCP_DSTFORMAT|SSTCP_PATTERN0ALIAS
		  |SSTCP_PATTERN1ALIAS|SSTCP_COLORFORE|
		  SSTCP_COLORBACK);
  TDFXWriteLong(pTDFX, SST_2D_DSTFORMAT, fmt);
  TDFXWriteLong(pTDFX, SST_2D_PATTERN0, patx);
  TDFXWriteLong(pTDFX, SST_2D_PATTERN1, paty);
  TDFXWriteLong(pTDFX, SST_2D_COLORBACK, bg);
  TDFXWriteLong(pTDFX, SST_2D_COLORFORE, fg);
}

static void
TDFXSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, int patx, int paty,
				     int x, int y, int w, int h)
{
  TDFXPtr pTDFX;

  TDFXTRACEACCEL("TDFXSubsequentMono8x8PatternFillRect start patx=%x paty=%x x=%d y=%d w=%d h=%d\n", patx, paty, x, y, w, h);
  pTDFX=TDFXPTR(pScrn);
  pTDFX->Cmd |= ((patx&0x7)<<SST_2D_X_PATOFFSET_SHIFT) |
    ((paty&0x7)<<SST_2D_Y_PATOFFSET_SHIFT);
  TDFXMatchState(pTDFX);
  TDFXSubsequentSolidFillRect(pScrn, x, y, w, h);
}

static void
TDFXSetupForSolidLine(ScrnInfoPtr pScrn, int color, int rop, 
		      unsigned int planemask)
{
  TDFXPtr pTDFX;

  TDFXTRACEACCEL("TDFXSetupForSolidLine start\n");
  pTDFX=TDFXPTR(pScrn);
  pTDFX->Cmd = (TDFXROPCvt[rop]<<24) | SST_2D_MONOCHROME_PATTERN;
  TDFXFirstSync(pScrn);
  TDFXMakeRoom(pTDFX, 2);
  DECLARE(SSTCP_COLORFORE|SSTCP_COLORBACK);
  TDFXWriteLong(pTDFX, SST_2D_COLORBACK, color);
  TDFXWriteLong(pTDFX, SST_2D_COLORFORE, color);
}

static void
TDFXSubsequentSolidTwoPointLine(ScrnInfoPtr pScrn, int srcx, int srcy,
				int dstx, int dsty, int flags)
{
  TDFXPtr pTDFX;

  TDFXTRACEACCEL("TDFXSubsequentSolidTwoPointLine start\n");
  pTDFX=TDFXPTR(pScrn);
  if (flags&OMIT_LAST) pTDFX->Cmd|=SST_2D_POLYLINE;
  else pTDFX->Cmd|=SST_2D_LINE;
  TDFXMatchState(pTDFX);
  TDFXMakeRoom(pTDFX, 3);
  DECLARE(SSTCP_SRCXY|SSTCP_DSTXY|SSTCP_COMMAND);
  TDFXWriteLong(pTDFX, SST_2D_SRCXY, (srcy&0x1FFF)<<16 | (srcx&0x1FFF));
  TDFXWriteLong(pTDFX, SST_2D_DSTXY, (dsty&0x1FFF)<<16 | (dstx&0x1FFF));
  TDFXWriteLong(pTDFX, SST_2D_COMMAND, pTDFX->Cmd|SST_2D_GO);
}

static void
TDFXSubsequentSolidHorVertLine(ScrnInfoPtr pScrn, int x, int y, int len, 
			       int dir)
{
  TDFXPtr pTDFX;

  TDFXTRACEACCEL("TDFXSubsequentSolidHorVertLine start\n");
  pTDFX=TDFXPTR(pScrn);
  TDFXMatchState(pTDFX);
  TDFXMakeRoom(pTDFX, 3);
  DECLARE(SSTCP_SRCXY|SSTCP_DSTXY|SSTCP_COMMAND);
  TDFXWriteLong(pTDFX, SST_2D_SRCXY, (y&0x1FFF)<<16 | (x&0x1FFF));
  if (dir == DEGREES_0)
    TDFXWriteLong(pTDFX, SST_2D_DSTXY, (y&0x1FFF)<<16 | ((x+len)&0x1FFF));
  else
    TDFXWriteLong(pTDFX, SST_2D_DSTXY, ((y+len)&0x1FFF)<<16 | (x&0x1FFF));
  TDFXWriteLong(pTDFX, SST_2D_COMMAND, pTDFX->Cmd|SST_2D_POLYLINE|SST_2D_GO);
}

#if 0
static void
TDFXNonTEGlyphRenderer(ScrnInfoPtr pScrn, int x, int y, int n, 
		       NonTEGlyphPtr glyphs, BoxPtr pbox, int fg, int rop,
		       unsigned int planemask)
{
  TDFXPtr pTDFX;
  int ndwords;
  int g;
  int clip0min, clip0max, srcformat;

  TDFXTRACEACCEL("TDFXNonTEGlyphRenderer start\n");
  pTDFX = TDFXPTR(pScrn);
  clip0min=TDFXReadLongMMIO(pTDFX, SST_2D_CLIP0MIN);
  clip0max=TDFXReadLongMMIO(pTDFX, SST_2D_CLIP0MAX);
  srcformat=TDFXReadLongMMIO(pTDFX, SST_2D_SRCFORMAT);
  TDFXMakeRoom(pTDFX, 6);
  DECLARE(SSTCP_CLIP0MIN|SSTCP_CLIP0MAX|SSTCP_SRCFORMAT|
		  SSTCP_SRCXY|SSTCP_COLORFORE|SSTCP_COMMAND);
  TDFXWriteLong(pTDFX, SST_2D_CLIP0MIN, (pbox->y1<<16) | pbox->x1);
  TDFXWriteLong(pTDFX, SST_2D_CLIP0MAX, (pbox->u2<<16) | pbox->x2);
  TDFXWriteLong(pTDFX, SST_2D_SRCFORMAT, SST_2D_SRC_PIXFMT_1BPP |
		    SST_2D_SOURCE_PACKING_WORD);
  TDFXWriteLong(pTDFX, SST_2D_SRCXY, 0);
  TDFXWriteLong(pTDFX, SST_2D_COLORFORE, fg);
  TDFXWriteLong(pTDFX, SST_2D_COMMAND, pTDFX->Cmd | (TDFXROPCvt[rop]<<24) |
		    SST_2D_TRANSPARENT_MONOCHROME | SST_2D_SCRNTOSCRNBLIT);
  for (g=0, glyph=glyphs; g<n; g++, glyph++) {
    int dx = x+glyph->start;
    int dy = y-glyph->yoff;
    char *bitmap = glyph->bits;
    int w = glyph->end - glyph->start;

    ndwords = (glyph->srcwidth+31)>>5;
    ndwords *= glyph->height;

    TDFXMakeRoom(pTDFX, 2);
    DECLARE(SSTCP_DSTSIZE|SSTCP_DSTXY);
    TDFXWriteLong(pTDFX, SST_2D_DSTSIZE, (glyph->height<<16)|w);
    TDFXWriteLong(pTDFX, SST_2D_DSTXY, (dy<<16)|dx&0xFFFF);

    do {
      int i = ndwords;
      int j;
      int *glyph_data = (int*)bitmap;
      int dwords_per_line = (glyph->srcwidth+3)>>2;

      /* !!! This needs to change to for proprietary interface !!! */
      if (i>LAUNCH_AREA_SIZE) i=LAUNCH_AREA_SIZE;
      TDFXMakeRoom(pTDFX, i);
      for (j=0; j<i; j++) {
	TDFXWriteLong(pTDFX, SST_2D_LAUNCH+j, XAAReverseBitOrder(*glyph_data));
	glyph_data += dwords_per_line;
      }
      bitmap += i*4;
      ndwords -= i;
    } while (ndwords);
  }

  TDFXMakeRoom(pTDFX, 3);
  DECLARE(SSTCP_CLIP0MIN|SSTCP_CLIP0MAX|SSTCP_SRCFORMAT);
  TDFXWriteLong(pTDFX, SST_2D_CLIP0MIN, clip0min);
  TDFXWriteLong(pTDFX, SST_2D_CLIP0MAX, clip0max);
  TDFXWriteLong(pTDFX, SST_2D_SRCFORMAT, srcformat);
}
#endif


