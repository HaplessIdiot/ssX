/*
 *  Permedia 3 Xv Driver
 *
 *  Copyright (C) 2001 Sven Luther <luther@dpt-info.u-strasbg.fr>
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *
 *  Based on work of Michael H. Schimek <m.schimek@netway.at>
 */
 
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/pm2_video.c,v 1.18 2001/01/31 16:14:59 alanh Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "xf86Xinput.h"
#include "xf86fbman.h"
#include "xf86xv.h"
#include "Xv.h"

#include "glint_regs.h"
#include "pm3_regs.h"
#include "glint.h"

#ifndef XvExtension

void Permedia3VideoInit(ScreenPtr pScreen) {}
void Permedia3VideoUninit(ScrnInfoPtr pScrn) {}
void Permedia3VideoEnterVT(ScrnInfoPtr pScrn) {}
void Permedia3VideoLeaveVT(ScrnInfoPtr pScrn) {}

#else

#undef MIN
#undef ABS
#undef CLAMP
#undef ENTRIES

#define MIN(a, b) (((a) < (b)) ? (a) : (b)) 
#define ABS(n) (((n) < 0) ? -(n) : (n))
#define CLAMP(v, min, max) (((v) < (min)) ? (min) : MIN(v, max))
#define ENTRIES(array) (sizeof(array) / sizeof((array)[0]))

#define ADAPTORS 1
#define PORTS 1

#define MAX_BUFFERS 3

typedef struct {
    CARD32			xy, wh;				/* 16.0 16.0 */
    INT32			s, t;				/* 12.20 fp */
    short			y1, y2;
} CookieRec, *CookiePtr;

typedef struct _PortPrivRec {
    struct _AdaptorPrivRec *    pAdaptor;

    INT32			Attribute[8];			/* Brig, Con, Sat, Hue, Int, Filt, BkgCol, Alpha */

    int				BuffersRequested;
    int				BuffersAllocated;
    FBAreaPtr			pFBArea[MAX_BUFFERS];
    CARD32			BufferBase[MAX_BUFFERS];	/* FB byte offset */
    int				CurrentBuffer;
    CARD32			BufferStride;			/* bytes */

    INT32			vx, vy, vw, vh;			/* 12.10 fp */
    int				dx, dy, dw, dh;
    int				fw, fh;

    int				Id, Bpp;			/* Scaler */

    int                         Plug;
    int				BkgCol;				/* RGB 5:6:5; 5:6:5 */
    int				StopDelay;
    

} PortPrivRec, *PortPrivPtr;

enum { VIDEO_OFF, VIDEO_ONE_SHOT, VIDEO_ON };

typedef struct _LFBAreaRec {
    struct _LFBAreaRec *	Next;
    int				Linear;
    FBAreaPtr			pFBArea;
} LFBAreaRec, *LFBAreaPtr;

typedef struct _AdaptorPrivRec {
    struct _AdaptorPrivRec *	Next;
    ScrnInfoPtr			pScrn;

    OsTimerPtr			Timer;
    int				TimerUsers;
    int				Delay, Instant;

    int				FramesPerSec;
    int				FrameLines;
    int				IntLine;			/* Frame, not field */
    int				LinePer;			/* nsec */

    int                         VideoStd;

    PortPrivRec                 Port[PORTS];

} AdaptorPrivRec, *AdaptorPrivPtr;

static AdaptorPrivPtr AdaptorPrivList = NULL;

#define PORTNUM(p) ((int)((p) - &pAPriv->Port[0]))

#define DEBUG(x)	x

static const Bool ColorBars = FALSE;

/*
 *  Attributes
 */
 
#define XV_ENCODING	"XV_ENCODING"
#define XV_BRIGHTNESS	"XV_BRIGHTNESS"
#define XV_CONTRAST 	"XV_CONTRAST"
#define XV_SATURATION	"XV_SATURATION"
#define XV_HUE		"XV_HUE"

/* Proprietary */

#define XV_INTERLACE	"XV_INTERLACE"	/* Interlaced (bool) */
#define XV_FILTER	"XV_FILTER"	/* Bilinear filter (bool) */
#define XV_BKGCOLOR	"XV_BKGCOLOR"	/* Output background (0x00RRGGBB) */
#define XV_ALPHA	"XV_ALPHA"	/* Scaler alpha channel (bool) */

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static Atom xvEncoding, xvBrightness, xvContrast, xvSaturation, xvHue;
static Atom xvInterlace, xvFilter, xvBkgColor, xvAlpha;


/* Scaler */

static XF86VideoEncodingRec
ScalerEncodings[] =
{
    { 0, "XV_IMAGE", 2047, 2047, { 1, 1 }},
};

static XF86AttributeRec
ScalerAttributes[] =
{
    { XvSettable | XvGettable, 0, 1, XV_FILTER },
    { XvSettable | XvGettable, 0, 1, XV_ALPHA },
};

static XF86VideoFormatRec
ScalerVideoFormats[] =
{
    { 8,  TrueColor }, /* Dithered */
    { 15, TrueColor },
    { 16, TrueColor },
    { 24, TrueColor },
};

/*
 *  FOURCC from http://www.webartz.com/fourcc
 *  Generic GUID for legacy FOURCC XXXXXXXX-0000-0010-8000-00AA00389B71
 */
#define LE4CC(a,b,c,d) (((CARD32)(a)&0xFF)|(((CARD32)(b)&0xFF)<<8)|(((CARD32)(c)&0xFF)<<16)|(((CARD32)(d)&0xFF)<<24))
#define GUID4CC(a,b,c,d) { a,b,c,d,0,0,0,0x10,0x80,0,0,0xAA,0,0x38,0x9B,0x71 }

#define NoOrder LSBFirst

