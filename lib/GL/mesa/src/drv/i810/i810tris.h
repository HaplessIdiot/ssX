/*
 * GLX Hardware Device Driver for Intel i810
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
 *
 */

#ifndef I810TRIS_INC
#define I810TRIS_INC

#include "types.h"
#include "i810ioctl.h"

extern void i810PrintRenderState( const char *msg, GLuint state );
extern void i810DDChooseRenderState(GLcontext *ctx);
extern void i810DDTrifuncInit( void );


/* shared */
#define I810_FLAT_BIT 	     0x1

/* triangle */
#define I810_OFFSET_BIT	     0x2	
#define I810_TWOSIDE_BIT     0x4

/* line */
#define I810_WIDE_LINE_BIT    0x2 
#define I810_STIPPLE_LINE_BIT 0x4 

/* shared */
#define I810_FALLBACK_BIT    0x8 





static i810_vertex __inline__ *i810AllocTriangles( i810ContextPtr imesa, int nr)
{
   GLuint *start = i810AllocDwords( imesa, 30*nr, PR_TRIANGLES );
   return (i810_vertex *)start;
}

static i810_vertex __inline__ *i810AllocLine( i810ContextPtr imesa ) 
{
   GLuint *start = i810AllocDwords( imesa, 20, PR_LINES );
   return (i810_vertex *)start;
}

static i810_vertex __inline__ *i810AllocRect( i810ContextPtr imesa ) 
{
   GLuint *start = i810AllocDwords( imesa, 30, PR_RECTS );
   return (i810_vertex *)start;
}



static void __inline__ i810_draw_triangle( i810ContextPtr imesa,
					   i810_vertex *v0, 
					   i810_vertex *v1, 
					   i810_vertex *v2 )
{
   i810_vertex *wv = i810AllocTriangles( imesa, 1 );
   wv[0] = *v0;
   wv[1] = *v1;
   wv[2] = *v2;
}


static __inline__ void i810_draw_point( i810ContextPtr imesa,
					i810_vertex *tmp, float sz )
{
   i810_vertex *wv = i810AllocTriangles( imesa, 2 );

   wv[0] = *tmp;
   wv[0].x = tmp->x - sz;
   wv[0].y = tmp->y - sz;

   wv[1] = *tmp;
   wv[1].x = tmp->x + sz;
   wv[1].y = tmp->y - sz;

   wv[2] = *tmp;
   wv[2].x = tmp->x + sz;
   wv[2].y = tmp->y + sz;

   wv[3] = *tmp;
   wv[3].x = tmp->x + sz;
   wv[3].y = tmp->y + sz;

   wv[4] = *tmp;
   wv[4].x = tmp->x - sz;
   wv[4].y = tmp->y + sz;

   wv[5] = *tmp;
   wv[5].x = tmp->x - sz;
   wv[5].y = tmp->y - sz;

}


static __inline__ void i810_draw_line_line( i810ContextPtr imesa, 
					    i810_vertex *tmp0, 
					    i810_vertex *tmp1 )
{
   i810_vertex *wv = i810AllocLine( imesa );
   wv[0] = *tmp0;
   wv[1] = *tmp1;
}

static __inline__ void i810_draw_tri_line( i810ContextPtr imesa, 
					   i810_vertex *tmp0, 
					   i810_vertex *tmp1,
					   float width )
{
   i810_vertex *wv = i810AllocTriangles( imesa, 2 );
      
   float dx, dy, ix, iy;

   dx = tmp0->x - tmp1->x;
   dy = tmp0->y - tmp1->y;

   ix = width * .5; iy = 0;
   if (dx * dx > dy * dy) {
      iy = ix; ix = 0;
   }

   wv[0] = *tmp0;
   wv[0].x = tmp0->x - ix;
   wv[0].y = tmp0->y - iy;

   wv[1] = *tmp1;
   wv[1].x = tmp1->x + ix;
   wv[1].y = tmp1->y + iy;

   wv[2] = *tmp0;
   wv[2].x = tmp0->x + ix;
   wv[2].y = tmp0->y + iy;
	 
   wv[3] = *tmp0;
   wv[3].x = tmp0->x - ix;
   wv[3].y = tmp0->y - iy;

   wv[4] = *tmp1;
   wv[4].x = tmp1->x - ix;
   wv[4].y = tmp1->y - iy;

   wv[5] = *tmp1;
   wv[5].x = tmp1->x + ix;
   wv[5].y = tmp1->y + iy;
}


static __inline__ void i810_draw_line( i810ContextPtr imesa, 
				       i810_vertex *tmp0, 
				       i810_vertex *tmp1,
				       float width )
{
   i810_draw_line_line( imesa, tmp0, tmp1 );
}

#endif
