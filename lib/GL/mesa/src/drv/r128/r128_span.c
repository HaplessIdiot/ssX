/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_span.c,v 1.1 2000/06/17 00:03:07 martin Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Keith Whitwell <keith@precisioninsight.com>
 *   Gareth Hughes <gareth@precisioninsight.com>
 *
 */

#include "r128_init.h"
#include "r128_xmesa.h"
#include "r128_context.h"
#include "r128_lock.h"
#include "r128_state.h"
#include "r128_reg.h"
#include "r128_cce.h"
#include "r128_span.h"

#define DBG 0

#define LOCAL_VARS                                                            \
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);                               \
    r128ScreenPtr r128scrn = r128ctx->r128Screen;                             \
    __DRIdrawablePrivate *dPriv = r128ctx->driDrawable;                       \
    GLuint pitch = r128scrn->fbStride;                                        \
    GLuint height = dPriv->h;                                                 \
    char *buf = (char *)(r128scrn->fb +                                       \
			 (r128ctx->drawX + dPriv->x) * (r128scrn->bpp/8) +    \
			 (r128ctx->drawY + dPriv->y) * pitch);                \
    char *read_buf = (char *)(r128scrn->fb +                                  \
			      (r128ctx->readX + dPriv->x) * (r128scrn->bpp/8)+\
			      (r128ctx->readY + dPriv->y) * pitch);           \
    GLushort p;                                                               \
    (void) read_buf; (void) buf; (void) p

#define LOCAL_DEPTH_VARS                                                      \
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);                               \
    r128ScreenPtr r128scrn = r128ctx->r128Screen;                             \
    __DRIdrawablePrivate *dPriv = r128ctx->driDrawable;                       \
    GLuint pitch = r128scrn->fbStride;                                        \
    GLuint height = dPriv->h;                                                 \
    char *buf = (char *)(r128scrn->fb +                                       \
			 (r128scrn->depthX + dPriv->x) * (r128scrn->bpp/8) +  \
			 (r128scrn->depthY + dPriv->y) * pitch);              \
    (void) buf

#define INIT_MONO_PIXEL(p)                                                    \
    p = R128_CONTEXT(ctx)->Color

#define CLIPPIXEL(_x, _y)                                                     \
    ((_x >= minx) && (_x < maxx) && (_y >= miny) && (_y < maxy))


#define CLIPSPAN(_x, _y, _n, _x1, _n1, _i)                                    \
    if (( _y < miny) || (_y >= maxy)) {                                       \
	_n1 = 0, _x1 = x;                                                     \
    } else {                                                                  \
	_n1 = _n;                                                             \
	_x1 = _x;                                                             \
	if (_x1 < minx) _i += (minx - _x1), _x1 = minx;                       \
	if (_x1 + _n1 >= maxx) n1 -= (_x1 + n1 - maxx) + 1;                   \
    }

#define Y_FLIP(_y) (height - _y - 1)


#define HW_LOCK()                                                             \
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);                               \
    FLUSH_BATCH(r128ctx);                                                     \
    LOCK_HARDWARE(r128ctx);                                                   \
    r128WaitForIdleLocked(r128ctx);

#define HW_CLIPLOOP()                                                         \
    do {                                                                      \
	__DRIdrawablePrivate *dPriv = r128ctx->driDrawable;                   \
	int _nc = dPriv->numClipRects;                                        \
                                                                              \
	while (_nc--) {                                                       \
	    int minx = dPriv->pClipRects[_nc].x1 - dPriv->x;                  \
	    int miny = dPriv->pClipRects[_nc].y1 - dPriv->y;                  \
	    int maxx = dPriv->pClipRects[_nc].x2 - dPriv->x;                  \
	    int maxy = dPriv->pClipRects[_nc].y2 - dPriv->y;

#define HW_ENDCLIPLOOP()                                                      \
	}                                                                     \
    } while (0)

#define HW_UNLOCK()                                                           \
    UNLOCK_HARDWARE(r128ctx)



/* 16 bit, RGB565 color spanline and pixel functions */
#define WRITE_RGBA(_x, _y, r, g, b, a)                                        \
    *(GLushort *)(buf + _x*2 + _y*pitch) = ((((int)r & 0xf8) << 8) |          \
					    (((int)g & 0xfc) << 3) |          \
					    (((int)b & 0xf8) >> 3) )

#define WRITE_PIXEL(_x, _y, p)                                                \
    *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA(rgba, _x, _y)                                               \
    do {                                                                      \
	GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);               \
	rgba[0] = (p >> 8) & 0xf8;                                            \
	rgba[1] = (p >> 3) & 0xfc;                                            \
	rgba[2] = (p << 3) & 0xf8;                                            \
	rgba[3] = 0xff;                                                       \
    } while (0)

