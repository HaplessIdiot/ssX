/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_context.h,v 1.3 2002/02/22 21:45:00 dawes Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

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
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith_whitwell@yahoo.com>
 */

#ifndef __RADEON_CONTEXT_H__
#define __RADEON_CONTEXT_H__

#ifdef GLX_DIRECT_RENDERING

#include <X11/Xlibint.h>

#include "dri_util.h"

#include "xf86drm.h"
#include "xf86drmRadeon.h"

#include "macros.h"
#include "mtypes.h"

#include "radeon_reg.h"

struct radeon_context;
typedef struct radeon_context radeonContextRec;
typedef struct radeon_context *radeonContextPtr;

#include "radeon_lock.h"
#include "radeon_screen.h"
#include "mm.h"

/* Flags for software fallback cases */
/* See correponding strings in radeon_tris.c */
#define RADEON_FALLBACK_TEXTURE		0x0001
#define RADEON_FALLBACK_DRAW_BUFFER	0x0002
#define RADEON_FALLBACK_STENCIL		0x0004
#define RADEON_FALLBACK_RENDER_MODE	0x0008
#define RADEON_FALLBACK_BLEND_EQ	0x0010
#define RADEON_FALLBACK_BLEND_FUNC	0x0020

/* Use the templated vertex format:
 */
#define COLOR_IS_RGBA
#define TAG(x) radeon##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

typedef void (*radeon_tri_func)( radeonContextPtr,
				 radeonVertex *,
				 radeonVertex *,
				 radeonVertex * );

typedef void (*radeon_line_func)( radeonContextPtr,
				  radeonVertex *,
				  radeonVertex * );

typedef void (*radeon_point_func)( radeonContextPtr,
				   radeonVertex * );

typedef void (*radeon_prim_func)( GLcontext *ctx );


struct radeon_colorbuffer_state {
   GLuint clear;

   GLint drawOffset, drawPitch;
};


struct radeon_depthbuffer_state {
   GLuint clear;
   GLfloat scale;
};

struct radeon_pixel_state {
   GLint readOffset, readPitch;
};

struct radeon_scissor_state {
   XF86DRIClipRectRec rect;
   GLboolean enabled;
};

struct radeon_stencilbuffer_state {
   GLboolean hwBuffer;
   GLuint clear;			/* rb3d_stencilrefmask value */
};

struct radeon_stipple_state {
   GLuint mask[32];
};



#define TEX_0 1
#define TEX_1 2

typedef struct radeon_tex_obj radeonTexObj, *radeonTexObjPtr;

/* Texture object in locally shared texture space.
 */
struct radeon_tex_obj {
   radeonTexObjPtr next, prev;

   struct gl_texture_object *tObj;	/* Mesa texture object */

   PMemBlock memBlock;			/* Memory block containing texture */
   GLuint bufAddr;			/* Offset to start of locally
					   shared texture block */

   GLuint dirty_images;			/* Flags for whether or not
					   images need to be uploaded to
					   local or AGP texture space */

   GLint bound;				/* Texture unit currently bound to */
   GLint heap;				/* Texture heap currently stored in */

   drmRadeonTexImage image[RADEON_MAX_TEXTURE_LEVELS];

   GLint totalSize;			/* Total size of the texture
					   including all mipmap levels */

   GLuint pp_txfilter;		        /* hardware register values */
   GLuint pp_txformat;
   GLuint pp_txoffset;
   GLuint pp_border_color;

   /* texObj->Image[firstLevel] through texObj->Image[lastLevel] are the
    * images to upload.
    */
   GLint firstLevel;     
   GLint lastLevel;      
};


struct radeon_texture_env_state {
   radeonTexObjPtr texobj;
   GLenum format;
   GLenum envMode;
};

struct radeon_texture_state {
   struct radeon_texture_env_state unit[RADEON_MAX_TEXTURE_UNITS];
};

struct radeon_state {
   drmRadeonState hw;


   struct radeon_colorbuffer_state color;
   struct radeon_depthbuffer_state depth;
   struct radeon_pixel_state pixel;
   struct radeon_scissor_state scissor;
   struct radeon_stencilbuffer_state stencil;
   struct radeon_stipple_state stipple;
   struct radeon_texture_state texture;
};

