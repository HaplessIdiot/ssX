/* $XFree86: xc/lib/GL/mesa/dri/dri_mesa.c,v 1.9 2000/09/24 13:51:01 alanh Exp $ */
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
 *   Brian E. Paul <brian@precisioninsight.com>
 */

#ifdef GLX_DIRECT_RENDERING

#include <unistd.h>
#include <Xlibint.h>
#include <Xext.h>
#include <extutil.h>
#include "glxclient.h"
#include "xf86dri.h"
#include "sarea.h"
#include "dri_mesaint.h"
#include "dri_xmesaapi.h"
#include "../src/context.h"
#include "../src/mmath.h"


/* Context binding */
static Bool driMesaBindContext(Display *dpy, int scrn,
			       GLXDrawable draw, GLXContext gc);
static Bool driMesaUnbindContext(Display *dpy, int scrn,
				 GLXDrawable draw, GLXContext gc);

/* Drawable methods */
static void *driMesaCreateDrawable(Display *dpy, int scrn, GLXDrawable draw,
				   VisualID vid, __DRIdrawable *pdraw);
static __DRIdrawable *driMesaGetDrawable(Display *dpy, GLXDrawable draw);
static void driMesaSwapBuffers(Display *dpy, void *private);
static void driMesaDestroyDrawable(Display *dpy, void *private);

/* Context methods */
static void *driMesaCreateContext(Display *dpy, XVisualInfo *vis, void *shared,
				  __DRIcontext *pctx);
static void driMesaDestroyContext(Display *dpy, int scrn, void *private);

/* Screen methods */
static void *driMesaCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
				 int numConfigs, __GLXvisualConfig *config);
static void driMesaDestroyScreen(Display *dpy, int scrn, void *private);



/*
** Print message to stderr if LIBGL_DEBUG env var is set.
*/
void
__driMesaMessage(const char *msg)
{
    if (getenv("LIBGL_DEBUG")) {
        fprintf(stderr, "libGL error: %s\n", msg);
    }
}


/*****************************************************************/

/* Maintain a list of drawables */

void *drawHash = NULL; /* Hash table to hold DRI drawables */

static Bool __driMesaAddDrawable(__DRIdrawable *pdraw)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;

    /* Create the hash table */
    if (!drawHash) drawHash = drmHashCreate();

    if (drmHashInsert(drawHash, pdp->draw, pdraw))
	return GL_FALSE;

    return GL_TRUE;
}

static __DRIdrawable *__driMesaFindDrawable(GLXDrawable draw)
{
    int retcode;
    __DRIdrawable *pdraw;

    /* Create the hash table */
    if (!drawHash) drawHash = drmHashCreate();

    retcode = drmHashLookup(drawHash, draw, (void **)&pdraw);
    if (retcode)
	return NULL;

    return pdraw;
}

static void __driMesaRemoveDrawable(__DRIdrawable *pdraw)
{
    int retcode;
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;

    /* Create the hash table */
    if (!drawHash) drawHash = drmHashCreate();

    retcode = drmHashLookup(drawHash, pdp->draw, (void **)&pdraw);
    if (!retcode) { /* Found */
	drmHashDelete(drawHash, pdp->draw);
    }
}

static __DRIdrawable *__driMesaFindUnboundDrawable(void)
{
    GLXDrawable draw;
    __DRIdrawable *pdraw;

    while (drmHashFirst(drawHash, &draw, (void **)&pdraw)) {
	__DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)pdraw->private;
	if (!pdp->refcount)
	    return pdraw;
    }
    return NULL;
}

/*****************************************************************/

static void driMesaInitAPI(__MesaAPI *MesaAPI)
{
    MesaAPI->InitDriver = XMesaInitDriver;
    MesaAPI->ResetDriver = XMesaResetDriver;
    MesaAPI->CreateVisual = XMesaCreateVisual;
    MesaAPI->CreateContext = XMesaCreateContext;
    MesaAPI->DestroyContext = XMesaDestroyContext;
    MesaAPI->CreateWindowBuffer = XMesaCreateWindowBuffer;
    MesaAPI->CreatePixmapBuffer = XMesaCreatePixmapBuffer;
    MesaAPI->SwapBuffers = XMesaSwapBuffers;
    MesaAPI->MakeCurrent = XMesaMakeCurrent;
    MesaAPI->UnbindContext = XMesaUnbindContext;
}

/*****************************************************************/

