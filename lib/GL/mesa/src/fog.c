/* Id: fog.c,v 1.3 1999/02/26 08:52:33 martin Exp $ */

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
 * Log: fog.c,v $
 * Revision 1.3  1999/02/26 08:52:33  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.6  1999/02/24 22:48:06  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.5  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.4  1998/11/17 02:51:35  brianp
 * added a bunch of new device driver functions (Keith Harrison)
 *
 * Revision 3.3  1998/10/29 02:28:13  brianp
 * incorporated Keith Whitwell's transformation optimizations
 *
 * Revision 3.2  1998/03/27 03:37:40  brianp
 * fixed G++ warnings
 *
 * Revision 3.1  1998/02/08 20:18:41  brianp
 * removed unneeded headers
 *
 * Revision 3.0  1998/01/31 20:52:49  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/fog.c,v 1.2 1999/03/14 03:20:44 dawes Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <stdlib.h>
#endif
#include <math.h>
#include "context.h"
#include "fog.h"
#include "macros.h"
#include "types.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



void gl_Fogfv( GLcontext *ctx, GLenum pname, const GLfloat *params )
{
   GLenum m;

   switch (pname) {
      case GL_FOG_MODE:
         m = (GLenum) (GLint) *params;
	 if (m==GL_LINEAR || m==GL_EXP || m==GL_EXP2) {
	    ctx->Fog.Mode = m;
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glFog" );
            return;
	 }
	 break;
      case GL_FOG_DENSITY:
	 if (*params<0.0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glFog" );
            return;
	 }
	 else {
	    ctx->Fog.Density = *params;
	 }
	 break;
      case GL_FOG_START:
#if 0
         /* Prior to OpenGL 1.1, this was an error */
         if (*params<0.0F) {
            gl_error( ctx, GL_INVALID_VALUE, "glFog(GL_FOG_START)" );
            return;
         }
#endif
	 ctx->Fog.Start = *params;
	 break;
      case GL_FOG_END:
#if 0
         /* Prior to OpenGL 1.1, this was an error */
         if (*params<0.0F) {
            gl_error( ctx, GL_INVALID_VALUE, "glFog(GL_FOG_END)" );
            return;
         }
#endif
	 ctx->Fog.End = *params;
	 break;
      case GL_FOG_INDEX:
	 ctx->Fog.Index = *params;
	 break;
      case GL_FOG_COLOR:
	 ctx->Fog.Color[0] = params[0];
	 ctx->Fog.Color[1] = params[1];
	 ctx->Fog.Color[2] = params[2];
	 ctx->Fog.Color[3] = params[3];
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glFog" );
         return;
   }

   if (ctx->Driver.Fogfv) {
      (*ctx->Driver.Fogfv)( ctx, pname, params );
   }
}




/*
 * Compute the fogged color for an array of vertices.
 * Input:  n - number of vertices
 *         v - array of vertices
 *         color - the original vertex colors
 * Output:  color - the fogged colors
 */
void gl_fog_rgba_vertices( const GLcontext *ctx,
                           GLuint n, GLfloat v[][4], GLubyte color[][4] )
{
   GLuint i;
   GLfloat d;
   GLfloat rFog = ctx->Fog.Color[0] * 255.0F;
   GLfloat gFog = ctx->Fog.Color[1] * 255.0F;
   GLfloat bFog = ctx->Fog.Color[2] * 255.0F;
   GLfloat end = ctx->Fog.End;

   switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
         for (i=0;i<n;i++) {
            GLfloat f = (end - ABSF(v[i][2])) * d;
            f = CLAMP( f, 0.0F, 1.0F );
            color[i][0] = (GLint) (f * color[i][0] + (1.0F-f) * rFog);
            color[i][1] = (GLint) (f * color[i][1] + (1.0F-f) * gFog);
            color[i][2] = (GLint) (f * color[i][2] + (1.0F-f) * bFog);
         }
	 break;
      case GL_EXP:
         d = -ctx->Fog.Density;
         for (i=0;i<n;i++) {
            GLfloat f = exp( d * ABSF(v[i][2]) );
            f = CLAMP( f, 0.0F, 1.0F );
            color[i][0] = (GLint) (f * color[i][0] + (1.0F-f) * rFog);
            color[i][1] = (GLint) (f * color[i][1] + (1.0F-f) * gFog);
            color[i][2] = (GLint) (f * color[i][2] + (1.0F-f) * bFog);
         }
	 break;
      case GL_EXP2:
         d = -(ctx->Fog.Density*ctx->Fog.Density);
         for (i=0;i<n;i++) {
            GLfloat z = v[i][2]; /* kw: was previously ABSF(v[i][2])*/
            GLfloat f = exp( d * z*z );
            f = CLAMP( f, 0.0F, 1.0F );
            color[i][0] = (GLint) (f * color[i][0] + (1.0F-f) * rFog);
            color[i][1] = (GLint) (f * color[i][1] + (1.0F-f) * gFog);
            color[i][2] = (GLint) (f * color[i][2] + (1.0F-f) * bFog);
         }
	 break;
      default:
         gl_problem(ctx, "Bad fog mode in gl_fog_rgba_vertices");
         return;
   }
}



