/* $XFree86: xc/lib/GL/mesa/dri/dri_mesa.c,v 1.17 2001/08/27 17:40:57 dawes Exp $ */
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright (c) 2002 Apple Computer, Inc.
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
 *   Brian E. Paul <brian@precisioninsight.com>
 */

/*
 * This file gets compiled into each of the DRI 3D drivers.  The
 * functions defined here are called from the GL library via
 * function pointers in the __DRIdisplayRec, __DRIscreenRec,
 * __DRIcontextRec, __DRIdrawableRec structures defined in glxclient.h
 *
 * Those function pointers are initialized by code in this file.
 * The process starts when libGL calls the __driCreateScreen() function
 * at the end of this file.
 *
 * The above-mentioned DRI structures have no dependencies on Mesa.
 * Each structure instead has a generic (void *) private pointer that
 * points to a private structure.  For Mesa drivers, these private
 * structures are the __DRIdrawablePrivateRec, __DRIcontextPrivateRec,
 * __DRIscreenPrivateRec, and __DRIvisualPrivateRec structures defined
 * in dri_mesaint.h.  We allocate and attach those structs here in
 * this file.
 */


#ifdef GLX_DIRECT_RENDERING

#include <unistd.h>
#include <X11/Xlibint.h>
#include <X11/extensions/Xext.h>
#include "extutil.h"
#include "glxclient.h"
#include "appledri.h"
#include "dri_driver.h"
#include "x-list.h"
#include "x-hash.h"

/* Context binding */
static Bool driMesaBindContext(Display *dpy, int scrn,
			       GLXDrawable draw, GLXContext gc);
static Bool driMesaUnbindContext(Display *dpy, int scrn,
				 GLXDrawable draw, GLXContext gc,
				 int will_rebind);

/* Drawable methods */
static void *driMesaCreateDrawable(Display *dpy, int scrn, GLXDrawable draw,
				   VisualID vid, __DRIdrawable *pdraw);
static __DRIdrawable *driMesaGetDrawable(Display *dpy, GLXDrawable draw,
					 void *screenPrivate);
static void driMesaSwapBuffers(Display *dpy, void *drawPrivate);
static void driMesaDestroyDrawable(Display *dpy, void *drawPrivate);

/* Context methods */
static void *driMesaCreateContext(Display *dpy, XVisualInfo *vis, void *shared,
				  __DRIcontext *pctx);
static void driMesaDestroyContext(Display *dpy, int scrn, void *screenPrivate);

/* Screen methods */
static void *driMesaCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
				 int numConfigs, __GLXvisualConfig *config);
static void driMesaDestroyScreen(Display *dpy, int scrn, void *screenPrivate);

static void driMesaCreateSurface(Display *dpy, int scrn,
				 __DRIdrawablePrivate *pdp);

extern const CGLContextObj XAppleDRIGetIndirectContext(void);

/*****************************************************************/

/* Maintain a list of drawables */

static inline Bool
__driMesaAddDrawable(x_hash_table *drawHash, __DRIdrawable *pdraw)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;

    assert(drawHash != NULL);

    x_hash_table_insert(drawHash, (void *) pdp->draw, pdraw);

    return GL_TRUE;
}

static inline __DRIdrawable *
__driMesaFindDrawable(x_hash_table *drawHash, GLXDrawable draw)
{
    if (drawHash == NULL)
	return NULL;

    return x_hash_table_lookup(drawHash, (void *) draw, NULL);
}

struct find_by_uid_closure {
    unsigned int uid;
    __DRIdrawable *ret;
};

static void
find_by_uid_cb(void *k, void *v, void *data)
{
    __DRIdrawable *pdraw = v;
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;
    struct find_by_uid_closure *c = data;

    if (pdp->uid == c->uid)
	c->ret = pdraw;
}

static __DRIdrawable *
__driMesaFindDrawableByUID(x_hash_table *drawHash, unsigned int uid)
{
    struct find_by_uid_closure c;

    c.uid = uid;
    c.ret = NULL;
    x_hash_table_foreach(drawHash, find_by_uid_cb, &c);

    return c.ret;
}

static inline void
__driMesaRemoveDrawable(x_hash_table *drawHash, __DRIdrawable *pdraw)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;

    if (drawHash == NULL)
	return;

    x_hash_table_remove(drawHash, (void *) pdp->draw);
}

static Bool __driMesaWindowExistsFlag;

