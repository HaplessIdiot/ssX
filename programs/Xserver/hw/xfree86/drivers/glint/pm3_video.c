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
 
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/pm3_video.c,v 1.2 2001/04/10 20:33:31 dawes Exp $ */

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

#define DEBUG(x)

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

#define MAX_BUFFERS 3

enum {
    OVERLAY_DATA_NONE,
    OVERLAY_DATA_COLORKEY,
    OVERLAY_DATA_ALPHAKEY,
    OVERLAY_DATA_ALPHABLEND
} ;

typedef struct _PortPrivRec {
    struct _AdaptorPrivRec *    pAdaptor;

    /* Attributes */
    char			OverlayData;
    INT32			OverlayMode;
    INT32			OverlayControl;
    INT32			Attribute[3];

    /* Buffers */
    int				Id, Bpp;
    int				Format, Bpp_shift;
    short			display, copy;
    FBLinearPtr			Buffer[MAX_BUFFERS];
    CARD32			BufferStride;			/* bytes */
    int				OverlayStride;			/* pixels */

    /* Buffer and Drawable size and position */
    INT32			vx, vy, vw, vh;			/* 12.10 fp */
    int				dx, dy, dw, dh;

    /* Timer stuff */
    OsTimerPtr			Timer;
    Bool			TimerInUse;
    int				Delay, Instant, StopDelay;

} PortPrivRec, *PortPrivPtr;

typedef struct _AdaptorPrivRec {
    struct _AdaptorPrivRec *	Next;
    ScrnInfoPtr			pScrn;
    PortPrivPtr                 pPort;
} AdaptorPrivRec, *AdaptorPrivPtr;

static AdaptorPrivPtr AdaptorPriv;


/*
 *  Proprietary Attributes
 */
 
#define XV_FILTER		"XV_FILTER"
/* We support 3 sorts of filters :
 * 0 : None.
 * 1 : Partial (only in the X directrion).
 * 2 : Full.
 */

#define XV_MIRROR		"XV_MIRROR"	
/* We also support mirroring of the image :
 * bit 0 : if set, will mirror in the X direction.
 * bit 1 : if set, will mirror in the Y direction.
 */

#define XV_OVERLAY_MODE		"XV_OVERLAY_MODE"
/* We support these different overlay modes (bit 0-2) :
 * 0 : Opaque video overlay (default).
 * 1 : Color keyed overlay, framebuffer color key.
 *     Data : bit 3-27 : color key in RGB 888 format.
 * 2 : Color keyed overlay, framebuffer alpha key.
 *     Data : bit 3-11 : 8 bit alpha key.
 * 3 : Color keyed overlay, overlay color key.
 *     Data : bit 3-27 : color key in RGB 888 format.
 * 4 : Per pixel alpha blending.
 * 5 : Constant alpha blending.
 *     Data : bit 3-11 : 8 bit alpha blend factor.
 * 6-7 : Reserved.
 */


static XF86AttributeRec
ScalerAttributes[] =
{
    { XvSettable | XvGettable, 0, 2, XV_FILTER },
    { XvSettable | XvGettable, 0, 3, XV_MIRROR },
    { XvSettable | XvGettable, 0, (2<<27)-1, XV_OVERLAY_MODE },
};

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static Atom xvFilter, xvMirror, xvOverlayMode;


/* Scaler */

