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
 *   Daryll Strauss <daryll@precisioninsight.com>
 *   Brian E. Paul <brian@precisioninsight.com>
 */

#ifdef GLX_DIRECT_RENDERING

#include <X11/Xlibint.h>
#include <glide.h>
#include "tdfx_init.h"
#include "context.h"
#include "matrix.h"
#include "mmath.h"
#include "vbxform.h"


__DRIcontextPrivate *gCC = 0;


GLboolean XMesaInitDriver(__DRIscreenPrivate *sPriv)
{
   tdfxScreenPrivate *gsp;

   /* Check the DRI version */
   {
      int major, minor, patch;
      if (XF86DRIQueryVersion(sPriv->display, &major, &minor, &patch)) {
         if (major != 3 || minor != 0 || patch < 0) {
            char msg[1000];
            sprintf(msg, "3dfx DRI driver expected DRI version 3.0.x but got version %d.%d.%d", major, minor, patch);
            __driMesaMessage(msg);
            return GL_FALSE;
         }
      }
   }

   /* Check that the DDX driver version is compatible */
   if (sPriv->ddxMajor != 1 ||
       sPriv->ddxMinor != 0 ||
       sPriv->ddxPatch < 0) {
      char msg[1000];
      sprintf(msg, "3dfx DRI driver expected DDX driver version 1.0.x but got version %d.%d.%d", sPriv->ddxMajor, sPriv->ddxMinor, sPriv->ddxPatch);
      __driMesaMessage(msg);
      return GL_FALSE;
   }

   /* Check that the DRM driver version is compatible */
   if (sPriv->drmMajor != 1 ||
       sPriv->drmMinor != 0 ||
       sPriv->drmPatch < 0) {
      char msg[1000];
      sprintf(msg, "3dfx DRI driver expected DRM driver version 1.0.x but got version %d.%d.%d", sPriv->drmMajor, sPriv->drmMinor, sPriv->drmPatch);
      __driMesaMessage(msg);
      return GL_FALSE;
   }

   /* Allocate the private area */
   gsp = (tdfxScreenPrivate *)Xmalloc(sizeof(tdfxScreenPrivate));
   if (!gsp)
      return GL_FALSE;

   gsp->driScrnPriv = sPriv;

   sPriv->private = (void *) gsp;

   if (!tdfxMapAllRegions(sPriv)) {
      Xfree(gsp);
      sPriv->private = NULL;
      return GL_FALSE;
   }

   return GL_TRUE;
}

void XMesaResetDriver(__DRIscreenPrivate *sPriv)
{
  tdfxUnmapAllRegions(sPriv);
  Xfree(sPriv->private);
  sPriv->private = NULL;
}

GLvisual *XMesaCreateVisual(Display *dpy,
                            __DRIscreenPrivate *driScrnPriv,
                            const XVisualInfo *visinfo,
                            const __GLXvisualConfig *config)
{
   /* Drivers may change the args to _mesa_create_visual() in order to
    * setup special visuals.
    */
   return _mesa_create_visual( config->rgba,
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
                               0 /* num samples */ );
}


GLboolean XMesaCreateContext(Display *dpy, GLvisual *mesaVis,
                             __DRIcontextPrivate *driContextPriv)
{
  tdfxContextPrivate *cPriv;
  __DRIscreenPrivate *driScrnPriv = driContextPriv->driScreenPriv;
  tdfxScreenPrivate *sPriv = (tdfxScreenPrivate *)driScrnPriv->private;
  TDFXSAREAPriv *saPriv;
  /*int **fifoPtr;*/

  cPriv = (tdfxContextPrivate *)Xmalloc(sizeof(tdfxContextPrivate));
  if (!cPriv) {
    return GL_FALSE;
  }

  cPriv->hHWContext = driContextPriv->hHWContext;
  cPriv->tdfxScrnPriv = sPriv;
  /* deviceID = 0x05 = Voodoo3 */
  /* deviceID = 0x09 = Voodoo5 (and Voodoo5?) */
  cPriv->haveHwStencil = sPriv->deviceID == 0x9 && sPriv->cpp == 4;

  cPriv->glVis=mesaVis;
  cPriv->glBuffer=gl_create_framebuffer(mesaVis,
                        GL_FALSE,  /* software depth buffer? */
                        mesaVis->StencilBits > 0 && !cPriv->haveHwStencil,
                        mesaVis->AccumRedBits > 0,
                        GL_FALSE   /* software alpha channel */
                      );

  cPriv->screen_width=sPriv->width;
  cPriv->screen_height=sPriv->height;
  cPriv->new_state = ~0;

  cPriv->glCtx = driContextPriv->mesaContext;
  cPriv->initDone=GL_FALSE;

  saPriv=(TDFXSAREAPriv*)((char*)driScrnPriv->pSAREA+sizeof(XF86DRISAREARec));
  grDRIOpen(driScrnPriv->pFB, sPriv->regs.map, sPriv->deviceID, 
	    sPriv->width, sPriv->height, sPriv->mem, sPriv->cpp, sPriv->stride,
	    sPriv->fifoOffset, sPriv->fifoSize, sPriv->fbOffset,
	    sPriv->backOffset, sPriv->depthOffset, sPriv->textureOffset, 
	    sPriv->textureSize, &saPriv->fifoPtr, &saPriv->fifoRead);

  driContextPriv->driverPrivate = (void *) cPriv;

  return GL_TRUE;
}