static int __driMesaWindowExistsErrorHandler(Display *dpy, XErrorEvent *xerr)
{
    if (xerr->error_code == BadWindow) {
	__driMesaWindowExistsFlag = GL_FALSE;
    }
    return 0;
}

static Bool __driMesaWindowExists(Display *dpy, GLXDrawable draw)
{
    XWindowAttributes xwa;
    int (*oldXErrorHandler)(Display *, XErrorEvent *);

    __driMesaWindowExistsFlag = GL_TRUE;
    oldXErrorHandler = XSetErrorHandler(__driMesaWindowExistsErrorHandler);
    XGetWindowAttributes(dpy, draw, &xwa); /* dummy request */
    XSetErrorHandler(oldXErrorHandler);
    return __driMesaWindowExistsFlag;
}

static void __driMesaCollectCallback(void *k, void *v, void *data)
{
    GLXDrawable draw = (GLXDrawable) k;
    __DRIdrawable *pdraw = v;
    x_list **todelete = data;

    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;
    Display *dpy;

    dpy = pdp->driScreenPriv->display;
    XSync(dpy, GL_FALSE);
    if (!pdp->destroyed && !__driMesaWindowExists(dpy, draw)) {
	/* Destroy the local drawable data in the hash table, if the
	   drawable no longer exists in the Xserver */
	pdp->destroyed = TRUE;
	*todelete = x_list_prepend(*todelete, pdraw);
    }
}

static void __driMesaGarbageCollectDrawables(void *drawHash)
{
    __DRIdrawable *pdraw;
    __DRIdrawablePrivate *pdp;
    Display *dpy;
    x_list *todelete = NULL, *node;

    x_hash_table_foreach(drawHash, __driMesaCollectCallback, &todelete);

    for (node = todelete; node != NULL; node = node->next)
    {
	pdraw = node->data;
	pdp = (__DRIdrawablePrivate *)pdraw->private;
	dpy = pdp->driScreenPriv->display;

	/* Destroy the local drawable data in the hash table, if the
	   drawable no longer exists in the Xserver */

	__driMesaRemoveDrawable(drawHash, pdraw);
	(*pdraw->destroyDrawable)(dpy, pdraw->private);
	Xfree(pdraw);
    }

    x_list_free(todelete);
}

/*****************************************************************/

static Bool
driMesaFindDrawableByUID(Display *dpy,unsigned int uid,
			  __DRIscreenPrivate **psp_ret,
			  __DRIdrawablePrivate **pdp_ret)
{
    __DRIscreen *pDRIScreen;
    __DRIscreenPrivate *psp;
    __DRIdrawable *pdraw;
    int scrn;

    for (scrn = 0; scrn < ScreenCount(dpy); scrn++)
    {
	if (!(pDRIScreen = __glXFindDRIScreen(dpy, scrn))) {
	    /* ERROR!!! */
	    return FALSE;
	} else if (!(psp = (__DRIscreenPrivate *)pDRIScreen->private)) {
	    /* ERROR!!! */
	    return FALSE;
	}

	pdraw = __driMesaFindDrawableByUID(psp->drawHash, uid);
	if (pdraw != NULL) {
	    *psp_ret = psp;
	    *pdp_ret = pdraw->private;
	    return TRUE;
	};
    }

    return FALSE;
}

static void
unbind_context(__DRIcontextPrivate *pcp)
{
    /* Unbind the context from its old drawable. */

    if (pcp->driDrawablePriv != NULL)
    {
	if (pcp->next != NULL)
	    pcp->next->prev = pcp->prev;
	if (pcp->prev != NULL)
	    pcp->prev->next = pcp->next;

	if (pcp->driDrawablePriv->driContextPriv == pcp)
	    pcp->driDrawablePriv->driContextPriv = pcp->next;

	pcp->driDrawablePriv = NULL;
	pcp->prev = pcp->next = NULL;
    }
}

static void
unbind_drawable(__DRIdrawablePrivate *pdp)
{
    __DRIcontextPrivate *pcp, *next;

    for (pcp = pdp->driContextPriv; pcp != NULL; pcp = next)
    {
	next = pcp->next;

	if (pcp->surface_id != 0)
	{
	    CGLClearDrawable(pcp->ctx);
	    pcp->surface_id = 0;
	}

	unbind_context(pcp);
    }
}