static XF86VideoEncodingRec
ScalerEncodings[] =
{
    { 0, "XV_IMAGE", 2047, 2047, { 1, 1 }},
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
RemoveAreaCallback(FBLinearPtr Buffer)
{
    PortPrivPtr pPPriv = (PortPrivPtr) Buffer->devPrivate.ptr;
    int i = -1;

    /* Find the buffer that is being removed */
    for (i = 0; i < MAX_BUFFERS && pPPriv->Buffer[i] != Buffer; i++);
    if (i >= MAX_BUFFERS) return;
	
    if (i == pPPriv->display) pPPriv->display = -1;
    if (i == pPPriv->copy) pPPriv->copy = -1;
    pPPriv->Buffer[i] = NULL;
}

static void
FreeBuffers(PortPrivPtr pPPriv, Bool from_timer)
{
    int i = -1;

    if (!from_timer) {
	if (pPPriv->TimerInUse) {
	    pPPriv->TimerInUse = FALSE;
	    TimerCancel(pPPriv->Timer);
	}
    }

    for (i=0; i < MAX_BUFFERS; i++)
	if (pPPriv->Buffer[i]) {
	    xf86FreeOffscreenLinear (pPPriv->Buffer[i]);
	    pPPriv->Buffer[i] = NULL;
	}
}

static CARD32
TimerCallback(OsTimerPtr pTim, CARD32 now, pointer p)
{
    PortPrivPtr pPPriv = (PortPrivPtr) p;

    if (pPPriv->StopDelay >= 0) {
	if (!(pPPriv->StopDelay--)) {
	    FreeBuffers(pPPriv, TRUE);
	    pPPriv->TimerInUse = FALSE;
	}
    }

    if (pPPriv->TimerInUse)
	return pPPriv->Instant;

    return 0; /* Cancel */
}

static int
AllocateBuffers(PortPrivPtr pPPriv,
    int w_bpp, int h)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    ScrnInfoPtr pScrn = pAPriv->pScrn;
    int i = -1;

    /* we start a timer to free the buffers if they are nto used within
     * 5 seconds (pPPriv->Delay * pPPriv->Instant) */
    pPPriv->StopDelay = pPPriv->Delay;
    if (!pPPriv->TimerInUse) {
	pPPriv->TimerInUse = TRUE;
	TimerSet(pPPriv->Timer, 0, 80, TimerCallback, pAPriv);
    }

    for (i=0; i < MAX_BUFFERS && (i == pPPriv->display || i == pPPriv->copy); i++);

    if (pPPriv->Buffer[i]) {
	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
	    "Buffer %d exists.\n", i));
	if (pPPriv->Buffer[i]->size == w_bpp * h) {
	    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
		"Buffer %d is of the good size, let's use it.\n", i));
	    return (pPPriv->copy = i);
	}
	else if (xf86ResizeOffscreenLinear (pPPriv->Buffer[i], w_bpp * h)) {
		DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
		    "I was able to resize buffer %d, let's use it.\n", i));
	    	return (pPPriv->copy = i);
	    }
	    else {
		DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
		    "I was not able to resize buffer %d.\n", i));
		xf86FreeOffscreenLinear (pPPriv->Buffer[i]);
		pPPriv->Buffer[i] = NULL;
	    }
    }
    if ((pPPriv->Buffer[i] = xf86AllocateOffscreenLinear (pScrn->pScreen,
	w_bpp * h, 0, NULL, NULL, (pointer) pPPriv))) {
	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
	    "Sucessfully allocated buffer %d, let's use it.\n", i));
	return (pPPriv->copy = i);
    }
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
	"Unable to allocate a buffer.\n"));
    return -1;
}

#define GET_OFFSET(pScrn, offset)		\
    (offset + (pScrn->virtualY*pScrn->displayWidth*pScrn->bitsPerPixel/8))

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
	for (;*dst_w > *src_w; (*dst_w)--) {
	    *zoom_delta = (*src_w << 16) / *dst_w;
	    if (((((*zoom_delta&0xf)+1) * *dst_w * *dst_w) >> 16) < *src_w) {
		*zoom_delta = ((*zoom_delta & ~0xf) + 0x10) & 0x0001fff0;
		return;
	    }
	}
	*zoom_delta = 1<<16;
    }
}

/* Some thougth about clipping :
 *
 * To support clipping, we will need to :
 *   - We need to convert the clipregion to a bounding box
 *     and a bitmap that is the mask associated with the clipregion.
 *   - Load this bitmap to offscreen memory.
 *   - Copy/expand this bitmap to the needed area, masking only the alpha
 *     channel of the framebuffer.
 *   - Use either the alpha blended or framebuffer alpha keyed overlay mode to
 *     mask the clipped away region.
 *   - If the clip region gets changed, we have to upload a new clip mask,
 *     clear the old alpha mask in the framebuffer and copy the new clip mask
 *     to the framebuffer again.
 *   - If the position of the region get's changed (but not the clip mask) we
 *     need to clear the old frambuffer clip mask in the alpha channel and
 *     upload the new one.
 *
 * All this will only work if :
 *
 *  1) we are using an framebuffer format with an alpha channel, that is
 *     RGBA 8888 (depth 24) and RGBA 5551 (depth 15).
 *  2) nobody else uses the alpha channel.
 *
 */ 