static XF86ImageRec
ScalerImages[] =
{
    /* Planar YVU 4:2:0 (emulated) */
    { LE4CC('Y','V','1','2'), XvYUV, NoOrder, GUID4CC('Y','V','1','2'),
      12, XvPlanar, 3, 0, 0, 0, 0,
      8, 8, 8,  1, 2, 2,  1, 2, 2, "YVU", XvTopToBottom },

    /* Packed YUYV 4:2:2 */
    { LE4CC('Y','U','Y','2'), XvYUV, NoOrder, GUID4CC('Y','U','Y','2'),
      16, XvPacked, 1, 0, 0, 0, 0,
      8, 8, 8,  1, 2, 2,  1, 1, 1, "YUYV", XvTopToBottom },

    /* Packed UYVY 4:2:2 */
    { LE4CC('U','Y','V','Y'), XvYUV, NoOrder, GUID4CC('U','Y','V','Y'),
      16, XvPacked, 1, 0, 0, 0, 0,
      8, 8, 8,  1, 2, 2,  1, 1, 1, "UYVY", XvTopToBottom },

    /* Packed YUVA 4:4:4 */
    { LE4CC('Y','U','V','A') /* XXX not registered */, XvYUV, LSBFirst, { 0 },
      32, XvPacked, 1, 0, 0, 0, 0,
      8, 8, 8,  1, 1, 1,  1, 1, 1, "YUVA", XvTopToBottom },

    /* Packed VUYA 4:4:4 */
    { LE4CC('V','U','Y','A') /* XXX not registered */, XvYUV, LSBFirst, { 0 },
      32, XvPacked, 1, 0, 0, 0, 0,
      8, 8, 8,  1, 1, 1,  1, 1, 1, "VUYA", XvTopToBottom },

    /* RGBA 8:8:8:8 */
    { 0x41, XvRGB, LSBFirst, { 0 },
      32, XvPacked, 1, 24, 0x0000FF, 0x00FF00, 0xFF0000, 
      0, 0, 0,  0, 0, 0,  0, 0, 0, "RGBA", XvTopToBottom },

    /* RGB 5:6:5 */
    { 0x42, XvRGB, LSBFirst, { 0 },
      16, XvPacked, 1, 16, 0x001F, 0x07E0, 0xF800, 
      0, 0, 0,  0, 0, 0,  0, 0, 0, "RGB", XvTopToBottom },

    /* RGBA 5:5:5:1 */
    { 0x43, XvRGB, LSBFirst, { 0 },
      16, XvPacked, 1, 15, 0x001F, 0x03E0, 0x7C00, 
      0, 0, 0,  0, 0, 0,  0, 0, 0, "RGBA", XvTopToBottom },

    /* RGBA 4:4:4:4 */
    { 0x44, XvRGB, LSBFirst, { 0 },
      16, XvPacked, 1, 12, 0x000F, 0x00F0, 0x0F00, 
      0, 0, 0,  0, 0, 0,  0, 0, 0, "RGBA", XvTopToBottom },

    /* RGB 3:3:2 */
    { 0x46, XvRGB, NoOrder, { 0 },
      8, XvPacked, 1, 8, 0x07, 0x38, 0xC0, 
      0, 0, 0,  0, 0, 0,  0, 0, 0, "RGB", XvTopToBottom },

    /* BGRA 8:8:8:8 */
    { 0x47, XvRGB, LSBFirst, { 0 },
      32, XvPacked, 1, 24, 0xFF0000, 0x00FF00, 0x0000FF,
      0, 0, 0,  0, 0, 0,  0, 0, 0, "BGRA", XvTopToBottom },

    /* BGR 5:6:5 */
    { 0x48, XvRGB, LSBFirst, { 0 },
      16, XvPacked, 1, 16, 0xF800, 0x07E0, 0x001F,
      0, 0, 0,  0, 0, 0,  0, 0, 0, "BGR", XvTopToBottom },

    /* BGRA 5:5:5:1 */
    { 0x49, XvRGB, LSBFirst, { 0 },
      16, XvPacked, 1, 15, 0x7C00, 0x03E0, 0x001F,
      0, 0, 0,  0, 0, 0,  0, 0, 0, "BGRA", XvTopToBottom },

    /* BGRA 4:4:4:4 */
    { 0x4A, XvRGB, LSBFirst, { 0 },
      16, XvPacked, 1, 12, 0x0F00, 0x00F0, 0x000F,
      0, 0, 0,  0, 0, 0,  0, 0, 0, "BGRA", XvTopToBottom },

    /* BGR 2:3:3 */
    { 0x4C, XvRGB, NoOrder, { 0 },
      8, XvPacked, 1, 8, 0xC0, 0x38, 0x07,
      0, 0, 0,  0, 0, 0,  0, 0, 0, "BGR", XvTopToBottom },
};

/*
 *  Buffer management
 */

static void
RemoveAreaCallback(FBAreaPtr pFBArea)
{
    PortPrivPtr pPPriv = (PortPrivPtr) pFBArea->devPrivate.ptr;
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    DEBUG(ScrnInfoPtr pScrn = pAPriv->pScrn;)
    int i;

    /* Well, for each buffer, just do nothing ? */
    for (i = 0; i < MAX_BUFFERS && pPPriv->pFBArea[i] != pFBArea; i++);

    if (i >= MAX_BUFFERS)
	return;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
	"RemoveAreaCallback port #%d, buffer #%d, pFB=%p, off=0x%08x\n",
	PORTNUM(pPPriv), i, pPPriv->pFBArea[i], pPPriv->BufferBase[i]));

    for (; i < MAX_BUFFERS - 1; i++)
	pPPriv->pFBArea[i] = pPPriv->pFBArea[i + 1];

    pPPriv->pFBArea[MAX_BUFFERS - 1] = NULL;

    pPPriv->BuffersAllocated--;
}

static void
RemoveableBuffers(PortPrivPtr pPPriv, Bool remove)
{
    int i;

    for (i = 0; i < MAX_BUFFERS; i++)
	if (pPPriv->pFBArea[i])
	    pPPriv->pFBArea[i]->RemoveAreaCallback =
		remove ? RemoveAreaCallback : NULL;
}