#define TAG(x) r128##x##_RGB565
#include "spantmp.h"



/* 15 bit, ARGB1555 color spanline and pixel functions */
#define WRITE_RGBA(_x, _y, r, g, b, a)                                        \
    *(GLushort *)(buf + _x*2 + _y*pitch) = (((r & 0xf8) << 7) |               \
					    ((g & 0xf8) << 2) |               \
					    ((b & 0xf8) >> 3) |               \
					    ((a) ? 0x8000 : 0))

#define WRITE_PIXEL(_x, _y, p)                                                \
    *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA(rgba, _x, _y)                                               \
    do {                                                                      \
	GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);               \
	rgba[0] = (p >> 7) & 0xf8;                                            \
	rgba[1] = (p >> 2) & 0xf8;                                            \
	rgba[2] = (p << 3) & 0xf8;                                            \
	rgba[3] = (p & 0x8000) ? 0xff : 0;                                    \
    } while (0)

#define TAG(x) r128##x##_ARGB1555
#include "spantmp.h"



/* 16 bit depthbuffer functions */
#define WRITE_DEPTH(_x, _y, d)                                                \
    *(GLushort *)(buf + _x*2 + _y*pitch) = d;

#define READ_DEPTH(d, _x, _y)                                                 \
    d = *(GLushort *)(buf + _x*2 + _y*pitch)

#define TAG(x) r128##x##_16
#include "depthtmp.h"



/* 24 bit, RGB888 color spanline and pixel functions */
#define WRITE_RGBA(_x, _y, r, g, b, a)                                        \
    *(GLuint *)(buf + _x*3 + _y*pitch) = ((r << 16) |                         \
					  (g << 8)  |                         \
					  (b << 0))

#define WRITE_PIXEL(_x, _y, p)                                                \
    *(GLuint *)(buf + _x*3 + _y*pitch) = p

#define READ_RGBA(rgba, _x, _y)                                               \
    do {                                                                      \
	GLuint p = *(GLuint *)(read_buf + _x*3 + _y*pitch);                   \
	rgba[0] = (p >> 16) & 0xff;                                           \
	rgba[1] = (p >> 8)  & 0xff;                                           \
	rgba[2] = (p >> 0)  & 0xff;                                           \
	rgba[3] = 0xff;                                                       \
    } while (0)

#define TAG(x) r128##x##_RGB888
#include "spantmp.h"



/* 24 bit depthbuffer functions */
#define WRITE_DEPTH(_x, _y, d)                                                \
    *(GLuint *)(buf + _x*3 + _y*pitch) = d

#define READ_DEPTH(d, _x, _y)                                                 \
    d = *(GLuint *)(buf + _x*3 + _y*pitch)

#define TAG(x) r128##x##_24
#include "depthtmp.h"



/* 32 bit, ARGB8888 color spanline and pixel functions */
#define WRITE_RGBA(_x, _y, r, g, b, a)                                        \
    *(GLuint *)(buf + _x*4 + _y*pitch) = ((r << 16) |                         \
					  (g << 8)  |                         \
					  (b << 0)  |                         \
					  (a << 24) )

#define WRITE_PIXEL(_x, _y, p)                                                \
    *(GLuint *)(buf + _x*4 + _y*pitch) = p

#define READ_RGBA(rgba, _x, _y)                                               \
    do {                                                                      \
	GLuint p = *(GLuint *)(read_buf + _x*4 + _y*pitch);                   \
	rgba[0] = (p >> 16) & 0xff;                                           \
	rgba[1] = (p >> 8)  & 0xff;                                           \
	rgba[2] = (p >> 0)  & 0xff;                                           \
	rgba[3] = (p >> 24) & 0xff;                                           \
    } while (0)

#define TAG(x) r128##x##_ARGB8888
#include "spantmp.h"



/* 32 bit depthbuffer functions */
#define WRITE_DEPTH(_x, _y, d)                                                \
    *(GLuint *)(buf + _x*4 + _y*pitch) = d

#define READ_DEPTH(d, _x, _y)                                                 \
    d = *(GLuint *)(buf + _x*4 + _y*pitch)

#define TAG(x) r128##x##_32
#include "depthtmp.h"