static void
BeginOverlay(PortPrivPtr pPPriv, int display, int bpp_shift, BoxPtr extent)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    ScrnInfoPtr pScrn = pAPriv->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    unsigned int src_x = pPPriv->vx, dst_x = pPPriv->dx;
    unsigned int src_y = pPPriv->vy, dst_y = pPPriv->dy;
    unsigned int src_w = pPPriv->vw, dst_w = pPPriv->dw;
    unsigned int src_h = pPPriv->vh, dst_h = pPPriv->dh;
    unsigned int shrink_delta, zoom_delta;

    /* Let's overlay only to visible parts of the screen
     * Note : this has no place here, and will not work if
     * clipping is not supported, since Xv will not show this. */
    if (pPPriv->dx < pScrn->frameX0) {
	dst_w = dst_w - pScrn->frameX0 + dst_x;
	dst_x = 0;
	src_w = dst_w * pPPriv->vw / pPPriv->dw;
	src_x = src_x + pPPriv->vw - src_w;
    } else if (pScrn->frameX0 > 0) dst_x = dst_x - pScrn->frameX0;
    if (pPPriv->dy < pScrn->frameY0) {
	dst_h = dst_h - pScrn->frameY0 + pPPriv->dy; 
	dst_y = 0;
	src_h = dst_h * pPPriv->vh / pPPriv->dh;
	src_y = src_y + pPPriv->vh - src_h;
    } else if (pScrn->frameY0 > 0) dst_y = dst_y - pScrn->frameY0;
    if (dst_x + dst_w > (pScrn->frameX1 - pScrn->frameX0)) {
	unsigned int old_w = dst_w;
	dst_w = pScrn->frameX1 - pScrn->frameX0 - dst_x;
	src_w = dst_w * src_w / old_w;
    }
    if (dst_y + dst_h > (pScrn->frameY1 - pScrn->frameY0)) {
	unsigned int old_h = dst_h;
	dst_h = pScrn->frameY1 - pScrn->frameY0 - dst_y;
	src_h = dst_h * src_h / old_h;
    }

    /* Let's adjust the width of source and dest to be compliant with 
     * the Permedia3 overlay unit requirement, and compute the X deltas. */
    compute_scale_factor(&src_w, &dst_w, &shrink_delta, &zoom_delta);

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3, "BeginOverlay\n"));
    if (src_w != pPPriv->vw)
	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	    "BeginOverlay : Padding video width to 4 pixels %d->%d.\n",
	    pPPriv->vw, src_w));
    if (dst_w != pPPriv->dw)
	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	    "BeginOverlay : Scaling destination width from %d to %d.\n"
	    "\tThe scaling factor is to high, and may cause problems.",
	    pPPriv->dw, dst_w));

    if (display != -1) pPPriv->display = display;
    GLINT_WAIT(12);
    GLINT_WRITE_REG(3|(12<<16), PM3VideoOverlayFifoControl);
    /* Updating the Video Overlay Source Image Parameters */
    GLINT_WRITE_REG(
	GET_OFFSET(pScrn, pPPriv->Buffer[pPPriv->display]->offset)>>bpp_shift,
	PM3VideoOverlayBase+(pPPriv->display*8));
    GLINT_WRITE_REG(pPPriv->display, PM3VideoOverlayIndex);
    GLINT_WRITE_REG(pPPriv->Format |
	PM3VideoOverlayMode_BUFFERSYNC_MANUAL |
	PM3VideoOverlayMode_FLIP_VIDEO |
	/* Filtering & Mirroring Attributes */
	pPPriv->OverlayMode |
	PM3VideoOverlayMode_ENABLE,
	PM3VideoOverlayMode);
    /* Let's set the source stride. */
    GLINT_WRITE_REG(PM3VideoOverlayStride_STRIDE(pPPriv->OverlayStride),
	PM3VideoOverlayStride);
    /* Let's set the position and size of the visible part of the source. */
    GLINT_WRITE_REG(PM3VideoOverlayWidth_WIDTH(src_w),
	PM3VideoOverlayWidth);
    GLINT_WRITE_REG(PM3VideoOverlayHeight_HEIGHT(src_h),
	PM3VideoOverlayHeight);
    GLINT_WRITE_REG(
	PM3VideoOverlayOrigin_XORIGIN(src_x) |
	PM3VideoOverlayOrigin_YORIGIN(src_y),
	PM3VideoOverlayOrigin);
    /* Scale the source to the destinationsize */
    if (src_h == dst_h) {
	GLINT_WRITE_REG(
	    PM3VideoOverlayYDelta_NONE,
	    PM3VideoOverlayYDelta);
    } else {
	GLINT_WRITE_REG(
	    PM3VideoOverlayYDelta_DELTA(src_h,dst_h),
	    PM3VideoOverlayYDelta);
    }
    GLINT_WRITE_REG(shrink_delta, PM3VideoOverlayShrinkXDelta);
    GLINT_WRITE_REG(zoom_delta, PM3VideoOverlayZoomXDelta);
    /* Launch the true update at the next FrameBlank */
    GLINT_WRITE_REG(PM3VideoOverlayUpdate_ENABLE,
	PM3VideoOverlayUpdate);
    /* Setting the ramdac video overlay rgion */
    /* Begining of overlay region */
    RAMDAC_WRITE((dst_x&0xff), PM3RD_VideoOverlayXStartLow);
    RAMDAC_WRITE((dst_x&0xf00)>>8, PM3RD_VideoOverlayXStartHigh);
    RAMDAC_WRITE((dst_y&0xff), PM3RD_VideoOverlayYStartLow); 
    RAMDAC_WRITE((dst_y&0xf00)>>8, PM3RD_VideoOverlayYStartHigh);
    /* End of overlay regions (+1) */
    RAMDAC_WRITE(((dst_x+dst_w)&0xff), PM3RD_VideoOverlayXEndLow);
    RAMDAC_WRITE(((dst_x+dst_w)&0xf00)>>8,PM3RD_VideoOverlayXEndHigh);
    RAMDAC_WRITE(((dst_y+dst_h)&0xff), PM3RD_VideoOverlayYEndLow); 
    RAMDAC_WRITE(((dst_y+dst_h)&0xf00)>>8,PM3RD_VideoOverlayYEndHigh);
    
    switch (pPPriv->OverlayData) {
	case OVERLAY_DATA_COLORKEY :
	    RAMDAC_WRITE(((pPPriv->OverlayControl>>8)&0xff),
		PM3RD_VideoOverlayKeyR);
	    RAMDAC_WRITE(((pPPriv->OverlayControl>>16)&0xff),
		PM3RD_VideoOverlayKeyG);
	    RAMDAC_WRITE(((pPPriv->OverlayControl>>24)&0xff),
		PM3RD_VideoOverlayKeyB);
	    break;
	case OVERLAY_DATA_ALPHAKEY :
	    RAMDAC_WRITE(((pPPriv->OverlayControl>>8)&0xff),
		PM3RD_VideoOverlayKeyR);
	    break;
	case OVERLAY_DATA_ALPHABLEND :
	    RAMDAC_WRITE(((pPPriv->OverlayControl>>8)&0xff),
		PM3RD_VideoOverlayBlend);
	    break;
    }
    /* And now enable Video Overlay in the RAMDAC */
    RAMDAC_WRITE(PM3RD_VideoOverlayControl_ENABLE |
	/* OverlayMode attribute */
	(pPPriv->OverlayControl&0xff),
	PM3RD_VideoOverlayControl);

    pPPriv->Buffer[pPPriv->display]->RemoveLinearCallback =
	RemoveAreaCallback;
    if (display != -1) pPPriv->copy = -1;
}

