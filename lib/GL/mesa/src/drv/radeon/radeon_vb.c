/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_vb.c,v 1.5 2002/02/22 21:45:01 dawes Exp $ */
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
 *   Keith Whitwell <keithw@valinux.com>
 *
 */

#include "glheader.h"
#include "mtypes.h"
#include "colormac.h"
#include "mem.h"
#include "mmath.h"
#include "macros.h"

#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "radeon_context.h"
#include "radeon_vb.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/


#define RADEON_XYZW_BIT		0x01
#define RADEON_RGBA_BIT		0x02
#define RADEON_FOG_BIT		0x04
#define RADEON_SPEC_BIT		0x08
#define RADEON_TEX0_BIT		0x10
#define RADEON_TEX1_BIT		0x20
#define RADEON_PTEX_BIT		0x40
#define RADEON_MAX_SETUP	0x80

static struct {
   void                (*emit)( GLcontext *, GLuint, GLuint, void *, GLuint );
   interp_func		interp;
   copy_pv_func	        copy_pv;
   GLboolean           (*check_tex_sizes)( GLcontext *ctx );
   GLuint               vertex_size;
   GLuint               vertex_stride_shift;
   GLuint               vertex_format;
} setup_tab[RADEON_MAX_SETUP];


#define TINY_VERTEX_FORMAT	        (RADEON_CP_VC_FRMT_XY |		\
					 RADEON_CP_VC_FRMT_Z |		\
					 RADEON_CP_VC_FRMT_PKCOLOR)

#define NOTEX_VERTEX_FORMAT	        (RADEON_CP_VC_FRMT_XY |		\
					 RADEON_CP_VC_FRMT_Z |		\
					 RADEON_CP_VC_FRMT_W0 |		\
					 RADEON_CP_VC_FRMT_PKCOLOR |	\
					 RADEON_CP_VC_FRMT_PKSPEC)

#define TEX0_VERTEX_FORMAT	        (RADEON_CP_VC_FRMT_XY |		\
					 RADEON_CP_VC_FRMT_Z |		\
					 RADEON_CP_VC_FRMT_W0 |		\
					 RADEON_CP_VC_FRMT_PKCOLOR |	\
					 RADEON_CP_VC_FRMT_PKSPEC |	\
					 RADEON_CP_VC_FRMT_ST0)

#define TEX1_VERTEX_FORMAT	        (RADEON_CP_VC_FRMT_XY |		\
					 RADEON_CP_VC_FRMT_Z |		\
					 RADEON_CP_VC_FRMT_W0 |		\
					 RADEON_CP_VC_FRMT_PKCOLOR |	\
					 RADEON_CP_VC_FRMT_PKSPEC |	\
					 RADEON_CP_VC_FRMT_ST0 |	\
					 RADEON_CP_VC_FRMT_ST1)

#define PROJ_TEX1_VERTEX_FORMAT	        (RADEON_CP_VC_FRMT_XY |		\
					 RADEON_CP_VC_FRMT_Z |		\
					 RADEON_CP_VC_FRMT_W0 |		\
					 RADEON_CP_VC_FRMT_PKCOLOR |	\
					 RADEON_CP_VC_FRMT_PKSPEC |	\
					 RADEON_CP_VC_FRMT_ST0 |	\
					 RADEON_CP_VC_FRMT_Q0 |         \
					 RADEON_CP_VC_FRMT_ST1 |	\
					 RADEON_CP_VC_FRMT_Q1)

#define TEX2_VERTEX_FORMAT 0
#define TEX3_VERTEX_FORMAT 0
#define PROJ_TEX3_VERTEX_FORMAT 0

#define DO_XYZW (IND & RADEON_XYZW_BIT)
#define DO_RGBA (IND & RADEON_RGBA_BIT)
#define DO_SPEC (IND & RADEON_SPEC_BIT)
#define DO_FOG  (IND & RADEON_FOG_BIT)
#define DO_TEX0 (IND & RADEON_TEX0_BIT)
#define DO_TEX1 (IND & RADEON_TEX1_BIT)
#define DO_TEX2 0
#define DO_TEX3 0
#define DO_PTEX (IND & RADEON_PTEX_BIT)

#define VERTEX radeonVertex
#define GET_VIEWPORT_MAT() 0
#define GET_TEXSOURCE(n)  n
#define GET_VERTEX_FORMAT() RADEON_CONTEXT(ctx)->vertex_format
#define GET_VERTEX_STORE() (GLubyte *)(RADEON_CONTEXT(ctx)->verts)
#define GET_VERTEX_STRIDE_SHIFT() RADEON_CONTEXT(ctx)->vertex_stride_shift
#define GET_UBYTE_COLOR_STORE() &RADEON_CONTEXT(ctx)->UbyteColor
#define GET_UBYTE_SPEC_COLOR_STORE() &RADEON_CONTEXT(ctx)->UbyteSecondaryColor