static Bool driMesaUnbindContext(Display *dpy, int scrn,
				 GLXDrawable draw, GLXContext gc)
{
    __DRIdrawable *pdraw;
    __DRIcontextPrivate *pcp;
    __DRIscreenPrivate *psp;
    __DRIdrawablePrivate *pdp;

    /*
    ** Assume error checking is done properly in glXMakeCurrent before
    ** calling driMesaUnbindContext.
    */

    if (gc == NULL || draw == None) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pdraw = __driMesaFindDrawable(draw);
    if (!pdraw) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    pcp = (__DRIcontextPrivate *)gc->driContext.private;
    pdp = (__DRIdrawablePrivate *)pdraw->private;
    psp = pdp->driScreenPriv;
    if (!psp) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    /* Unbind Mesa's drawable from Mesa's context */
    (*psp->MesaAPI.UnbindContext)(pcp);

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
	** When GLX 1.3 is integrated, the create and destroy drawable
	** functions will have user level counterparts and the memory
	** will be able to be recovered.
	*/

	/* Delete drawable if no longer referenced by any contexts */
	(*pdraw->destroyDrawable)(dpy, pdraw->private);
	__driMesaRemoveDrawable(pdraw);
	Xfree(pdraw);
#endif
    }

    /* Unbind the drawable */
    pcp->driDrawablePriv = NULL;
    pdp->driContextPriv = &psp->dummyContextPriv;

    return GL_TRUE;
}

static Bool driMesaBindContext(Display *dpy, int scrn,
			       GLXDrawable draw, GLXContext gc)
{
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

    pdraw = __driMesaFindDrawable(draw);
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
	if (!__driMesaAddDrawable(pdraw)) {
	    /* ERROR!!! */
	    (*pdraw->destroyDrawable)(dpy, pdraw->private);
	    Xfree(pdraw);
	    return GL_FALSE;
	}
    }

    pdp = (__DRIdrawablePrivate *)pdraw->private;
    psp = pdp->driScreenPriv;
    if (!psp) {
	/* ERROR!!! */
	return GL_FALSE;
    }

    /* Bind the drawable to the context */
    pcp = (__DRIcontextPrivate *)gc->driContext.private;
    pcp->driDrawablePriv = pdp;
    pdp->driContextPriv = pcp;
    pdp->refcount++;

    /*
    ** Now that we have a context associated with this drawable, we can
    ** initialize the drawable information if has not been done before.
    */
    if (!pdp->pStamp) {
	DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
	driMesaUpdateDrawableInfo(dpy, scrn, pdp);
	DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);
    }

    /* Call device-specific MakeCurrent */
    (*psp->MesaAPI.MakeCurrent)(pcp, pdp, pdp);

    return GL_TRUE;
}

/*****************************************************************/

/*
 * This function basically updates the __DRIdrawablePrivate struct's
 * cliprect information by calling XF86DRIGetDrawableInfo().
 * This is usually called by a macro which compares the
 * __DRIdrwablePrivate pStamp and lastStamp values.  If the values
 * are different that means we have to update the clipping info.
 */
void driMesaUpdateDrawableInfo(Display *dpy, int scrn,
			       __DRIdrawablePrivate *pdp)
{
    __DRIscreenPrivate *psp;
    __DRIcontextPrivate *pcp = pdp->driContextPriv;
    
    if (!pcp || (pdp != pcp->driDrawablePriv)) {
	/* ERROR!!! */
	return;
    }

    psp = pdp->driScreenPriv;
    if (!psp) {
	/* ERROR!!! */
	return;
    }

    if (pdp->pClipRects) {
	Xfree(pdp->pClipRects); 
    }

    if (pdp->pBackClipRects) {
	Xfree(pdp->pBackClipRects); 
    }

    DRM_SPINUNLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);

    if (!XF86DRIGetDrawableInfo(dpy, scrn, pdp->draw,
				&pdp->index, &pdp->lastStamp,
				&pdp->x, &pdp->y, &pdp->w, &pdp->h,
				&pdp->numClipRects, &pdp->pClipRects,
				&pdp->backX,
				&pdp->backY,
				&pdp->numBackClipRects,
				&pdp->pBackClipRects
                                )) {
	pdp->numClipRects = 0;
	pdp->pClipRects = NULL;
	pdp->numBackClipRects = 0;
	pdp->pBackClipRects = 0;
	/* ERROR!!! */
    }

    DRM_SPINLOCK(&psp->pSAREA->drawable_lock, psp->drawLockID);

    pdp->pStamp = &(psp->pSAREA->drawableTable[pdp->index].stamp);
}

/*****************************************************************/