static void
StopOverlay(PortPrivPtr pPPriv, int cleanup)
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
/* ReputImage may be used if only the position of the destination changes,
 * maybe while moving the window around or something such.
 */
static int
Permedia3ReputImage(ScrnInfoPtr pScrn,
    short drw_x, short drw_y,
    RegionPtr clipBoxes, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  
    GLINTPtr pGlint = GLINTPTR(pScrn);
    BoxPtr extent;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"ReputImage %d,%d.\n", drw_x, drw_y));
    /* If the buffer was freed, we cannot overlay it. */
    if (pPPriv->display == -1) {
	StopOverlay (pPPriv, FALSE);
	return Success;
    }
    if (REGION_SIZE(clipBoxes) != 0) {
	/* We need to transform the clipBoxes into a bitmap,
	 * and upload it to offscreen memory. */
	StopOverlay (pPPriv, FALSE);
	return Success;
    }
    /* Check that the dst area is some part of the visible screen. */
    if ((drw_x + pPPriv->dw) < pScrn->frameX0 ||
        (drw_y + pPPriv->dh) < pScrn->frameY0 ||
	drw_x > pScrn->frameX1 || drw_y > pScrn->frameY1) {
        return Success;
    }
    /* Copy the destinations coordinates */
    pPPriv->dx = drw_x;
    pPPriv->dy = drw_y;

    /* We sync the chip. */
    if (pGlint->MultiAperture) DualPermedia3Sync(pScrn);
    else Permedia3Sync(pScrn);

    extent = REGION_EXTENTS(pScrn, clipBoxes);
    BeginOverlay(pPPriv, -1, pPPriv->Bpp_shift, extent);

    /* We sync the chip. */
    if (pGlint->MultiAperture) DualPermedia3Sync(pScrn);
    else Permedia3Sync(pScrn);

    return Success;
}