void r128DDInitSpanFuncs(GLcontext *ctx)
{
    r128ContextPtr r128ctx = R128_CONTEXT(ctx);

    switch (r128ctx->r128Screen->pixel_code) {
    case 8: /* Color Index mode not supported */
	break;

    case 15:
	ctx->Driver.WriteRGBASpan       = r128WriteRGBASpan_ARGB1555;
	ctx->Driver.WriteRGBSpan        = r128WriteRGBSpan_ARGB1555;
	ctx->Driver.WriteMonoRGBASpan   = r128WriteMonoRGBASpan_ARGB1555;
	ctx->Driver.WriteRGBAPixels     = r128WriteRGBAPixels_ARGB1555;
	ctx->Driver.WriteMonoRGBAPixels = r128WriteMonoRGBAPixels_ARGB1555;
	ctx->Driver.ReadRGBASpan        = r128ReadRGBASpan_ARGB1555;
	ctx->Driver.ReadRGBAPixels      = r128ReadRGBAPixels_ARGB1555;

	ctx->Driver.ReadDepthSpan       = r128ReadDepthSpan_16;
	ctx->Driver.WriteDepthSpan      = r128WriteDepthSpan_16;
	ctx->Driver.ReadDepthPixels     = r128ReadDepthPixels_16;
	ctx->Driver.WriteDepthPixels    = r128WriteDepthPixels_16;
	break;

    case 16:
	ctx->Driver.WriteRGBASpan       = r128WriteRGBASpan_RGB565;
	ctx->Driver.WriteRGBSpan        = r128WriteRGBSpan_RGB565;
	ctx->Driver.WriteMonoRGBASpan   = r128WriteMonoRGBASpan_RGB565;
	ctx->Driver.WriteRGBAPixels     = r128WriteRGBAPixels_RGB565;
	ctx->Driver.WriteMonoRGBAPixels = r128WriteMonoRGBAPixels_RGB565;
	ctx->Driver.ReadRGBASpan        = r128ReadRGBASpan_RGB565;
	ctx->Driver.ReadRGBAPixels      = r128ReadRGBAPixels_RGB565;

	ctx->Driver.ReadDepthSpan       = r128ReadDepthSpan_16;
	ctx->Driver.WriteDepthSpan      = r128WriteDepthSpan_16;
	ctx->Driver.ReadDepthPixels     = r128ReadDepthPixels_16;
	ctx->Driver.WriteDepthPixels    = r128WriteDepthPixels_16;
	break;

    case 24:
	ctx->Driver.WriteRGBASpan       = r128WriteRGBASpan_RGB888;
	ctx->Driver.WriteRGBSpan        = r128WriteRGBSpan_RGB888;
	ctx->Driver.WriteMonoRGBASpan   = r128WriteMonoRGBASpan_RGB888;
	ctx->Driver.WriteRGBAPixels     = r128WriteRGBAPixels_RGB888;
	ctx->Driver.WriteMonoRGBAPixels = r128WriteMonoRGBAPixels_RGB888;
	ctx->Driver.ReadRGBASpan        = r128ReadRGBASpan_RGB888;
	ctx->Driver.ReadRGBAPixels      = r128ReadRGBAPixels_RGB888;

	ctx->Driver.ReadDepthSpan       = r128ReadDepthSpan_24;
	ctx->Driver.WriteDepthSpan      = r128WriteDepthSpan_24;
	ctx->Driver.ReadDepthPixels     = r128ReadDepthPixels_24;
	ctx->Driver.WriteDepthPixels    = r128WriteDepthPixels_24;
	break;

    case 32:
	ctx->Driver.WriteRGBASpan       = r128WriteRGBASpan_ARGB8888;
	ctx->Driver.WriteRGBSpan        = r128WriteRGBSpan_ARGB8888;
	ctx->Driver.WriteMonoRGBASpan   = r128WriteMonoRGBASpan_ARGB8888;
	ctx->Driver.WriteRGBAPixels     = r128WriteRGBAPixels_ARGB8888;
	ctx->Driver.WriteMonoRGBAPixels = r128WriteMonoRGBAPixels_ARGB8888;
	ctx->Driver.ReadRGBASpan        = r128ReadRGBASpan_ARGB8888;
	ctx->Driver.ReadRGBAPixels      = r128ReadRGBAPixels_ARGB8888;

	ctx->Driver.ReadDepthSpan       = r128ReadDepthSpan_32;
	ctx->Driver.WriteDepthSpan      = r128WriteDepthSpan_32;
	ctx->Driver.ReadDepthPixels     = r128ReadDepthPixels_32;
	ctx->Driver.WriteDepthPixels    = r128WriteDepthPixels_32;
	break;

    default:
	break;
    }

    ctx->Driver.WriteCI8Span      = NULL;
    ctx->Driver.WriteCI32Span     = NULL;
    ctx->Driver.WriteMonoCISpan   = NULL;
    ctx->Driver.WriteCI32Pixels   = NULL;
    ctx->Driver.WriteMonoCIPixels = NULL;
    ctx->Driver.ReadCI32Span      = NULL;
    ctx->Driver.ReadCI32Pixels    = NULL;
}