static void *driMesaCreateDrawable(Display *dpy, int scrn, GLXDrawable draw,
				   VisualID vid, __DRIdrawable *pdraw)
{
    __DRIscreen *pDRIScreen;
    __DRIscreenPrivate *psp;
    __DRIdrawablePrivate *pdp;
    int i;
    GLvisual *mesaVis = NULL;

    pdp = (__DRIdrawablePrivate *)Xmalloc(sizeof(__DRIdrawablePrivate));
    if (!pdp) {
	return NULL;
    }

    if (!XF86DRICreateDrawable(dpy, scrn, draw, &pdp->hHWDrawable)) {
	Xfree(pdp);
	return NULL;
    }

    pdp->draw = draw;
    pdp->refcount = 0;
    pdp->pStamp = NULL;
    pdp->lastStamp = 0;
    pdp->index = 0;
    pdp->x = 0;
    pdp->y = 0;
    pdp->w = 0;
    pdp->h = 0;
    pdp->numClipRects = 0;
    pdp->numBackClipRects = 0;
    pdp->pClipRects = NULL;
    pdp->pBackClipRects = NULL;

    pDRIScreen = __glXFindDRIScreen(dpy, scrn);
    pdp->driScreenPriv = psp = (__DRIscreenPrivate *)pDRIScreen->private;

    pdp->driContextPriv = &psp->dummyContextPriv;

    for (i = 0; i < psp->numVisuals; i++) {
	if (vid == psp->visuals[i].vid) {
	    mesaVis = psp->visuals[i].mesaVisual;
	    break;
	}
    }

    /* XXX pixmap rendering not implemented yet */
    if (1) {
       pdp->mesaBuffer = (*psp->MesaAPI.CreateWindowBuffer)(dpy, psp, pdp, mesaVis);
    }
    else {
       pdp->mesaBuffer = (*psp->MesaAPI.CreatePixmapBuffer)(dpy, psp, pdp, mesaVis);
    }
    if (!pdp->mesaBuffer) {
	(void)XF86DRIDestroyDrawable(dpy, scrn, pdp->draw);
	Xfree(pdp);
	return NULL;
    }

    pdraw->destroyDrawable = driMesaDestroyDrawable;
    pdraw->swapBuffers = driMesaSwapBuffers;

    return (void *)pdp;
}

static __DRIdrawable *driMesaGetDrawable(Display *dpy, GLXDrawable draw)
{
    /*
    ** Make sure this routine returns NULL if the drawable is not bound
    ** to a direct rendering context!
    */
    return __driMesaFindDrawable(draw);
}

static void driMesaSwapBuffers(Display *dpy, void *private)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)private;
    __DRIscreenPrivate *psp = pdp->driScreenPriv;

    (*psp->MesaAPI.SwapBuffers)(pdp);
}

static void driMesaDestroyDrawable(Display *dpy, void *private)
{
    __DRIdrawablePrivate *pdp = (__DRIdrawablePrivate *)private;
    __DRIscreenPrivate *psp = pdp->driScreenPriv;
    int scrn = psp->myNum;

    if (pdp) {
        gl_destroy_framebuffer(pdp->mesaBuffer);
	(void)XF86DRIDestroyDrawable(dpy, scrn, pdp->draw);
	if (pdp->pClipRects)
	    Xfree(pdp->pClipRects);
	Xfree(pdp);
    }
}

/*****************************************************************/

static void *driMesaCreateContext(Display *dpy, XVisualInfo *vis, void *shared,
				  __DRIcontext *pctx)
{
    __DRIcontextPrivate *pcp;
    __DRIcontextPrivate *pshare = (__DRIcontextPrivate *)shared;
    __DRIscreenPrivate *psp;
    __DRIscreen *pDRIScreen;
    int i;

    pDRIScreen = __glXFindDRIScreen(dpy, vis->screen);
    psp = (__DRIscreenPrivate *)pDRIScreen->private;

    if (!psp->dummyContextPriv.driScreenPriv) {
	if (!XF86DRICreateContext(dpy, vis->screen, vis->visual,
				  &psp->dummyContextPriv.contextID,
				  &psp->dummyContextPriv.hHWContext)) {
	    return NULL;
	}
	psp->dummyContextPriv.driScreenPriv = psp;
	psp->dummyContextPriv.mesaContext = NULL;
	psp->dummyContextPriv.driDrawablePriv = NULL;
	/* No other fields should be used! */
    }

    pcp = (__DRIcontextPrivate *)Xmalloc(sizeof(__DRIcontextPrivate));
    if (!pcp) {
	return NULL;
    }

    pcp->display = dpy;
    pcp->driScreenPriv = psp;
    pcp->mesaContext = NULL;
    pcp->driDrawablePriv = NULL;

    if (!XF86DRICreateContext(dpy, vis->screen, vis->visual,
			      &pcp->contextID, &pcp->hHWContext)) {
	Xfree(pcp);
	return NULL;
    }

    for (i = 0; i < psp->numVisuals; i++) {
        if (psp->visuals[i].vid == vis->visualid) {
            GLvisual *mesaVis = psp->visuals[i].mesaVisual;
            GLcontext *sharedMesaCtx = pshare ? pshare->mesaContext : NULL;
            pcp->mesaContext = gl_create_context(mesaVis,
                                                 sharedMesaCtx,
                                                 NULL, /* set below */
                                                 GL_TRUE);
            if (pcp->mesaContext) {
                /* Driver now creates its private context data */
                if ((*psp->MesaAPI.CreateContext)(dpy, mesaVis, pcp)) {
                    pcp->mesaContext->DriverCtx = pcp->driverPrivate;
                }
                else {
                    gl_destroy_context(pcp->mesaContext);
                    pcp->mesaContext = NULL;
                }
            }
        }
    }

    if (!pcp->mesaContext) {
	(void)XF86DRIDestroyContext(dpy, vis->screen, pcp->contextID);
	Xfree(pcp);
	return NULL;
    }

    pctx->destroyContext = driMesaDestroyContext;
    pctx->bindContext    = driMesaBindContext;
    pctx->unbindContext  = driMesaUnbindContext;

    return pcp;
}