/*
 * Compute the fogged color indexes for an array of vertices.
 * Input:  n - number of vertices
 *         v - array of vertices
 * In/Out: indx - array of vertex color indexes
 */
void gl_fog_ci_vertices( const GLcontext *ctx,
                         GLuint n, GLfloat v[][4], GLuint indx[] )
{
   /* NOTE: the extensive use of casts generates better/faster code for MIPS */
   switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         {
            GLfloat d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
            GLfloat fogindex = ctx->Fog.Index;
            GLfloat fogend = ctx->Fog.End;
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat f = (fogend - ABSF(v[i][2])) * d;
               f = CLAMP( f, 0.0F, 1.0F );
               indx[i] = (GLint)
                         ((GLfloat) (GLint) indx[i] + (1.0F-f) * fogindex);
            }
         }
	 break;
      case GL_EXP:
         {
            GLfloat d = -ctx->Fog.Density;
            GLfloat fogindex = ctx->Fog.Index;
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat f = exp( d * ABSF(v[i][2]) );
               f = CLAMP( f, 0.0F, 1.0F );
               indx[i] = (GLint)
                         ((GLfloat) (GLint) indx[i] + (1.0F-f) * fogindex);
            }
         }
	 break;
      case GL_EXP2:
         {
            GLfloat d = -(ctx->Fog.Density*ctx->Fog.Density);
            GLfloat fogindex = ctx->Fog.Index;
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat z = v[i][2]; /*ABSF(v[i][2]);*/
               GLfloat f = exp( -d * z*z );
               f = CLAMP( f, 0.0F, 1.0F );
               indx[i] = (GLint)
                         ((GLfloat) (GLint) indx[i] + (1.0F-f) * fogindex);
            }
         }
	 break;
      default:
         gl_problem(ctx, "Bad fog mode in gl_fog_ci_vertices");
         return;
   }
}




/*
 * Apply fog to an array of RGBA pixels.
 * Input:  n - number of pixels
 *         z - array of integer depth values
 *         red, green, blue, alpha - pixel colors
 * Output:  red, green, blue, alpha - fogged pixel colors
 */