static Bool driMesaUnbindContext(Display *dpy, int scrn,
				 GLXDrawable draw, GLXContext gc,
				 int will_rebind)
{
    __DRIscreen *pDRIScreen;
    __DRIdrawable *pdraw;
    __DRIcontextPrivate *pcp;
    __DRIscreenPrivate *psp;
    __DRIdrawablePrivate *pdp;
    void *current_ctx;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driMesaUnbindContext.
    */

    if (gc == NULL || draw == None) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    if (!(pDRIScreen = __glXFindDRIScreen(dpy, scrn))) {
	/* ERROR!!! */
	return GL_FALSE;
    } else if (!(psp = (__DRIscreenPrivate *)pDRIScreen->private)) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pcp = (__DRIcontextPrivate *)gc->driContext.private;

    pdraw = __driMesaFindDrawable(psp->drawHash, draw);
    if (!pdraw) {
	/* ERROR!!! */
	return GL_FALSE;
    }
    pdp = (__DRIdrawablePrivate *)pdraw->private;

    /* Unbind Mesa's drawable from Mesa's context */
    current_ctx = CGLGetCurrentContext();
    if (current_ctx == pcp->ctx) {
	CGLSetCurrentContext(XAppleDRIGetIndirectContext());
    } else {
	fprintf(stderr, "GL: huh, unbinding non-context context\n");
    }

#if 0
    /* FIXME: I'm going to leave contexts attached to surfaces until
       they need to be reconnected elsewhere, may help performance when
       we subsequently rebind to the same surface? What happens when
       the refcount goes to zero? */
    if (!will_rebind && pcp->surface_id != 0)
    {
	CGLClearDrawable(pcp->ctx);
	pcp->surface_id = 0;
    }
#endif

    unbind_context(pcp);

    if (pdp->refcount == 0) {
	/* ERROR!!! */
	return GL_FALSE;
    } else if (--pdp->refcount == 0) {
#if 0
	/*
	** NOT_DONE: When a drawable is unbound from one direct
	** rendering context and then bound to another, we do not want
	** to destroy the drawable data structure each time only to
	** recreate it immediatly afterwards when binding to the next
	** context.  This also causes conflicts with caching of the
	** drawable stamp.
	**
	** In addition, we don't destroy the drawable here since Mesa
	** keeps private data internally (e.g., software accumulation
	** buffers) that should not be destroyed unless the client
	** explicitly requests that the window be destroyed.
	**
	** When GLX 1.3 is integrated, the create and destroy drawable
	** functions will have user level counterparts and the memory
	** will be able to be recovered.
	** 
	** Below is an example of what needs to go into the destroy
	** drawable routine to support GLX 1.3.
	*/
	__driMesaRemoveDrawable(psp->drawHash, pdraw);
	(*pdraw->destroyDrawable)(dpy, pdraw->private);
	Xfree(pdraw);
#endif
    }

    return GL_TRUE;
}

static Bool driMesaBindContext(Display *dpy, int scrn,
			       GLXDrawable draw, GLXContext gc)
{
    __DRIscreen *pDRIScreen;
    __DRIdrawable *pdraw;
    __DRIdrawablePrivate *pdp;
    __DRIscreenPrivate *psp;
    __DRIcontextPrivate *pcp;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driMesaBindContext.
    */

    if (gc == NULL || draw == None) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    if (!(pDRIScreen = __glXFindDRIScreen(dpy, scrn))) {
	/* ERROR!!! */
	return GL_FALSE;
    } else if (!(psp = (__DRIscreenPrivate *)pDRIScreen->private)) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pdraw = __driMesaFindDrawable(psp->drawHash, draw);
    if (!pdraw) {
	/* Allocate a new drawable */
	pdraw = (__DRIdrawable *)Xmalloc(sizeof(__DRIdrawable));
	if (!pdraw) {
	    /* ERROR!!! */
	    return GL_FALSE;
	}

	/* Create a new drawable */
	pdraw->private = driMesaCreateDrawable(dpy, scrn, draw, gc->vid,
					       pdraw);
	if (!pdraw->private) {
	    /* ERROR!!! */
	    Xfree(pdraw);
	    return GL_FALSE;
	}

	/* Add pdraw to drawable list */
	if (!__driMesaAddDrawable(psp->drawHash, pdraw)) {
	    /* ERROR!!! */
	    (*pdraw->destroyDrawable)(dpy, pdraw->private);
	    Xfree(pdraw);
	    return GL_FALSE;
	}
    }

    pdp = (__DRIdrawablePrivate *)pdraw->private;
    pcp = (__DRIcontextPrivate *)gc->driContext.private;

    if (pdp->surface_id == 0)
    {
	/* Surface got destroyed. Try to create a new one. */

	driMesaCreateSurface(dpy, scrn, pdp);
    }

    unbind_context(pcp);

    /* Bind the drawable to the context */
    pcp->driDrawablePriv = pdp;
    pcp->prev = NULL;
    pcp->next = pdp->driContextPriv;
    pdp->driContextPriv = pcp;
    pdp->refcount++;

    /* And the physical surface to the physical context */
    if (pcp->surface_id != pdp->surface_id)
    {
	pcp->surface_id = 0;
	if (pdp->surface_id == 0)
	    CGLClearDrawable(pcp->ctx);
	else if (xp_attach_gl_context(pcp->ctx, pdp->surface_id) == Success)
	    pcp->surface_id = pdp->surface_id;
	else
	    fprintf(stderr, "failed to bind to surface\n");
    }

    /* Call device-specific MakeCurrent */
    CGLSetCurrentContext(pcp->ctx);

    return GL_TRUE;
}

