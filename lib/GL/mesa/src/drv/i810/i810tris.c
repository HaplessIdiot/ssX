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

#include <stdio.h>
#include <math.h>

#include "types.h"
#include "vb.h"
#include "pipeline.h"

#include "mm.h"
#include "i810tris.h"
#include "i810vb.h"
#include "i810log.h"

/* Used in i810tritmp.h
 */
#define I810_COLOR(to, from) {			\
  (to)[0] = (from)[2];				\
  (to)[1] = (from)[1];				\
  (to)[2] = (from)[0];				\
  (to)[3] = (from)[3];				\
}



static triangle_func tri_tab[0x20];   
static quad_func     quad_tab[0x20];  
static line_func     line_tab[0x20];  
static points_func   points_tab[0x20];


#define IND (0)
#define TAG(x) x
#include "i810tritmp.h"

#define IND (I810_FLAT_BIT)
#define TAG(x) x##_flat
#include "i810tritmp.h"

#define IND (I810_OFFSET_BIT)	/* wide */
#define TAG(x) x##_offset
#include "i810tritmp.h"

#define IND (I810_OFFSET_BIT|I810_FLAT_BIT) /* wide|flat */
#define TAG(x) x##_offset_flat
#include "i810tritmp.h"

#define IND (I810_TWOSIDE_BIT)	/* stipple */
#define TAG(x) x##_twoside
#include "i810tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_FLAT_BIT) /* stipple|flat */
#define TAG(x) x##_twoside_flat
#include "i810tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_OFFSET_BIT) /* stipple|wide */
#define TAG(x) x##_twoside_offset
#include "i810tritmp.h"

#define IND (I810_TWOSIDE_BIT|I810_OFFSET_BIT|I810_FLAT_BIT) /* stip|wide|flat*/
#define TAG(x) x##_twoside_offset_flat
#include "i810tritmp.h"

void i810DDTrifuncInit()
{
   init();
   init_flat();
   init_offset();
   init_offset_flat();
   init_twoside();
   init_twoside_flat();
   init_twoside_offset();
   init_twoside_offset_flat();
}




void i810DDChooseRenderState( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLuint flags = ctx->TriangleCaps;

   imesa->IndirectTriangles = 0;

   if (flags) {
      GLuint ind = 0;
      GLuint shared = 0;

      if (flags & DD_FLATSHADE)                    shared |= I810_FLAT_BIT;
      if (flags & DD_MULTIDRAW)                    shared |= I810_FALLBACK_BIT;
      if (flags & DD_SELECT)                       shared |= I810_FALLBACK_BIT;
      if (flags & DD_FEEDBACK)                     shared |= I810_FALLBACK_BIT;
      if (flags & DD_STENCIL)                      shared |= I810_FALLBACK_BIT;

      imesa->renderindex = shared;
      imesa->PointsFunc = points_tab[shared]; 

      ind = shared;
      if (flags & DD_LINE_WIDTH)                    ind |= I810_WIDE_LINE_BIT;
      if (flags & DD_LINE_STIPPLE)                  ind |= I810_FALLBACK_BIT;

      imesa->renderindex |= ind;
      imesa->LineFunc = line_tab[ind];
      if (ind & I810_FALLBACK_BIT) 
	 imesa->IndirectTriangles |= DD_LINE_SW_RASTERIZE;

      ind = shared;
      if (flags & DD_TRI_OFFSET)                    ind |= I810_OFFSET_BIT;
      if (flags & DD_TRI_LIGHT_TWOSIDE)             ind |= I810_TWOSIDE_BIT;
      if (flags & DD_TRI_UNFILLED)                  ind |= I810_FALLBACK_BIT;
      if ((flags & DD_TRI_STIPPLE) &&
	  (ctx->IndirectTriangles & DD_TRI_STIPPLE)) ind |= I810_FALLBACK_BIT;	

      imesa->renderindex |= ind;
      imesa->TriangleFunc = tri_tab[ind];
      imesa->QuadFunc = quad_tab[ind];
      if (ind & I810_FALLBACK_BIT)
	 imesa->IndirectTriangles |= (DD_TRI_SW_RASTERIZE|DD_QUAD_SW_RASTERIZE);
   } 
   else if (imesa->renderindex)
   {
      imesa->renderindex  = 0;
      imesa->PointsFunc   = points_tab[0]; 
      imesa->LineFunc     = line_tab[0];
      imesa->TriangleFunc = tri_tab[0];
      imesa->QuadFunc     = quad_tab[0];
   }


   if (I810_DEBUG&DEBUG_VERBOSE_API) {
      gl_print_tri_caps("tricaps", ctx->TriangleCaps);
   }
}



