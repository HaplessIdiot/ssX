/*
 * GLX Hardware Device Driver for Intel i830
 * Copyright (C) 1999 Keith Whitwell
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * KEITH WHITWELL, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/* Adapted for use in the I830M driver: 
 *   Jeff Hartmann <jhartmann@2d3d.com>
 */
/* $XFree86$ */

#ifndef I830CONTEXT_INC
#define I830CONTEXT_INC

typedef struct i830_context_t i830Context;
typedef struct i830_context_t *i830ContextPtr;
typedef struct i830_texture_object_t *i830TextureObjectPtr;


#include "mtypes.h"
#include "drm.h"
#include "mm.h"

#include "i830_screen.h"
#include "i830_tex.h"
#include "i830_drv.h"

#define TAG(x) i830##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG


typedef void (*i830_tri_func)(i830ContextPtr, i830Vertex *, i830Vertex *,
							  i830Vertex *);
typedef void (*i830_line_func)(i830ContextPtr, i830Vertex *, i830Vertex *);
typedef void (*i830_point_func)(i830ContextPtr, i830Vertex *);

#define I830_FALLBACK_TEXTURE		 0x1
#define I830_FALLBACK_DRAW_BUFFER	 0x2
#define I830_FALLBACK_READ_BUFFER	 0x4
#define I830_FALLBACK_COLORMASK		 0x8
#define I830_FALLBACK_SPECULAR		 0x20
#define I830_FALLBACK_LOGICOP		 0x40
#define I830_FALLBACK_RENDERMODE	 0x80
#define I830_FALLBACK_STENCIL		 0x100
#define I830_FALLBACK_BLEND_EQ		 0x200
#define I830_FALLBACK_BLEND_FUNC 	 0x400
#define I830_FALLBACK_STIPPLE		 0x800

struct i830_context_t 
{
   GLint refcount;   
   GLcontext *glCtx;

   /*From I830 stuff*/
   int TextureMode;
   GLuint renderindex;
   GLuint TexBlendWordsUsed[I830_TEXBLEND_COUNT];
   GLuint TexBlend[I830_TEXBLEND_COUNT][I830_TEXBLEND_SIZE];
   GLuint Init_TexBlend[I830_TEXBLEND_COUNT][I830_TEXBLEND_SIZE];
   GLuint Init_TexBlendWordsUsed[I830_TEXBLEND_COUNT];
   GLuint Init_TexBlendColorPipeNum[I830_TEXBLEND_COUNT];
   GLuint TexBlendColorPipeNum[I830_TEXBLEND_COUNT];
   GLuint Init_BufferSetup[I830_DEST_SETUP_SIZE];
   
   GLenum palette_format;
   GLuint palette[256];
   
   
   GLuint Init_Setup[I830_CTX_SETUP_SIZE];
   GLuint vertex_prim;
   drmBufPtr vertex_dma_buffer;
   
   GLboolean mask_red;
   GLboolean mask_green;
   GLboolean mask_blue;
   GLboolean mask_alpha;

   GLboolean clear_red;
   GLboolean clear_green;
   GLboolean clear_blue;
   GLboolean clear_alpha;

   GLfloat depth_scale;
   int depth_clear_mask;
   int stencil_clear_mask;
   int ClearDepth;
   int hw_stencil;
   
   GLuint MonoColor;
   
   GLuint LastTexEnabled;
   GLuint TexEnabledMask;
   
   /* Textures
    */
   i830TextureObjectPtr CurrentTexObj[2];
   struct i830_texture_object_t TexObjList;
   struct i830_texture_object_t SwappedOut; 
   memHeap_t *texHeap;

   /* Bit flag to keep track of fallbacks.
    */
   GLuint Fallback;

   /* Temporaries for translating away float colors:
    */
   struct gl_client_array UbyteColor;
   struct gl_client_array UbyteSecondaryColor;

   /* State for i830vb.c and i830tris.c.
    */
   GLuint new_state;		/* _NEW_* flags */
   GLuint SetupNewInputs;
   GLuint SetupIndex;
   GLuint RenderIndex;
   GLmatrix ViewportMatrix;
   GLenum render_primitive;
   GLenum reduced_primitive;
   GLuint hw_primitive;
   GLuint vertex_format;
   char *verts;