static int
Permedia3PutImage(ScrnInfoPtr pScrn,
    short src_x, short src_y, short drw_x, short drw_y,
    short src_w, short src_h, short drw_w, short drw_h,
    int id, unsigned char *buf, short width, short height,
    Bool sync, RegionPtr clipBoxes, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int copy = -1;
    BoxPtr extent;
    BoxRec ext;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"PutImage %d,%d,%d,%d -> %d,%d,%d,%d "
	"id=0x%08x buf=%p w=%d h=%d sync=%d\n",
	src_x, src_y, src_w, src_h, drw_x, drw_y, drw_w, drw_h,
	id, buf, width, height, sync));

    extent = REGION_EXTENTS(pScrn, clipBoxes);
    ext = *extent;
    if (REGION_SIZE(clipBoxes) != 0) return Success;

    /* Check that the src area to put is included in the buffer. */
    if ((src_x + src_w) > width ||
        (src_y + src_h) > height ||
	src_x < 0 || src_y < 0)
        return BadValue;

    /* Check that the dst area is some part of the visible screen. */
    if ((drw_x + drw_w) < pScrn->frameX0 ||
        (drw_y + drw_h) < pScrn->frameY0 ||
	drw_x > pScrn->frameX1 || drw_y > pScrn->frameY1) {
        return Success;
    }

    /* Copy the source and destinations coordinates and size */
    pPPriv->vx = src_x;
    pPPriv->vy = src_y;
    pPPriv->vw = src_w;
    pPPriv->vh = src_h;

    pPPriv->dx = drw_x;
    pPPriv->dy = drw_y;
    pPPriv->dw = drw_w;
    pPPriv->dh = drw_h;

    /* If the image format changed since a previous call,
     * let's check if it is supported. By default, we suppose that
     * the previous image format was ScalerImages[0].id */
    if (id != pPPriv->Id) {
	int i;
	for (i = 0; i < ENTRIES(ScalerImages); i++)
	    if (id == ScalerImages[i].id)
		break;
	if (i >= ENTRIES(ScalerImages))
	    return XvBadAlloc;
	pPPriv->Id = id;
	pPPriv->Bpp = ScalerImages[i].bits_per_pixel;
    }

    /* We sync the chip. I don't know if it is really
     * needed but X crashed when i didn't do it. */
    if (pGlint->MultiAperture) DualPermedia3Sync(pScrn);
    else Permedia3Sync(pScrn);

    /* Let's define the different strides values */
    pPPriv->OverlayStride = width;
    pPPriv->BufferStride = ((pPPriv->Bpp+7)>>3) * width;

    /* Now we allocate a buffer, if it is needed */
    if ((copy = AllocateBuffers(pPPriv, pPPriv->BufferStride, height)) == -1)
	return XvBadAlloc;
    
    /* Now, we can copy the image to the buffer */
    switch (id) {
    case LE4CC('Y','V','1','2'):
#if X_BYTE_ORDER == X_LITTLE_ENDIAN
	CopyYV12LE(buf,
	    (CARD32 *)((CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset)),
	    width, height, pPPriv->BufferStride);
#else
	if (pGlint->FBDev)
	    CopyYV12LE(buf,
		(CARD32 *)((CARD8 *) pGlint->FbBase +
		    GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset)),
		width, height, pPPriv->BufferStride);
	else
	    CopyYV12BE(buf,
		(CARD32 *)((CARD8 *) pGlint->FbBase +
		    GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset)),
		width, height, pPPriv->BufferStride);