/*****************************************************************/

static xp_client_id
get_client_id(void)
{
    static xp_client_id id;

    if (id == 0)
    {
	if (xp_init(XP_IN_BACKGROUND) != Success
	    || xp_get_client_id(&id) != Success)
	{
	    return 0;
	}
    }

    return id;
}

static void driMesaCreateSurface(Display *dpy, int scrn,
				 __DRIdrawablePrivate *pdp)
{
    xp_client_id client_id;
    unsigned int key[2];

    pdp->surface_id = 0;
    pdp->uid = 0;

    client_id = get_client_id();
    if (client_id == 0)
	return;

    if (XAppleDRICreateSurface(dpy, scrn, pdp->draw,
                               client_id, key, &pdp->uid))
    {
	xp_import_surface(key, &pdp->surface_id);
    }
}

static void *driMesaCreateDrawable(Display *dpy, int scrn, GLXDrawable draw,
				   VisualID vid, __DRIdrawable *pdraw)
{
    __DRIscreen *pDRIScreen;
    __DRIscreenPrivate *psp;
    __DRIdrawablePrivate *pdp;

    pdp = (__DRIdrawablePrivate *)Xmalloc(sizeof(__DRIdrawablePrivate));
    if (!pdp) {
	return NULL;
    }

    pdp->draw = draw;
    pdp->refcount = 0;
    pdp->surface_id = 0;
    pdp->uid = 0;
    pdp->destroyed = FALSE;

    if (!(pDRIScreen = __glXFindDRIScreen(dpy, scrn))) {
	Xfree(pdp);
	return NULL;
    } else if (!(psp = (__DRIscreenPrivate *)pDRIScreen->private)) {
	Xfree(pdp);
	return NULL;
    }
    pdp->driScreenPriv = psp;
    pdp->driContextPriv = NULL;

    driMesaCreateSurface(dpy, scrn, pdp);
    if (pdp->surface_id == 0) {
	Xfree(pdp);
	return NULL;
    }

    pdraw->destroyDrawable = driMesaDestroyDrawable;
    pdraw->swapBuffers = driMesaSwapBuffers;

    return (void *)pdp;
}

static __DRIdrawable *driMesaGetDrawable(Display *dpy, GLXDrawable draw,
					 void *screenPrivate)
{
    __DRIscreenPrivate *psp = (__DRIscreenPrivate *) screenPrivate;

    /*
    ** Make sure this routine returns NULL if the drawable is not bound
    ** to a direct rendering context!
    */
    return __driMesaFindDrawable(psp->drawHash, draw);
}

static void driMesaSwapBuffers(Display *dpy, void *drawPrivate)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *) drawPrivate;

    if (pdp->driContextPriv != NULL)
    {
	CGLFlushDrawable(pdp->driContextPriv->ctx);
    }
}

static void driMesaDestroyDrawable(Display *dpy, void *drawPrivate)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)drawPrivate;

    if (pdp) {
	unbind_drawable(pdp);
	if (pdp->surface_id != 0) {
	    xp_destroy_surface(pdp->surface_id);
	    pdp->surface_id = 0;
	}
	if (!pdp->destroyed) {
	    /* don't try to destroy an already destroyed surface. */
	    XAppleDRIDestroySurface(dpy, pdp->driScreenPriv->myNum, pdp->draw);
	}
	Xfree(pdp);
    }
}