static void
FreeBuffers(PortPrivPtr pPPriv)
{
    DEBUG(AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;)
    DEBUG(ScrnInfoPtr pScrn = pAPriv->pScrn;)
    int i;

    RemoveableBuffers(pPPriv, FALSE);

    for (i = MAX_BUFFERS - 1; i >= 0; i--)
	if (pPPriv->pFBArea[i]) {
	    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
		"FreeBuffers port #%d, buffer #%d, pFB=%p, off=0x%08x\n",
		PORTNUM(pPPriv), i, pPPriv->pFBArea[i], pPPriv->BufferBase[i]));

	    xf86FreeOffscreenArea(pPPriv->pFBArea[i]);

	    pPPriv->pFBArea[i] = NULL;
	}

    pPPriv->BuffersAllocated = 0;
}

enum { FORCE_LINEAR = 1, FORCE_RECT };

static int
AllocateBuffers(PortPrivPtr pPPriv,
    int w, int h, int bytespp,
    int num, int force)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    ScrnInfoPtr pScrn = pAPriv->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    Bool linear = (force != FORCE_RECT);
    int i;
    if (!linear) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	    "Sorry, only linear buffers allowed right now ...\n");
	return 0;
    }
    FreeBuffers(pPPriv);

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"SVEN : Tryig ot allocate (%d) %s buffer with : %dx%d, bpp = %d.\n",
	num, (force==FORCE_RECT)?"rectangular":"linear", w, h, bytespp));
    for (i = 0; i < num; i++) {
	if (linear) {
	    pPPriv->BufferStride = w * bytespp;

	    /* Well, we need to set alignement correctly ... */
#if 0
	    pPPriv->pFBArea[i] = xf86AllocateLinearOffscreenArea(pScrn->pScreen,
    		(pPPriv->BufferStride * h
		 + (1 << pGlint->BppShift) -1) >> pGlint->BppShift,
		16 >> pGlint->BppShift, NULL, NULL, (pointer) pPPriv);
#endif
	    pPPriv->pFBArea[i] = xf86AllocateLinearOffscreenArea(pScrn->pScreen,
    		pPPriv->BufferStride * h, 0, NULL, NULL, (pointer) pPPriv);
	    /* Well, not that we have allocated a buffer, let's see
	     * to what it correspond in the cards memory space ... */
	    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
		"SVEN : Let's see what kind of buffer got allocated ...\n"
		"\t BOX : x1 = %d, y1 = %d, x2 = %d, y2 = %d.\n"
		"\t pScrn->virtualX = %d, pGlint->BppShift = %d.\n",
		pPPriv->pFBArea[i]->box.x1, pPPriv->pFBArea[i]->box.y1,
		pPPriv->pFBArea[i]->box.x2, pPPriv->pFBArea[i]->box.y2,
		pScrn->virtualX, pGlint->BppShift));
		
	    if (pPPriv->pFBArea[i])
		pPPriv->BufferBase[i] =
		    ((pPPriv->pFBArea[i]->box.y1 * pScrn->virtualX) +
		     pPPriv->pFBArea[i]->box.x1) << pGlint->BppShift;
	    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
		"New linear buffer %dx%d, rec %dx%d -> pFB=%p, off=0x%08x\n"
		"\tNotice : Framebuffer is at : 0x%08x, 0x%08x.\n",
		w, h, pPPriv->BufferStride, h, pPPriv->pFBArea[i],
		pPPriv->BufferBase[i], pGlint->FbAddress, pGlint->FbBase));
	} else {
#if 0
	    /* SVEN : well for now, all buffers are linear, so we don't need
	     * to worry. */
	    pPPriv->BufferStride = pScrn->displayWidth << BPPSHIFT(pGlint);
	    pPPriv->pFBArea[i] = xf86AllocateOffscreenArea(pScrn->pScreen,
    		w, h, 8 >> BPPSHIFT(pGlint), NULL, NULL, (pointer) pPPriv);
	    if (pPPriv->pFBArea[i])
		pPPriv->BufferBase[i] =
		    ((pPPriv->pFBArea[i]->box.y1 * pScrn->displayWidth) +
		    pPPriv->pFBArea[i]->box.x1) << BPPSHIFT(pGlint);
	    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
		"New rect buffer %dx%d, stride %d, pFB=%p, off=0x%08x\n",
		w, h, pPPriv->BufferStride,  pPPriv->pFBArea[i],
		pPPriv->BufferBase[i]));
#endif
	}
	if (pPPriv->pFBArea[i])
	    continue;
    }

    return pPPriv->BuffersAllocated = i;
    return pPPriv->CurrentBuffer = 0;
}

/* os/WaitFor.c */

static CARD32
TimerCallback(OsTimerPtr pTim, CARD32 now, pointer p)
{
    AdaptorPrivPtr pAPriv = (AdaptorPrivPtr) p;
    int i;

    for (i = 0; i <= PORTS; i++) {
	if (pAPriv->Port[i].StopDelay >= 0) {
	    if (!(pAPriv->Port[i].StopDelay--)) {
		FreeBuffers(&pAPriv->Port[i]);
		pAPriv->TimerUsers &= ~(1 << i);
	    }
	}
    }

    if (pAPriv->TimerUsers)
	return pAPriv->Instant;

    return 0; /* Cancel */
}

/*
 *  Xv interface
 */

static void
CopyYV12LE(CARD8 *Y, CARD32 *dst, int width, int height, int pitch)
{
    int Y_size = width * height;
    CARD8 *V = Y + Y_size;
    CARD8 *U = V + (Y_size >> 2);
    int pad = (pitch >> 2) - (width >> 1);
    int x;

    width >>= 1;

    for (height >>= 1; height > 0; height--) {
	for (x = 0; x < width; Y += 2, x++)
	    *dst++ = Y[0] + (U[x] << 8) + (Y[1] << 16) + (V[x] << 24);
	dst += pad;
	for (x = 0; x < width; Y += 2, x++)
	    *dst++ = Y[0] + (U[x] << 8) + (Y[1] << 16) + (V[x] << 24);
	dst += pad;
	U += width;
	V += width;
    }
}

