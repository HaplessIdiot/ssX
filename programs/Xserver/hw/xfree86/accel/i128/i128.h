/* $XConsortium: i128.h /main/4 1996/10/19 17:52:08 kaleb $ */
/*
 * Copyright 1994 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Robin Cutshaw not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Robin Cutshaw makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ROBIN CUTSHAW DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ROBIN CUTSHAW BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/i128/i128.h,v 3.7 1998/04/07 18:30:15 robin Exp $ */

#ifndef _I128_H_
#define _I128_H_

#define I128_PATCHLEVEL "0"

#ifndef LINKKIT

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "pixmap.h"
#include "region.h"
#include "gc.h"
#include "gcstruct.h"
#include "colormap.h"
#include "colormapst.h"
#include "miscstruct.h"
#include "scrnintstr.h"
#include "mibstore.h"
#include "mipointer.h"
#include "cursorstr.h"
#include "windowstr.h"
#include "compiler.h"
#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "regionstr.h"
#include "xf86fcache.h"
#include "xf86Procs.h"

#include <X11/Xfuncproto.h>

#else /* !LINKKIT */
#include "X.h"
#include "input.h"
#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#endif /* !LINKKIT */

#if !defined(__GNUC__) || defined(NO_INLINE)
#define __inline__ /**/
#endif

extern ScrnInfoRec i128InfoRec;

#define UNKNOWN_DAC -1
#define TI3025_DAC   0
#define IBM524_DAC   1
#define IBM526_DAC   2
#define IBM528_DAC   3


/* Types */
typedef struct {
	unsigned char r, b, g;
} LUTENTRY;

/* save the registers needed for restoration in this structure */
typedef struct {
	unsigned short iobase;		/* saved only for iobase indexing    */
	CARD32 config1;			/* iobase+0x1C register              */
	CARD32 config2;			/* iobase+0x20 register              */
	CARD32 vga_ctl;			/* iobase+0x30 register              */
	CARD32 i128_base_g[0x60/4];	/* base g registers                  */
	CARD32 i128_base_w[0x28/4];	/* base w registers                  */
	unsigned char Ti302X[0x40];	/* Ti302[05] registers               */
	unsigned char Ti3025[9];	/* Ti3025 N,M,P for PCLK, MCLK, LCLK */
	unsigned char IBMRGB[0x101];	/* IBMRGB registers                  */
} i128Registers;


/* i128 globals */
extern LUTENTRY	currenti128dac[];
extern int	i128AdjustCursorXPos;
extern Bool	i128DAC8Bit;
extern int	i128DisplayOffset;
extern int	i128DisplayWidth;
extern int	i128HDisplay;
extern int	i128InitCursorFlag;
extern int	i128ValidTokens[];
extern pointer	i128VideoMem;
extern CARD32	i128alu[];
extern int	i128hotX;
extern int	i128hotY;
extern ScreenPtr i128savepScreen;
extern i128Registers iR;



#ifndef LINKKIT
_XFUNCPROTOBEGIN

extern void (*i128ImageReadFunc)(
#if NeedFunctionPrototypes
    int, int, int, int, char *, int, int, int, unsigned long
#endif
);
extern void (*i128ImageWriteFunc)(
#if NeedFunctionPrototypes
    int, int, int, int, char *, int, int, int, short, unsigned long
#endif
);
extern void (*i128ImageFillFunc)(
#if NeedFunctionPrototypes
    int, int, int, int, char *, int, int, int, int, int, short, unsigned long
#endif
);

/* Function Prototypes */