/*****************************************************************/

static CGLPixelFormatObj
driCreatePixelFormat(Display *dpy, __DRIscreenPrivate *psp,
                     XVisualInfo *visinfo, __GLXvisualConfig *config)
{
    int i;
    CGLPixelFormatAttribute attr[64]; // currently uses max of 30
    CGLPixelFormatObj result;
    long n_formats;

    i = 0;

    if (!config->rgba)
	return NULL;

    if (config->stereo)
	attr[i++] = kCGLPFAStereo;

    if (config->doubleBuffer)
	attr[i++] = kCGLPFADoubleBuffer;

    attr[i++] = kCGLPFAColorSize;
    attr[i++] = config->redSize + config->greenSize + config->blueSize;
    attr[i++] = kCGLPFAAlphaSize;
    attr[i++] = 1; /* FIXME: ignoring config->alphaSize which is always 0 */

    if (config->accumRedSize + config->accumGreenSize
	+ config->accumBlueSize + config->accumAlphaSize > 0)
    {
	attr[i++] = kCGLPFAAccumSize;
	attr[i++] = (config->accumRedSize + config->accumGreenSize
		     + config->accumBlueSize + config->accumAlphaSize);
    }

    if (config->depthSize > 0) {
	attr[i++] = kCGLPFADepthSize;
	attr[i++] = config->depthSize;
    }

    if (config->stencilSize > 0) {
	attr[i++] = kCGLPFAStencilSize;
	attr[i++] = config->stencilSize;
    }

    if (config->auxBuffers > 0) {
	attr[i++] = kCGLPFAAuxBuffers;
	attr[i++] = config->auxBuffers;
    }

    /* FIXME: things we don't handle: color/alpha masks, level,
       visualrating, transparentFoo */

    attr[i++] = 0;

    result = NULL;
    CGLChoosePixelFormat(attr, &result, &n_formats);

    return result;
}

static void *driMesaCreateContext(Display *dpy, XVisualInfo *vis, void *shared,
				  __DRIcontext *pctx)
{
    __DRIscreen *pDRIScreen;
    __DRIcontextPrivate *pcp;
    __DRIcontextPrivate *pshare = (__DRIcontextPrivate *)shared;
    __DRIscreenPrivate *psp;
    int i;

    if (!(pDRIScreen = __glXFindDRIScreen(dpy, vis->screen))) {
	/* ERROR!!! */
	return NULL;
    } else if (!(psp = (__DRIscreenPrivate *)pDRIScreen->private)) {
	/* ERROR!!! */
	return NULL;
    }

    /* Create the hash table */
    if (!psp->drawHash)
	psp->drawHash = x_hash_table_new(NULL, NULL, NULL, NULL);

    pcp = (__DRIcontextPrivate *)Xmalloc(sizeof(__DRIcontextPrivate));
    if (!pcp) {
	return NULL;
    }

    pcp->display = dpy;
    pcp->driScreenPriv = psp;
    pcp->driDrawablePriv = NULL;

    pcp->ctx = NULL;
    pcp->surface_id = 0;

    pcp->ctx = NULL;
    for (i = 0; pcp->ctx == NULL && i < psp->numVisuals; i++) {
        if (psp->visuals[i].vid == vis->visualid) {
	    CGLCreateContext(psp->visuals[i].pixel_format,
			      pshare ? pshare->ctx : NULL, &pcp->ctx);
        }
    }

    if (!pcp->ctx) {
	Xfree(pcp);
	return NULL;
    }

    pctx->destroyContext = driMesaDestroyContext;
    pctx->bindContext    = driMesaBindContext;
    pctx->unbindContext  = driMesaUnbindContext;

    __driMesaGarbageCollectDrawables(pcp->driScreenPriv->drawHash);

    return pcp;
}

static void driMesaDestroyContext(Display *dpy, int scrn, void *contextPrivate)
{
    __DRIcontextPrivate  *pcp   = (__DRIcontextPrivate *) contextPrivate;

    if (pcp) {
	unbind_context(pcp);
	__driMesaGarbageCollectDrawables(pcp->driScreenPriv->drawHash);
	CGLDestroyContext(pcp->ctx);
	Xfree(pcp);
    }
}

/*****************************************************************/