#if X_BYTE_ORDER == X_BIG_ENDIAN

static void
CopyYV12BE(CARD8 *Y, CARD32 *dst, int width, int height, int pitch)
{
    int Y_size = width * height;
    CARD8 *V = Y + Y_size;
    CARD8 *U = V + (Y_size >> 2);
    int pad = (pitch >> 2) - (width >> 1);
    int x;

    width >>= 1;

    for (height >>= 1; height > 0; height--) {
	for (x = 0; x < width; Y += 2, x++)
	    *dst++ = V[x] + (Y[1] << 8) + (U[x] << 16) + (Y[0] << 24);
	dst += pad;
	for (x = 0; x < width; Y += 2, x++)
	    *dst++ = V[x] + (Y[1] << 8) + (U[x] << 16) + (Y[0] << 24);
	dst += pad;
	U += width;
	V += width;
    }
}

#endif /* X_BYTE_ORDER == X_BIG_ENDIAN */

static void
CopyFlat(CARD8 *src, CARD8 *dst, int width, int height, int pitch)
{
    if (width == pitch) {
	memcpy(dst, src, width * height);
	return;
    }

    while (height > 0) {
	memcpy(dst, src, width);
	dst += pitch;
	src += width;
	height--;
    }
}

#define FORMAT_RGB8888	PM3VideoOverlayMode_COLORFORMAT_RGB8888 
#define FORMAT_RGB4444	PM3VideoOverlayMode_COLORFORMAT_RGB4444
#define FORMAT_RGB5551	PM3VideoOverlayMode_COLORFORMAT_RGB5551
#define FORMAT_RGB565	PM3VideoOverlayMode_COLORFORMAT_RGB565
#define FORMAT_RGB332	PM3VideoOverlayMode_COLORFORMAT_RGB332
#define FORMAT_BGR8888	PM3VideoOverlayMode_COLORFORMAT_BGR8888
#define FORMAT_BGR4444	PM3VideoOverlayMode_COLORFORMAT_BGR4444
#define FORMAT_BGR5551	PM3VideoOverlayMode_COLORFORMAT_BGR5551
#define FORMAT_BGR565	PM3VideoOverlayMode_COLORFORMAT_BGR565
#define FORMAT_BGR332	PM3VideoOverlayMode_COLORFORMAT_BGR332
#define FORMAT_CI8	PM3VideoOverlayMode_COLORFORMAT_CI8
#define FORMAT_VUY444	PM3VideoOverlayMode_COLORFORMAT_VUY444
#define FORMAT_YUV444	PM3VideoOverlayMode_COLORFORMAT_YUV444
#define FORMAT_VUY422	PM3VideoOverlayMode_COLORFORMAT_VUY422
#define FORMAT_YUV422	PM3VideoOverlayMode_COLORFORMAT_YUV422

#define	RAMDAC_WRITE(data,index)				\
do{                                                             \
	mem_barrier();						\
	GLINT_WAIT(3);						\
	mem_barrier();						\
	GLINT_WRITE_REG(((index)>>8)&0xff, PM3RD_IndexHigh);	\
	mem_barrier();						\
 	GLINT_WRITE_REG((index)&0xff, PM3RD_IndexLow);		\
	mem_barrier();						\
	GLINT_WRITE_REG(data, PM3RD_IndexedData);		\
	mem_barrier();						\
}while(0)

/* Notice, have to check that we dont overflow the deltas here ... */
static void
compute_scale_factor(
    unsigned int* src_w, unsigned int* dst_w,
    unsigned int* shrink_delta, unsigned int* zoom_delta)
{
    if (*src_w >= *dst_w) {
	*dst_w &= ~0x3;
	*shrink_delta = (((*src_w << 16) / *dst_w) & 0x0ffffff0) + 0x10;
	*zoom_delta = 1<<16;
    } else {
	if (*src_w & 0x3) *src_w = (*src_w & ~0x3) + 4;
	*shrink_delta = 1<<16;
	for (;*dst_w > *src_w; *dst_w--) {
	    *zoom_delta = (*src_w << 16) / *dst_w;
	    if (((((*zoom_delta&0xf)+1) * *dst_w * *dst_w) >> 16) < *src_w) {
		*zoom_delta = ((*zoom_delta & ~0xf) + 0x10) & 0x0001fff0;
		return;
	    }
	}
	*zoom_delta = 1<<16;
    }
}

/* BeginOverlay ...
 * This is still not workign correctly, in particular if we are 
 * using shrinking, since what we have to check is not a multiple of 4
 * width is the shrinked width, not the original width.
 */
