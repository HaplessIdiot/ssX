/* xf86drmRadeon.h -- OS-independent header for Radeon DRM user-level
 *                    library interface
 *
 * Copyright 2000 VA Linux Systems, Inc., Fremont, California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author:
 *   Gareth Hughes <gareth@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *   Keith Whitwell <keith_whitwell@yahoo.com>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86drmRadeon.h,v 1.6 2001/04/16 15:02:13 tsi Exp $
 *
 */

#ifndef _XF86DRI_RADEON_H_
#define _XF86DRI_RADEON_H_

/* WARNING: If you change any of these defines, make sure to change
 * the kernel include file as well (radeon_drm.h)
 */

#define RADEON_FRONT	0x1
#define RADEON_BACK	0x2
#define RADEON_DEPTH	0x4
#define RADEON_STENCIL	0x8



typedef struct {
   unsigned long sarea_priv_offset;
   int is_pci;
   int cp_mode;
   int agp_size;
   int ring_size;
   int usec_timeout;

   unsigned int fb_bpp;
   unsigned int front_offset, front_pitch;
   unsigned int back_offset, back_pitch;
   unsigned int depth_bpp;
   unsigned int depth_offset, depth_pitch;

   unsigned long fb_offset;
   unsigned long mmio_offset;
   unsigned long ring_offset;
   unsigned long ring_rptr_offset;
   unsigned long buffers_offset;
   unsigned long agp_textures_offset;
} drmRadeonInit;

typedef struct {
   unsigned int x;
   unsigned int y;
   unsigned int width;
   unsigned int height;
   void *data;
} drmRadeonTexImage;


#define RADEON_MAX_TEXTURE_UNITS 3

/* Layout matches drm_radeon_state_t in linux drm_radeon.h.  
 */
typedef struct {
	struct {
		unsigned int pp_misc;				/* 0x1c14 */
		unsigned int pp_fog_color;
		unsigned int re_solid_color;
		unsigned int rb3d_blendcntl;
		unsigned int rb3d_depthoffset;
		unsigned int rb3d_depthpitch;
		unsigned int rb3d_zstencilcntl;
		unsigned int pp_cntl;				/* 0x1c38 */
		unsigned int rb3d_cntl;
		unsigned int rb3d_coloroffset;
		unsigned int re_width_height;
		unsigned int rb3d_colorpitch;
	} context;
	struct {
		unsigned int se_cntl;
	} setup1;
	struct {
		unsigned int se_coord_fmt;			/* 0x1c50 */
	} vertex;
	struct {
		unsigned int re_line_pattern;			/* 0x1cd0 */
		unsigned int re_line_state;
		unsigned int se_line_width;			/* 0x1db8 */
	} line;
	struct {
		unsigned int pp_lum_matrix;			/* 0x1d00 */
		unsigned int pp_rot_matrix_0;			/* 0x1d58 */
		unsigned int pp_rot_matrix_1;
	} bumpmap;
	struct {
		unsigned int rb3d_stencilrefmask;		/* 0x1d7c */
		unsigned int rb3d_ropcntl;
		unsigned int rb3d_planemask;
	} mask;
	struct {
		unsigned int se_vport_xscale;			/* 0x1d98 */
		unsigned int se_vport_xoffset;
		unsigned int se_vport_yscale;
		unsigned int se_vport_yoffset;
		unsigned int se_vport_zscale;
		unsigned int se_vport_zoffset;
	} viewport;
	struct {
		unsigned int se_cntl_status;			/* 0x2140 */
	} setup2;
	struct {
		unsigned int re_top_left;	/*ignored*/	/* 0x26c0 */
		unsigned int re_misc;
	} misc;
	struct {
		unsigned int pp_txfilter;
		unsigned int pp_txformat;
		unsigned int pp_txoffset;
		unsigned int pp_txcblend;
		unsigned int pp_txablend;
		unsigned int pp_tfactor;
		unsigned int pp_border_color;
	} texture[RADEON_MAX_TEXTURE_UNITS];
	struct {
		unsigned int se_zbias_factor; 
		unsigned int se_zbias_constant;
	} zbias;
	unsigned int dirty;
} drmRadeonState;


typedef struct {
	unsigned int start;
	unsigned int finish;
	unsigned int prim:8;
	unsigned int stateidx:8;
	unsigned int numverts:16; /* overloaded as offset/64 for elt prims */
        unsigned int vc_format;
} drmRadeonPrim;

#define RADEON_MAX_STATES 16
#define RADEON_MAX_PRIMS  64

extern int drmRadeonInitCP( int fd, drmRadeonInit *info );
extern int drmRadeonCleanupCP( int fd );

extern int drmRadeonStartCP( int fd );
extern int drmRadeonStopCP( int fd );
extern int drmRadeonResetCP( int fd );
extern int drmRadeonWaitForIdleCP( int fd );

extern int drmRadeonEngineReset( int fd );

extern int drmRadeonFullScreen( int fd, int enable );

extern int drmRadeonSwapBuffers( int fd );
extern int drmRadeonClear( int fd, unsigned int flags,
			   unsigned int clear_color, unsigned int clear_depth,
			   unsigned int color_mask, unsigned int stencil,
			   void *boxes, int nbox );

/* Obsolete
 */
extern int drmRadeonFlushVertexBuffer( int fd, int prim, int indx,
				       int count, int discard );
/* Obsolete
 */
extern int drmRadeonFlushIndices( int fd, int prim, int indx,
				  int start, int end, int discard );

/* Replaces FlushVertexBuffer, FlushIndices
 */
extern int drmRadeonFlushPrims( int fd, int indx,
				int discard, 
				int nr_states, 
				drmRadeonState *state, 
				int nr_prims,
				drmRadeonPrim *prim );

extern int drmRadeonLoadTexture( int fd, int offset, int pitch, int format,
				 int width, int height,
				 drmRadeonTexImage *image );

extern int drmRadeonPolygonStipple( int fd, unsigned int *mask );

extern int drmRadeonFlushIndirectBuffer( int fd, int indx,
					 int start, int end, int discard );

#endif