/* i128Conf.c */
/* i128.c */
ScrnInfoRec *ServerInit(void);
void i128PrintIdent(
#if NeedFunctionPrototypes
    void
#endif
);
Bool i128Probe(
#if NeedFunctionPrototypes
    void
#endif
);
Bool i128ProgramIBMRGB(int /*freq*/, int /*flags*/);
Bool i128ProgramTi3025(int /*freq*/);
/* i128misc.c */
Bool i128Initialize(
#if NeedFunctionPrototypes
    int,
    ScreenPtr,
    int,
    char **
#endif
);
void i128EnterLeaveVT(
#if NeedFunctionPrototypes
    Bool,
    int 
#endif
);
Bool i128CloseScreen(
#if NeedFunctionPrototypes
    int,
    ScreenPtr
#endif
);
Bool i128SaveScreen(
#if NeedFunctionPrototypes
    ScreenPtr,
    Bool 
#endif
);
Bool i128SwitchMode(
#if NeedFunctionPrototypes
    DisplayModePtr 
#endif
);
void i128AdjustFrame(
#if NeedFunctionPrototypes
    int,
    int 
#endif
);
/* i128cmap.c */
int i128ListInstalledColormaps(
#if NeedFunctionPrototypes
    ScreenPtr,
    Colormap *
#endif
);
void i128RestoreDACvalues(
#if NeedFunctionPrototypes
    void
#endif
);
int i128GetInstalledColormaps(
#if NeedFunctionPrototypes
    ScreenPtr,
    ColormapPtr *
#endif
);
void i128StoreColors(
#if NeedFunctionPrototypes
    ColormapPtr,
    int,
    xColorItem *
#endif
);
void i128InstallColormap(
#if NeedFunctionPrototypes
    ColormapPtr 
#endif
);
void i128UninstallColormap(
#if NeedFunctionPrototypes
    ColormapPtr 
#endif
);
void i128RestoreColor0(
#if NeedFunctionPrototypes
    ScreenPtr 
#endif
);
/* i128gc.c */
Bool i128CreateGC(
#if NeedFunctionPrototypes
    GCPtr 
#endif
);
/* i128gc16.c */
Bool i128CreateGC16(
#if NeedFunctionPrototypes
    GCPtr 
#endif
);
/* i128gc32.c */
Bool i128CreateGC32(
#if NeedFunctionPrototypes
    GCPtr 
#endif
);
/* i128fs.c */
void i128SolidFSpans(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int 
#endif
);
void i128TiledFSpans(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int 
#endif
);
void i128StipFSpans(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int 
#endif
);
void i128OStipFSpans(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int 
#endif
);
/* i128ss.c */
void i128SetSpans(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    char *,
    DDXPointPtr,
    int *,
    int,
    int 
#endif
);
/* i128gs.c */
void i128GetSpans(
#if NeedFunctionPrototypes
    DrawablePtr,
    int,
    DDXPointPtr,
    int *,
    int,
    char *
#endif
);
/* i128win.c */
void i128CopyWindow(
#if NeedFunctionPrototypes
    WindowPtr,
    DDXPointRec,
    RegionPtr 
#endif
);
/* i128init.c */
void i128CleanUp(
#if NeedFunctionPrototypes
    void
#endif
);
Bool i128Init(
#if NeedFunctionPrototypes
    DisplayModePtr 
#endif
);
void i128InitEnvironment(
#if NeedFunctionPrototypes
    void
#endif
);
void i128Unlock(
#if NeedFunctionPrototypes
    void
#endif
);
/* i128im.c */
void i128ImageInit(
#if NeedFunctionPrototypes
    void
#endif
);
void i128ImageWriteNoMem(
#if NeedFunctionPrototypes
    int,
    int,
    int,
    int,
    char *,
    int,
    int,
    int,
    short,
    unsigned long
#endif
);
void i128ImageStipple(
#if NeedFunctionPrototypes
    int,
    int,
    int,
    int,
    char *,
    int,
    int,
    int,
    int,
    int,
    Pixel,
    short,
    unsigned long
#endif
);
void i128ImageOpStipple(
#if NeedFunctionPrototypes
    int,
    int,
    int,
    int,
    char *,
    int,
    int,
    int,
    int,
    int,
    Pixel,
    Pixel,
    short,
    unsigned long 
#endif
);
/* i128bstor.c */
void i128SaveAreas(
#if NeedFunctionPrototypes
    PixmapPtr,
    RegionPtr,
    int,
    int,
    WindowPtr 
#endif
);
void i128RestoreAreas(
#if NeedFunctionPrototypes
    PixmapPtr,
    RegionPtr,
    int,
    int,
    WindowPtr 
#endif
);
/* i128scrin.c */
Bool i128ScreenInit(
#if NeedFunctionPrototypes
    ScreenPtr,
    pointer,
    int,
    int,
    int,
    int,
    int 
#endif
);
/* i128blt.c */
RegionPtr i128CopyArea(
#if NeedFunctionPrototypes
    DrawablePtr,
    DrawablePtr,
    GC *,
    int,
    int,
    int,
    int,
    int,
    int
#endif
);
void i128FindOrdering(
#if NeedFunctionPrototypes
    DrawablePtr,
    DrawablePtr,
    GC *,
    int,
    BoxPtr,
    int,
    int,
    int,
    int,
    unsigned int *
#endif
);
RegionPtr i128CopyPlane(
#if NeedFunctionPrototypes
    DrawablePtr,
    DrawablePtr,
    GCPtr,
    int,
    int,
    int,
    int,
    int,
    int,
    unsigned long 
#endif
);
/* i128plypt.c */
void i128PolyPoint(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    int,
    xPoint *
#endif
);
/* i128line.c */
void i128Line(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    int,
    DDXPointPtr 
#endif
);
/* i128seg.c */
void i128Segment(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    xSegment *
#endif
);
/* i128frect.c */
void i128PolyFillRect(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    xRectangle *
#endif
);
void i128InitFrect(
#if NeedFunctionPrototypes
    int,
    int,
    int
#endif
);
/* i128text.c */
int i128NoCPolyText(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    int,
    int,
    char *,
    Bool 
#endif
);