#endif
	pPPriv->Format = FORMAT_YUV422;
	pPPriv->Bpp_shift = 1;
	break;
    case LE4CC('Y','U','Y','2'):
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 1, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_YUV422;
	pPPriv->Bpp_shift = 1;
	break;

    case LE4CC('U','Y','V','Y'):
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 1, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_VUY422;
	pPPriv->Bpp_shift = 1;
	break;

    case LE4CC('Y','U','V','A'):
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 2, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_YUV444;
	pPPriv->Bpp_shift = 2;
	break;

    case LE4CC('V','U','Y','A'):
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 2, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_VUY444;
	pPPriv->Bpp_shift = 2;
	break;

    case 0x41: /* RGBA 8:8:8:8 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 2, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_RGB8888;
	pPPriv->Bpp_shift = 2;
	break;

    case 0x42: /* RGB 5:6:5 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 1, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_RGB565;
	pPPriv->Bpp_shift = 1;
	break;

    case 0x43: /* RGB 1:5:5:5 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 1, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_RGB5551;
	pPPriv->Bpp_shift = 1;
	break;

    case 0x44: /* RGB 4:4:4:4 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 1, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_RGB4444;
	pPPriv->Bpp_shift = 1;
	break;

    case 0x46: /* RGB 2:3:3 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_RGB332;
	pPPriv->Bpp_shift = 0;
	break;

    case 0x47: /* BGRA 8:8:8:8 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 2, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_BGR8888;
	pPPriv->Bpp_shift = 2;
	break;

    case 0x48: /* BGR 5:6:5 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 1, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_BGR565;
	pPPriv->Bpp_shift = 1;
	break;

    case 0x49: /* BGR 1:5:5:5 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 1, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_BGR5551;
	pPPriv->Bpp_shift = 1;
	break;

    case 0x4A: /* BGR 4:4:4:4 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 1, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_BGR4444;
	pPPriv->Bpp_shift = 1;
	break;

    case 0x4C: /* BGR 2:3:3 */
	CopyFlat(buf, (CARD8 *) pGlint->FbBase +
		GET_OFFSET(pScrn, pPPriv->Buffer[copy]->offset),
	    width << 0, height, pPPriv->BufferStride);
	pPPriv->Format = FORMAT_BGR332;
	pPPriv->Bpp_shift = 0;
	break;
    default:
	return XvBadAlloc;
    }
    /* We sync the chip. */
    if (pGlint->MultiAperture) DualPermedia3Sync(pScrn);
    else Permedia3Sync(pScrn);

    /* Don't know why we need this,
     * but the server will crash if i remove it. */
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"Starting the overlay.\n");

    /* We start the overlay */
    BeginOverlay(pPPriv, copy, pPPriv->Bpp_shift, extent);

    /* We sync the chip again. */
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

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"StopVideo : exit=%d\n", cleanup));

    StopOverlay(pPPriv, cleanup);

    if (cleanup) {
	FreeBuffers(pPPriv, FALSE);
    }
}

