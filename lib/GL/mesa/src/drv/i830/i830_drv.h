/**************************************************************************

Copyright 2001 VA Linux Systems Inc., Fremont, California.

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

/* $XFree86$ */

/*
 * Author:
 *   Jeff Hartmann <jhartmann@2d3d.com>
 *
 * Heavily based on the I810 driver, which was written by:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef __I830_MESA_DRV_H__
#define __I830_MESA_DRV_H__

#define DEBUGGING 0

#define TRACE_IN 	{\
   if (DEBUGGING)	\
	 fprintf(stderr,"\nWe are IN function %s on line %d\n",__FUNCTION__,__LINE__);\
} 
#define TRACE_OUT 	{\
   if (DEBUGGING)	\
	 fprintf(stderr,"\nWe are OUT OF function %s on line %d\n",__FUNCTION__,__LINE__);\
} 

   


/* To remove all debugging, make sure I830_DEBUG is defined as a
 * preprocessor symbol, and equal to zero.  
 */
#define I830_DEBUG 0/*DEBUG_VERBOSE_2D|DEBUG_VERBOSE_STATE|DEBUG_VERBOSE_TRACE|DEBUG_VERBOSE_IOCTL|DEBUG_VERBOSE_DRI|DEBUG_VERBOSE_LRU|DEBUG_VALIDATE_RING|DEBUG_VERBOSE_API|DEBUG_VERBOSE_MSG*/
#ifndef I830_DEBUG
#warning "Debugging enabled - expect reduced performance"
extern int I830_DEBUG;
#endif

#define DEBUG_VERBOSE_2D     0x1
#define DEBUG_VERBOSE_RING   0x8
#define DEBUG_VERBOSE_OUTREG 0x10
#define DEBUG_ALWAYS_SYNC    0x40
#define DEBUG_VERBOSE_MSG    0x80
#define DEBUG_NO_OUTRING     0x100
#define DEBUG_NO_OUTREG      0x200
#define DEBUG_VERBOSE_API    0x400
#define DEBUG_VALIDATE_RING  0x800
#define DEBUG_VERBOSE_LRU    0x1000
#define DEBUG_VERBOSE_DRI    0x2000
#define DEBUG_VERBOSE_IOCTL  0x4000
#define DEBUG_VERBOSE_TRACE  0x8000
#define DEBUG_VERBOSE_STATE  0x10000

#define DV_PF_555  (1<<8)
#define DV_PF_565  (2<<8)
#define DV_PF_8888 (3<<8)

/* Reasons to fallback on all primitives.  (see also
 * imesa->IndirectTriangles).  
 */
/*
#define I830_FALLBACK_TEXTURE        0x1
#define I830_FALLBACK_DRAW_BUFFER    0x2
#define I830_FALLBACK_READ_BUFFER    0x4
#define I830_FALLBACK_STIPPLE        0x8  
#define I830_FALLBACK_SPECULAR       0x10
#define I830_FALLBACK_STENCIL	     0x20
#define I830_FALLBACK_COLORMASK      0x40
*/

/* for i830ctx.new_state - manage GL->driver state changes
 */
#define I830_NEW_TEXTURE 0x1

#define I830_TEX_MAXLEVELS 10


#define I830_NO_PALETTE        0x0
#define I830_USE_PALETTE       0x1
#define I830_UPDATE_PALETTE    0x2
#define I830_FALLBACK_PALETTE  0x4

/*#define I830_SPEC_BIT	0x1
#define I830_FOG_BIT	0x2*/
#define I830_ALPHA_BIT	0x4	/* GL_BLEND, not used */
/*#define I830_TEX0_BIT	0x8
#define I830_TEX1_BIT	0x10*/	
/*#define I830_RGBA_BIT	0x20*/
#define I830_WIN_BIT	0x40


/* For shared texture space managment, these texture objects may also
 * be used as proxies for regions of texture memory containing other
 * client's textures.  Such proxy textures (not to be confused with GL
 * proxy textures) are subject to the same LRU aging we use for our
 * own private textures, and thus we have a mechanism where we can
 * fairly decide between kicking out our own textures and those of
 * other clients.
 *
 * Non-local texture objects have a valid MemBlock to describe the
 * region managed by the other client, and can be identified by
 * 't->globj == 0' 
 */
#define TEX_0	1
#define TEX_1	2

typedef void (*i830_interp_func)( GLfloat t, 
				  GLfloat *result,
				  const GLfloat *in,
				  const GLfloat *out );






#define I830_TEX_UNIT_ENABLED(unit)		(1<<unit)
#define VALID_I830_TEXTURE_OBJECT(tobj)	(tobj)

#define I830_CONTEXT(ctx)	((i830ContextPtr)(ctx->DriverCtx))
#define I830_DRIVER_DATA(vb) ((i830VertexBufferPtr)((vb)->driver_data))
#define GET_DISPATCH_AGE(imesa) imesa->sarea->last_dispatch
#define GET_ENQUEUE_AGE(imesa)	imesa->sarea->last_enqueue

#endif /* __I830_MESA_DRV_H__ */
