/* $XFree86$ */
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

#if !defined(TAG) || !defined(IND)
    this is an error
#endif

/* Draw a single triangle.  Note that the device-dependent vertex data
   might need to be changed based on the render state. */
static __inline void TAG(triangle)(GLcontext *ctx,
				   GLuint e0, GLuint e1, GLuint e2,
				   GLuint pv)
{
    r128ContextPtr        r128ctx   = R128_CONTEXT(ctx);
    int                   vertsize  = r128ctx->vertsize;
    CARD32               *vb        = r128AllocVertexDwords(r128ctx,
							    3 * vertsize);
    struct vertex_buffer *VB        = ctx->VB;
    r128VertexPtr         r128verts = R128_DRIVER_DATA(VB)->verts;
    const r128Vertex     *v[3];
    int                   i, j;

#if (IND & R128_OFFSET_BIT)
    GLfloat offset = ctx->Polygon.OffsetUnits * r128ctx->depth_scale;
#endif

#if (IND & (R128_FLAT_BIT | R128_TWOSIDE_BIT))
    int c[3];

    c[0] = c[1] = c[2] = *(int *)&r128verts[pv].vert1.dif_argb;
#endif

    v[0] = &r128verts[e0];
    v[1] = &r128verts[e1];
    v[2] = &r128verts[e2];

#if (IND & (R128_TWOSIDE_BIT | R128_OFFSET_BIT))
    {
	GLfloat ex = v[0]->vert1.x - v[2]->vert1.x;
	GLfloat ey = v[0]->vert1.y - v[2]->vert1.y;
	GLfloat fx = v[1]->vert1.x - v[2]->vert1.x;
	GLfloat fy = v[1]->vert1.y - v[2]->vert1.y;
	GLfloat cc  = ex*fy - ey*fx;

#if (IND & R128_TWOSIDE_BIT)
	{
	    GLuint  facing        = (cc > 0.0) ^ ctx->Polygon.FrontBit;
	    GLubyte (*vbcolor)[4] = VB->Color[facing]->data;
	    if (IND & R128_FLAT_BIT) {
		R128_COLOR((char *)&c[0], vbcolor[pv]);
		c[2] = c[1] = c[0];
	    } else {
		R128_COLOR((char *)&c[0], vbcolor[e0]);
		R128_COLOR((char *)&c[1], vbcolor[e1]);
		R128_COLOR((char *)&c[2], vbcolor[e2]);
	    }
	}
#endif

#if (IND & R128_OFFSET_BIT)
	{
	    if (cc * cc > 1e-16) {
		GLfloat factor = ctx->Polygon.OffsetFactor;
		GLfloat ez     = v[0]->vert1.z - v[2]->vert1.z;
		GLfloat fz     = v[1]->vert1.z - v[2]->vert1.z;
		GLfloat a      = ey*fz - ez*fy;
		GLfloat b      = ez*fx - ex*fz;
		GLfloat ic     = 1.0 / cc;
		GLfloat ac     = a * ic;
		GLfloat bc     = b * ic;
		if (ac < 0.0f) ac = -ac;
		if (bc < 0.0f) bc = -bc;
		offset += MAX2(ac, bc) * factor;
	    }
	}
#endif
    }
#endif

    for (j = 0 ; j < 3 ; j++, vb += vertsize) {

       for (i = 0 ; i < vertsize ; i++)
	  vb[i] = v[j]->ui[i];

#if (IND & (R128_FLAT_BIT|R128_TWOSIDE_BIT))
       vb[4] = c[j];		/* color is the fifth element... */
#endif
#if (IND & R128_OFFSET_BIT)
       *(float *)&vb[2] = v[j]->vert1.z + offset;
#endif
   }
}


static void TAG(quad)(GLcontext *ctx,
		      GLuint v0, GLuint v1, GLuint v2, GLuint v3,
		      GLuint pv)
{
    TAG(triangle)(ctx, v0, v1, v3, pv);
    TAG(triangle)(ctx, v1, v2, v3, pv);
}


/* Draw a single line.  Note that the device-dependent vertex data might
   need to be changed based on the render state. */
static void TAG(line)(GLcontext *ctx,
		      GLuint v0, GLuint v1,
		      GLuint pv)
{
    r128ContextPtr  r128ctx   = R128_CONTEXT(ctx);
    r128VertexPtr   r128verts = R128_DRIVER_DATA(ctx->VB)->verts;
    float           width     = ctx->Line.Width;

   if (IND & (R128_TWOSIDE_BIT|R128_FLAT_BIT)) {
      r128Vertex tmp0 = r128verts[v0];
      r128Vertex tmp1 = r128verts[v1];

      if (IND & R128_TWOSIDE_BIT) {
	 GLubyte (*vbcolor)[4] = ctx->VB->ColorPtr->data;

	 if (IND & R128_FLAT_BIT) {
 	    R128_COLOR((char *)&tmp0.vert1.dif_argb, vbcolor[pv]);
	    *(int *)&tmp1.vert1.dif_argb = *(int *)&tmp0.vert1.dif_argb;
	 } else {
	    R128_COLOR((char *)&tmp0.vert1.dif_argb, vbcolor[v0]);
	    R128_COLOR((char *)&tmp1.vert1.dif_argb, vbcolor[v1]);
	 }
      } else {
	 *(int *)&tmp0.vert1.dif_argb = *(int *)&r128verts[pv].vert1.dif_argb;
	 *(int *)&tmp1.vert1.dif_argb = *(int *)&r128verts[pv].vert1.dif_argb;
      }

      r128DrawLineVB( r128ctx, &tmp0, &tmp1, width );
   }
   else
      r128DrawLineVB( r128ctx, &r128verts[v0], &r128verts[v1], width );
}

/* Draw a set of points.  Note that the device-dependent vertex data
   might need to be changed based on the render state. */
static void TAG(points)(GLcontext *ctx,
			GLuint first, GLuint last)
{
    r128ContextPtr        r128ctx   = R128_CONTEXT(ctx);
    struct vertex_buffer *VB        = ctx->VB;
    r128VertexPtr         r128verts = R128_DRIVER_DATA(VB)->verts;
    GLfloat               size      = ctx->Point.Size * 0.5;
    int                   i;

    for(i = first; i <= last; i++) {
	if(VB->ClipMask[i] == 0) {
	   if (IND & R128_TWOSIDE_BIT) {
	      GLubyte (*vbcolor)[4] = VB->ColorPtr->data;
	      r128Vertex tmp0 = r128verts[i];
	      R128_COLOR((char *)&tmp0.vert1.dif_argb, vbcolor[i]);
	      r128DrawPointVB( r128ctx, &tmp0, size );
	   } else
	      r128DrawPointVB( r128ctx, &r128verts[i], size );
	}
    }
}

/* Initialize the table of primitives to render. */
static void TAG(init)(void)
{
    tri_tab[IND]    = TAG(triangle);
    quad_tab[IND]   = TAG(quad);
    line_tab[IND]   = TAG(line);
    points_tab[IND] = TAG(points);
}

#undef IND
#undef TAG