#define HAVE_HW_VIEWPORT    1
#define HAVE_HW_DIVIDE      (IND & ~(RADEON_XYZW_BIT|RADEON_RGBA_BIT))
#define HAVE_RGBA_COLOR     1
#define HAVE_TINY_VERTICES  1
#define HAVE_NOTEX_VERTICES 1
#define HAVE_TEX0_VERTICES  1
#define HAVE_TEX1_VERTICES  1
#define HAVE_TEX2_VERTICES  0
#define HAVE_TEX3_VERTICES  0
#define HAVE_PTEX_VERTICES  1

#define CHECK_HW_DIVIDE    (!(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE| \
                                                    DD_TRI_UNFILLED)))


#define IMPORT_FLOAT_COLORS radeon_import_float_colors
#define IMPORT_FLOAT_SPEC_COLORS radeon_import_float_spec_colors

#define INTERP_VERTEX setup_tab[RADEON_CONTEXT(ctx)->SetupIndex].interp
#define COPY_PV_VERTEX setup_tab[RADEON_CONTEXT(ctx)->SetupIndex].copy_pv


/***********************************************************************
 *         Generate  pv-copying and translation functions              *
 ***********************************************************************/

#define TAG(x) radeon_##x
#define IND ~0
#include "tnl_dd/t_dd_vb.c"
#undef IND


/***********************************************************************
 *             Generate vertex emit and interp functions               *
 ***********************************************************************/

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT)
#define TAG(x) x##_wg
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_SPEC_BIT)
#define TAG(x) x##_wgs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_TEX0_BIT)
#define TAG(x) x##_wgt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_TEX0_BIT|RADEON_PTEX_BIT)
#define TAG(x) x##_wgpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_TEX0_BIT|RADEON_TEX1_BIT)
#define TAG(x) x##_wgt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_TEX0_BIT|RADEON_TEX1_BIT|\
             RADEON_PTEX_BIT)
#define TAG(x) x##_wgpt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_SPEC_BIT|RADEON_TEX0_BIT)
#define TAG(x) x##_wgst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_SPEC_BIT|RADEON_TEX0_BIT|\
	     RADEON_PTEX_BIT)
#define TAG(x) x##_wgspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_SPEC_BIT|RADEON_TEX0_BIT|\
             RADEON_TEX1_BIT)
#define TAG(x) x##_wgst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_SPEC_BIT|RADEON_TEX0_BIT|\
             RADEON_TEX1_BIT|RADEON_PTEX_BIT)
#define TAG(x) x##_wgspt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_FOG_BIT)
#define TAG(x) x##_wgf
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_FOG_BIT|RADEON_SPEC_BIT)
#define TAG(x) x##_wgfs
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_FOG_BIT|RADEON_TEX0_BIT)
#define TAG(x) x##_wgft0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_FOG_BIT|RADEON_TEX0_BIT|\
	     RADEON_PTEX_BIT)
#define TAG(x) x##_wgfpt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_FOG_BIT|RADEON_TEX0_BIT|\
             RADEON_TEX1_BIT)
#define TAG(x) x##_wgft0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_FOG_BIT|RADEON_TEX0_BIT|\
             RADEON_TEX1_BIT|RADEON_PTEX_BIT)
#define TAG(x) x##_wgfpt0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_FOG_BIT|RADEON_SPEC_BIT|\
	     RADEON_TEX0_BIT)
#define TAG(x) x##_wgfst0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_FOG_BIT|RADEON_SPEC_BIT|\
	     RADEON_TEX0_BIT|RADEON_PTEX_BIT)
#define TAG(x) x##_wgfspt0
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_FOG_BIT|RADEON_SPEC_BIT|\
	     RADEON_TEX0_BIT|RADEON_TEX1_BIT)
#define TAG(x) x##_wgfst0t1
#include "tnl_dd/t_dd_vbtmp.h"

#define IND (RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_FOG_BIT|RADEON_SPEC_BIT|\
	     RADEON_TEX0_BIT|RADEON_TEX1_BIT|RADEON_PTEX_BIT)
#define TAG(x) x##_wgfspt0t1
#include "tnl_dd/t_dd_vbtmp.h"


/* Specialized emit, hardwired with q3 strides.
 */