struct radeon_texture {
   radeonTexObj objects[RADEON_NR_TEX_HEAPS];
   radeonTexObj swapped;

   memHeap_t *heap[RADEON_NR_TEX_HEAPS];
   GLint age[RADEON_NR_TEX_HEAPS];

   GLint numHeaps;
};


struct radeon_dma {
   drmBufPtr buffer;
   drmBufPtr retained;
   GLubyte *address;
   GLuint low, high, last;
   GLuint offset;
};

struct radeon_dri_mirror {
   Display *display;			/* X server display */

   __DRIcontextPrivate	*context;	/* DRI context */
   __DRIscreenPrivate	*screen;	/* DRI screen */
   __DRIdrawablePrivate	*drawable;	/* DRI drawable bound to this ctx */

   drmContext hwContext;
   drmLock *hwLock;
   int fd;
};

struct radeon_store {
   radeonTexObjPtr texture[2][RADEON_MAX_STATES];
   drmRadeonState state[RADEON_MAX_STATES];
   drmRadeonPrim prim[RADEON_MAX_PRIMS];
   GLuint statenr;
   GLuint primnr;
};




struct radeon_context {
   GLcontext *glCtx;			/* Mesa context */

   /* Driver and hardware state management
    */
   struct radeon_state state;

   /* Texture object bookkeeping
    */
   struct radeon_texture texture;

   /* Fallback rasterization functions
    */
   radeon_point_func draw_point;
   radeon_line_func draw_line;
   radeon_tri_func draw_tri;

   /* Rasterization and vertex state:
    */
   GLuint NewGLState;
   GLuint Fallback;
   GLuint SetupIndex;
   GLuint SetupNewInputs;
   GLuint RenderIndex;

   GLuint vertex_size;
   GLuint vertex_stride_shift;
   GLuint vertex_format;
   GLuint num_verts;
   char *verts;
   
   /* Temporaries for translating away float colors:
    */
   struct gl_client_array UbyteColor;
   struct gl_client_array UbyteSecondaryColor;

   /* Vertex buffers
    */
   struct radeon_dma dma;

   struct radeon_store store;
   GLboolean upload_cliprects;

   GLuint hw_primitive;
   GLenum render_primitive;

   /* Page flipping
    */
   GLuint doPageFlip;
   GLuint currentPage;

   /* Drawable, cliprect and scissor information
    */
   GLuint numClipRects;			/* Cliprects for the draw buffer */
   XF86DRIClipRectPtr pClipRects;
   GLuint lastStamp;

   /* Mirrors of some DRI state
    */
   struct radeon_dri_mirror dri;

   radeonScreenPtr radeonScreen;	/* Screen private DRI data */
   RADEONSAREAPrivPtr sarea;		/* Private SAREA data */

   GLboolean debugFallbacks;

   /* Performance counters
    */
   GLuint boxes;			/* Draw performance boxes */
   GLuint hardwareWentIdle;
   GLuint c_clears;
   GLuint c_drawWaits;
   GLuint c_textureSwaps;
   GLuint c_textureBytes;
   GLuint c_vertexBuffers;
};

#define RADEON_CONTEXT(ctx)		((radeonContextPtr)(ctx->DriverCtx))


static __inline GLuint radeonPackColor( GLuint cpp,
					GLubyte r, GLubyte g,
					GLubyte b, GLubyte a )
{
   switch ( cpp ) {
   case 2:
      return PACK_COLOR_565( r, g, b );
   case 4:
      return PACK_COLOR_8888( a, r, g, b );
   default:
      return 0;
   }
}


/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		0

#if DO_DEBUG
extern int RADEON_DEBUG;
#else
#define RADEON_DEBUG		0
#endif

#define DEBUG_ALWAYS_SYNC	0x01
#define DEBUG_VERBOSE_API	0x02
#define DEBUG_VERBOSE_MSG	0x04
#define DEBUG_VERBOSE_LRU	0x08
#define DEBUG_VERBOSE_DRI	0x10
#define DEBUG_VERBOSE_IOCTL	0x20
#define DEBUG_VERBOSE_2D	0x40
#define DEBUG_VERBOSE_TEXTURE	0x80

#endif
#endif /* __RADEON_CONTEXT_H__ */
