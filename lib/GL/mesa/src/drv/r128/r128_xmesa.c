/* $XFree86$ */
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
 *
 */

#ifdef GLX_DIRECT_RENDERING

/* r128 Mesa driver includes */
#include "r128_init.h"
#include "r128_context.h"
#include "r128_xmesa.h"
#include "r128_state.h"
#include "r128_tex.h"
#include "r128_swap.h"

/* Mesa src includes */
#include "context.h"
#include "simple_list.h"
#include "mmath.h"

#ifndef R128_DEBUG_FLAGS
int R128_DEBUG_FLAGS = (0
/*			| DEBUG_ALWAYS_SYNC */
/*			| DEBUG_VERBOSE_CCE */
/*			| DEBUG_VERBOSE_OUTREG */
/*			| DEBUG_VERBOSE_MSG */
/*			| DEBUG_NO_OUTRING */
/*			| DEBUG_NO_OUTREG */
/*			| DEBUG_VERBOSE_API */
/*			| DEBUG_VERBOSE_2D */
/*			| DEBUG_VERBOSE_DRI */
/*			| DEBUG_VALIDATE_RING */
/*			| DEBUG_VERBOSE_IOCTL */
    );
#endif

#if DEBUG_LOCKING
char *prevLockFile = NULL;
int   prevLockLine = 0;
#endif

static r128ContextPtr r128Context = NULL;


/* Initialize the driver specific screen private data */
GLboolean XMesaInitDriver(__DRIscreenPrivate *sPriv)
{
    sPriv->private = (void *)r128CreateScreen(sPriv);
    if (!sPriv->private) {
	r128DestroyScreen(sPriv);
	return GL_FALSE;
    }

    return GL_TRUE;
}

/* Reset the driver specific screen private data */
void XMesaResetDriver(__DRIscreenPrivate *sPriv)
{
    r128DestroyScreen(sPriv);
}

/* Create and initialize the Mesa and driver specific visual data */
GLvisual *XMesaCreateVisual(Display *dpy,
			    __DRIscreenPrivate *driScrnPriv,
			    const XVisualInfo *visinfo,
			    const __GLXvisualConfig *config)
{
    /* Drivers may change the args to _mesa_create_visual() in order to
     * setup special visuals.
     */
    return _mesa_create_visual(config->rgba,
			       config->doubleBuffer,
			       config->stereo,
			       _mesa_bitcount(visinfo->red_mask),
			       _mesa_bitcount(visinfo->green_mask),
			       _mesa_bitcount(visinfo->blue_mask),
			       config->alphaSize,
			       0, /* index bits */
			       config->depthSize,
			       config->stencilSize,
			       config->accumRedSize,
			       config->accumGreenSize,
			       config->accumBlueSize,
			       config->accumAlphaSize,
			       0 /* num samples */);
}

/* Create and initialize the Mesa and driver specific context data */
GLboolean XMesaCreateContext(Display *dpy, GLvisual *mesaVis,
			     __DRIcontextPrivate *driContextPriv)
{
    return r128CreateContext(dpy, mesaVis, driContextPriv);
}

/* Destroy the Mesa and driver specific context data */
void XMesaDestroyContext(__DRIcontextPrivate *driContextPriv)
{
    r128ContextPtr r128ctx = (r128ContextPtr)driContextPriv->driverPrivate;

    if (r128ctx == (void *)r128Context) r128Context = NULL;
    r128DestroyContext(r128ctx);
}

/* Create and initialize the Mesa and driver specific pixmap buffer data */
GLframebuffer *XMesaCreateWindowBuffer(Display *dpy,
				       __DRIscreenPrivate *driScrnPriv,
				       __DRIdrawablePrivate *driDrawPriv,
				       GLvisual *mesaVis)
{
   return gl_create_framebuffer(mesaVis,
                                GL_FALSE,  /* software depth buffer? */
                                mesaVis->StencilBits > 0,
                                mesaVis->AccumRedBits > 0,
                                mesaVis->AlphaBits > 0
                                );
}

