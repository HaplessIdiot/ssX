/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_context.h,v 1.1 2000/06/17 00:03:05 martin Exp $ */
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

#ifndef _R128_CONTEXT_H_
#define _R128_CONTEXT_H_

#ifdef GLX_DIRECT_RENDERING

struct r128_context;
typedef struct r128_context r128ContextRec;
typedef struct r128_context *r128ContextPtr;

#include "r128_texobj.h"

/* Flags for what needs to be updated before a new primitive is rendered */
#define R128_CLEAN                  0x0000
#define R128_REQUIRE_QUIESCENCE     0x0001
#define R128_UPDATE_CONTEXT         0x0002
#define R128_UPDATE_WINPOS          0x0004
#define R128_UPDATE_TEX0IMAGES      0x0008
#define R128_UPDATE_TEX1IMAGES      0x0010
#define R128_UPDATE_TEXSTATE        0x0020
#define R128_ALL_DIRTY              0xffff

/* Flags for what context state needs to be updated */
#define R128_CTX_CLEAN              0x0000
#define R128_CTX_MISC               0x0001
#define R128_CTX_ENGINESTATE        0x0002
#define R128_CTX_TEX0STATE          0x0004
#define R128_CTX_TEX1STATE          0x0008
#define R128_CTX_TEXENVSTATE        0x0010
#define R128_CTX_FOGSTATE           0x0020
#define R128_CTX_ZSTENSTATE         0x0080
#define R128_CTX_SCISSORS           0x0100
#define R128_CTX_ALPHASTATE         0x0200
#define R128_CTX_SETUPSTATE         0x0400
#define R128_CTX_WIN_Z_POS          0x0800
#define R128_CTX_FLUSH_PIX_CACHE    0x1000
#define R128_CTX_ALL_DIRTY          0xffff

/* Flags for software fallback cases */
#define R128_FALLBACK_TEXTURE       0x0001
#define R128_FALLBACK_DRAW_BUFFER   0x0002
#define R128_FALLBACK_READ_BUFFER   0x0004
#define R128_FALLBACK_COLORMASK     0x0008
#define R128_FALLBACK_STIPPLE       0x0010
#define R128_FALLBACK_SWONLY        0x0020
#define R128_FALLBACK_RENDER_MODE   0x0040
#define R128_FALLBACK_MULTIDRAW     0x0080
#define R128_FALLBACK_LOGICOP       0x0100

typedef void (*r128InterpFunc)( GLfloat t,
				GLfloat *result,
				const GLfloat *in,
				const GLfloat *out );

/* NOTE: The groups below need to be kept together so that a single
   memcpy can be used to transfer data to the ring buffer */
typedef struct {
    CARD32  scale_3d_cntl;                        /* 0x1a00 */

    CARD32  aux_sc_cntl;                          /* 0x1660 */
    CARD32  aux1_sc_left;
    CARD32  aux1_sc_right;
    CARD32  aux1_sc_top;
    CARD32  aux1_sc_bottom;
    CARD32  aux2_sc_left;
    CARD32  aux2_sc_right;
    CARD32  aux2_sc_top;
    CARD32  aux2_sc_bottom;
    CARD32  aux3_sc_left;
    CARD32  aux3_sc_right;
    CARD32  aux3_sc_top;
    CARD32  aux3_sc_bottom;                       /* 0x1690 */

    CARD32  dst_pitch_offset_c;                   /* 0x1c80 */
    CARD32  dp_gui_master_cntl;
    CARD32  sc_top_left_c;
    CARD32  sc_bottom_right_c;
    CARD32  z_offset_c;
    CARD32  z_pitch_c;
    CARD32  z_sten_cntl_c;
    CARD32  tex_cntl_c;
    CARD32  misc_3d_state_cntl_reg;
    CARD32  texture_clr_cmp_clr_c;
    CARD32  texture_clr_cmp_msk_c;
    CARD32  fog_color_c;
    CARD32  prim_tex_cntl_c;
    CARD32  prim_texture_combine_cntl_c;
    CARD32  tex_size_pitch_c;
    CARD32  prim_tex_offset[R128_TEX_MAXLEVELS];  /* 0x1ce4 */

    CARD32  sec_tex_cntl_c;                       /* 0x1d00 */
    CARD32  sec_tex_combine_cntl_c;
    CARD32  sec_tex_offset[R128_TEX_MAXLEVELS];
    CARD32  constant_color_c;
    CARD32  prim_texture_border_color_c;
    CARD32  sec_texture_border_color_c;
    CARD32  sten_ref_mask_c;
    CARD32  plane_3d_mask_c;                      /* 0x1d44 */

    CARD32  setup_cntl;                           /* 0x1bc4 */

    CARD32  pm4_vc_fpu_setup;                     /* 0x071c */

    CARD32  window_xy_offset;                     /* 0x1bcc */

    CARD32  dp_write_mask;                        /* 0x16cc */

    CARD32  pc_gui_ctlstat;                       /* 0x1748 */
} r128ContextRegs;