void gl_fog_rgba_pixels( const GLcontext *ctx,
                         GLuint n, const GLdepth z[], GLubyte rgba[][4] )
{
   GLfloat c = ctx->ProjectionMatrix.m[10];
   GLfloat d = ctx->ProjectionMatrix.m[14];
   GLuint i;

   GLfloat rFog = ctx->Fog.Color[0] * 255.0F;
   GLfloat gFog = ctx->Fog.Color[1] * 255.0F;
   GLfloat bFog = ctx->Fog.Color[2] * 255.0F;

   GLfloat tz = ctx->Viewport.Tz;
   GLfloat szInv = 1.0F / ctx->Viewport.Sz;

   switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         {
            GLfloat fogEnd = ctx->Fog.End;
            GLfloat fogScale = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = -d / (c+ndcz);
               GLfloat f, g;
               if (eyez < 0.0)  eyez = -eyez;
               f = (fogEnd - eyez) * fogScale;
               f = CLAMP( f, 0.0F, 1.0F );
               g = 1.0F - f;
               rgba[i][RCOMP] = (GLint) (f * rgba[i][RCOMP] + g * rFog);
               rgba[i][GCOMP] = (GLint) (f * rgba[i][GCOMP] + g * gFog);
               rgba[i][BCOMP] = (GLint) (f * rgba[i][BCOMP] + g * bFog);
            }
         }
	 break;
      case GL_EXP:
	 for (i=0;i<n;i++) {
	    GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
	    GLfloat eyez = -d / (c+ndcz);
            GLfloat f, g;
	    if (eyez < 0.0)  eyez = -eyez;
	    f = exp( -ctx->Fog.Density * eyez );
	    f = CLAMP( f, 0.0F, 1.0F );
            g = 1.0F - f;
            rgba[i][RCOMP] = (GLint) (f * rgba[i][RCOMP] + g * rFog);
            rgba[i][GCOMP] = (GLint) (f * rgba[i][GCOMP] + g * gFog);
            rgba[i][BCOMP] = (GLint) (f * rgba[i][BCOMP] + g * bFog);
	 }
	 break;
      case GL_EXP2:
         {
            GLfloat negDensitySquared = -ctx->Fog.Density * ctx->Fog.Density;
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = -d / (c+ndcz);
               GLfloat f, g;
               if (eyez < 0.0)  eyez = -eyez;
               f = exp( negDensitySquared * eyez*eyez );
               f = CLAMP( f, 0.0F, 1.0F );
               g = 1.0F - f;
               rgba[i][RCOMP] = (GLint) (f * rgba[i][RCOMP] + g * rFog);
               rgba[i][GCOMP] = (GLint) (f * rgba[i][GCOMP] + g * gFog);
               rgba[i][BCOMP] = (GLint) (f * rgba[i][BCOMP] + g * bFog);
            }
         }
	 break;
      default:
         gl_problem(ctx, "Bad fog mode in gl_fog_rgba_pixels");
         return;
   }
}




/*
 * Apply fog to an array of color index pixels.
 * Input:  n - number of pixels
 *         z - array of integer depth values
 *         index - pixel color indexes
 * Output:  index - fogged pixel color indexes
 */
void gl_fog_ci_pixels( const GLcontext *ctx,
                       GLuint n, const GLdepth z[], GLuint index[] )
{
   GLfloat c = ctx->ProjectionMatrix.m[10];
   GLfloat d = ctx->ProjectionMatrix.m[14];
   GLuint i;

   GLfloat tz = ctx->Viewport.Tz;
   GLfloat szInv = 1.0F / ctx->Viewport.Sz;

   switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         {
            GLfloat fogEnd = ctx->Fog.End;
            GLfloat fogScale = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = -d / (c+ndcz);
               GLfloat f;
               if (eyez < 0.0)  eyez = -eyez;
               f = (fogEnd - eyez) * fogScale;
               f = CLAMP( f, 0.0F, 1.0F );
               index[i] = (GLuint) ((GLfloat) index[i] + (1.0F-f) * ctx->Fog.Index);
            }
	 }
	 break;
      case GL_EXP:
         for (i=0;i<n;i++) {
	    GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
	    GLfloat eyez = -d / (c+ndcz);
            GLfloat f;
	    if (eyez < 0.0)  eyez = -eyez;
	    f = exp( -ctx->Fog.Density * eyez );
	    f = CLAMP( f, 0.0F, 1.0F );
	    index[i] = (GLuint) ((GLfloat) index[i] + (1.0F-f) * ctx->Fog.Index);
	 }
	 break;
      case GL_EXP2:
         {
            GLfloat negDensitySquared = -ctx->Fog.Density * ctx->Fog.Density;
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = -d / (c+ndcz);
               GLfloat f;
               if (eyez < 0.0)  eyez = -eyez;
               f = exp( negDensitySquared * eyez*eyez );
               f = CLAMP( f, 0.0F, 1.0F );
               index[i] = (GLuint) ((GLfloat) index[i] + (1.0F-f) * ctx->Fog.Index);
            }
	 }
	 break;
      default:
         gl_problem(ctx, "Bad fog mode in gl_fog_ci_pixels");
         return;
   }
}

