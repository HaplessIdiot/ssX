
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *
 * Header: /p0/cvs/X39-3D/xc/programs/Xserver/GL/mesa/src/X/xf86glx.c,v 1.2 1999/02/26 08:52:46 martin Exp $
 */

#include <miscstruct.h>
#include <resource.h>
#include <GL/gl.h>
#include <GL/glxint.h>

#include <scrnintstr.h>
#include <config.h>
#include <glxserver.h>
#include <glxscreens.h>
#include <glxdrawable.h>
#include <glxcontext.h>
#include <glxext.h>
#include <glxutil.h>
#include "xf86glxint.h"
#include "xmesaP.h"
#include <GL/xf86glx.h>

/*
 * This define is for the glcore.h header file.
 * If you comment it out, then make sure you also comment it out
 * in ../../../decode/Imakefile.
 */
#define DEBUG
#include <GL/internal/glcore.h>
#undef DEBUG


/*
 * This structure is statically allocated in the __glXScreens[]
 * structure.  This struct is not used anywhere other than in
 * __glXScreenInit to initialize each of the active screens
 * (__glXActiveScreens[]).  Several of the fields must be initialized by
 * the screenProbe routine before they are copied to the active screens
 * struct.  In particular, the contextCreate, pGlxVisual, numVisuals,
 * and numUsableVisuals fields must be initialized.
 */
__GLXscreenInfo __glDDXScreenInfo = {
    __MESA_screenProbe,   /* Must be generic and handle all screens */
    __MESA_createContext, /* Substitute screen's createContext routine */
    __MESA_createBuffer,  /* Substitute screen's createBuffer routine */
    NULL,                 /* Set up pGlxVisual in probe */
    0,                    /* Set up numVisuals in probe */
    0,                    /* Set up numUsableVisuals in probe */
    "Vendor String",      /* GLXvendor is overwritten by __glXScreenInit */
    "Version String",     /* GLXversion is overwritten by __glXScreenInit */
    "Extensions String",  /* GLXextensions is overwritten by __glXScreenInit */
    NULL                  /* WrappedPositionWindow is overwritten */
};

__GLXextensionInfo __glDDXExtensionInfo = {
    GL_CORE_MESA,
    __MESA_resetExtension,
    __MESA_initVisuals
};

__MESA_visual  MESAVisualPtr = NULL;
int            MESANumVisuals = 0;
__GLcontext   *MESA_CC = NULL;

static int count_bits(unsigned int n)
{
   int bits = 0;

   while (n > 0) {
      if (n & 1) bits++;
      n >>= 1;
   }
   return bits;
}

static XMesaVisual find_mesa_visual(VisualID vid)
{
    XMesaVisual xm_vis = NULL;
    int i;

    for (i = 0; i < MESANumVisuals; i++) {
	if (MESAVisualPtr[i].glx_vis->vid == vid) {
	    break;
	}
    }

    /* This should always be true */
    if (i < MESANumVisuals) {
	xm_vis = MESAVisualPtr[i].xm_vis;
    }
    return xm_vis;
}

static int num_visuals(int numVisuals, VisualPtr pVisual)
{
    int num_visuals = 0;
    int i, n;

    for (i = 0; i < numVisuals; i++) {
	/*
	 * 2*1*2*2 = 8 different visuals
	 *     
	 * 2 for double buffer
	 * 1 for stereo
	 * 2 for depth
	 * 2 for stencil
	 */
	n = 8;

	if (pVisual[i].class == TrueColor ||
	    pVisual[i].class == DirectColor) {
	    n *= 4; /* 2 for alpha and 2 for accum */
	}
	num_visuals += n;
    }

    return num_visuals;
}