   drmBufPtr  vertex_buffer;
   char *vertex_addr;
   GLuint vertex_low;
   GLuint vertex_high;
   GLuint vertex_last_prim;
   
   GLboolean upload_cliprects;


   /* Fallback rasterization functions 
    */
   i830_point_func draw_point;
   i830_line_func draw_line;
   i830_tri_func draw_tri;

   /* Hardware state 
    */
   GLuint dirty;		/* I810_UPLOAD_* */
   GLuint Setup[I830_CTX_SETUP_SIZE];
   GLuint BufferSetup[I830_DEST_SETUP_SIZE];
   int vertex_size;
   int vertex_stride_shift;
   GLint lastStamp;
   GLboolean stipple_in_hw;

   GLenum TexEnvImageFmt[2];

   /* State which can't be computed completely on the fly:
    */
   GLuint LcsCullMode;
   GLuint LcsLineWidth;
   GLuint LcsPointSize;

   /* Funny mesa mirrors
    */
   GLuint ClearColor;

   /* DRI stuff
    */
   GLuint needClip;
   GLframebuffer *glBuffer;
   GLboolean doPageFlip;

   /* These refer to the current draw (front vs. back) buffer:
    */
   char *drawMap;		/* draw buffer address in virtual mem */
   char *readMap;	
   int drawX;			/* origin of drawable in draw buffer */
   int drawY;
   GLuint numClipRects;		/* cliprects for that buffer */
   XF86DRIClipRectPtr pClipRects;

   int lastSwap;
   int texAge;
   int ctxAge;
   int dirtyAge;

   GLboolean scissor;
   XF86DRIClipRectRec draw_rect;
   XF86DRIClipRectRec scissor_rect;

   drmContext hHWContext;
   drmLock *driHwLock;
   int driFd;
   Display *display;

   __DRIdrawablePrivate *driDrawable;
   __DRIscreenPrivate *driScreen;
   i830ScreenPrivate *i830Screen; 
   I830SAREAPtr sarea;
};


#define I830_TEX_UNIT_ENABLED(unit)     (1<<unit)
#define VALID_I830_TEXTURE_OBJECT(tobj) (tobj)

#define I830_CONTEXT(ctx)   ((i830ContextPtr)(ctx->DriverCtx))
#define I830_DRIVER_DATA(vb) ((i830VertexBufferPtr)((vb)->driver_data))
#define GET_DISPATCH_AGE(imesa) imesa->sarea->last_dispatch
#define GET_ENQUEUE_AGE(imesa)  imesa->sarea->last_enqueue


/* Lock the hardware and validate our state.  
 */
#define LOCK_HARDWARE( imesa )          	    	\
do {              					\
    char __ret=0;                   			\
    DRM_CAS(imesa->driHwLock, imesa->hHWContext,    	\
        (DRM_LOCK_HELD|imesa->hHWContext), __ret);  	\
    if (__ret)                      			\
        i830GetLock( imesa, 0 );            	 	\
}while (0)
 
  
  /* Unlock the hardware using the global current context 
   */
#define UNLOCK_HARDWARE(imesa)                  			\
   DRM_UNLOCK(imesa->driFd, imesa->driHwLock, imesa->hHWContext);


  /* This is the wrong way to do it, I'm sure.  Otherwise the drm
   * bitches that I've already got the heavyweight lock.  At worst,
   * this is 3 ioctls.  The best solution probably only gets me down 
   * to 2 ioctls in the worst case.
   */
#define LOCK_HARDWARE_QUIESCENT( imesa ) do {		 \
   LOCK_HARDWARE( imesa );        	   		 \
   i830RegetLockQuiescent( imesa );     		 \
} while(0)



extern void i830GetLock(i830ContextPtr imesa, GLuint flags);
extern void i830EmitHwStateLocked(i830ContextPtr imesa);
extern void i830EmitDrawingRectangle(i830ContextPtr imesa);
extern void i830XMesaSetBackClipRects(i830ContextPtr imesa);
extern void i830XMesaSetFrontClipRects(i830ContextPtr imesa);
extern void i830DDExtensionsInit(GLcontext *ctx);
extern void i830DDInitDriverFuncs(GLcontext *ctx);
extern void i830DDUpdateHwState(GLcontext *ctx);

#define SUBPIXEL_X 0.125
#define SUBPIXEL_Y 0.125
	
#endif