static void driMesaDestroyContext(Display *dpy, int scrn, void *private)
{
    __DRIcontextPrivate  *pcp   = (__DRIcontextPrivate *)private;
    __DRIdrawablePrivate *pdp;
    __DRIdrawable        *pdraw;

    if (pcp) {
	pdp = pcp->driDrawablePriv;
	if (pdp) {
	    /* Unbind Mesa's drawable from Mesa's context */
	    (*pcp->driScreenPriv->MesaAPI.UnbindContext)(pcp);

	    if (pdp->refcount == 1) {
		/* If refcount is 1, then this is the last context to
                   reference this drawable, so we can destroy the
                   drawable and remove it from the hash table */
		pdraw = __driMesaFindDrawable(pdp->draw);
		(*pdraw->destroyDrawable)(dpy, pdraw->private);
		__driMesaRemoveDrawable(pdraw);
		Xfree(pdraw);
	    }
	}
	while ((pdraw = __driMesaFindUnboundDrawable()) != NULL)
	    __driMesaRemoveDrawable(pdraw);

	(void)XF86DRIDestroyContext(dpy, scrn, pcp->contextID);
	(*pcp->driScreenPriv->MesaAPI.DestroyContext)(pcp);
        gl_destroy_context(pcp->mesaContext);
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
    drmHandle hFB, hSAREA;
    char *BusID, *driverName;
    drmMagic magic;

    if (!XF86DRIQueryDirectRenderingCapable(dpy, scrn, &directCapable)) {
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

    if (!XF86DRIOpenConnection(dpy, scrn, &hSAREA, &BusID)) {
	Xfree(psp);
	return NULL;
    }

    /*
    ** NOT_DONE: This is used by the X server to detect when the client
    ** has died while holding the drawable lock.  The client sets the
    ** drawable lock to this value.
    */
    psp->drawLockID = 1;

    psp->fd = drmOpen(NULL,BusID);
    if (psp->fd < 0) {
	Xfree(BusID);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }
    Xfree(BusID); /* No longer needed */

    if (drmGetMagic(psp->fd, &magic)) {
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    {
        drmVersionPtr version = drmGetVersion(psp->fd);
        if (version) {
            psp->drmMajor = version->version_major;
            psp->drmMinor = version->version_minor;
            psp->drmPatch = version->version_patchlevel;
            drmFreeVersion(version);
        }
        else {
            psp->drmMajor = -1;
            psp->drmMinor = -1;
            psp->drmPatch = -1;
        }
    }

    if (!XF86DRIAuthConnection(dpy, scrn, magic)) {
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    if (!XF86DRIGetClientDriverName(dpy, scrn,
				    &psp->ddxMajor,
				    &psp->ddxMinor,
				    &psp->ddxPatch,
				    &driverName)) {
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    driMesaInitAPI(&psp->MesaAPI);

    if (!XF86DRIGetDeviceInfo(dpy, scrn,
			      &hFB,
			      &psp->fbOrigin,
			      &psp->fbSize,
			      &psp->fbStride,
			      &psp->devPrivSize,
			      &psp->pDevPriv)) {
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }
    psp->fbWidth = DisplayWidth(dpy, scrn);
    psp->fbHeight = DisplayHeight(dpy, scrn);
    psp->fbBPP = 32; /* NOT_DONE: Get this from X server */

    if (drmMap(psp->fd, hFB, psp->fbSize, (drmAddressPtr)&psp->pFB)) {
	Xfree(psp->pDevPriv);
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    if (drmMap(psp->fd, hSAREA, SAREA_MAX, (drmAddressPtr)&psp->pSAREA)) {
	(void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	Xfree(psp->pDevPriv);
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    psp->numVisuals = numConfigs;
    psp->visuals = (__DRIvisualPrivate *)Xmalloc(numConfigs *
						 sizeof(__DRIvisualPrivate));
    if (!psp->visuals) {
	(void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	(void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	Xfree(psp->pDevPriv);
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    visTmpl.screen = scrn;
    visinfo = XGetVisualInfo(dpy, VisualScreenMask, &visTmpl, &n);
    if (n != numConfigs) {
	(void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	(void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	Xfree(psp->pDevPriv);
	(void)drmClose(psp->fd);
	Xfree(psp);
	(void)XF86DRICloseConnection(dpy, scrn);
	return NULL;
    }

    for (i = 0; i < numConfigs; i++, config++) {
	psp->visuals[i].vid = visinfo[i].visualid;
        psp->visuals[i].mesaVisual =
                 (*psp->MesaAPI.CreateVisual) (dpy, psp, &visinfo[i], config);

	if (!psp->visuals[i].mesaVisual) {
	    /* Free the visuals so far created */
	    while (--i >= 0) {
               _mesa_destroy_visual(psp->visuals[i].mesaVisual);
	    }
	    Xfree(psp->visuals);
	    XFree(visinfo);
	    (void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	    (void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	    Xfree(psp->pDevPriv);
	    (void)drmClose(psp->fd);
	    Xfree(psp);
	    (void)XF86DRICloseConnection(dpy, scrn);
	    return NULL;
	}
    }
    XFree(visinfo);

    /* Initialize the screen specific GLX driver */
    if (psp->MesaAPI.InitDriver) {
	if (!(*psp->MesaAPI.InitDriver)(psp)) {
	    while (--psp->numVisuals >= 0) {
               _mesa_destroy_visual(psp->visuals[psp->numVisuals].mesaVisual);
	    }
	    Xfree(psp->visuals);
	    (void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	    (void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	    Xfree(psp->pDevPriv);
	    (void)drmClose(psp->fd);
	    Xfree(psp);
	    (void)XF86DRICloseConnection(dpy, scrn);
	    return NULL;
	}
    }

    /*
    ** Do not init dummy context here; actual initialization will be
    ** done when the first DRI context is created.  Init screen priv ptr
    ** to NULL to let CreateContext routine that it needs to be inited.
    */
    psp->dummyContextPriv.driScreenPriv = NULL;

    psc->destroyScreen  = driMesaDestroyScreen;
    psc->createContext  = driMesaCreateContext;
    psc->createDrawable = driMesaCreateDrawable;
    psc->getDrawable    = driMesaGetDrawable;

    return (void *)psp;
}

static void driMesaDestroyScreen(Display *dpy, int scrn, void *private)
{
    __DRIscreenPrivate *psp = (__DRIscreenPrivate *)private;

    if (psp) {
	if (psp->dummyContextPriv.driScreenPriv) {
	    (void)XF86DRIDestroyContext(dpy, scrn,
					psp->dummyContextPriv.contextID);
	}
	if (psp->MesaAPI.ResetDriver)
	    (*psp->MesaAPI.ResetDriver)(psp);
	while (--psp->numVisuals >= 0) {
           _mesa_destroy_visual(psp->visuals[psp->numVisuals].mesaVisual);
	}
	Xfree(psp->visuals);
	(void)drmUnmap((drmAddress)psp->pSAREA, SAREA_MAX);
	(void)drmUnmap((drmAddress)psp->pFB, psp->fbSize);
	Xfree(psp->pDevPriv);
	(void)drmClose(psp->fd);
	Xfree(psp);

#if 0
	/*
	** NOT_DONE: Normally, we would call XF86DRICloseConnection()
	** here, but since this routine is called after the
	** XCloseDisplay() function has already shut down the connection
	** to the Display, there is no protocol stream open to the X
	** server anymore.  Luckily, XF86DRICloseConnection() does not
	** really do anything (for now).
	*/
	(void)XF86DRICloseConnection(dpy, scrn);
#endif
    }
}


/*
 * This is the entrypoint into the driver.
 * The driCreateScreen name is the symbol that libGL.so fetches.
 */
void *__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
                        int numConfigs, __GLXvisualConfig *config)
{
   return driMesaCreateScreen(dpy, scrn, psc, numConfigs, config);
}



#endif
