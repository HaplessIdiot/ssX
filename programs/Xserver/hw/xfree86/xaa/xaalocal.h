/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaalocal.h,v 1.1.2.10 1998/07/24 11:36:47 dawes Exp $ */

#ifndef _XAALOCAL_H
#define _XAALOCAL_H

/* This file is very unorganized ! */


#include "gcstruct.h"
#include "regionstr.h"
#include "xaa.h"
#include "mi.h"

#define GCWhenForced		(GCArcMode << 1)

#define DO_COLOR_8x8		0x00000001
#define DO_MONO_8x8		0x00000002
#define DO_CACHE_BLT		0x00000003
#define DO_COLOR_EXPAND		0x00000004


typedef CARD32 * (*GlyphScanlineFuncPtr)(
    CARD32 *base, unsigned int **glyphp, int line, int nglyph, int width
);

typedef CARD32 *(*StippleScanlineProcPtr)(CARD32*, CARD32*, int, int, int); 



typedef struct _XAAScreen {
   CreateGCProcPtr 		CreateGC;
   CloseScreenProcPtr 		CloseScreen;
   GetImageProcPtr 		GetImage;
   GetSpansProcPtr 		GetSpans;
   SourceValidateProcPtr 	SourceValidate;
   PaintWindowBackgroundProcPtr PaintWindowBackground;
   PaintWindowBorderProcPtr 	PaintWindowBorder;
   CopyWindowProcPtr 		CopyWindow;
   ClearToBackgroundProcPtr 	ClearToBackground;
   BSFuncRec 			BackingStoreFuncs;
   CreatePixmapProcPtr 		CreatePixmap;
   XAAInfoRecPtr 		AccelInfoRec;
   Bool                		(*EnterVT)(int, int);
   void                		(*LeaveVT)(int, int);
} XAAScreenRec, *XAAScreenPtr;

typedef struct _XAAGC {
    GCOps 	*wrapOps;
    GCFuncs 	*wrapFuncs;
    GCOps 	*XAAOps;
    Bool	isPixmap;
} XAAGCRec, *XAAGCPtr;

#define REDUCIBILITY_CHECKED	0x00000001
#define REDUCIBLE_TO_8x8	0x00000002
#define REDUCIBLE_TO_2_COLOR	0x00000004

typedef struct _XAAPixmap {
    int flags;
    CARD32 pattern0;
    CARD32 pattern1;
    int fg;
    int bg;    
} XAAPixmapRec, *XAAPixmapPtr;


Bool 
XAACreateGC(
    GCPtr pGC
);

Bool
XAAInitAccel(
    ScreenPtr pScreen, 
    XAAInfoRecPtr infoRec
);

RegionPtr
XAABitBlt(
    DrawablePtr pSrcDrawable,
    DrawablePtr pDstDrawable,
    GC *pGC,
    int srcx,
    int srcy,
    int width,
    int height,
    int dstx,
    int dsty,
    void (*doBitBlt)(DrawablePtr, DrawablePtr, GCPtr, RegionPtr, DDXPointPtr),
    unsigned long bitPlane
);

void 
XAAScreenToScreenBitBlt(
    ScrnInfoPtr pScrn,
    int nbox,
    DDXPointPtr pptSrc,
    BoxPtr pbox,
    int xdir, 
    int ydir,
    int alu,
    unsigned int planemask
);

void
XAADoBitBlt(
    DrawablePtr	    pSrc, 
    DrawablePtr     pDst,
    GC		    *pGC,
    RegionPtr	    prgnDst,
    DDXPointPtr	    pptSrc
);

void
XAADoImageWrite(
    DrawablePtr	    pSrc, 
    DrawablePtr     pDst,
    GC		    *pGC,
    RegionPtr	    prgnDst,
    DDXPointPtr	    pptSrc
);

void 
XAACopyWindow(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc
);


RegionPtr 
XAACopyArea(
    DrawablePtr pSrcDrawable,
    DrawablePtr pDstDrawable,
    GC *pGC,
    int srcx, 
    int srcy,
    int width, 
    int height,
    int dstx, 
    int dsty
);