struct r128_context {
    GLcontext            *glCtx;	/* Mesa context */
    int                   dirty;	/* Hardware state to be updated */
    int                   dirty_context; /* Context state to be updated */

    int                   vertsize;	/* Size of current vertices */
    CARD32                vc_format;	/* Format of current vertices */
    int                   multitex;
    GLfloat               depth_scale;

    int                   SWfallbackDisable; /* Disable software fallbacks */

    r128TexObjPtr         CurrentTexObj[2]; /* Ptr to current texture
					       object associated with
					       each texture unit */
					/* List of tex swapped in per heap*/
    r128TexObj            TexObjList[R128_NR_TEX_HEAPS];
    r128TexObj            SwappedOut;	/* List of textures swapped out */
    memHeap_t            *texHeap[R128_NR_TEX_HEAPS]; /* Global tex heaps */
					/* Last known global tex heap ages */
    int                   lastTexAge[R128_NR_TEX_HEAPS];

    CARD32                lastSwapAge;  /* Last known swap age */

    int                   Scissor;
    XF86DRIClipRectRec    ScissorRect;  /* Current software scissor */

    int                   useFastPath;	/* Currently using Fast Path code */
    int                   SetupIndex;	/* Raster setup function index */
    int                   SetupDone;	/* Partial raster setup done? */
    int                   RenderIndex;	/* Render state function index */
    r128InterpFunc        interp;	/* Current vert interp function */

    drmBufPtr             vert_buf;	/* VB currently being filled */

#if 0
    drmBufPtr		  elt_buf, retained_buf, index_buf;
    GLushort		 *first_elt, *next_elt, *last_elt;
    GLfloat		 *next_vert, *last_vert;
    CARD32		  next_vert_index;
    CARD32		  first_vert_index;
    struct r128_elt_tab	 *elt_tab;
    GLfloat		  device_matrix[16];
#endif

    points_func           PointsFunc;	/* Current Points, Line, Triangle */
    line_func             LineFunc;	/* and Quad rendering functions */
    triangle_func         TriangleFunc;
    quad_func             QuadFunc;

    CARD32                IndirectTriangles; /* Flags for point, line,
						tri and quad software
						fallbacks */
    CARD32                Fallback;     /* Need software fallback */

    r128ContextRegs       regs;		/* Hardware state */
    CARD32                Color;	/* Current draw color */
    CARD32                ClearColor;	/* Color used to clear color buffer */
    CARD32                ClearDepth;	/* Value used to clear depth buffer */

    int                   drawX;	/* x-offset to current draw buffer */
    int                   drawY;	/* y-offset to current draw buffer */

    int                   readX;	/* x-offset to current read buffer */
    int                   readY;	/* y-offset to current read buffer */

    CARD32               *CCEbuf;	/* buffer to submit to CCE */
    int                   CCEcount;	/* number of dwords in CCEbuf */

    int                   CCEtimeout;   /* number of times to loop
					   before exiting */

    Display              *display;	/* X server display */

    __DRIcontextPrivate  *driContext;	/* DRI context */
    __DRIdrawablePrivate *driDrawable;	/* DRI drawable bound to this ctx */

    r128ScreenPtr         r128Screen;	/* Screen private DRI data */

    /* Performance counters: */
    int                   boxes;	/* Draw performance boxes */
    int			  hardwareWentIdle;
    int			  c_clears;
    int			  c_drawWaits;
    int			  c_textureSwaps;
    int			  c_textureBytes;
    int			  c_vertexBuffers;
};

#define R128_CONTEXT(ctx)		((r128ContextPtr)(ctx->DriverCtx))

#define R128_MESACTX(r128ctx)		((r128ctx)->glCtx)
#define R128_DRIDRAWABLE(r128ctx)	((r128ctx)->driDrawable)
#define R128_DRISCREEN(r128ctx)		((r128ctx)->r128Screen->driScreen)

extern GLboolean r128CreateContext(Display *dpy, GLvisual *glVisual,
				   __DRIcontextPrivate *driContextPriv);
extern void           r128DestroyContext(r128ContextPtr r128ctx);
extern r128ContextPtr r128MakeCurrent(r128ContextPtr oldCtx,
				      r128ContextPtr newCtx,
				      __DRIdrawablePrivate *dPriv);

#endif
#endif /* _R128_CONTEXT_H_ */