static void emit_q3( GLcontext *ctx,
		     GLuint start, GLuint end,
		     void *dest,
		     GLuint stride )
{
   LOCALVARS
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint *coord;
   GLuint coord_stride;
   int i;

   if (CHECK_HW_DIVIDE) {
      coord = (GLuint *)VB->ClipPtr->data;
      coord_stride = VB->ClipPtr->stride;
   }
   else {
      coord = (GLuint *)VB->ProjectedClipPtr->data;
      coord_stride = VB->ProjectedClipPtr->stride;
   }

   if (VB->ColorPtr[0]->Type != GL_UNSIGNED_BYTE)
      IMPORT_FLOAT_COLORS( ctx );

   if (coord_stride != 4 * sizeof(GLfloat) ||
       VB->TexCoordPtr[0]->stride != 2 * sizeof(GLfloat) ||
       VB->ColorPtr[0]->StrideB != 4 * sizeof(GLubyte) ||
       start != 0) {      
      emit_wgt0( ctx, start, end, dest, stride );
      return;
   }

   ASSERT(stride == 8*sizeof(GLuint));

   {
      GLuint *tc0 = (GLuint *)VB->TexCoordPtr[0]->data;
      GLuint *col = (GLuint *)VB->ColorPtr[0]->Ptr;
      GLuint *v = (GLuint *)dest;
      
      for (i=end+1; --i ; ) {
	 GLuint *x;
  	 x = coord; coord += 4; 
	 v[0] = x[0];
	 v[1] = x[1];
	 v[2] = x[2];
	 v[3] = x[3];
	 v[4] = *col++;
  	 x = tc0; tc0 += 2; 
	 v[6] = x[0];
	 v[7] = x[1]; 
	 v += 8;
      }
   }
}



/***********************************************************************
 *                         Initialization 
 ***********************************************************************/

static void init_setup_tab( void )
{
   init_wg();
   init_wgs();
   init_wgt0();
   init_wgpt0();
   init_wgt0t1();
   init_wgpt0t1();
   init_wgst0();
   init_wgspt0();
   init_wgst0t1();
   init_wgspt0t1();
   init_wgf();
   init_wgfs();
   init_wgft0();
   init_wgfpt0();
   init_wgft0t1();
   init_wgfpt0t1();
   init_wgfst0();
   init_wgfspt0();
   init_wgfst0t1();
   init_wgfspt0t1();

   setup_tab[RADEON_XYZW_BIT|RADEON_RGBA_BIT|RADEON_TEX0_BIT].emit = emit_q3;
}



void radeonPrintSetupFlags(char *msg, GLuint flags )
{
   fprintf(stderr, "%s(%x): %s%s%s%s%s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & RADEON_XYZW_BIT)      ? " xyzw," : "",
	   (flags & RADEON_RGBA_BIT)     ? " rgba," : "",
	   (flags & RADEON_SPEC_BIT)     ? " spec," : "",
	   (flags & RADEON_FOG_BIT)      ? " fog," : "",
	   (flags & RADEON_TEX0_BIT)     ? " tex-0," : "",
	   (flags & RADEON_TEX1_BIT)     ? " tex-1," : "",
	   (flags & RADEON_PTEX_BIT)     ? " proj-tex," : "");
}