void XMesaDestroyContext(__DRIcontextPrivate *driContextPriv)
{
  tdfxContextPrivate *cPriv = (tdfxContextPrivate *) driContextPriv->driverPrivate;
  if (cPriv) {
    XFree(cPriv);
    driContextPriv->driverPrivate = NULL;
  }

  if (driContextPriv == gCC) {
    gCC = 0;
  }
}


GLframebuffer *XMesaCreateWindowBuffer( Display *dpy,
                                        __DRIscreenPrivate *driScrnPriv,
                                        __DRIdrawablePrivate *driDrawPriv,
                                        GLvisual *mesaVis)
{
   return gl_create_framebuffer(mesaVis,
                                GL_FALSE,  /* software depth buffer? */
                                mesaVis->StencilBits > 0,
                                mesaVis->AccumRedBits > 0,
                                GL_FALSE   /* software alpha channel? */
                                );
}


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
                                mesaVis->AlphaBits > 0
                                );
#else
   return NULL;  /* not implemented yet */
#endif
}


void XMesaSwapBuffers(__DRIdrawablePrivate *driDrawPriv)
{
  FxI32 result;
#ifdef STATS
  int stalls;
  extern int texSwaps;
  static int prevStalls=0;
#endif
  tdfxContextPrivate *gCCPriv;

  /*
  ** NOT_DONE: This assumes buffer is currently bound to a context.
  ** This needs to be able to swap buffers when not currently bound.
  */
  if (gCC == NULL)
     return;

  gCCPriv = (tdfxContextPrivate *) gCC->driverPrivate;

  FLUSH_VB( gCCPriv->glCtx, "swap buffers" );

  if (gCC->mesaContext->Visual->DBflag) {
#ifdef STATS
    stalls=grFifoGetStalls();
    if (stalls!=prevStalls) {
      fprintf(stderr, "%d stalls occurred\n", stalls-prevStalls);
      prevStalls=stalls;
    }
    if (texSwaps) {
      fprintf(stderr, "%d texture swaps occurred\n", texSwaps);
      texSwaps=0;
    }
#endif
    FX_grDRIBufferSwap(gCCPriv->swapInterval);
    do {
      result = FX_grGetInteger(FX_PENDING_BUFFERSWAPS);
    } while (result > gCCPriv->maxPendingSwapBuffers);
    gCCPriv->stats.swapBuffer++;
  }
}

GLboolean XMesaUnbindContext(__DRIcontextPrivate *driContextPriv)
{
   if (driContextPriv && driContextPriv == gCC) {
      tdfxContextPrivate *gCCPriv;
      gCCPriv = (tdfxContextPrivate *) gCC->driverPrivate;
      FX_grGlideGetState((GrState*)gCCPriv->state);
   }
   return GL_TRUE;
}

GLboolean XMesaMakeCurrent(__DRIcontextPrivate *driContextPriv,
                           __DRIdrawablePrivate *driDrawPriv,
                           __DRIdrawablePrivate *driReadPriv)
{
  if (driContextPriv) {
    tdfxContextPrivate *gCCPriv;

    gCC = driContextPriv;
    gCCPriv = (tdfxContextPrivate *)driContextPriv->driverPrivate;

    if (!gCCPriv->initDone) {
      if (!tdfxInitHW(driDrawPriv, gCCPriv))
        return GL_FALSE;
      gCCPriv->width=0;
      XMesaWindowMoved();
      FX_grGlideGetState((GrState*)gCCPriv->state);
    }
    else {
      FX_grSstSelect(gCCPriv->board);
      FX_grGlideSetState((GrState*)gCCPriv->state);
      XMesaWindowMoved();
    }

    gl_make_current2(gCCPriv->glCtx, driDrawPriv->mesaBuffer, driReadPriv->mesaBuffer);

    fxSetupDDPointers(gCCPriv->glCtx);
    if (!gCCPriv->glCtx->Viewport.Width)
      gl_Viewport(gCCPriv->glCtx, 0, 0, driDrawPriv->w, driDrawPriv->h);
  }
  else {
    gl_make_current(0,0);
    gCC = NULL;
  }
  return GL_TRUE;
}