void
XAAValidateCopyArea(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw
);

void
XAAValidateCopyPlane(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw
);

void
XAAValidatePushPixels(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw
);

void
XAAValidateFillSpans(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw
);

void
XAAValidatePolyGlyphBlt(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw
);

void
XAAValidateImageGlyphBlt(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw
);

void
XAAValidateFillPolygon(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw
);

void
XAAValidatePolyFillArc(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw
);

void
XAAValidatePolylines(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw
);


RegionPtr
XAACopyPlaneColorExpansion(
    DrawablePtr		pSrc,
    DrawablePtr		pDst,
    GCPtr		pGC,
    int			srcx, 
    int			srcy,
    int			width, 
    int			height,
    int			dstx, 
    int			dsty,
    unsigned long	bitPlane
);


void
XAAPushPixelsSolidColorExpansion(
    GCPtr	pGC,
    PixmapPtr	pBitMap,
    DrawablePtr pDrawable,
    int		dx, 
    int		dy, 
    int		xOrg, 
    int		yOrg
);

void
XAAWriteBitmapColorExpandMSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapColorExpand3MSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapColorExpandMSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapColorExpand3MSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapColorExpandLSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapColorExpand3LSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapColorExpandLSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapColorExpand3LSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);


void
XAAWriteBitmapScanlineColorExpandMSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapScanlineColorExpand3MSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapScanlineColorExpandMSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapScanlineColorExpand3MSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapScanlineColorExpandLSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapScanlineColorExpand3LSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapScanlineColorExpandLSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void
XAAWriteBitmapScanlineColorExpand3LSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
);

void 
XAAWritePixmap (
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   unsigned char *src,
   int srcwidth,
   int rop,
   unsigned int planemask,
   int transparency_color,
   int bpp, int depth
);

void 
XAAWritePixmapScanline (
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   unsigned char *src,
   int srcwidth,
   int rop,
   unsigned int planemask,
   int transparency_color,
   int bpp, int depth
);

int
XAAGetRectClipBoxes(
    RegionPtr prgnClip,
    BoxPtr pboxClippedBase,
    int nrectFill,
    xRectangle *prectInit
);

void
XAAFillSolidRects(
    ScrnInfoPtr pScrn,
    int fg, int rop,
    unsigned int planemask,
    int		nBox,
    BoxPtr	pBox 
);

void
XAAFillMono8x8PatternRects(
    ScrnInfoPtr pScrn,
    int	fg, int bg, int rop,
    unsigned int planemask,
    int	nBox,
    BoxPtr pBox,
    int pat0, int pat1,
    int xorg, int yorg
);

void
XAAFillMono8x8PatternRectsScreenOrigin(
    ScrnInfoPtr pScrn,
    int	fg, int bg, int rop,
    unsigned int planemask,
    int	nBox,
    BoxPtr pBox,
    int pat0, int pat1,
    int xorg, int yorg
);


void
XAAFillColor8x8PatternRectsScreenOrigin(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorigin, int yorigin,
   XAACacheInfoPtr pCache
);

void
XAAFillColor8x8PatternRects(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorigin, int yorigin,
   XAACacheInfoPtr pCache
);


void 
XAAFillCacheBltRects(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorg, int yorg,
   XAACacheInfoPtr pCache
);
void
XAAPolyFillRectSolid(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	nrectFill,
    xRectangle *prectInit
);

void
XAAPolyFillRectMono8x8Pattern(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	nrectFill,
    xRectangle *prectInit
);

void
XAAPolyFillRectColor8x8Pattern(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	nrectFill,
    xRectangle *prectInit
);


void
XAAPolyFillRectCacheBlt(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	nrectFill,
    xRectangle *prectInit
);

void
XAAPolyFillRectColorExpand(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	nrectFill,
    xRectangle *prectInit
);

void
XAATEGlyphRendererMSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
);

void
XAATEGlyphRenderer3MSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
);