static int
Permedia3SetPortAttribute(ScrnInfoPtr pScrn,
    Atom attribute, INT32 value, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;

    /* Note, we could decode and store attributes directly here */
    if (attribute == xvFilter) {
	switch (value) {
	    case 0:	/* No Filtering */
		pPPriv->OverlayMode =
		    (pPPriv->OverlayMode &
		      ~PM3VideoOverlayMode_FILTER_MASK) |
		    PM3VideoOverlayMode_FILTER_OFF;
		break;
	    case 1:	/* No Filtering */
		pPPriv->OverlayMode =
		    (pPPriv->OverlayMode &
		      ~PM3VideoOverlayMode_FILTER_MASK) |
		    PM3VideoOverlayMode_FILTER_PARTIAL;
		break;
	    case 2:	/* No Filtering */
		pPPriv->OverlayMode =
		    (pPPriv->OverlayMode &
		      ~PM3VideoOverlayMode_FILTER_MASK) |
		    PM3VideoOverlayMode_FILTER_FULL;
		break;
	    default:
		return BadValue;
	}
	pPPriv->Attribute[0] = value;
    }
    else if (attribute == xvMirror) {
	switch (value) {
	    case 0:	/* No Mirroring */
		pPPriv->OverlayMode =
		    (pPPriv->OverlayMode &
		      ~PM3VideoOverlayMode_MIRROR_MASK) |
		    PM3VideoOverlayMode_MIRRORX_OFF |
		    PM3VideoOverlayMode_MIRRORY_OFF;
		break;
	    case 1:	/* X Axis Mirroring */
		pPPriv->OverlayMode =
		    (pPPriv->OverlayMode &
		      ~PM3VideoOverlayMode_MIRROR_MASK) |
		    PM3VideoOverlayMode_MIRRORX_ON |
		    PM3VideoOverlayMode_MIRRORY_OFF;
		break;
	    case 2:	/* Y Axis Mirroring */
		pPPriv->OverlayMode =
		    (pPPriv->OverlayMode &
		      ~PM3VideoOverlayMode_MIRROR_MASK) |
		    PM3VideoOverlayMode_MIRRORX_OFF |
		    PM3VideoOverlayMode_MIRRORY_ON;
		break;
	    case 3:	/* X and Y Axis Mirroring */
		pPPriv->OverlayMode =
		    (pPPriv->OverlayMode &
		      ~PM3VideoOverlayMode_MIRROR_MASK) |
		    PM3VideoOverlayMode_MIRRORX_ON |
		    PM3VideoOverlayMode_MIRRORY_ON;
		break;
	    default:
		return BadValue;
	}
	pPPriv->Attribute[1] = value;
    }
    else if (attribute == xvOverlayMode) {
	pPPriv->OverlayControl =
	    PM3RD_VideoOverlayControl_ENABLE;
	switch (value&&0xff) {
	    case 0:	/* Opaque video overlay */
		pPPriv->OverlayData = 
		    OVERLAY_DATA_NONE;
		pPPriv->OverlayControl =
		    PM3RD_VideoOverlayControl_MODE_ALWAYS;
		break;	
	    case 1:	/* Color keyed overlay, framebuffer color key */
		pPPriv->OverlayData = 
		    OVERLAY_DATA_COLORKEY;
		pPPriv->OverlayControl =
		    /* color key in RGB 888 mode */
		    ((value<<5)&0xffffff00) |
		    PM3RD_VideoOverlayControl_MODE_MAINKEY |
		    PM3RD_VideoOverlayControl_KEY_COLOR;
		break;	
	    case 2:	/* Color keyed overlay, framebuffer alpha key */
		pPPriv->OverlayData = 
		    OVERLAY_DATA_ALPHAKEY;
		pPPriv->OverlayControl =
		    /* 8 bit alpha key */
		    ((value<<5)&0xff00) |
		    PM3RD_VideoOverlayControl_MODE_MAINKEY |
		    PM3RD_VideoOverlayControl_KEY_ALPHA;
		break;	
	    case 3:	/* Color keyed overlay, overlay color key */
		pPPriv->OverlayData = 
		    OVERLAY_DATA_COLORKEY;
		pPPriv->OverlayControl =
		    /* color key in RGB 888 mode */
		    ((value<<5)&0xffffff00) |
		    PM3RD_VideoOverlayControl_MODE_OVERLAYKEY;
		break;	
	    case 4:	/* Per pixel alpha blending */
		pPPriv->OverlayData = 
		    OVERLAY_DATA_NONE;
		pPPriv->OverlayControl =
		    PM3RD_VideoOverlayControl_MODE_BLEND |
		    PM3RD_VideoOverlayControl_BLENDSRC_MAIN;
		break;	
	    case 5:	/* Constant alpha blending */
		pPPriv->OverlayData = 
		    OVERLAY_DATA_ALPHABLEND;
		pPPriv->OverlayControl =
		    /* 8 bit alpha blend factor
		     * (only the 2 top bits are used) */
		    ((value<<5)&0xff00) |
		    PM3RD_VideoOverlayControl_MODE_BLEND |
		    PM3RD_VideoOverlayControl_BLENDSRC_REGISTER;
		break;	
	    default:
		return BadValue;
	}
	pPPriv->Attribute[2] = value;
    }
    else return BadMatch;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"SPA attr=%d val=%d\n",
	attribute, value));

    return Success;
}

static int
Permedia3GetPortAttribute(ScrnInfoPtr pScrn, 
    Atom attribute, INT32 *value, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;

    if (attribute == xvFilter)
	*value = pPPriv->Attribute[0];
    else if (attribute == xvMirror)
	*value = pPPriv->Attribute[1];
    else if (attribute == xvOverlayMode)
	*value = pPPriv->Attribute[2];
    else return BadMatch;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3,
	"GPA attr=%d val=%d\n",
	attribute, *value));

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
    FreeBuffers(pAPriv->pPort, FALSE);

    TimerFree(pAPriv->pPort->Timer);

    xfree(pAPriv);
}