void XMesaWindowMoved(void)
{
  __DRIdrawablePrivate *dPriv = gCC->driDrawablePriv;
  tdfxContextPrivate *gCCPriv = (tdfxContextPrivate *) gCC->driverPrivate;
  GLcontext *ctx = gCCPriv->glCtx;

  grDRIPosition(dPriv->x, dPriv->y, dPriv->w, dPriv->h,
		dPriv->numClipRects, dPriv->pClipRects);
  gCCPriv->numClipRects=dPriv->numClipRects;
  gCCPriv->pClipRects=dPriv->pClipRects;
  if (dPriv->x!=gCCPriv->x_offset || dPriv->y!=gCCPriv->y_offset ||
      dPriv->w!=gCCPriv->width || dPriv->h!=gCCPriv->height) {
    gCCPriv->x_offset=dPriv->x;
    gCCPriv->y_offset=dPriv->y;
    gCCPriv->width=dPriv->w;
    gCCPriv->height=dPriv->h;
    gCCPriv->y_delta=gCCPriv->screen_height-gCCPriv->y_offset-gCCPriv->height;
  }
  gCCPriv->needClip=1;
  switch (dPriv->numClipRects) {
  case 0:
    gCCPriv->clipMinX=dPriv->x;
    gCCPriv->clipMaxX=dPriv->x+dPriv->w;
    gCCPriv->clipMinY=dPriv->y;
    gCCPriv->clipMaxY=dPriv->y+dPriv->h;
    fxSetScissorValues(ctx);
    gCCPriv->needClip=0;
    break;
  case 1:
    gCCPriv->clipMinX=dPriv->pClipRects[0].x1;
    gCCPriv->clipMaxX=dPriv->pClipRects[0].x2;
    gCCPriv->clipMinY=dPriv->pClipRects[0].y1;
    gCCPriv->clipMaxY=dPriv->pClipRects[0].y2;
    fxSetScissorValues(ctx);
    gCCPriv->needClip=0;
    break;
  default:
  }
}

/* This is called from within the LOCK_HARDWARE routine */
void XMesaUpdateState(int windowMoved)
{
  __DRIdrawablePrivate *dPriv = gCC->driDrawablePriv;
  __DRIscreenPrivate *sPriv = dPriv->driScreenPriv;
  tdfxContextPrivate *gCCPriv = (tdfxContextPrivate *) gCC->driverPrivate;
  TDFXSAREAPriv *saPriv=(TDFXSAREAPriv*)(((char*)sPriv->pSAREA)+sizeof(XF86DRISAREARec));

  /* fprintf(stderr, "In FifoPtr=%d FifoRead=%d\n", saPriv->fifoPtr, saPriv->fifoRead); */
  if (saPriv->fifoOwner!=dPriv->driContextPriv->hHWContext) {
    grDRIImportFifo(saPriv->fifoPtr, saPriv->fifoRead);
  }
  if (saPriv->ctxOwner!=dPriv->driContextPriv->hHWContext) {
    /* This sequence looks a little odd. Glide mirrors the state, and
       when you get the state you are forcing the mirror to be up to
       date, and then getting a copy from the mirror. You can then force
       that state onto the hardware when you set the state. */
    void *state;
    state=malloc(FX_grGetInteger_NoLock(FX_GLIDE_STATE_SIZE));
    FX_grGlideGetState_NoLock(state);
    FX_grGlideSetState_NoLock(state);
    free(state);
  }
  if (saPriv->texOwner!=dPriv->driContextPriv->hHWContext) {
    fxTMRestoreTextures_NoLock(gCCPriv);
  }
  if (windowMoved)
    XMesaWindowMoved();
}

void XMesaSetSAREA(void)
{
  __DRIdrawablePrivate *dPriv = gCC->driDrawablePriv;
  __DRIscreenPrivate *sPriv = dPriv->driScreenPriv;
  TDFXSAREAPriv *saPriv=(TDFXSAREAPriv*)(((char*)sPriv->pSAREA)+sizeof(XF86DRISAREARec));

  saPriv->fifoOwner=dPriv->driContextPriv->hHWContext;
  saPriv->ctxOwner=dPriv->driContextPriv->hHWContext;
  saPriv->texOwner=dPriv->driContextPriv->hHWContext;
  grDRIResetSAREA();
  /* fprintf(stderr, "Out FifoPtr=%d FifoRead=%d\n", saPriv->fifoPtr, saPriv->fifoRead); */
}


extern void __driRegisterExtensions(void); /* silence compiler warning */

/* This function is called by libGL.so as soon as libGL.so is loaded.
 * This is where we'd register new extension functions with the dispatcher.
 */
void __driRegisterExtensions(void)
{
#if 0
   /* Example.  Also look in fxdd.c for more details. */
   {
      const int _gloffset_FooBarEXT = 555;  /* just an example number! */
      if (_glapi_add_entrypoint("glFooBarEXT", _gloffset_FooBarEXT)) {
         void *f = glXGetProcAddressARB("glFooBarEXT");
         assert(f);
      }
   }
#endif
}


#endif