static __GLXvisualConfig *init_visuals(int numMesaVisuals,
				       int *nvisualp, VisualPtr *visualp,
				       VisualID *defaultVisp,
				       int ndepth, DepthPtr pdepth,
				       int rootDepth)
{
    int numVisuals = *nvisualp;
    VisualPtr pVisual = *visualp;
    VisualPtr pVisualNew = NULL;
    VisualID *orig_vid = NULL;
    __GLXvisualConfig *glXVisualPtr = NULL;
    int found_default = FALSE;
    int i, j;
    int is_rgb;
    int db, stereo, alpha, depth, stencil, accum;
    int max_db, max_stereo, max_alpha, max_depth, max_stencil, max_accum;

    orig_vid = (VisualID *)__glXMalloc(numMesaVisuals * sizeof(VisualID));
    if (!orig_vid)
	return NULL;

    glXVisualPtr = (__GLXvisualConfig *)__glXMalloc(numMesaVisuals *
						    sizeof(__GLXvisualConfig));
    if (!glXVisualPtr) {
	__glXFree(orig_vid);
	return NULL;
    }

    MESAVisualPtr = (__MESA_visual)__glXMalloc(numMesaVisuals *
					       sizeof(struct __MESA_visualRec));
    if (!MESAVisualPtr) {
	__glXFree(orig_vid);
	__glXFree(glXVisualPtr);
	return NULL;
    }

    pVisualNew = (VisualPtr)__glXMalloc(numMesaVisuals * sizeof(VisualRec));
    if (!pVisualNew) {
	__glXFree(orig_vid);
	__glXFree(MESAVisualPtr);
	MESAVisualPtr = NULL;
	__glXFree(glXVisualPtr);
	return NULL;
    }

    /* Initialize the new visuals */
    for (i = j = 0; i < numVisuals; i++) {
	switch (pVisual[i].class) {
	case StaticGray:
	case GrayScale:
	case StaticColor:
	case PseudoColor:
	    is_rgb = GL_FALSE;
	    max_db = 1;
	    max_stereo = 0;
	    max_alpha = 0;
	    max_depth = 1;
	    max_stencil = 1;
	    max_accum = 0;
	    break;
	case TrueColor:
	case DirectColor:
	    is_rgb = GL_TRUE;
	    max_db = 1;
	    max_stereo = 0;
	    max_alpha = 1;
	    max_depth = 1;
	    max_stencil = 1;
	    max_accum = 1;
	    break;
	default:
	    is_rgb = GL_TRUE;
	    max_db = max_stereo = max_alpha = max_depth = max_stencil = 
		max_accum = 0;
	    break;
	}

	for (db = max_db; db >= 0; db--) {
	    for (stereo = max_stereo; stereo >= 0; stereo--) {
		for (alpha = max_alpha; alpha >= 0; alpha--) {
		    for (depth = max_depth; depth >= 0; depth--) {
			for (stencil = max_stencil; stencil >= 0; stencil--) {
			    for (accum = max_accum; accum >= 0; accum--) {
				/* Initialize the new visual */
				pVisualNew[j] = pVisual[i];
				pVisualNew[j].vid = FakeClientID(0);

				/* Check for the default visual */
				if (!found_default &&
				    pVisual[i].vid == *defaultVisp) {
				    *defaultVisp = pVisualNew[j].vid;
				    found_default = TRUE;
				}

				/* Save the old visual ID */
				orig_vid[j] = pVisual[i].vid;

				/* Initialize the glXVisual */
				glXVisualPtr[j].vid = pVisualNew[j].vid;
				glXVisualPtr[j].class = pVisual[i].class;
				glXVisualPtr[j].rgba = is_rgb;
				glXVisualPtr[j].redSize =
				    count_bits(pVisual[i].redMask);
				glXVisualPtr[j].greenSize =
				    count_bits(pVisual[i].greenMask);
				glXVisualPtr[j].blueSize =
				    count_bits(pVisual[i].blueMask);
				glXVisualPtr[j].alphaSize = 0;
				glXVisualPtr[j].redMask = pVisual[i].redMask;
				glXVisualPtr[j].greenMask =
				    pVisual[i].greenMask;
				glXVisualPtr[j].blueMask = pVisual[i].blueMask;
				glXVisualPtr[j].alphaMask = 0;
				glXVisualPtr[j].accumRedSize =
				    accum * ACCUM_BITS;
				glXVisualPtr[j].accumGreenSize =
				    accum * ACCUM_BITS;
				glXVisualPtr[j].accumBlueSize =
				    accum * ACCUM_BITS;
				glXVisualPtr[j].accumAlphaSize =
				    accum * ACCUM_BITS;
				glXVisualPtr[j].doubleBuffer = db;
				glXVisualPtr[j].stereo = GL_FALSE;
				glXVisualPtr[j].bufferSize = rootDepth;
				glXVisualPtr[j].depthSize = depth * DEPTH_BITS;
				glXVisualPtr[j].stencilSize =
				    stencil * STENCIL_BITS;
				glXVisualPtr[j].auxBuffers = 0;
				glXVisualPtr[j].level = 0;
				glXVisualPtr[j].visualRating = 0;
				glXVisualPtr[j].transparentPixel = 0;
				glXVisualPtr[j].transparentRed = 0;
				glXVisualPtr[j].transparentGreen = 0;
				glXVisualPtr[j].transparentBlue = 0;
				glXVisualPtr[j].transparentAlpha = 0;
				glXVisualPtr[j].transparentIndex = 0;

				/* Create the Mesa visual */
				MESAVisualPtr[j].glx_vis = &glXVisualPtr[j];
				MESAVisualPtr[j].xm_vis =
				    XMesaCreateVisual(NULL,
						      &pVisualNew[j],
						      is_rgb,
						      alpha,
						      db,
						      stereo,
						      GL_TRUE, /*ximage_flag*/
						      depth * DEPTH_BITS,
						      stencil * STENCIL_BITS,
						      accum * ACCUM_BITS,
						      0);

				j++;
			    }
			}
		    }
		}
	    }
	}
    }

    /* Set up depth's visual IDs */
    for (i = 0; i < ndepth; i++) {
	int numVids = 0;
	VisualID *pVids = NULL;
	int k, n = 0;

	/* Count the new number of visual IDs at this depth */
	for (j = 0; j < pdepth[i].numVids; j++)
	    for (k = 0; k < numMesaVisuals; k++)
		if (pdepth[i].vids[j] == orig_vid[k])
		    numVids++;

	/* Allocate a new list of visual IDs for this depth */
	pVids = (VisualID *)__glXMalloc(numVids * sizeof(VisualID));

	/* Initialize the new list of visual IDs for this depth */
	for (j = 0; j < pdepth[i].numVids; j++)
	    for (k = 0; k < numMesaVisuals; k++)
		if (pdepth[i].vids[j] == orig_vid[k])
		    pVids[n++] = pVisualNew[k].vid;

	/* Update this depth's list of visual IDs */
	__glXFree(pdepth[i].vids);
	pdepth[i].vids = pVids;
	pdepth[i].numVids = numVids;
    }

    /* Update the X server's visuals */
    *nvisualp = numMesaVisuals;
    *visualp = pVisualNew;
    __glXFree(pVisual);

    /* Clean up temporary allocations */
    __glXFree(orig_vid);

    return glXVisualPtr;
}