void i128FontStipple(
#if NeedFunctionPrototypes
    int,
    int,
    int,
    int,
    unsigned char *,
    int,
    Pixel
#endif
);


int i128NoCImageText(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    int,
    int,
    char *,
    Bool 
#endif
);
/* i128font.c */
Bool i128RealizeFont(
#if NeedFunctionPrototypes
    ScreenPtr,
    FontPtr 
#endif
);
Bool i128UnrealizeFont(
#if NeedFunctionPrototypes
    ScreenPtr,
    FontPtr 
#endif
);
/* i128fcach.c */
void i128FontCache8Init(
#if NeedFunctionPrototypes
    void
#endif
);
void i128GlyphWrite(
#if NeedFunctionPrototypes
    int,
    int,
    int,
    unsigned char *,
    CacheFont8Ptr,
    GCPtr,
    BoxPtr,
    int
#endif
);
/* i128bcach.c */
void i128CacheMoveBlock(
#if NeedFunctionPrototypes
    int,
    int,
    int,
    int,
    int,
    int,
    unsigned int
#endif
);
/* i128Cursor.c */
Bool i128CursorInit(
#if NeedFunctionPrototypes
    char *,
    ScreenPtr 
#endif
);
void i128ShowCursor(
#if NeedFunctionPrototypes
    void
#endif
);
void i128HideCursor(
#if NeedFunctionPrototypes
    void
#endif
);
void i128RestoreCursor(
#if NeedFunctionPrototypes
    ScreenPtr 
#endif
);
void i128RepositionCursor(
#if NeedFunctionPrototypes
    ScreenPtr 
#endif
);
void i128RenewCursorColor(
#if NeedFunctionPrototypes
    ScreenPtr 
#endif
);
void i128WarpCursor(
#if NeedFunctionPrototypes
    ScreenPtr,
    int,
    int 
#endif
);
void i128QueryBestSize(
#if NeedFunctionPrototypes
    int,
    unsigned short *,
    unsigned short *,
    ScreenPtr 
#endif
);
/* i128dline.c */
void i128Dline(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    int,
    DDXPointPtr 
#endif
);
/* i128dseg.c */
void i128Dsegment(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    xSegment *
#endif
);
/* i128gtimg.c */
void i128GetImage(
#if NeedFunctionPrototypes
    DrawablePtr,
    int,
    int,
    int,
    int,
    unsigned int,
    unsigned long,
    char * 
#endif
);
/* i128TiCursor.c */
Bool i128TiRealizeCursor(
#if NeedFunctionPrototypes
    ScreenPtr,
    CursorPtr 
#endif
);
void i128TiCursorOn(
#if NeedFunctionPrototypes
    void
#endif
);
void i128TiCursorOff(
#if NeedFunctionPrototypes
    void
#endif
);
void i128TiMoveCursor(
#if NeedFunctionPrototypes
    ScreenPtr,
    int,
    int 
#endif
);
void i128TiRecolorCursor(
#if NeedFunctionPrototypes
    ScreenPtr,
    CursorPtr,
    Bool
#endif
);
void i128TiLoadCursor(
#if NeedFunctionPrototypes
    ScreenPtr,
    CursorPtr,
    int,
    int 
#endif
);
/* i128IBMCursor.c */
Bool i128IBMRealizeCursor(
#if NeedFunctionPrototypes
    ScreenPtr,
    CursorPtr 
#endif
);
void i128IBMCursorOn(
#if NeedFunctionPrototypes
    void
#endif
);
void i128IBMCursorOff(
#if NeedFunctionPrototypes
    void
#endif
);
void i128IBMMoveCursor(
#if NeedFunctionPrototypes
    ScreenPtr,
    int,
    int 
#endif
);
void i128IBMRecolorCursor(
#if NeedFunctionPrototypes
    ScreenPtr,
    CursorPtr,
    Bool
#endif
);
void i128IBMLoadCursor(
#if NeedFunctionPrototypes
    ScreenPtr,
    CursorPtr,
    int,
    int 
#endif
);

_XFUNCPROTOEND

#endif /* !LINKKIT */

extern struct i128mem i128mem;
#endif /* _I128_H_ */