static AdaptorPrivPtr
NewAdaptorPriv(ScrnInfoPtr pScrn)
{
    AdaptorPrivPtr pAPriv = (AdaptorPrivPtr) xcalloc(1, sizeof(AdaptorPrivRec));
    PortPrivPtr pPPriv = (PortPrivPtr) xcalloc(1, sizeof(PortPrivRec));
    int i;

    if (!pAPriv) return NULL;
    pAPriv->pScrn = pScrn;
    if (!pPPriv) return NULL;
    pAPriv->pPort = pPPriv;

    
    /* We allocate a timer */
    if (!(pPPriv->Timer = TimerSet(NULL, 0, 0, TimerCallback, pPPriv))) {
	DeleteAdaptorPriv(pAPriv);
	return NULL;
    }

    /* Attributes */
    pPPriv->pAdaptor = pAPriv;
    pPPriv->Attribute[0] = 0;	/* Full filtering enabled */
    pPPriv->Attribute[1] = 0;	/* No mirroring */
    pPPriv->Attribute[2] = 0;	/* Opaque overlay mode */
    pPPriv->OverlayData = 0;
    pPPriv->OverlayMode =
	PM3VideoOverlayMode_FILTER_FULL |
	PM3VideoOverlayMode_MIRRORX_OFF |
	PM3VideoOverlayMode_MIRRORY_OFF;
    pPPriv->OverlayControl =
	PM3RD_VideoOverlayControl_ENABLE |
	PM3RD_VideoOverlayControl_MODE_ALWAYS;

    /* Buffers */
    pPPriv->Id = ScalerImages[0].id;
    pPPriv->Bpp = ScalerImages[0].bits_per_pixel;
    pPPriv->copy = -1;
    pPPriv->display = -1;
    for (i = 0; i < MAX_BUFFERS; i++)
	pPPriv->Buffer[i] = NULL;

    /* Timer */
    pPPriv->StopDelay = -1;
    pPPriv->Delay = 125;
    pPPriv->Instant = 1000 / 25;

    return pAPriv;
}

/*
 *  Glint interface
 */

void
Permedia3VideoEnterVT(ScrnInfoPtr pScrn)
{
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv enter VT\n"));
}

void
Permedia3VideoLeaveVT(ScrnInfoPtr pScrn)
{
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv leave VT\n"));
}

void
Permedia3VideoUninit(ScrnInfoPtr pScrn)
{
    if (AdaptorPriv->pScrn == pScrn) {
	DeleteAdaptorPriv(AdaptorPriv);
	AdaptorPriv = NULL;
    }
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv cleanup\n"));
}

void
Permedia3VideoInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    AdaptorPrivPtr pAPriv;
    DevUnion Private[1];
    XF86VideoAdaptorRec VAR;
    XF86VideoAdaptorPtr VARPtrs;

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

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 1,
	"Initializing Permedia3 Xv driver rev. 1\n");

    if (pGlint->NoAccel) {
	xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 1,
	    "Xv : Sorry, Xv is not supported without accelerations");
	return;
#if 0	/* This works, but crashes the X server after a time. */
	xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 1,
	    "Xv : Acceleration disabled, starting offscreen memory manager\n.");
	Permedia3EnableOffscreen (pScreen);
#endif
    }

    if (!(pAPriv = NewAdaptorPriv(pScrn))) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Xv initialization failed\n");
	return;
    }

    memset(&VAR, 0, sizeof(VAR));

    Private[0].ptr = (pointer) pAPriv->pPort;

    VARPtrs = &VAR;

    VAR.type = XvInputMask | XvWindowMask | XvImageMask;
    VAR.flags = VIDEO_OVERLAID_IMAGES;
    VAR.name = "Permedia 3 Frontend Scaler";
    VAR.nEncodings = ENTRIES(ScalerEncodings);
    VAR.pEncodings = ScalerEncodings;
    VAR.nFormats	= ENTRIES(ScalerVideoFormats);
    VAR.pFormats	= ScalerVideoFormats;
    VAR.nPorts = 1;
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
    VAR.ReputImage = Permedia3ReputImage;
    VAR.QueryImageAttributes = Permedia3QueryImageAttributes;

    if (xf86XVScreenInit(pScreen, &VARPtrs, 1)) {
	xvFilter	= MAKE_ATOM(XV_FILTER);
	xvMirror	= MAKE_ATOM(XV_MIRROR);
	xvOverlayMode	= MAKE_ATOM(XV_OVERLAY_MODE);

	/* Add it to the AdaptatorList */
	AdaptorPriv = pAPriv;

	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Xv frontend scaler enabled\n");
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Xv initialization failed\n");
	DeleteAdaptorPriv(pAPriv);
    }
}

#endif /* XvExtension */
