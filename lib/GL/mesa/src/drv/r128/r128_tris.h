/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_tris.h,v 1.1 2000/06/17 00:03:08 martin Exp $ */
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

#ifndef _R128_TRIS_H_
#define _R128_TRIS_H_

#ifdef GLX_DIRECT_RENDERING

#include "r128_vb.h"

extern void r128DDChooseRenderState(GLcontext *ctx);
extern void r128DDTriangleFuncsInit(void);

#define R128_FLAT_BIT        0x01
#define R128_OFFSET_BIT      0x02
#define R128_TWOSIDE_BIT     0x04
#define R128_FALLBACK_BIT    0x08

/* Draw a triangle from the vertices in the vertex buffer */
static __inline void r128DrawTriangleVB( r128ContextPtr r128ctx,
					 r128Vertex *v0,
					 r128Vertex *v1,
					 r128Vertex *v2 )
{
    int vertsize = r128ctx->vertsize;
    CARD32 *vb = r128AllocVertexDwordsInlined( r128ctx, 3 * vertsize );
    int j;

#if defined (USE_X86_ASM)
    /* GTH: We can safely assume the vertex stride is some number of
     * dwords, and thus a "rep movsd" is okay.  The vb pointer is
     * automagically updated with this instruction, so we don't have
     * to manually take care of incrementing it.
     */
    __asm__ __volatile__( "rep ; movsl"
			  : "=%c" (j)
			  : "0" (vertsize), "D" ((long)vb), "S" ((long)v0)
			  : "memory" );
    __asm__ __volatile__( "rep ; movsl"
			  : "=%c" (j)
			  : "0" (vertsize), "S" ((long)v1)
			  : "memory" );
    __asm__ __volatile__( "rep ; movsl"
			  : "=%c" (j)
			  : "0" (vertsize), "S" ((long)v2)
			  : "memory" );
#else
    for (j = 0 ; j < vertsize ; j++)
        vb[j] = v0->ui[j];

    vb += vertsize;
    for (j = 0 ; j < vertsize ; j++)
        vb[j] = v1->ui[j];

    vb += vertsize;
    for (j = 0 ; j < vertsize ; j++)
        vb[j] = v2->ui[j];
#endif
}

/* Draw a line from the vertices in the vertex buffer */
static __inline void r128DrawLineVB( r128ContextPtr r128ctx,
				     r128Vertex *tmp0,
				     r128Vertex *tmp1,
				     float width )
{
    int vertsize = r128ctx->vertsize;
    CARD32 *vb = r128AllocVertexDwordsInlined( r128ctx, 6 * vertsize );
    float dx, dy, ix, iy;
    int j;

    dx = tmp0->vert1.x - tmp1->vert1.x;
    dy = tmp0->vert1.y - tmp1->vert1.y;

    ix = width * .5; iy = 0;

    if ((ix<.5) && (ix>0.1)) ix = .5; /* I want to see lines with width
					 0.5 also */

    if (dx * dx > dy * dy) {
        iy = ix; ix = 0;
    }

    *(float *)&vb[0] = tmp0->vert1.x - ix;
    *(float *)&vb[1] = tmp0->vert1.y - iy;
    for (j = 2 ; j < vertsize ; j++)
        vb[j] = tmp0->ui[j];
    vb += vertsize;

    *(float *)&vb[0] = tmp1->vert1.x + ix;
    *(float *)&vb[1] = tmp1->vert1.y + iy;
    for (j = 2 ; j < vertsize ; j++)
        vb[j] = tmp1->ui[j];
    vb += vertsize;

    *(float *)&vb[0] = tmp0->vert1.x + ix;
    *(float *)&vb[1] = tmp0->vert1.y + iy;
    for (j = 2 ; j < vertsize ; j++)
        vb[j] = tmp0->ui[j];
    vb += vertsize;

    *(float *)&vb[0] = tmp0->vert1.x - ix;
    *(float *)&vb[1] = tmp0->vert1.y - iy;
    for (j = 2 ; j < vertsize ; j++)
        vb[j] = tmp0->ui[j];
    vb += vertsize;

    *(float *)&vb[0] = tmp1->vert1.x - ix;
    *(float *)&vb[1] = tmp1->vert1.y - iy;
    for (j = 2 ; j < vertsize ; j++)
        vb[j] = tmp1->ui[j];
    vb += vertsize;

    *(float *)&vb[0] = tmp1->vert1.x + ix;
    *(float *)&vb[1] = tmp1->vert1.y + iy;
    for (j = 2 ; j < vertsize ; j++)
        vb[j] = tmp1->ui[j];
}

/* Draw a point from the vertices in the vertex buffer */
static __inline void r128DrawPointVB( r128ContextPtr r128ctx,
				      r128Vertex *tmp, float sz )
{
    int vertsize = r128ctx->vertsize;
    CARD32 *vb = r128AllocVertexDwordsInlined( r128ctx, 6 * vertsize );
    int j;

    *(float *)&vb[0] = tmp->vert1.x - sz;
    *(float *)&vb[1] = tmp->vert1.y - sz;
    for (j = 2 ; j < vertsize ; j++)
	vb[j] = tmp->ui[j];
    vb += vertsize;

    *(float *)&vb[0] = tmp->vert1.x + sz;
    *(float *)&vb[1] = tmp->vert1.y - sz;
    for (j = 2 ; j < vertsize ; j++)
	vb[j] = tmp->ui[j];
    vb += vertsize;

    *(float *)&vb[0] = tmp->vert1.x + sz;
    *(float *)&vb[1] = tmp->vert1.y + sz;
    for (j = 2 ; j < vertsize ; j++)
	vb[j] = tmp->ui[j];
    vb += vertsize;

    *(float *)&vb[0] = tmp->vert1.x + sz;
    *(float *)&vb[1] = tmp->vert1.y + sz;
    for (j = 2 ; j < vertsize ; j++)
	vb[j] = tmp->ui[j];
    vb += vertsize;

    *(float *)&vb[0] = tmp->vert1.x - sz;
    *(float *)&vb[1] = tmp->vert1.y + sz;
    for (j = 2 ; j < vertsize ; j++)
	vb[j] = tmp->ui[j];
    vb += vertsize;

    *(float *)&vb[0] = tmp->vert1.x - sz;
    *(float *)&vb[1] = tmp->vert1.y - sz;
    for (j = 2 ; j < vertsize ; j++)
	vb[j] = tmp->ui[j];
}

#endif
#endif /* _R128_TRIS_H_ */