static void *driMesaCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
				 int numConfigs, __GLXvisualConfig *config)
{
    int directCapable, i, n;
    __DRIscreenPrivate *psp;
    XVisualInfo visTmpl, *visinfo;

    if (!XAppleDRIQueryDirectRenderingCapable(dpy, scrn, &directCapable)) {
	return NULL;
    }

    if (!directCapable) {
	return NULL;
    }

    psp = (__DRIscreenPrivate *)Xmalloc(sizeof(__DRIscreenPrivate));
    if (!psp) {
	return NULL;
    }

    psp->display = dpy;
    psp->myNum = scrn;

#if 0
    if (!XAppleDRIAuthConnection(dpy, scrn, magic)) {
	Xfree(psp);
	(void)XAppleDRICloseConnection(dpy, scrn);
	return NULL;
    }
#endif

    /*
     * Allocate space for an array of visual records and initialize them.
     */
    psp->visuals = (__DRIvisualPrivate *)Xmalloc(numConfigs *
						 sizeof(__DRIvisualPrivate));
    if (!psp->visuals) {
	Xfree(psp);
	return NULL;
    }

    visTmpl.screen = scrn;
    visinfo = XGetVisualInfo(dpy, VisualScreenMask, &visTmpl, &n);
    if (n != numConfigs) {
	Xfree(psp);
	return NULL;
    }

    psp->numVisuals = 0;
    for (i = 0; i < numConfigs; i++, config++) {
	psp->visuals[psp->numVisuals].vid = visinfo[i].visualid;
        psp->visuals[psp->numVisuals].pixel_format =
                 driCreatePixelFormat(dpy, psp, &visinfo[i], config);
	if (psp->visuals[psp->numVisuals].pixel_format != NULL) {
	    psp->numVisuals++;
	}
    }

    XFree(visinfo);

    if (psp->numVisuals == 0) {
	/* Couldn't create any pixel formats. */
	Xfree(psp->visuals);
	Xfree(psp);
	return NULL;
    }

    /* Initialize the drawHash when the first context is created */
    psp->drawHash = NULL;

    psc->destroyScreen  = driMesaDestroyScreen;
    psc->createContext  = driMesaCreateContext;
    psc->createDrawable = driMesaCreateDrawable;
    psc->getDrawable    = driMesaGetDrawable;

    return (void *)psp;
}

static void driMesaDestroyScreen(Display *dpy, int scrn, void *screenPrivate)
{
    __DRIscreenPrivate *psp = (__DRIscreenPrivate *) screenPrivate;

    if (psp) {
	//FIXME resetDriver ?
	Xfree(psp->visuals);
	Xfree(psp);
    }
}

/* Note: definitely can't make any X protocol requests here. */
static void driAppleSurfaceNotify(Display *dpy, unsigned int uid, int kind)
{
    __DRIscreenPrivate *psp;
    __DRIdrawablePrivate *pdp;
    __DRIcontextPrivate *pcp;

    if (driMesaFindDrawableByUID(dpy, uid, &psp, &pdp))
    {
	switch (kind)
	{
	case AppleDRISurfaceNotifyDestroyed:
	    xp_destroy_surface(pdp->surface_id);
	    pdp->surface_id = 0;

	    for (pcp = pdp->driContextPriv; pcp != NULL; pcp = pcp->next)
	    {
		CGLClearDrawable(pcp->ctx);
		pcp->surface_id = 0;
	    }
	    break;

	case AppleDRISurfaceNotifyChanged:
	    for (pcp = pdp->driContextPriv; pcp != NULL; pcp = pcp->next)
	    {
		xp_update_gl_context(pcp->ctx);
	    }
	    break;
	}
    }
}

/*
 * This is the entrypoint into the DRI 3D driver.
 * The driCreateScreen name is the symbol that libGL.so fetches via
 * dlsym() in order to bootstrap the driver.
 */
void *__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
                        int numConfigs, __GLXvisualConfig *config)
{
    static int here_before;

    if (!here_before)
    {
	XAppleDRISetSurfaceNotifyHandler(driAppleSurfaceNotify);
	here_before = True;
    }

    return driMesaCreateScreen(dpy, scrn, psc, numConfigs, config);
}

void __driRegisterExtensions(void)
{
}

__private_extern__ void XAppleDRIUseIndirectDispatch(void)
{
    CGLSetCurrentContext(XAppleDRIGetIndirectContext());
}

#endif /* GLX_DIRECT_RENDERING */