static void
BeginOverlay(PortPrivPtr pPPriv, CARD32* BufferBase, int num,
    int format, int bpp_shift, int alpha)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    ScrnInfoPtr pScrn = pAPriv->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    unsigned int src_w = pPPriv->vw;
    unsigned int dst_w = pPPriv->dw;
    unsigned int shrink_delta, zoom_delta;

    /* Let's adjust the width of source and dest to be compliant with 
     * the Permedia3 overlay unit requirement, and compute the X deltas. */
    compute_scale_factor(&src_w, &dst_w, &shrink_delta, &zoom_delta);

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"BeginOverlay %08x %08x %d %d\n",
	BufferBase[0], format, bpp_shift, alpha));
    if (src_w != pPPriv->vw)
	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	    "BeginOverlay : Padding video width to 4 pixels %d->%d.\n",
	    pPPriv->vw, src_w));
    if (dst_w != pPPriv->dw)
	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	    "BeginOverlay : Scaling destination width from %d to %d.\n"
	    "\tThe scaling factor is to high, and may cause problems.",
	    pPPriv->dw, dst_w));
    if (alpha)
	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 0,
	    "BeginOverlay Sorry, Alpha is not yet supported, disabling it.\n"));

    GLINT_WAIT(12);
    GLINT_WRITE_REG(3|(12<<16), PM3VideoOverlayFifoControl);
    /* Updating the Video Overlay Source Image Parameters */
    switch (num) {
	case 0:
	    GLINT_WRITE_REG((BufferBase[0]>>bpp_shift), PM3VideoOverlayBase0);
	    break;
	case 1:
	    GLINT_WRITE_REG((BufferBase[1]>>bpp_shift), PM3VideoOverlayBase1);
	    break;
	case 2:
	    GLINT_WRITE_REG((BufferBase[2]>>bpp_shift), PM3VideoOverlayBase2);
	    break;
    }
    GLINT_WRITE_REG(num, PM3VideoOverlayIndex);
    GLINT_WRITE_REG(format |
	PM3VideoOverlayMode_BUFFERSYNC_MANUAL |
	PM3VideoOverlayMode_FLIP_VIDEO |
	PM3VideoOverlayMode_FILTER_FULL |
	PM3VideoOverlayMode_ENABLE,
	PM3VideoOverlayMode);
    /* Let's set the source stride. */
    GLINT_WRITE_REG(PM3VideoOverlayStride_STRIDE(pPPriv->fw),
	PM3VideoOverlayStride);
    /* Let's set the position and size of the visible part of the source. */
    GLINT_WRITE_REG(PM3VideoOverlayWidth_WIDTH(src_w),
	PM3VideoOverlayWidth);
    GLINT_WRITE_REG(PM3VideoOverlayHeight_HEIGHT(pPPriv->vh),
	PM3VideoOverlayHeight);
    GLINT_WRITE_REG(
	PM3VideoOverlayOrigin_XORIGIN(pPPriv->vx) |
	PM3VideoOverlayOrigin_YORIGIN(pPPriv->vy),
	PM3VideoOverlayOrigin);
    /* Scale the source to the destinationsize */
    if (pPPriv->vh == pPPriv->dh) {
	GLINT_WRITE_REG(
	    PM3VideoOverlayYDelta_NONE,
	    PM3VideoOverlayYDelta);
    } else {
	GLINT_WRITE_REG(
	    PM3VideoOverlayYDelta_DELTA(pPPriv->vh,pPPriv->dh),
	    PM3VideoOverlayYDelta);
    }
    GLINT_WRITE_REG(shrink_delta, PM3VideoOverlayShrinkXDelta);
    GLINT_WRITE_REG(zoom_delta, PM3VideoOverlayZoomXDelta);
    /* Launch the true update at the next FrameBlank */
    GLINT_WRITE_REG(PM3VideoOverlayUpdate_ENABLE,
	PM3VideoOverlayUpdate);
    /* Setting the ramdac video overlay rgion */
    /* Begining of overlay region */
    RAMDAC_WRITE((pPPriv->dx&0xff), PM3RD_VideoOverlayXStartLow);
    RAMDAC_WRITE((pPPriv->dx&0xf00)>>8, PM3RD_VideoOverlayXStartHigh);
    RAMDAC_WRITE((pPPriv->dy&0xff), PM3RD_VideoOverlayYStartLow); 
    RAMDAC_WRITE((pPPriv->dy&0xf00)>>8, PM3RD_VideoOverlayYStartHigh);
    /* End of overlay regions (+1) */
    RAMDAC_WRITE(((pPPriv->dx+dst_w)&0xff), PM3RD_VideoOverlayXEndLow);
    RAMDAC_WRITE(((pPPriv->dx+dst_w)&0xf00)>>8,PM3RD_VideoOverlayXEndHigh);
    RAMDAC_WRITE(((pPPriv->dy+pPPriv->dh)&0xff), PM3RD_VideoOverlayYEndLow); 
    RAMDAC_WRITE(((pPPriv->dy+pPPriv->dh)&0xf00)>>8,PM3RD_VideoOverlayYEndHigh);
    
    /* And now enable Video Overlay in the RAMDAC */
    RAMDAC_WRITE(PM3RD_VideoOverlayControl_ENABLE |
	PM3RD_VideoOverlayControl_MODE_ALWAYS,
	PM3RD_VideoOverlayControl);

    /* Let's use a double buffering scheme */
    pPPriv->CurrentBuffer = 1-pPPriv->CurrentBuffer;
}

