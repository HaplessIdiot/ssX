/* Id: rastpos.c,v 1.3 1999/02/26 08:52:36 martin Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Log: rastpos.c,v $
 * Revision 1.3  1999/02/26 08:52:36  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.8  1999/02/24 22:48:07  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.7  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.6  1998/10/31 17:06:15  brianp
 * variety of multi-texture changes
 *
 * Revision 3.5  1998/10/29 02:28:13  brianp
 * incorporated Keith Whitwell's transformation optimizations
 *
 * Revision 3.4  1998/07/30 02:40:41  brianp
 * always feedback texture coords, even when texturing disabled
 *
 * Revision 3.3  1998/02/20 04:50:44  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.2  1998/02/08 20:20:08  brianp
 * added assert.h
 *
 * Revision 3.1  1998/02/02 03:09:34  brianp
 * added GL_LIGHT_MODEL_COLOR_CONTROL (separate specular color interpolation)
 *
 * Revision 3.0  1998/01/31 21:02:29  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/rastpos.c,v 1.0tsi Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <assert.h>
#endif
#include "clip.h"
#include "feedback.h"
#include "light.h"
#include "macros.h"
#include "matrix.h"
#include "mmath.h"
#include "types.h"
#include "xform.h"
#include "context.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif


/*
 * Caller:  context->API.RasterPos4f
 */
void gl_RasterPos4f( GLcontext *ctx,
                     GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GLfloat v[4], eye[4], clip[4], ndc[3], d;
   GLfloat eyenorm[3];

   ASSIGN_4V( v, x, y, z, w );

   /* transform v to eye coords:  eye = ModelView * v */
   TRANSFORM_POINT( eye, ctx->ModelView.m, v );

   /* raster color */
   if (ctx->Light.Enabled) {
      GLubyte color[1][4];
      struct gl_shade_context sc;

      if (ctx->NewState)
	 gl_update_state( ctx );

      sc.vertices = (ctx->NeedEyeCoords ? eye : v);
      sc.vertex_stride = 4;
      sc.side_flags = 0x1;
      sc.Fcolor = color;
      sc.Findex = &ctx->Current.RasterIndex;
       
      if (ctx->NeedEyeNormals) {
	 GLfloat *inv = ctx->ModelView.inv;
	 TRANSFORM_NORMAL( eyenorm, ctx->Current.Normal, inv );
	 sc.normals = eyenorm;
      } else {
	 sc.normals = ctx->Current.Normal;
      }

      (ctx->shade_func)( ctx, &sc, 0, 1 );

      if (ctx->Visual->RGBAflag) {
         ctx->Current.RasterColor[0] = color[0][0] * (1.0F / 255.0F);
         ctx->Current.RasterColor[1] = color[0][1] * (1.0F / 255.0F);
         ctx->Current.RasterColor[2] = color[0][2] * (1.0F / 255.0F);
         ctx->Current.RasterColor[3] = color[0][3] * (1.0F / 255.0F);
      }
   }
   else {
      /* use current color or index */
      if (ctx->Visual->RGBAflag) {
         GLfloat *rc = ctx->Current.RasterColor;
         rc[0] = ctx->Current.ByteColor[0] * (1.0F / 255.0F);
         rc[1] = ctx->Current.ByteColor[1] * (1.0F / 255.0F);
         rc[2] = ctx->Current.ByteColor[2] * (1.0F / 255.0F);
         rc[3] = ctx->Current.ByteColor[3] * (1.0F / 255.0F);
      }
      else {
	 ctx->Current.RasterIndex = ctx->Current.Index;
      }
   }

   /* clip to user clipping planes */
   if ( ctx->Transform.AnyClip &&
	gl_userclip_point(ctx, eye) == 0) 
   {
      ctx->Current.RasterPosValid = GL_FALSE;
      return;
   }

   /* compute raster distance */
   ctx->Current.RasterDistance =
                      GL_SQRT( eye[0]*eye[0] + eye[1]*eye[1] + eye[2]*eye[2] );

   /* apply projection matrix:  clip = Proj * eye */
   TRANSFORM_POINT( clip, ctx->ProjectionMatrix.m, eye );

   /* clip to view volume */
   if (gl_viewclip_point( clip )==0) {
      ctx->Current.RasterPosValid = GL_FALSE;
      return;
   }

   /* ndc = clip / W */
   ASSERT( clip[3]!=0.0 );
   d = 1.0F / clip[3];
   ndc[0] = clip[0] * d;
   ndc[1] = clip[1] * d;
   ndc[2] = clip[2] * d;

   ctx->Current.RasterPos[0] = ndc[0] * ctx->Viewport.Sx + ctx->Viewport.Tx;
   ctx->Current.RasterPos[1] = ndc[1] * ctx->Viewport.Sy + ctx->Viewport.Ty;
   ctx->Current.RasterPos[2] = (ndc[2] * ctx->Viewport.Sz + ctx->Viewport.Tz)
                               / DEPTH_SCALE;
   ctx->Current.RasterPos[3] = clip[3];
   ctx->Current.RasterPosValid = GL_TRUE;

   /* FOG??? */

   {
      GLuint texSet;
      for (texSet=0; texSet<MAX_TEXTURE_UNITS; texSet++) {
         COPY_4V( ctx->Current.RasterMultiTexCoord[texSet],
                  ctx->Current.MultiTexCoord[texSet] );
      }
   }

   if (ctx->RenderMode==GL_SELECT) {
      gl_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }

}