Bool __MESA_initVisuals(VisualPtr *visualp, DepthPtr *depthp,
			int *nvisualp, int *ndepthp, int *rootDepthp,
			VisualID *defaultVisp, unsigned long sizes,
			int bitsPerRGB)
{
    /*
     * Setup the visuals supported by this particular screen.
     */
    __glDDXScreenInfo.numVisuals =
	__glDDXScreenInfo.numUsableVisuals =
	MESANumVisuals = num_visuals(*nvisualp, *visualp);
    __glDDXScreenInfo.pGlxVisual = init_visuals(MESANumVisuals,
						nvisualp, visualp,
						defaultVisp,
						*ndepthp, *depthp,
						*rootDepthp);

    return TRUE;
}

static void fixup_visuals(int screen)
{
    ScreenPtr pScreen = screenInfo.screens[screen];
    int i;

    for (i = 0; i < MESANumVisuals; i++) {
	if (MESAVisualPtr[i].glx_vis->rgba) {
	    /* Fixup the masks */
	    MESAVisualPtr[i].glx_vis->redMask =
		GET_REDMASK(MESAVisualPtr[i].xm_vis);
	    MESAVisualPtr[i].glx_vis->greenMask =
		GET_GREENMASK(MESAVisualPtr[i].xm_vis);
	    MESAVisualPtr[i].glx_vis->blueMask =
		GET_BLUEMASK(MESAVisualPtr[i].xm_vis);

	    /* Recalc the sizes */
	    MESAVisualPtr[i].glx_vis->redSize =
		count_bits(MESAVisualPtr[i].glx_vis->redMask);
	    MESAVisualPtr[i].glx_vis->blueSize =
		count_bits(MESAVisualPtr[i].glx_vis->blueMask);
	    MESAVisualPtr[i].glx_vis->greenSize =
		count_bits(MESAVisualPtr[i].glx_vis->greenMask);
	}

	/* Set the "display" in the context to pScreen */
	XMesaSetVisualDisplay(pScreen, MESAVisualPtr[i].xm_vis);
    }
}

Bool __MESA_screenProbe(int screen)
{
    /*
     * Update the current screen's createContext routine.  This could be
     * wrapped by a DDX GLX context creation routine.
     */
    __glDDXScreenInfo.createContext = __MESA_createContext;

    /*
     * The ordering of the rgb compenents might have been changed by the
     * driver after mi initialized them.  This could be called from the
     * 2D/3D driver during initialization.  Also, the ScreenPtr needs to
     * be set for each Mesa visual.
     */
    fixup_visuals(screen);

    return TRUE;
}