void
XAATEGlyphRendererMSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
);

void
XAATEGlyphRenderer3MSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
);

void
XAATEGlyphRendererLSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
);


void
XAATEGlyphRenderer3LSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
);

void
XAATEGlyphRendererLSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
);

void
XAATEGlyphRenderer3LSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
);


void
XAATEGlyphRendererScanlineMSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
);

void
XAATEGlyphRendererScanline3MSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
);

void
XAATEGlyphRendererScanlineLSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
);

void
XAATEGlyphRendererScanline3LSBFirst (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft, int startline, 
    unsigned int **glyphs, int glyphWidth,
    int fg, int bg, int rop, unsigned planemask
);


extern CARD32 *(*XAAGlyphScanlineFuncMSBFirstFixedBase[32])(
   CARD32 *base, unsigned int **glyphp, int line, int nglyph, int width
);

extern CARD32 *(*XAAGlyphScanlineFuncMSBFirst[32])(
   CARD32 *base, unsigned int **glyphp, int line, int nglyph, int width
);

extern CARD32 *(*XAAGlyphScanlineFuncLSBFirstFixedBase[32])(
   CARD32 *base, unsigned int **glyphp, int line, int nglyph, int width
);

extern CARD32 *(*XAAGlyphScanlineFuncLSBFirst[32])(
   CARD32 *base, unsigned int **glyphp, int line, int nglyph, int width
);

void
XAAFillColorExpandRectsLSBFirst(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorg, int yorg,
   PixmapPtr pPix
);

void
XAAFillColorExpandRectsLSBFirstFixedBase(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorg, int yorg,
   PixmapPtr pPix
);


void
XAAFillColorExpandRectsMSBFirst(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorg, int yorg,
   PixmapPtr pPix
);


void
XAAFillColorExpandRectsMSBFirstFixedBase(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorg, int yorg,
   PixmapPtr pPix
);



void
XAAFillScanlineColorExpandRectsLSBFirst(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorg, int yorg,
   PixmapPtr pPix
);


void
XAAFillScanlineColorExpandRectsMSBFirst(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorg, int yorg,
   PixmapPtr pPix
);


void
XAAFillColorExpandSpansLSBFirst(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth,
   int fSorted,
   int xorg, int yorg,
   PixmapPtr pPix
);

void
XAAFillColorExpandSpansLSBFirstFixedBase(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth,
   int fSorted,
   int xorg, int yorg,
   PixmapPtr pPix
);


void
XAAFillColorExpandSpansMSBFirst(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth,
   int fSorted,
   int xorg, int yorg,
   PixmapPtr pPix
);


void
XAAFillColorExpandSpansMSBFirstFixedBase(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth,
   int fSorted,
   int xorg, int yorg,
   PixmapPtr pPix
);


void
XAAFillScanlineColorExpandSpansLSBFirst(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth,
   int fSorted,
   int xorg, int yorg,
   PixmapPtr pPix
);


void
XAAFillScanlineColorExpandSpansMSBFirst(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth,
   int fSorted,
   int xorg, int yorg,
   PixmapPtr pPix
);


extern CARD32 *(*XAAStippleScanlineFuncMSBFirstFixedBase[6])(
   CARD32* base, CARD32* src, int offset, int width, int dwords
);

extern CARD32 *(*XAAStippleScanlineFuncMSBFirst[6])(
   CARD32* base, CARD32* src, int offset, int width, int dwords
);

extern CARD32 *(*XAAStippleScanlineFuncLSBFirstFixedBase[6])(
   CARD32* base, CARD32* src, int offset, int width, int dwords
);

extern CARD32 *(*XAAStippleScanlineFuncLSBFirst[6])(
   CARD32* base, CARD32* src, int offset, int width, int dwords
);

int
XAAPolyText8TEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	x, int y,
    int count,
    char *chars
);

int
XAAPolyText16TEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	x, int y,
    int count,
    unsigned short *chars
);

void
XAAImageText8TEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	x, int y,
    int count,
    char *chars
);