/*
 * This is a MESA extension function.  Pretty much just like glRasterPos
 * except we don't apply the modelview or projection matrices; specify a
 * window coordinate directly.
 * Caller:  context->API.WindowPos4fMESA pointer.
 */
void gl_windowpos( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   /* set raster position */
   ctx->Current.RasterPos[0] = x;
   ctx->Current.RasterPos[1] = y;
   ctx->Current.RasterPos[2] = CLAMP( z, 0.0F, 1.0F );
   ctx->Current.RasterPos[3] = w;

   ctx->Current.RasterPosValid = GL_TRUE;

   /* raster color */
   if (ctx->Light.Enabled) {
      GLfloat pos[4];
      GLfloat eyenorm[3];
      GLubyte color[1][4];
      struct gl_shade_context sc;

      if (ctx->NewState)
	 gl_update_state( ctx );

      sc.vertices = pos;
      sc.vertex_stride = 4;
      sc.side_flags = 0x1;
      sc.Fcolor = color;
      sc.Findex = &ctx->Current.RasterIndex;

      if (ctx->Light.NeedVertices && !ctx->NeedEyeCoords) {
	 /* Lighting requires a vertex position in Object space! 
	  */
	 TRANSFORM_POINT( pos, ctx->ModelView.inv, 
			  ctx->Current.RasterPos );
      } else {
	 COPY_4V( pos, ctx->Current.RasterPos );
      }
       
      if (ctx->NeedEyeNormals) {
	 GLfloat *inv = ctx->ModelView.inv;
	 TRANSFORM_NORMAL( eyenorm, ctx->Current.Normal, inv );
	 sc.normals = eyenorm;
      } else {
	 sc.normals = ctx->Current.Normal;
      }

      (ctx->shade_func)( ctx, &sc, 0, 1 );

      if (ctx->Visual->RGBAflag) {
         ctx->Current.RasterColor[0] = color[0][0] * (1.0F / 255.0F);
         ctx->Current.RasterColor[1] = color[0][1] * (1.0F / 255.0F);
         ctx->Current.RasterColor[2] = color[0][2] * (1.0F / 255.0F);
         ctx->Current.RasterColor[3] = color[0][3] * (1.0F / 255.0F);
      }
   }
   else {
      /* use current color or index */
      if (ctx->Visual->RGBAflag) {
         ASSIGN_4V( ctx->Current.RasterColor,
                    ctx->Current.ByteColor[0] * (1.0F / 255.0F),
                    ctx->Current.ByteColor[1] * (1.0F / 255.0F),
                    ctx->Current.ByteColor[2] * (1.0F / 255.0F),
                    ctx->Current.ByteColor[3] * (1.0F / 255.0F) );
      }
      else {
	 ctx->Current.RasterIndex = ctx->Current.Index;
      }
   }

   ctx->Current.RasterDistance = 0.0;

   {
      GLuint texSet;
      for (texSet=0; texSet<MAX_TEXTURE_UNITS; texSet++) {
         COPY_4V( ctx->Current.RasterMultiTexCoord[texSet],
                  ctx->Current.MultiTexCoord[texSet] );
      }
   }

   if (ctx->RenderMode==GL_SELECT) {
      gl_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }
}