extern void __MESA_resetExtension(void)
{
    XMesaReset();

    __glXFree(MESAVisualPtr);
    MESAVisualPtr = NULL;
    MESANumVisuals = 0;
    MESA_CC = NULL;
}

void __MESA_createBuffer(__GLXdrawablePrivate *glxPriv)
{
    DrawablePtr pDraw = glxPriv->pDraw;
    XMesaVisual xm_vis = find_mesa_visual(glxPriv->pGlxVisual->vid);
    __GLdrawablePrivate *glPriv = &glxPriv->glPriv;
    __MESA_buffer buf;

    buf = (__MESA_buffer)__glXMalloc(sizeof(struct __MESA_bufferRec));

    /* Create Mesa's buffers */
    if (glxPriv->type == DRAWABLE_WINDOW) {
	buf->xm_buf = (void *)XMesaCreateWindowBuffer(xm_vis,
						      (WindowPtr)pDraw);
    } else {
	buf->xm_buf = (void *)XMesaCreatePixmapBuffer(xm_vis,
						      (PixmapPtr)pDraw, 0);
    }

    /* Wrap the front buffer's resize routine */
    buf->fbresize = glPriv->frontBuffer.resize;
    glPriv->frontBuffer.resize = __MESA_resizeBuffers;

    /* Wrap the swap buffers routine */
    buf->fbswap = glxPriv->swapBuffers;
    glxPriv->swapBuffers = __MESA_swapBuffers;

    /* Save Mesa's private buffer structure */
    glPriv->private = (void *)buf;
    glPriv->freePrivate = __MESA_destroyBuffer;
}

GLboolean __MESA_resizeBuffers(__GLdrawableBuffer *buffer,
			       GLint x, GLint y,
			       GLuint width, GLuint height, 
			       __GLdrawablePrivate *glPriv,
			       GLuint bufferMask)
{
    __MESA_buffer buf = (__MESA_buffer)glPriv->private;

    if (buf->xm_buf && buf->xm_buf->xm_context) {
	XMesaForceCurrent(buf->xm_buf->xm_context);
	(*buf->xm_buf->xm_context->gl_ctx->API.ResizeBuffersMESA)(buf->xm_buf->xm_context->gl_ctx);
	XMesaForceCurrent(MESA_CC->xm_ctx);
    }

    return (*buf->fbresize)(buffer, x, y, width, height, glPriv, bufferMask);
}

GLboolean __MESA_swapBuffers(__GLXdrawablePrivate *glxPriv)
{
    __MESA_buffer buf = (__MESA_buffer)glxPriv->glPriv.private;

    /*
    ** Do not call the wrapped swap buffers routine since Mesa has
    ** already done the swap.
    */
    XMesaSwapBuffers(buf->xm_buf);

    return GL_TRUE;
}

void __MESA_destroyBuffer(__GLdrawablePrivate *glPriv)
{
    __MESA_buffer buf = (__MESA_buffer)glPriv->private;
    __GLXdrawablePrivate *glxPriv = (__GLXdrawablePrivate *)glPriv->other;

    /* Destroy Mesa's buffers */
    XMesaDestroyBuffer(buf->xm_buf);

    /* Unwrap these routines */
    glxPriv->swapBuffers = buf->fbswap;
    glPriv->frontBuffer.resize = buf->fbresize;

    __glXFree(glPriv->private);
    glPriv->private = NULL;
}

__GLinterface *__MESA_createContext(__GLimports *imports,
				    __GLcontextModes *modes,
				    __GLinterface *shareGC,
				    __GLXvisualConfig *pGlxVisual)
{
    __GLcontext *gl_ctx;
    XMesaContext m_share = NULL;
    XMesaVisual xm_vis;

    gl_ctx = (__GLcontext *)__glXMalloc(sizeof(__GLcontext));
    if (!gl_ctx)
	return NULL;

    gl_ctx->iface.imports = *imports;

    gl_ctx->iface.exports.destroyContext = __MESA_destroyContext;
    gl_ctx->iface.exports.loseCurrent = __MESA_loseCurrent;
    gl_ctx->iface.exports.makeCurrent = __MESA_makeCurrent;
    gl_ctx->iface.exports.shareContext = __MESA_shareContext;
    gl_ctx->iface.exports.copyContext = __MESA_copyContext;
    gl_ctx->iface.exports.forceCurrent = __MESA_forceCurrent;
    gl_ctx->iface.exports.notifyResize = __MESA_notifyResize;
    gl_ctx->iface.exports.notifyDestroy = __MESA_notifyDestroy;
    gl_ctx->iface.exports.notifySwapBuffers = __MESA_notifySwapBuffers;
    gl_ctx->iface.exports.dispatchExec = __MESA_dispatchExec;
    gl_ctx->iface.exports.beginDispatchOverride = __MESA_beginDispatchOverride;
    gl_ctx->iface.exports.endDispatchOverride = __MESA_endDispatchOverride;

    if (shareGC) m_share = ((__GLcontext *)shareGC)->xm_ctx;
    xm_vis = find_mesa_visual(pGlxVisual->vid);
    if (xm_vis) {
	gl_ctx->xm_ctx = XMesaCreateContext(xm_vis, m_share);
    } else {
	__glXFree(gl_ctx);
	gl_ctx = NULL;
    }

    return (__GLinterface *)gl_ctx;
}