void radeonCheckTexSizes( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   radeonContextPtr rmesa = RADEON_CONTEXT( ctx );

   if (!setup_tab[rmesa->SetupIndex].check_tex_sizes(ctx)) {
      GLuint ind = rmesa->SetupIndex |= (RADEON_PTEX_BIT|RADEON_RGBA_BIT);

      /* Radeon handles projective textures nicely; just have to change
       * up to the new vertex format.
       */
      if (setup_tab[ind].vertex_format != rmesa->vertex_format) {
	 RADEON_STATECHANGE(rmesa, 0);
	 rmesa->vertex_format = setup_tab[ind].vertex_format;
	 rmesa->vertex_size = setup_tab[ind].vertex_size;
	 rmesa->vertex_stride_shift = setup_tab[ind].vertex_stride_shift;
      }

      if (!(ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 tnl->Driver.Render.Interp = setup_tab[rmesa->SetupIndex].interp;
	 tnl->Driver.Render.CopyPV = setup_tab[rmesa->SetupIndex].copy_pv;
      }
   }
}


void radeonBuildVertices( GLcontext *ctx, GLuint start, GLuint count,
			   GLuint newinputs )
{
   radeonContextPtr rmesa = RADEON_CONTEXT( ctx );
   GLubyte *v = ((GLubyte *)rmesa->verts + (start<<rmesa->vertex_stride_shift));
   GLuint stride = 1<<rmesa->vertex_stride_shift;

   newinputs |= rmesa->SetupNewInputs;
   rmesa->SetupNewInputs = 0;

   if (!newinputs)
      return;

   /* No longer try to repair vertices.
    */
   setup_tab[rmesa->SetupIndex].emit( ctx, start, count, v, stride );
}

void radeonChooseVertexState( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint ind = (RADEON_XYZW_BIT | RADEON_RGBA_BIT);

   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
      ind |= RADEON_SPEC_BIT;

   if (ctx->Fog.Enabled)
      ind |= RADEON_FOG_BIT;

   if (ctx->Texture._ReallyEnabled & 0x0f0)
      ind |= RADEON_TEX0_BIT|RADEON_TEX1_BIT;
   else if (ctx->Texture._ReallyEnabled & 0x00f)
      ind |= RADEON_TEX0_BIT;

   rmesa->SetupIndex = ind;

   if (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED)) {
      /* In these modes the hardware vertices don't contain all the
       * necessary information for interpolation (edgeflag,
       * back-colors).
       *
       * Wrap the standard functions with ones that also interpolate
       * the back colors and edgeflags.
       */
      tnl->Driver.Render.Interp = radeon_interp_extras;
      tnl->Driver.Render.CopyPV = radeon_copy_pv_extras;
   }
   else {
      tnl->Driver.Render.Interp = setup_tab[ind].interp;
      tnl->Driver.Render.CopyPV = setup_tab[ind].copy_pv;
   }


   if (setup_tab[ind].vertex_format != rmesa->vertex_format) {
      RADEON_STATECHANGE(rmesa, 0);
      rmesa->vertex_format = setup_tab[ind].vertex_format;
      rmesa->vertex_size = setup_tab[ind].vertex_size;
      rmesa->vertex_stride_shift = setup_tab[ind].vertex_stride_shift;
   }

   {
      GLuint se_coord_fmt, needproj;

      /* HW perspective divide is a win, but tiny vertex formats are a
       * bigger one.
       */
      if (setup_tab[ind].vertex_format == TINY_VERTEX_FORMAT ||
	  (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
	 needproj = GL_TRUE;
	 se_coord_fmt = (RADEON_VTX_XY_PRE_MULT_1_OVER_W0 |
			 RADEON_VTX_Z_PRE_MULT_1_OVER_W0 |
			 RADEON_TEX1_W_ROUTING_USE_Q1);
      }
      else {
	 needproj = GL_FALSE;
	 se_coord_fmt = (RADEON_VTX_W0_IS_NOT_1_OVER_W0 |
			 RADEON_TEX1_W_ROUTING_USE_Q1);
      }

      if ( se_coord_fmt != rmesa->state.hw.vertex.se_coord_fmt ) {
	 RADEON_STATECHANGE(rmesa, RADEON_UPLOAD_VERTFMT);
	 rmesa->state.hw.vertex.se_coord_fmt = se_coord_fmt;
	 _tnl_need_projected_coords( ctx, needproj );
      }
   }
}



void radeon_emit_contiguous_verts( GLcontext *ctx, GLuint start, GLuint count )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint vertex_size = rmesa->vertex_size * 4;
   CARD32 *dest = radeonAllocDmaLow( rmesa, (count-start)*vertex_size,
				     __FUNCTION__ );
   rmesa->num_verts += count - start;
   setup_tab[rmesa->SetupIndex].emit( ctx, start, count, dest, vertex_size );
}


void radeon_emit_indexed_verts( GLcontext *ctx, GLuint start, GLuint count )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint vertex_size = rmesa->vertex_size * 4;
   GLuint bufsz = (count-start) * vertex_size;
   CARD32 *dest;

   rmesa->dma.low = (rmesa->dma.low + 63) & ~63; /* alignment */
   rmesa->dma.last = rmesa->dma.low;

   dest = radeonAllocDmaLow( rmesa, bufsz, __FUNCTION__);
   setup_tab[rmesa->SetupIndex].emit( ctx, start, count, dest, vertex_size );

   rmesa->dma.retained = rmesa->dma.buffer;
   rmesa->dma.offset = (rmesa->dma.buffer->idx * RADEON_BUFFER_SIZE +
			rmesa->dma.low - bufsz);

   rmesa->dma.low = (rmesa->dma.low + 0x7) & ~0x7;  /* alignment */
   rmesa->dma.last = rmesa->dma.low;
}


void radeonInitVB( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint size = TNL_CONTEXT(ctx)->vb.Size;
   rmesa->verts = (char *)ALIGN_MALLOC( size * 16 * 4, 32 );

   {
      static int firsttime = 1;
      if (firsttime) {
	 init_setup_tab();
	 firsttime = 0;
      }
   }
}


void radeonFreeVB( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   if (rmesa->verts) {
      ALIGN_FREE(rmesa->verts);
      rmesa->verts = 0;
   }

   if (rmesa->UbyteSecondaryColor.Ptr) {
      ALIGN_FREE(rmesa->UbyteSecondaryColor.Ptr);
      rmesa->UbyteSecondaryColor.Ptr = 0;
   }

   if (rmesa->UbyteColor.Ptr) {
      ALIGN_FREE(rmesa->UbyteColor.Ptr);
      rmesa->UbyteColor.Ptr = 0;
   }
}