static void
StopOverlay(PortPrivPtr pPPriv, CARD32* BufferBase, int num, int cleanup)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    ScrnInfoPtr pScrn = pAPriv->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3, "StopOverlay.\n"));
    /* Stop the Video Overlay in the RAMDAC */
    RAMDAC_WRITE(PM3RD_VideoOverlayControl_DISABLE,
	PM3RD_VideoOverlayControl);
    /* Stop the Video Overlay in the Video Overlay Unit */
    GLINT_WAIT(1);
    GLINT_WRITE_REG(PM3VideoOverlayMode_DISABLE,
	PM3VideoOverlayMode);
}
static int
Permedia3PutImage(ScrnInfoPtr pScrn,
    short src_x, short src_y, short drw_x, short drw_y,
    short src_w, short src_h, short drw_w, short drw_h,
    int id, unsigned char *buf, short width, short height,
    Bool sync, RegionPtr clipBoxes, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int i;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"PutImage %d,%d,%d,%d -> %d,%d,%d,%d id=0x%08x buf=%p w=%d h=%d sync=%d\n",
	src_x, src_y, src_w, src_h, drw_x, drw_y, drw_w, drw_h,
	id, buf, width, height, sync));

    /* SVEN : Let's check if the source window is contained in the
     * actual source */
    if ((src_x + src_w) > width ||
        (src_y + src_h) > height)
        return BadValue;

    pPPriv->vx = src_x;
    pPPriv->vy = src_y;
    pPPriv->vw = src_w;
    pPPriv->vh = src_h;

    pPPriv->dx = drw_x;
    pPPriv->dy = drw_y;
    pPPriv->dw = drw_w;
    pPPriv->dh = drw_h;

    /* SVEN : Let's check if there is a allocated buffer already of the same
     * id, width and height. If yes, use it, if not, we need to allocate one.
     */
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"SVEN : PutImage Do we need to allocate buffers ?\n"
	"\t existing : buffers = %d, id = %d, fw = %d, fh = %d.\n"
	"\t requested : buffers = %d, id = %d, fw = %d, fh = %d.\n",
	pPPriv->BuffersAllocated, pPPriv->Id, pPPriv->fw, pPPriv->fh,
	3, id, width, height));

    /* Maybe we would test for correct bpp instead of if here, ? */
    if (pPPriv->BuffersAllocated <= 0 ||
	id != pPPriv->Id || /* same bpp */
	width != pPPriv->fw ||
	height != pPPriv->fh)
    {
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"SVEN : PutImage, yes, we need to allocate buffers, isn't it ?.\n"));
	/* SVEN : search for a ScalerImage corresponding to the id */
	for (i = 0; i < ENTRIES(ScalerImages); i++)
	    if (id == ScalerImages[i].id)
		break;

	/* SVEN : If no scaler was found, we stop here and then */
	if (i >= ENTRIES(ScalerImages))
	    return XvBadAlloc;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"SVEN : PutImage, We did found a compatible image format (%d).\n", i));
	/* SVEN : Let's sync the chip ... (we do that a lot ...) */
	if (pGlint->MultiAperture) DualPermedia3Sync(pScrn);
	else Permedia3Sync(pScrn);
#if 0 /* SVEN : This was not disabled by me, don't know what it was */
	if (pPPriv->BuffersAllocated <= 0 ||
	    pPPriv->Bpp != ScalerImages[i].bits_per_pixel ||
	    width > pPPriv->fw || height > pPPriv->fw ||
	    pPPriv->fw * pPPriv->fh > width * height * 2)
#else
	if (1)
#endif
	{
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"SVEN : PutImage, We will allocate a buffer ...\n"));
	    /* SVEN : Let's allocate a buffer to store the image.
	     * If it is not possible, we exit */
	    if (!AllocateBuffers(pPPriv, width, height,
		(ScalerImages[i].bits_per_pixel + 7) >> 3, 3, 0)) {
		pPPriv->Id = 0;
		pPPriv->Bpp = 0;
		pPPriv->fw = 0;
		pPPriv->fh = 0;

		return XvBadAlloc;
	    }

	    /* SVEN : We set the id, bpp, width and height values of 
	     * the allocated buffer, so that we will not need to
	     * allocate a buffer again on the next frame. */
	    pPPriv->Id = id;
	    pPPriv->Bpp = ScalerImages[i].bits_per_pixel;
	    pPPriv->fw = width;
	    pPPriv->fh = height;

	    /* SVEN : Add RemoveAreaCallBack to the pFBAreas of each buffer */
	    /* Let's see if this cause the crash ??? No, it doesn't ... */
	    RemoveableBuffers(pPPriv, TRUE);
	}
    } else
	if (pGlint->MultiAperture) DualPermedia3Sync(pScrn);
	else Permedia3Sync(pScrn);

    switch (id) {
    case LE4CC('Y','V','1','2'):
#if X_BYTE_ORDER == X_LITTLE_ENDIAN
	CopyYV12LE(buf,
	    (CARD32 *)((CARD8 *) pGlint->FbBase + pPPriv->BufferBase[pPPriv->CurrentBuffer]),
	    width, height, pPPriv->BufferStride);
#else
	if (pGlint->FBDev)
	    CopyYV12LE(buf,
		(CARD32 *)((CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0]),
		width, height, pPPriv->BufferStride);
	else
	    CopyYV12BE(buf,
		(CARD32 *)((CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0]),
		width, height, pPPriv->BufferStride);
#endif
	BeginOverlay(pPPriv, pPPriv->BufferBase, pPPriv->CurrentBuffer, FORMAT_YUV422, 1, 0);
	break;

    case LE4CC('Y','U','Y','2'):
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 1, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_YUV422, 1, 0);
	break;

    case LE4CC('U','Y','V','Y'):
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 1, height, pPPriv->BufferStride);
	/* Not sure about this one ... */
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_VUY422, 1, 0);
	break;

    case LE4CC('Y','U','V','A'):
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 2, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_YUV444, 2, pPPriv->Attribute[7]);
	break;

    case LE4CC('V','U','Y','A'):
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 2, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_VUY444, 2, pPPriv->Attribute[7]);
	break;

    case 0x41: /* RGBA 8:8:8:8 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 2, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_RGB8888, 2, pPPriv->Attribute[7]);
	break;

    case 0x42: /* RGB 5:6:5 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 1, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_RGB565, 1, 0);
	break;

    case 0x43: /* RGB 1:5:5:5 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 1, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_RGB5551, 1, pPPriv->Attribute[7]);
	break;

    case 0x44: /* RGB 4:4:4:4 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 1, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_RGB4444, 1, pPPriv->Attribute[7]);
	break;

    case 0x46: /* RGB 2:3:3 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_RGB332, 0, 0);
	break;

    case 0x47: /* BGRA 8:8:8:8 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 2, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_BGR8888, 2, pPPriv->Attribute[7]);
	break;

    case 0x48: /* BGR 5:6:5 */
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"PutImage case (13) BGR565.\n"));
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 1, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_BGR565, 1, 0);
	break;

    case 0x49: /* BGR 1:5:5:5 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 1, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_BGR5551, 1, pPPriv->Attribute[7]);
	break;

    case 0x4A: /* BGR 4:4:4:4 */
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"PutImage case (15) BGR4444.\n"));
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 1, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_BGR4444, 1, pPPriv->Attribute[7]);
	break;

    case 0x4C: /* BGR 2:3:3 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase + pPPriv->BufferBase[0],
	    width << 0, height, pPPriv->BufferStride);
	BeginOverlay(pPPriv, pPPriv->BufferBase, 0, FORMAT_BGR332, 0, 0);
	break;

    default:
	return XvBadAlloc;
    }

    /* SVEN : Don't know what these two are for, let them stay as is */
    pPPriv->StopDelay = pAPriv->Delay;
    if (!pAPriv->TimerUsers) {
	pAPriv->TimerUsers |= 1 << PORTNUM(pPPriv);
	TimerSet(pAPriv->Timer, 0, 80, TimerCallback, pAPriv);
    }

    if (sync) {
	if (pGlint->MultiAperture) DualPermedia3Sync(pScrn);
	else Permedia3Sync(pScrn);
    }

    return Success;
}