void
XAAImageText16TEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	x, int y,
    int count,
    unsigned short *chars
);

void
XAAImageGlyphBltTEColorExpansion(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int xInit, int yInit,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase
);

void
XAAPolyGlyphBltTEColorExpansion(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int xInit, int yInit,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase
);


int
XAAPolyText8NonTEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	x, int y,
    int count,
    char *chars
);

int
XAAPolyText16NonTEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	x, int y,
    int count,
    unsigned short *chars
);

void
XAAImageText8NonTEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	x, int y,
    int count,
    char *chars
);

void
XAAImageText16NonTEColorExpansion(
    DrawablePtr pDraw,
    GCPtr pGC,
    int	x, int y,
    int count,
    unsigned short *chars
);

void
XAAImageGlyphBltNonTEColorExpansion(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int xInit, int yInit,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase
);

void
XAAPolyGlyphBltNonTEColorExpansion(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int xInit, int yInit,
    unsigned int nglyph,
    CharInfoPtr *ppci,
    pointer pglyphBase
);

void
XAANonTEGlyphRendererMSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);

void
XAANonTEGlyphRenderer3MSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);

void
XAANonTEGlyphRendererMSBFirst (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);

void
XAANonTEGlyphRenderer3MSBFirst (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);

void
XAANonTEGlyphRendererLSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);

void
XAANonTEGlyphRenderer3LSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);

void
XAANonTEGlyphRendererLSBFirst (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);

void
XAANonTEGlyphRenderer3LSBFirst (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);


void
XAANonTEGlyphRendererScanlineMSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);

void
XAANonTEGlyphRendererScanlineMSBFirst (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);

void
XAANonTEGlyphRendererScanline3MSBFirst (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);

void
XAANonTEGlyphRendererScanlineLSBFirstFixedBase (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);

void
XAANonTEGlyphRendererScanlineLSBFirst (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);

void
XAANonTEGlyphRendererScanline3LSBFirst (
    ScrnInfoPtr pScrn,
    int xback, int wback, int xtext, int wtext, 
    int w, int h, int skipleft, int startline, 
    NonTEGlyphInfo *glyphs,
    int fg, int bg, int rop, unsigned planemask
);


CARD32* 
XAANonTEGlyphScanlineFuncMSBFirstFixedBase(
    CARD32 *base,
    NonTEGlyphInfo *glyphp,
    int line, int TotalWidth, int skipleft
);

CARD32* 
XAANonTEGlyphScanlineFunc3MSBFirstFixedBase(
    CARD32 *base,
    NonTEGlyphInfo *glyphp,
    int line, int TotalWidth, int skipleft
);

CARD32* 
XAANonTEGlyphScanlineFuncMSBFirst(
    CARD32 *base,
    NonTEGlyphInfo *glyphp,
    int line, int TotalWidth, int skipleft
);

CARD32* 
XAANonTEGlyphScanlineFunc3MSBFirst(
    CARD32 *base,
    NonTEGlyphInfo *glyphp,
    int line, int TotalWidth, int skipleft
);

CARD32* 
XAANonTEGlyphScanlineFuncLSBFirstFixedBase(
    CARD32 *base,
    NonTEGlyphInfo *glyphp,
    int line, int TotalWidth, int skipleft
);

CARD32* 
XAANonTEGlyphScanlineFunc3LSBFirstFixedBase(
    CARD32 *base,
    NonTEGlyphInfo *glyphp,
    int line, int TotalWidth, int skipleft
);

CARD32* 
XAANonTEGlyphScanlineFuncLSBFirst(
    CARD32 *base,
    NonTEGlyphInfo *glyphp,
    int line, int TotalWidth, int skipleft
);

CARD32* 
XAANonTEGlyphScanlineFunc3LSBFirst(
    CARD32 *base,
    NonTEGlyphInfo *glyphp,
    int line, int TotalWidth, int skipleft
);

void 
XAAFillSolidSpans(
   ScrnInfoPtr pScrn,
   int fg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth, int fSorted 
);