GLboolean __MESA_destroyContext(__GLcontext *gc)
{
    XMesaDestroyContext(gc->xm_ctx);
    __glXFree(gc);
    return GL_TRUE;
}

GLboolean __MESA_loseCurrent(__GLcontext *gc)
{
    MESA_CC = NULL;
    __glXLastContext = NULL;
    return XMesaLoseCurrent(gc->xm_ctx);
}

GLboolean __MESA_makeCurrent(__GLcontext *gc, __GLdrawablePrivate *glPriv)
{
    __MESA_buffer buf = (__MESA_buffer)glPriv->private;

    MESA_CC = gc;
    return XMesaMakeCurrent(gc->xm_ctx, buf->xm_buf);
}

GLboolean __MESA_shareContext(__GLcontext *gc, __GLcontext *gcShare)
{
    /* NOT_DONE */
    ErrorF("__MESA_shareContext\n");
    return GL_FALSE;
}

GLboolean __MESA_copyContext(__GLcontext *dst, const __GLcontext *src,
			     GLuint mask)
{
    /* NOT_DONE */
    ErrorF("__MESA_copyContext\n");
    return GL_FALSE;
}

GLboolean __MESA_forceCurrent(__GLcontext *gc)
{
    MESA_CC = gc;
    return XMesaForceCurrent(gc->xm_ctx);
}

GLboolean __MESA_notifyResize(__GLcontext *gc)
{
    /* NOT_DONE */
    ErrorF("__MESA_notifyResize\n");
    return GL_FALSE;
}

void __MESA_notifyDestroy(__GLcontext *gc)
{
    /* NOT_DONE */
    ErrorF("__MESA_notifyDestroy\n");
    return;
}

void __MESA_notifySwapBuffers(__GLcontext *gc)
{
    /* NOT_DONE */
    ErrorF("__MESA_notifySwapBuffers\n");
    return;
}

struct __GLdispatchStateRec *__MESA_dispatchExec(__GLcontext *gc)
{
    /* NOT_DONE */
    ErrorF("__MESA_dispatchExec\n");
    return NULL;
}

void __MESA_beginDispatchOverride(__GLcontext *gc)
{
    /* NOT_DONE */
    ErrorF("__MESA_beginDispatchOverride\n");
    return;
}

void __MESA_endDispatchOverride(__GLcontext *gc)
{
    /* NOT_DONE */
    ErrorF("__MESA_endDispatchOverride\n");
    return;
}

GLint __glEvalComputeK(GLenum target)
{
    switch (target) {
    case GL_MAP1_VERTEX_4:
    case GL_MAP1_COLOR_4:
    case GL_MAP1_TEXTURE_COORD_4:
    case GL_MAP2_VERTEX_4:
    case GL_MAP2_COLOR_4:
    case GL_MAP2_TEXTURE_COORD_4:
	return 4;
    case GL_MAP1_VERTEX_3:
    case GL_MAP1_TEXTURE_COORD_3:
    case GL_MAP1_NORMAL:
    case GL_MAP2_VERTEX_3:
    case GL_MAP2_TEXTURE_COORD_3:
    case GL_MAP2_NORMAL:
	return 3;
    case GL_MAP1_TEXTURE_COORD_2:
    case GL_MAP2_TEXTURE_COORD_2:
	return 2;
    case GL_MAP1_TEXTURE_COORD_1:
    case GL_MAP2_TEXTURE_COORD_1:
    case GL_MAP1_INDEX:
    case GL_MAP2_INDEX:
	return 1;
    default:
	return 0;
    }
}

GLuint __glFloorLog2(GLuint val)
{
    int c = 0;

    while (val > 1) {
	c++;
	val >>= 1;
    }
    return c;
}