static void
Permedia3StopVideo(ScrnInfoPtr pScrn, pointer data, Bool cleanup)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"StopVideo port=%d, exit=%d\n", PORTNUM(pPPriv), cleanup));

    StopOverlay(pPPriv, pPPriv->BufferBase, pPPriv->CurrentBuffer, cleanup);

    if (cleanup) {
	FreeBuffers(pPPriv);
	if (pAPriv->TimerUsers) {
	    pAPriv->TimerUsers &= ~PORTNUM(pPPriv);
	    if (!pAPriv->TimerUsers)
		TimerCancel(pAPriv->Timer);
	}
    }
}

static int
Permedia3SetPortAttribute(ScrnInfoPtr pScrn,
    Atom attribute, INT32 value, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"SPA attr=%d val=%d port=%d\n",
	attribute, value, PORTNUM(pPPriv)));

    return Success;
}

static int
Permedia3GetPortAttribute(ScrnInfoPtr pScrn, 
    Atom attribute, INT32 *value, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    if (PORTNUM(pPPriv) >= 2 &&
	attribute != xvFilter &&
	attribute != xvAlpha)
	return BadMatch;

    if (attribute == xvEncoding) {
	if (pAPriv->VideoStd < 0)
	    return XvBadAlloc;
	else
	    if (pPPriv == &pAPriv->Port[0])
		*value = pAPriv->VideoStd * 3 + pPPriv->Plug;
	    else
		*value = pAPriv->VideoStd * 2 + pPPriv->Plug - 1;
    } else if (attribute == xvBrightness)
	*value = pPPriv->Attribute[0];
    else if (attribute == xvContrast)
	*value = pPPriv->Attribute[1];
    else if (attribute == xvSaturation)
	*value = pPPriv->Attribute[2];
    else if (attribute == xvHue)
	*value = pPPriv->Attribute[3];
    else if (attribute == xvInterlace)
	*value = pPPriv->Attribute[4];
    else if (attribute == xvFilter)
	*value = pPPriv->Attribute[5];
    else if (attribute == xvBkgColor)
	*value = pPPriv->Attribute[6];
    else if (attribute == xvAlpha)
	*value = pPPriv->Attribute[7];
    else
	return BadMatch;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"GPA attr=%d val=%d port=%d\n",
	attribute, *value, PORTNUM(pPPriv)));

    return Success;
}

static void
Permedia3QueryBestSize(ScrnInfoPtr pScrn, Bool motion,
    short vid_w, short vid_h, short drw_w, short drw_h,
    unsigned int *p_w, unsigned int *p_h, pointer data)
{
    unsigned int zoom_delta, shrink_delta, src_w;

    *p_w = drw_w;
    compute_scale_factor (&src_w, p_w, &shrink_delta, &zoom_delta);
    *p_h = drw_h;
}

static int
Permedia3QueryImageAttributes(ScrnInfoPtr pScrn,
    int id, unsigned short *width, unsigned short *height,
    int *pitches, int *offsets)
{
    int i, pitch;

    *width = CLAMP(*width, 1, 2047);
    *height = CLAMP(*height, 1, 2047);

    if (offsets)
	offsets[0] = 0;

    switch (id) {
    case LE4CC('Y','V','1','2'): /* Planar YVU 4:2:0 (emulated) */
	*width = CLAMP((*width + 1) & ~1, 2, 2046);
	*height = CLAMP((*height + 1) & ~1, 2, 2046);

	pitch = *width; /* luma component */

	if (offsets) {
	    offsets[1] = pitch * *height;
	    offsets[2] = offsets[1] + (offsets[1] >> 2);
	}

	if (pitches) {
	    pitches[0] = pitch;
	    pitches[1] = pitches[2] = pitch >> 1;
	}

	return pitch * *height * 3 / 2;

    case LE4CC('Y','U','Y','2'): /* Packed YUYV 4:2:2 */
    case LE4CC('U','Y','V','Y'): /* Packed UYVY 4:2:2 */
	*width = CLAMP((*width + 1) & ~1, 2, 2046);

	pitch = *width * 2;

	if (pitches)
	    pitches[0] = pitch;

	return pitch * *height;

    default:
	for (i = 0; i < ENTRIES(ScalerImages); i++)
	    if (ScalerImages[i].id == id)
		break;

	if (i >= ENTRIES(ScalerImages))
	    break;

	pitch = *width * (ScalerImages[i].bits_per_pixel >> 3);

	if (pitches)
	    pitches[0] = pitch;

	return pitch * *height;
    }

    return 0;
}

static void
DeleteAdaptorPriv(AdaptorPrivPtr pAPriv)
{
    int i;

    for (i = 0; i < PORTS; i++) {
        FreeBuffers(&pAPriv->Port[i]);
    }

    TimerFree(pAPriv->Timer);

    xfree(pAPriv);
}