void 
XAAFillMono8x8PatternSpans(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth, int fSorted,
   int patx, int paty,
   int xorg, int yorg 
);

void 
XAAFillMono8x8PatternSpansScreenOrigin(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth, int fSorted,
   int patx, int paty,
   int xorg, int yorg 
);

void 
XAAFillColor8x8PatternSpansScreenOrigin(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth, int fSorted,
   XAACacheInfoPtr,
   int xorigin, int yorigin 
);

void 
XAAFillColor8x8PatternSpans(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth, int fSorted,
   XAACacheInfoPtr,
   int xorigin, int yorigin 
);

void
XAAFillCacheBltSpans(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr points,
   int *widths,
   int fSorted,
   XAACacheInfoPtr pCache,
   int xorg, int yorg
);


void
XAAFillSpansSolid(
    DrawablePtr pDrawable,
    GC		*pGC,
    int		nInit,
    DDXPointPtr pptInit,
    int *pwidth,
    int fSorted 
);

void
XAAFillSpansMono8x8Pattern(
    DrawablePtr pDrawable,
    GC		*pGC,
    int		nInit,
    DDXPointPtr pptInit,
    int *pwidth,
    int fSorted 
);

void
XAAFillSpansColor8x8Pattern(
    DrawablePtr pDrawable,
    GC		*pGC,
    int		nInit,
    DDXPointPtr pptInit,
    int *pwidth,
    int fSorted 
);


void
XAAFillSpansCacheBlt(
    DrawablePtr pDrawable,
    GC		*pGC,
    int		nInit,
    DDXPointPtr pptInit,
    int *pwidth,
    int fSorted 
);

void
XAAFillSpansColorExpand(
    DrawablePtr pDrawable,
    GC		*pGC,
    int		nInit,
    DDXPointPtr pptInit,
    int *pwidth,
    int fSorted 
);


void 
XAAInitPixmapCache(
    ScreenPtr pScreen, 
    RegionPtr areas,
    pointer data
);

void 
XAAWriteBitmapToCache(
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   unsigned char *src,
   int srcwidth,
   int fg, int bg
);
 
void 
XAAWritePixmapToCache(
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   unsigned char *src,
   int srcwidth,
   int bpp, int depth
);

void
XAAPaintWindow(
  WindowPtr pWin,
  RegionPtr prgn,
  int what 
);

void 
XAASolidHorVertLineAsRects(
   ScrnInfoPtr pScrn,
   int x, int y, int len, int dir
);

void 
XAASolidHorVertLineAsTwoPoint(
   ScrnInfoPtr pScrn,
   int x, int y, int len, int dir
);

void 
XAASolidHorVertLineAsBresenham(
   ScrnInfoPtr pScrn,
   int x, int y, int len, int dir
);


void
XAAPolyRectangleThinSolid(
    DrawablePtr  pDrawable,
    GCPtr        pGC,    
    int	         nRectsInit,
    xRectangle  *pRectsInit 
);

void
XAAFillPolygonSolid(
    DrawablePtr	pDrawable,
    GCPtr	pGC,
    int		shape,
    int		mode,
    int		count,
    DDXPointPtr	ptsIn 
);

void
XAAPolylinesWideSolid (
   DrawablePtr	pDrawable,
   GCPtr	pGC,
   int		mode,
   int 		npt,
   DDXPointPtr	pPts
);
 
void 
XAAWriteMono8x8PatternToCache(ScrnInfoPtr pScrn, XAACacheInfoPtr pCache);

void 
XAAWriteColor8x8PatternToCache(
   ScrnInfoPtr pScrn, 
   PixmapPtr pPix, 
   XAACacheInfoPtr pCache
);

void 
XAARotateMonoPattern(
    int *pat0, int *pat1,
    int xoffset, int yoffset,
    Bool msbfirst
);

void
XAAPolyFillArcSolid(DrawablePtr pDraw, GCPtr pGC, int narcs, xArc *parcs);
 
XAACacheInfoPtr
XAACacheTile(ScrnInfoPtr Scrn, PixmapPtr pPix);