/* Create and initialize the Mesa and driver specific pixmap buffer data */
GLframebuffer *XMesaCreatePixmapBuffer( Display *dpy,
                                        __DRIscreenPrivate *driScrnPriv,
                                        __DRIdrawablePrivate *driDrawPriv,
                                        GLvisual *mesaVis)
{
#if 0
    /* Different drivers may have different combinations of hardware and
     * software ancillary buffers.
     */
    return gl_create_framebuffer(mesaVis,
				 GL_FALSE,  /* software depth buffer? */
				 mesaVis->StencilBits > 0,
				 mesaVis->AccumRedBits > 0,
				 mesaVis->AlphaBits > 0);
#else
    return NULL;  /* not implemented yet */
#endif
}

/* Copy the back color buffer to the front color buffer */
void XMesaSwapBuffers(__DRIdrawablePrivate *driDrawPriv)
{
    /* FIXME: This assumes buffer is currently bound to a context.  This
       needs to be able to swap buffers when not currently bound.  Also,
       this needs to swap according to buffer, and NOT according to
       context! */
    if (r128Context == NULL) return;

    /* Only swap buffers when a back buffer exists */
    if (R128_MESACTX(r128Context)->Visual->DBflag) {
	FLUSH_VB(R128_MESACTX(r128Context), "swap buffers");
	r128SwapBuffers(r128Context);
    }
}

/* Force the context `c' to be the current context and associate with it
   buffer `b' */
GLboolean XMesaMakeCurrent(__DRIcontextPrivate *driContextPriv,
			   __DRIdrawablePrivate *driDrawPriv,
			   __DRIdrawablePrivate *driReadPriv)
{
    if (driContextPriv) {
	r128ContextPtr r128ctx = (r128ContextPtr)driContextPriv->driverPrivate;

	if (r128Context &&
	    r128ctx == (void *)r128Context &&
	    driDrawPriv == R128_DRIDRAWABLE(r128Context))
	    return GL_TRUE;

	r128Context = r128MakeCurrent(r128Context, r128ctx, driDrawPriv);

	gl_make_current2(R128_MESACTX(r128Context),
			 driDrawPriv->mesaBuffer, driReadPriv->mesaBuffer);

	if (!R128_MESACTX(r128Context)->Viewport.Width) {
	    gl_Viewport(R128_MESACTX(r128Context), 0, 0,
			driDrawPriv->w, driDrawPriv->h);
	}
    } else {
	gl_make_current(0,0);
	r128Context = NULL;
    }

    return GL_TRUE;
}

/* Force the context `c' to be unbound from its buffer */
GLboolean XMesaUnbindContext(__DRIcontextPrivate *driContextPriv)
{
    return GL_TRUE;
}

/* Update the hardware state.  This is called if another context has
   grabbed the hardware lock, which includes the X server.  This
   function also updates the driver's window state after the X server
   moves, resizes or restacks a window -- the change will be reflected
   in the drawable position and clip rects.  Since the X server grabs
   the hardware lock when it changes the window state, this routine will
   automatically be called after such a change. */
/* NOTE: This routine is only called while holding the hardware lock. */
void XMesaUpdateState(__DRIcontextPrivate *driContextPriv)
{
    r128ContextPtr        r128ctx = driContextPriv->driverPrivate;
    __DRIscreenPrivate   *sPriv   = R128_DRISCREEN(r128ctx);
    __DRIdrawablePrivate *dPriv   = R128_DRIDRAWABLE(r128ctx);
    int                   stamp   = dPriv->lastStamp;

    /* The window might have moved, so we might need to get new clip
       rects.

       NOTE: This releases and regrabs the hw lock to allow the X server
       to respond to the DRI protocol request for new drawable info.
       Since the hardware state depends on having the latest drawable
       clip rects, all state checking must be done _after_ this call. */
    XMESA_VALIDATE_DRAWABLE_INFO(r128ctx->display, sPriv, dPriv);

    r128UpdateState(r128ctx, (stamp != dPriv->lastStamp));
}

/* This function is called by libGL.so as soon as libGL.so is loaded.
 * This is where we'd register new extension functions with the dispatcher.
 */
void __driRegisterExtensions(void)
{
}

#endif