static AdaptorPrivPtr
NewAdaptorPriv(ScrnInfoPtr pScrn)
{
    AdaptorPrivPtr pAPriv = (AdaptorPrivPtr) xcalloc(1, sizeof(AdaptorPrivRec));
    int i;

    if (!pAPriv) return NULL;

    pAPriv->pScrn = pScrn;
    
    if (!(pAPriv->Timer = TimerSet(NULL, 0, 0, TimerCallback, pAPriv))) {
	DeleteAdaptorPriv(pAPriv);
	return NULL;
    }

    for (i = 0; i < PORTS; i++) {
	pAPriv->Port[i].pAdaptor = pAPriv;

    	pAPriv->Port[i].StopDelay = -1;
	pAPriv->Port[i].fw = 0;
	pAPriv->Port[i].fh = 0;
	pAPriv->Port[i].BuffersRequested = 1;
	pAPriv->Delay = 125;
	pAPriv->Instant = 1000 / 25;

	pAPriv->Port[i].Attribute[5] = 0;	/* Bilinear Filter (Bool) */
	pAPriv->Port[i].Attribute[7] = 0;	/* Alpha Enable (Bool) */
    }

    return pAPriv;
}


/*
 *  Glint interface
 */

void
Permedia3VideoEnterVT(ScrnInfoPtr pScrn)
{
    AdaptorPrivPtr pAPriv;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv enter VT\n"));

    for (pAPriv = AdaptorPrivList; pAPriv != NULL; pAPriv = pAPriv->Next)
	if (pAPriv->pScrn == pScrn) {
	    /* SVEN : nothing is done here, since we disabled
	     * input/ouput video support. */
	}
}

void
Permedia3VideoLeaveVT(ScrnInfoPtr pScrn)
{
    AdaptorPrivPtr pAPriv;

    for (pAPriv = AdaptorPrivList; pAPriv != NULL; pAPriv = pAPriv->Next)
	if (pAPriv->pScrn == pScrn) {
	    /* SVEN : nothing is done here, since we disabled
	     * input/ouput video support. */
	}

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv leave VT\n"));
}

void
Permedia3VideoUninit(ScrnInfoPtr pScrn)
{
    AdaptorPrivPtr pAPriv, *ppAPriv;

    for (ppAPriv = &AdaptorPrivList; (pAPriv = *ppAPriv); ppAPriv = &(pAPriv->Next))
	if (pAPriv->pScrn == pScrn) {
	    *ppAPriv = pAPriv->Next;
	    DeleteAdaptorPriv(pAPriv);
	    break;
	}
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv cleanup\n"));
}

void
Permedia3VideoInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    AdaptorPrivPtr pAPriv;
    DevUnion Private[PORTS];
    XF86VideoAdaptorRec VAR;
    XF86VideoAdaptorPtr VARPtrs;
    int i;

    switch (pGlint->Chipset) {
	case PCI_VENDOR_3DLABS_CHIP_PERMEDIA3:
            break;

	case PCI_VENDOR_3DLABS_CHIP_GAMMA:
	    if (pGlint->MultiChip == PCI_CHIP_PERMEDIA3) break;

	default:
	    xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 1,
		"No Xv support for chipset %d.\n", pGlint->Chipset);
            return;
    }

    if (pGlint->NoAccel) {
	xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 1,
	    "Sorry, need to enable accel for Xv support for Permedia3.\n");
	return;
    }

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 1,
	"Initializing Xv driver rev. 1\n");

    if (!(pAPriv = NewAdaptorPriv(pScrn))) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
	    "Xv driver initialization failed\n");
	return;
    }

    memset(&VAR, 0, sizeof(VAR));

    for (i = 0; i < PORTS; i++)
	Private[i].ptr = (pointer) &pAPriv->Port[i];

    VARPtrs = &VAR;

    VAR.type = XvInputMask | XvWindowMask | XvImageMask;
    VAR.flags = VIDEO_OVERLAID_IMAGES;
    VAR.name = "Permedia 3 Frontend Scaler";
    VAR.nEncodings = ENTRIES(ScalerEncodings);
    VAR.pEncodings = ScalerEncodings;
    VAR.nFormats	= ENTRIES(ScalerVideoFormats);
    VAR.pFormats	= ScalerVideoFormats;
    VAR.nPorts = 3;
    VAR.pPortPrivates = &Private[0];
    VAR.nAttributes = ENTRIES(ScalerAttributes);
    VAR.pAttributes = ScalerAttributes;
    VAR.nImages	= ENTRIES(ScalerImages);
    VAR.pImages	= ScalerImages;

    VAR.PutVideo = NULL;
    VAR.PutStill = NULL;
    VAR.GetVideo = NULL;
    VAR.GetStill = NULL;
    VAR.StopVideo = Permedia3StopVideo;
    VAR.SetPortAttribute = Permedia3SetPortAttribute;
    VAR.GetPortAttribute = Permedia3GetPortAttribute;
    VAR.QueryBestSize = Permedia3QueryBestSize;
    VAR.PutImage = Permedia3PutImage;
    VAR.QueryImageAttributes = Permedia3QueryImageAttributes;

    if (xf86XVScreenInit(pScreen, &VARPtrs, 1)) {
	xvEncoding	= MAKE_ATOM(XV_ENCODING);
	xvHue		= MAKE_ATOM(XV_HUE);
	xvSaturation	= MAKE_ATOM(XV_SATURATION);
	xvBrightness	= MAKE_ATOM(XV_BRIGHTNESS);
	xvContrast	= MAKE_ATOM(XV_CONTRAST);
	xvInterlace	= MAKE_ATOM(XV_INTERLACE);
	xvFilter	= MAKE_ATOM(XV_FILTER);
	xvBkgColor	= MAKE_ATOM(XV_BKGCOLOR);
	xvAlpha		= MAKE_ATOM(XV_ALPHA);

	/* Add it to the AdaptatorList */
	pAPriv->Next = AdaptorPrivList;
	AdaptorPrivList = pAPriv;

	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Xv frontend scaler enabled\n");
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Xv initialization failed\n");
	DeleteAdaptorPriv(pAPriv);
    }
}

#endif /* XvExtension */