XAACacheInfoPtr
XAACacheStipple(ScrnInfoPtr Scrn, PixmapPtr pPix, int fg, int bg);

XAACacheInfoPtr
XAACacheMono8x8Pattern(ScrnInfoPtr Scrn, int pat0, int pat1);

XAACacheInfoPtr
XAACacheColor8x8Pattern(ScrnInfoPtr Scrn, PixmapPtr pPix, int fg, int bg);


void XAAClosePixmapCache(ScreenPtr pScreen);
void XAAInvalidatePixmapCache(ScreenPtr pScreen);

Bool XAACheckStippleReducibility(PixmapPtr pPixmap);
Bool XAACheckTileReducibility(PixmapPtr pPixmap, Bool checkMono);

int XAAStippledFillChooser(GCPtr pGC);
int XAAOpaqueStippledFillChooser(GCPtr pGC);
int XAATiledFillChooser(GCPtr pGC);

extern GCOps XAAFallbackOps;
extern GCFuncs XAAGCFuncs;
extern int XAAScreenIndex;
extern int XAAGCIndex;
extern int XAAPixmapIndex;

extern unsigned int XAAShiftMasks[32];

extern unsigned int byte_expand3[256], byte_reversed_expand3[256];

#if defined(__GNUC__) && defined(__i386__) && !defined(DLOPEN_HACK)
 __inline__ CARD32 XAAReverseBitOrder(CARD32 data);
#else
#ifndef __GNUC__
#define __inline__ /**/
#endif
 __inline__ CARD32 XAAReverseBitOrder(CARD32 data);
#endif


#define GET_XAASCREENPTR_FROM_SCREEN(pScreen)\
	(pScreen)->devPrivates[XAAScreenIndex].ptr

#define GET_XAASCREENPTR_FROM_GC(pGC)\
	(pGC)->pScreen->devPrivates[XAAScreenIndex].ptr

#define GET_XAASCREENPTR_FROM_DRAWABLE(pDraw)\
	(pDraw)->pScreen->devPrivates[XAAScreenIndex].ptr

#define GET_XAAINFORECPTR_FROM_SCREEN(pScreen)\
   ((XAAScreenPtr)((pScreen)->devPrivates[XAAScreenIndex].ptr))->AccelInfoRec

#define GET_XAAINFORECPTR_FROM_GC(pGC)\
((XAAScreenPtr)((pGC)->pScreen->devPrivates[XAAScreenIndex].ptr))->AccelInfoRec

#define GET_XAAINFORECPTR_FROM_DRAWABLE(pDraw)\
((XAAScreenPtr)((pDraw)->pScreen->devPrivates[XAAScreenIndex].ptr))->AccelInfoRec

#define GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn)\
((XAAScreenPtr)((pScrn)->pScreen->devPrivates[XAAScreenIndex].ptr))->AccelInfoRec

#define XAA_GET_PIXMAP_PRIVATE(pix)\
	(XAAPixmapPtr)((pix)->devPrivates[XAAPixmapIndex].ptr)

#define SET_SYNC_FLAG(infoRec)	infoRec->NeedToSync = TRUE

#define CHECK_RGB_EQUAL(c) (!((((c) >> 8) ^ (c)) & 0xffff))

#define CHECK_FG(pGC, flags) \
	(!(flags & RGB_EQUAL) || CHECK_RGB_EQUAL(pGC->fgPixel))

#define CHECK_ROP(pGC, flags) \
	(!(flags & GXCOPY_ONLY) || (pGC->alu == GXcopy))

#define CHECK_PLANEMASK(pGC, flags) \
	(!(flags & NO_PLANEMASK) || (pGC->planemask == ~0))

#define CHECK_COLORS(pGC, flags) \
	(!(flags & RGB_EQUAL) || \
	(CHECK_RGB_EQUAL(pGC->fgPixel) && CHECK_RGB_EQUAL(pGC->bgPixel)))


#endif /* _XAALOCAL_H */
