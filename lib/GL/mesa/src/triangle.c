/* Id: triangle.c,v 1.3 1999/02/26 08:52:38 martin Exp $ */

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
 * Log: triangle.c,v $
 * Revision 1.3  1999/02/26 08:52:38  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.18  1999/02/24 22:48:08  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.17  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.16  1998/11/17 01:53:35  brianp
 * fixed multi-texture bug/typo (use invQ1 instead of invQ)
 *
 * Revision 3.15  1998/11/08 22:34:07  brianp
 * renamed texture sets to texture units
 *
 * Revision 3.14  1998/11/03 01:16:57  brianp
 * added Texture.ReallyEnabled to simply testing for enabled textures
 *
 * Revision 3.13  1998/10/31 17:06:15  brianp
 * variety of multi-texture changes
 *
 * Revision 3.12  1998/10/29 02:34:36  brianp
 * cleaned-up code, fixed incomplete texture object selection bug
 *
 * Revision 3.11  1998/09/18 02:33:05  brianp
 * fixed sampling-out-of-bounds bug in optimized textured triangle functions
 *
 * Revision 3.10  1998/07/09 03:16:24  brianp
 * added Marten's latest texture triangle code
 *
 * Revision 3.9  1998/06/24 02:52:05  brianp
 * fixed texture coord interp problems.  lots of code clean-up
 *
 * Revision 3.8  1998/06/18 02:49:35  brianp
 * added Marten Stromberg's optimized textured triangle code
 *
 * Revision 3.7  1998/06/07 22:18:52  brianp
 * implemented GL_EXT_multitexture extension
 *
 * Revision 3.6  1998/05/31 23:50:36  brianp
 * cleaned up a few Solaris compiler warnings
 *
 * Revision 3.5  1998/04/01 02:58:52  brianp
 * applied Miklos Fazekas's 3-31-98 Macintosh changes
 *
 * Revision 3.4  1998/03/27 04:26:44  brianp
 * fixed G++ warnings
 *
 * Revision 3.3  1998/02/20 04:54:02  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.2  1998/02/04 00:44:29  brianp
 * fixed casts and conditional expression problems for Amiga StormC compiler
 *
 * Revision 3.1  1998/02/02 03:09:34  brianp
 * added GL_LIGHT_MODEL_COLOR_CONTROL (separate specular color interpolation)
 *
 * Revision 3.0  1998/01/31 21:05:24  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/triangle.c,v 1.2 1999/03/14 03:20:53 dawes Exp $ */

/*
 * Triangle rasterizers
 */


#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <assert.h>
#include <stdio.h>
#endif
#include <math.h>
#include "context.h"
#include "depth.h"
#include "feedback.h"
#include "macros.h"
#include "span.h"
#include "texstate.h"
#include "triangle.h"
#include "types.h"
#include "vb.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif


/*
 * Put triangle in feedback buffer.
 */
static void feedback_triangle( GLcontext *ctx,
                               GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   struct vertex_buffer *VB = ctx->VB;
   GLfloat color[4];
   GLuint i;
   GLuint texUnit = ctx->Texture.CurrentTransformUnit;

   FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_POLYGON_TOKEN );
   FEEDBACK_TOKEN( ctx, (GLfloat) 3 );        /* three vertices */

   if (ctx->Light.ShadeModel==GL_FLAT) {
      /* flat shading - same color for each vertex */
      color[0] = (GLfloat) VB->Color[pv][0] / 255.0;
      color[1] = (GLfloat) VB->Color[pv][1] / 255.0;
      color[2] = (GLfloat) VB->Color[pv][2] / 255.0;
      color[3] = (GLfloat) VB->Color[pv][3] / 255.0;
   }

   for (i=0;i<3;i++) {
      GLfloat x, y, z, w;
      GLfloat tc[4];
      GLuint v;
      GLfloat invq;

      if (i==0)       v = v0;
      else if (i==1)  v = v1;
      else            v = v2;

      x = VB->Win[v][0];
      y = VB->Win[v][1];
      z = VB->Win[v][2] / DEPTH_SCALE;
      w = VB->Clip[v][3];

      if (ctx->Light.ShadeModel==GL_SMOOTH) {
         /* smooth shading - different color for each vertex */
         color[0] = (GLfloat) VB->Color[v][0] / 255.0;
         color[1] = (GLfloat) VB->Color[v][1] / 255.0;
         color[2] = (GLfloat) VB->Color[v][2] / 255.0;
         color[3] = (GLfloat) VB->Color[v][3] / 255.0;
      }

      if (VB->MultiTexCoord[texUnit][v][3]==0.0)
         invq =  1.0;
      else
         invq = 1.0F / VB->MultiTexCoord[texUnit][v][3];
      tc[0] = VB->MultiTexCoord[texUnit][v][0] * invq;
      tc[1] = VB->MultiTexCoord[texUnit][v][1] * invq;
      tc[2] = VB->MultiTexCoord[texUnit][v][2] * invq;
      tc[3] = VB->MultiTexCoord[texUnit][v][3];

      gl_feedback_vertex( ctx, x, y, z, w, color, (GLfloat) VB->Index[v], tc );
   }
}



/*
 * Put triangle in selection buffer.
 */
static void select_triangle( GLcontext *ctx,
                             GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   struct vertex_buffer *VB = ctx->VB;
   (void) pv;
   gl_update_hitflag( ctx, VB->Win[v0][2] / DEPTH_SCALE );
   gl_update_hitflag( ctx, VB->Win[v1][2] / DEPTH_SCALE );
   gl_update_hitflag( ctx, VB->Win[v2][2] / DEPTH_SCALE );
}



/*
 * Render a flat-shaded color index triangle.
 */
static void flat_ci_triangle( GLcontext *ctx,
                              GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
#define INTERP_Z 1

#define SETUP_CODE				\
   GLuint index = VB->Index[pv];		\
   if (!VB->MonoColor) {			\
      /* set the color index */			\
      (*ctx->Driver.Index)( ctx, index );	\
   }

#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   GLint i, n = RIGHT-LEFT;				\
	   GLdepth zspan[MAX_WIDTH];				\
	   if (n>0) {						\
	      for (i=0;i<n;i++) {				\
		 zspan[i] = FixedToDepth(ffz);			\
		 ffz += fdzdx;					\
	      }							\
	      gl_write_monoindex_span( ctx, n, LEFT, Y,		\
	                            zspan, index, GL_POLYGON );	\
	   }							\
	}

#include "tritemp.h"	      
}



/*
 * Render a smooth-shaded color index triangle.
 */
static void smooth_ci_triangle( GLcontext *ctx,
                                GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   (void) pv;
#define INTERP_Z 1
#define INTERP_INDEX 1

#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   GLint i, n = RIGHT-LEFT;				\
	   GLdepth zspan[MAX_WIDTH];				\
           GLuint index[MAX_WIDTH];				\
	   if (n>0) {						\
	      for (i=0;i<n;i++) {				\
		 zspan[i] = FixedToDepth(ffz);			\
                 index[i] = FixedToInt(ffi);			\
		 ffz += fdzdx;					\
		 ffi += fdidx;					\
	      }							\
	      gl_write_index_span( ctx, n, LEFT, Y, zspan,	\
	                           index, GL_POLYGON );		\
	   }							\
	}

#include "tritemp.h"
}



/*
 * Render a flat-shaded RGBA triangle.
 */
static void flat_rgba_triangle( GLcontext *ctx,
                                GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
#define INTERP_Z 1

#define SETUP_CODE				\
   if (!VB->MonoColor) {			\
      /* set the color */			\
      GLubyte r = VB->Color[pv][0];		\
      GLubyte g = VB->Color[pv][1];		\
      GLubyte b = VB->Color[pv][2];		\
      GLubyte a = VB->Color[pv][3];		\
      (*ctx->Driver.Color)( ctx, r, g, b, a );	\
   }

#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   GLint i, n = RIGHT-LEFT;				\
	   GLdepth zspan[MAX_WIDTH];				\
	   if (n>0) {						\
	      for (i=0;i<n;i++) {				\
		 zspan[i] = FixedToDepth(ffz);			\
		 ffz += fdzdx;					\
	      }							\
              gl_write_monocolor_span( ctx, n, LEFT, Y, zspan,	\
                             VB->Color[pv][0], VB->Color[pv][1],\
                             VB->Color[pv][2], VB->Color[pv][3],\
			     GL_POLYGON );			\
	   }							\
	}

#include "tritemp.h"
}



/*
 * Render a smooth-shaded RGBA triangle.
 */
static void smooth_rgba_triangle( GLcontext *ctx,
                                  GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   (void) pv;
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1

#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   GLint i, n = RIGHT-LEFT;				\
	   GLdepth zspan[MAX_WIDTH];				\
	   GLubyte rgba[MAX_WIDTH][4];				\
	   if (n>0) {						\
	      for (i=0;i<n;i++) {				\
		 zspan[i] = FixedToDepth(ffz);			\
		 rgba[i][RCOMP] = FixedToInt(ffr);		\
		 rgba[i][GCOMP] = FixedToInt(ffg);		\
		 rgba[i][BCOMP] = FixedToInt(ffb);		\
		 rgba[i][ACOMP] = FixedToInt(ffa);		\
		 ffz += fdzdx;					\
		 ffr += fdrdx;					\
		 ffg += fdgdx;					\
		 ffb += fdbdx;					\
		 ffa += fdadx;					\
	      }							\
	      gl_write_rgba_span( ctx, n, LEFT, Y, zspan,	\
	                           rgba, GL_POLYGON );		\
	   }							\
	}

#include "tritemp.h"
}


/*
 * Render an RGB, GL_DECAL, textured triangle.
 * Interpolate S,T only w/out mipmapping or perspective correction.
 */
static void simple_textured_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                      GLuint v2, GLuint pv )
{
#define INTERP_INT_ST 1
#define S_SCALE twidth
#define T_SCALE theight
#define SETUP_CODE							\
   struct gl_texture_object *obj = ctx->Texture.Unit[0].Current2D;	\
   GLint b = obj->BaseLevel;						\
   GLfloat twidth = (GLfloat) obj->Image[b]->Width;			\
   GLfloat theight = (GLfloat) obj->Image[b]->Height;			\
   GLint twidth_log2 = obj->Image[b]->WidthLog2;			\
   GLubyte *texture = obj->Image[b]->Data;				\
   GLint smask = obj->Image[b]->Width - 1;				\
   GLint tmask = obj->Image[b]->Height - 1;
   (void) pv;

#ifdef USE_X86_ASM
# define RGB_TEX *(GLint *)rgba[i] = *(GLint *)(texture + pos) | 0xff000000
#else
# define RGB_TEX                         \
        rgba[i][RCOMP] = texture[pos];	 \
        rgba[i][GCOMP] = texture[pos+1]; \
        rgba[i][BCOMP] = texture[pos+2]; \
        rgba[i][ACOMP] = 255
#endif



#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   GLint i, n = RIGHT-LEFT;				\
	   GLubyte rgba[MAX_WIDTH][4];				\
	   if (n>0) {						\
              ffs -= FIXED_HALF; /* off-by-one error? */        \
              fft -= FIXED_HALF;                                \
	      for (i=0;i<n;i++) {				\
                 GLint s = FixedToInt(ffs) & smask;		\
                 GLint t = FixedToInt(fft) & tmask;		\
                 GLint pos = (t << twidth_log2) + s;		\
                 pos = pos + pos  + pos;  /* multiply by 3 */	\
                 RGB_TEX;                                       \
		 ffs += fdsdx;					\
		 fft += fdtdx;					\
	      }							\
              (*ctx->Driver.WriteRGBASpan)( ctx, n, LEFT, Y,	\
                                             rgba, NULL );	\
	   }							\
	}

#include "tritemp.h"
}



/*
 * Render an RGB, GL_DECAL, textured triangle.
 * Interpolate S,T, GL_LESS depth test, w/out mipmapping or
 * perspective correction.
 */
static void simple_z_textured_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                        GLuint v2, GLuint pv )
{
#define INTERP_Z 1
#define INTERP_INT_ST 1
#define S_SCALE twidth
#define T_SCALE theight
#define SETUP_CODE							\
   struct gl_texture_object *obj = ctx->Texture.Unit[0].Current2D;	\
   GLint b = obj->BaseLevel;						\
   GLfloat twidth = (GLfloat) obj->Image[b]->Width;			\
   GLfloat theight = (GLfloat) obj->Image[b]->Height;			\
   GLint twidth_log2 = obj->Image[b]->WidthLog2;			\
   GLubyte *texture = obj->Image[b]->Data;				\
   GLint smask = obj->Image[b]->Width - 1;				\
   GLint tmask = obj->Image[b]->Height - 1;
   (void) pv;

#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   GLint i, n = RIGHT-LEFT;				\
	   GLubyte rgba[MAX_WIDTH][4];				\
           GLubyte mask[MAX_WIDTH];				\
	   if (n>0) {						\
              ffs -= FIXED_HALF; /* off-by-one error? */        \
              fft -= FIXED_HALF;                                \
	      for (i=0;i<n;i++) {				\
                 GLdepth z = FixedToDepth(ffz);			\
                 if (z < zRow[i]) {				\
                    GLint s = FixedToInt(ffs) & smask;		\
                    GLint t = FixedToInt(fft) & tmask;		\
                    GLint pos = (t << twidth_log2) + s;		\
                    pos = pos + pos  + pos;  /* multiply by 3 */\
                    RGB_TEX;                                    \
		    zRow[i] = z;				\
                    mask[i] = 1;				\
                 }						\
                 else {						\
                    mask[i] = 0;				\
                 }						\
		 ffz += fdzdx;					\
		 ffs += fdsdx;					\
		 fft += fdtdx;					\
	      }							\
              (*ctx->Driver.WriteRGBASpan)( ctx, n, LEFT, Y,	\
                                             rgba, mask );	\
	   }							\
	}

#include "tritemp.h"
}


/*
 * Render an RGB/RGBA textured triangle without perspective correction.
 */
static void affine_textured_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
				      GLuint v2, GLuint pv )
{
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_INT_ST 1
#define S_SCALE twidth
#define T_SCALE theight
#define SETUP_CODE							\
   struct gl_texture_unit *unit = ctx->Texture.Unit+0;                     \
   struct gl_texture_object *obj = unit->Current2D;                      \
   GLint b = obj->BaseLevel;						\
   GLfloat twidth = (GLfloat) obj->Image[b]->Width;			\
   GLfloat theight = (GLfloat) obj->Image[b]->Height;			\
   GLint twidth_log2 = obj->Image[b]->WidthLog2;			\
   GLubyte *texture = obj->Image[b]->Data;				\
   GLint smask = obj->Image[b]->Width - 1;				\
   GLint tmask = obj->Image[b]->Height - 1;                             \
   GLint format = obj->Image[b]->Format;                                \
   GLint filter = obj->MinFilter;                                       \
   GLint envmode = unit->EnvMode;                                        \
   GLint comp, tbytesline, tsize;                                       \
   GLfixed er, eg, eb, ea;                                              \
   GLint tr, tg, tb, ta;                                                \
   if (envmode == GL_BLEND || envmode == GL_ADD) {                      \
      /* potential off-by-one error here? (1.0f -> 2048 -> 0) */        \
      er = FloatToFixed(unit->EnvColor[0]);                              \
      eg = FloatToFixed(unit->EnvColor[1]);                              \
      eb = FloatToFixed(unit->EnvColor[2]);                              \
      ea = FloatToFixed(unit->EnvColor[3]);                              \
   }                                                                    \
   switch (format) {                                                    \
   case GL_ALPHA:                                                       \
   case GL_LUMINANCE:                                                   \
   case GL_INTENSITY:                                                   \
      comp = 1;                                                         \
      break;                                                            \
   case GL_LUMINANCE_ALPHA:                                             \
      comp = 2;                                                         \
      break;                                                            \
   case GL_RGB:                                                         \
      comp = 3;                                                         \
      break;                                                            \
   case GL_RGBA:                                                        \
      comp = 4;                                                         \
      break;                                                            \
   default:                                                             \
      gl_problem(NULL, "Bad texture format in affine_texture_triangle");\
      return;                                                           \
   }                                                                    \
   tbytesline = obj->Image[b]->Width * comp;                            \
   tsize = theight * tbytesline;
   (void) pv;

  /* Instead of defining a function for each mode, a test is done 
   * between the outer and inner loops. This is to reduce code size
   * and complexity. Observe that an optimizing compiler kills 
   * unused variables (for instance tf,sf,ti,si in case of GL_NEAREST).
   */ 

#define NEAREST_RGB    \
        tr = tex00[0]; \
        tg = tex00[1]; \
        tb = tex00[2]; \
        ta = 0xff

#define LINEAR_RGB                                                    \
	tr = (ti * (si * tex00[0] + sf * tex01[0]) +                    \
              tf * (si * tex10[0] + sf * tex11[0])) >> 2 * FIXED_SHIFT; \
	tg = (ti * (si * tex00[1] + sf * tex01[1]) +                    \
              tf * (si * tex10[1] + sf * tex11[1])) >> 2 * FIXED_SHIFT; \
	tb = (ti * (si * tex00[2] + sf * tex01[2]) +                    \
              tf * (si * tex10[2] + sf * tex11[2])) >> 2 * FIXED_SHIFT; \
	ta = 0xff

#define NEAREST_RGBA   \
        tr = tex00[0]; \
        tg = tex00[1]; \
        tb = tex00[2]; \
        ta = tex00[3]

#define LINEAR_RGBA                                                   \
	tr = (ti * (si * tex00[0] + sf * tex01[0]) +                    \
              tf * (si * tex10[0] + sf * tex11[0])) >> 2 * FIXED_SHIFT; \
	tg = (ti * (si * tex00[1] + sf * tex01[1]) +                    \
              tf * (si * tex10[1] + sf * tex11[1])) >> 2 * FIXED_SHIFT; \
	tb = (ti * (si * tex00[2] + sf * tex01[2]) +                    \
              tf * (si * tex10[2] + sf * tex11[2])) >> 2 * FIXED_SHIFT; \
	ta = (ti * (si * tex00[3] + sf * tex01[3]) +                    \
              tf * (si * tex10[3] + sf * tex11[3])) >> 2 * FIXED_SHIFT

#define MODULATE                                     \
        dest[0] = ffr * (tr + 1) >> (FIXED_SHIFT + 8); \
        dest[1] = ffg * (tg + 1) >> (FIXED_SHIFT + 8); \
        dest[2] = ffb * (tb + 1) >> (FIXED_SHIFT + 8); \
        dest[3] = ffa * (ta + 1) >> (FIXED_SHIFT + 8)

#define DECAL                                                                            \
	dest[0] = ((0xff - ta) * ffr + ((ta + 1) * tr << FIXED_SHIFT)) >> (FIXED_SHIFT + 8); \
	dest[1] = ((0xff - ta) * ffg + ((ta + 1) * tg << FIXED_SHIFT)) >> (FIXED_SHIFT + 8); \
	dest[2] = ((0xff - ta) * ffb + ((ta + 1) * tb << FIXED_SHIFT)) >> (FIXED_SHIFT + 8); \
	dest[3] = FixedToInt(ffa)

#define BLEND                                                           \
        dest[0] = ((0xff - tr) * ffr + (tr + 1) * er) >> (FIXED_SHIFT + 8); \
        dest[1] = ((0xff - tg) * ffg + (tg + 1) * eg) >> (FIXED_SHIFT + 8); \
        dest[2] = ((0xff - tb) * ffb + (tb + 1) * eb) >> (FIXED_SHIFT + 8); \
	dest[3] = ffa * (ta + 1) >> (FIXED_SHIFT + 8)

#define REPLACE       \
        dest[0] = tr; \
        dest[1] = tg; \
        dest[2] = tb; \
        dest[3] = ta

#define ADD                                                      \
        dest[0] = ((ffr << 8) + (tr + 1) * er) >> (FIXED_SHIFT + 8); \
        dest[1] = ((ffg << 8) + (tg + 1) * eg) >> (FIXED_SHIFT + 8); \
        dest[2] = ((ffb << 8) + (tb + 1) * eb) >> (FIXED_SHIFT + 8); \
	dest[3] = ffa * (ta + 1) >> (FIXED_SHIFT + 8)

/* shortcuts */

#if defined(USE_X86_ASM)
/* assumes (i) unaligned load capacity, and (ii) little endian: */
# define NEAREST_RGB_REPLACE  *(GLint *)dest = (*(GLint *)tex00 & 0x00ffffff) \
	                                       | ((ffa << (24 - FIXED_SHIFT)) & 0xff000000)
#else
# define NEAREST_RGB_REPLACE  NEAREST_RGB;REPLACE
#endif
#define NEAREST_RGBA_REPLACE  *(GLint *)dest = *(GLint *)tex00

#define SPAN1(DO_TEX,COMP)                                 \
	for (i=0;i<n;i++) {                                \
           GLint s = FixedToInt(ffs) & smask;              \
           GLint t = FixedToInt(fft) & tmask;              \
           GLint pos = (t << twidth_log2) + s;             \
           GLubyte *tex00 = texture + COMP * pos;          \
	   zspan[i] = FixedToDepth(ffz);                   \
           DO_TEX;                                         \
	   ffz += fdzdx;                                   \
           ffr += fdrdx;                                   \
	   ffg += fdgdx;                                   \
           ffb += fdbdx;                                   \
	   ffa += fdadx;                                   \
	   ffs += fdsdx;                                   \
	   fft += fdtdx;                                   \
           dest += 4;                                      \
	}

#define SPAN2(DO_TEX,COMP)                                 \
	for (i=0;i<n;i++) {                                \
           GLint s = FixedToInt(ffs) & smask;              \
           GLint t = FixedToInt(fft) & tmask;              \
           GLint sf = ffs & FIXED_FRAC_MASK;               \
           GLint tf = fft & FIXED_FRAC_MASK;               \
           GLint si = FIXED_FRAC_MASK - sf;                \
           GLint ti = FIXED_FRAC_MASK - tf;                \
           GLint pos = (t << twidth_log2) + s;             \
           GLubyte *tex00 = texture + COMP * pos;          \
           GLubyte *tex10 = tex00 + tbytesline;            \
           GLubyte *tex01 = tex00 + COMP;                  \
           GLubyte *tex11 = tex10 + COMP;                  \
           if (t == tmask) {                               \
              tex10 -= tsize;                              \
              tex11 -= tsize;                              \
           }                                               \
           if (s == smask) {                               \
              tex01 -= tbytesline;                         \
              tex11 -= tbytesline;                         \
           }                                               \
	   zspan[i] = FixedToDepth(ffz);                   \
           DO_TEX;                                         \
	   ffz += fdzdx;                                   \
           ffr += fdrdx;                                   \
	   ffg += fdgdx;                                   \
           ffb += fdbdx;                                   \
	   ffa += fdadx;                                   \
	   ffs += fdsdx;                                   \
	   fft += fdtdx;                                   \
           dest += 4;                                      \
	}

/* here comes the heavy part.. (something for the compiler to chew on) */
#define INNER_LOOP( LEFT, RIGHT, Y )	                   \
	{				                   \
	   GLint i, n = RIGHT-LEFT;	                   \
	   GLdepth zspan[MAX_WIDTH];	                   \
	   GLubyte rgba[MAX_WIDTH][4];                     \
	   if (n>0) {                                      \
              GLubyte *dest = rgba[0];                     \
              ffs -= FIXED_HALF; /* off-by-one error? */   \
              fft -= FIXED_HALF;                           \
              switch (filter) {                            \
   	      case GL_NEAREST:                             \
		 switch (format) {                         \
                 case GL_RGB:                              \
	            switch (envmode) {                     \
	            case GL_MODULATE:                      \
                       SPAN1(NEAREST_RGB;MODULATE,3);      \
                       break;                              \
	            case GL_DECAL:                         \
                    case GL_REPLACE:                       \
                       SPAN1(NEAREST_RGB_REPLACE,3);       \
                       break;                              \
                    case GL_BLEND:                         \
                       SPAN1(NEAREST_RGB;BLEND,3);         \
                       break;                              \
                    case GL_ADD:                           \
                       SPAN1(NEAREST_RGB;ADD,3);           \
                       break;                              \
	            }                                      \
                    break;                                 \
		 case GL_RGBA:                             \
		    switch(envmode) {                      \
		    case GL_MODULATE:                      \
                       SPAN1(NEAREST_RGBA;MODULATE,4);     \
                       break;                              \
		    case GL_DECAL:                         \
                       SPAN1(NEAREST_RGBA;DECAL,4);        \
                       break;                              \
		    case GL_BLEND:                         \
                       SPAN1(NEAREST_RGBA;BLEND,4);        \
                       break;                              \
		    case GL_REPLACE:                       \
                       SPAN1(NEAREST_RGBA_REPLACE,4);      \
                       break;                              \
		    case GL_ADD:                           \
                       SPAN1(NEAREST_RGBA;ADD,4);          \
                       break;                              \
		    }                                      \
                    break;                                 \
	         }                                         \
                 break;                                    \
	      case GL_LINEAR:                              \
                 ffs -= FIXED_HALF;                        \
                 fft -= FIXED_HALF;                        \
		 switch (format) {                         \
		 case GL_RGB:                              \
		    switch (envmode) {                     \
		    case GL_MODULATE:                      \
		       SPAN2(LINEAR_RGB;MODULATE,3);       \
                       break;                              \
		    case GL_DECAL:                         \
		    case GL_REPLACE:                       \
                       SPAN2(LINEAR_RGB;REPLACE,3);        \
                       break;                              \
		    case GL_BLEND:                         \
		       SPAN2(LINEAR_RGB;BLEND,3);          \
		       break;                              \
		    case GL_ADD:                           \
		       SPAN2(LINEAR_RGB;ADD,3);            \
		       break;                              \
		    }                                      \
		    break;                                 \
		 case GL_RGBA:                             \
		    switch (envmode) {                     \
		    case GL_MODULATE:                      \
		       SPAN2(LINEAR_RGBA;MODULATE,4);      \
		       break;                              \
		    case GL_DECAL:                         \
		       SPAN2(LINEAR_RGBA;DECAL,4);         \
		       break;                              \
		    case GL_BLEND:                         \
		       SPAN2(LINEAR_RGBA;BLEND,4);         \
		       break;                              \
		    case GL_REPLACE:                       \
		       SPAN2(LINEAR_RGBA;REPLACE,4);       \
		       break;                              \
		    case GL_ADD:                           \
		       SPAN2(LINEAR_RGBA;ADD,4);           \
		       break;                              \
		    }                                      \
		    break;                                 \
	         }                                         \
                 break;                                    \
	      }                                            \
              gl_write_rgba_span(ctx, n, LEFT, Y, zspan,   \
                                 rgba, GL_POLYGON);        \
              /* explicit kill of variables: */            \
              ffr = ffg = ffb = ffa = 0;                   \
           }                                               \
	}

#include "tritemp.h"
#undef SPAN1
#undef SPAN2
}


/*
 * Render an perspective corrected RGB/RGBA textured triangle.
 * The Q (aka V in Mesa) coordinate must be zero such that the divide
 * by interpolated Q/W comes out right.
 */
static void persp_textured_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
				     GLuint v2, GLuint pv )
{

#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_STUV 1
#define SETUP_CODE							\
   struct gl_texture_unit *unit = ctx->Texture.Unit+0;                     \
   struct gl_texture_object *obj = unit->Current2D;                      \
   GLint b = obj->BaseLevel;						\
   GLfloat twidth = (GLfloat) obj->Image[b]->Width;			\
   GLfloat theight = (GLfloat) obj->Image[b]->Height;			\
   GLint twidth_log2 = obj->Image[b]->WidthLog2;			\
   GLubyte *texture = obj->Image[b]->Data;				\
   GLint smask = (obj->Image[b]->Width - 1);                            \
   GLint tmask = (obj->Image[b]->Height - 1);                           \
   GLint format = obj->Image[b]->Format;                                \
   GLint filter = obj->MinFilter;                                       \
   GLint envmode = unit->EnvMode;                                        \
   GLfloat sscale, tscale;                                              \
   GLint comp, tbytesline, tsize;                                       \
   GLfixed er, eg, eb, ea;                                              \
   GLint tr, tg, tb, ta;                                                \
   if (envmode == GL_BLEND || envmode == GL_ADD) {                      \
      er = FloatToFixed(unit->EnvColor[0]);                              \
      eg = FloatToFixed(unit->EnvColor[1]);                              \
      eb = FloatToFixed(unit->EnvColor[2]);                              \
      ea = FloatToFixed(unit->EnvColor[3]);                              \
   }                                                                    \
   switch (format) {                                                    \
   case GL_ALPHA:                                                       \
   case GL_LUMINANCE:                                                   \
   case GL_INTENSITY:                                                   \
      comp = 1;                                                         \
      break;                                                            \
   case GL_LUMINANCE_ALPHA:                                             \
      comp = 2;                                                         \
      break;                                                            \
   case GL_RGB:                                                         \
      comp = 3;                                                         \
      break;                                                            \
   case GL_RGBA:                                                        \
      comp = 4;                                                         \
      break;                                                            \
   default:                                                             \
      gl_problem(NULL, "Bad texture format in persp_texture_triangle"); \
      return;                                                           \
   }                                                                    \
   if (filter == GL_NEAREST) {                                          \
      sscale = twidth;                                                  \
      tscale = theight;                                                 \
   }                                                                    \
   else {                                                               \
      sscale = FIXED_SCALE * twidth;                                    \
      tscale = FIXED_SCALE * theight;                                   \
   }                                                                    \
   tbytesline = obj->Image[b]->Width * comp;                            \
   tsize = theight * tbytesline;
   (void) pv;

#define SPAN1(DO_TEX,COMP)                                 \
        for (i=0;i<n;i++) {                                \
           GLfloat invQ = 1.0f / vv;                       \
           GLint s = (int)(SS * invQ) & smask;             \
           GLint t = (int)(TT * invQ) & tmask;             \
           GLint pos = COMP * ((t << twidth_log2) + s);    \
           GLubyte *tex00 = texture + pos;                 \
	   zspan[i] = FixedToDepth(ffz);                   \
           DO_TEX;                                         \
	   ffz += fdzdx;                                   \
           ffr += fdrdx;                                   \
	   ffg += fdgdx;                                   \
           ffb += fdbdx;                                   \
	   ffa += fdadx;                                   \
           SS += dSdx;                                     \
           TT += dTdx;                                     \
	   vv += dvdx;                                     \
           dest += 4;                                      \
	}

#define SPAN2(DO_TEX,COMP)                                 \
        for (i=0;i<n;i++) {                                \
           GLfloat invQ = 1.0f / vv;                       \
           GLfixed ffs = (int)(SS * invQ);                 \
           GLfixed fft = (int)(TT * invQ);                 \
	   GLint s = FixedToInt(ffs) & smask;              \
	   GLint t = FixedToInt(fft) & tmask;              \
           GLint sf = ffs & FIXED_FRAC_MASK;               \
           GLint tf = fft & FIXED_FRAC_MASK;               \
           GLint si = FIXED_FRAC_MASK - sf;                \
           GLint ti = FIXED_FRAC_MASK - tf;                \
           GLint pos = COMP * ((t << twidth_log2) + s);    \
           GLubyte *tex00 = texture + pos;                 \
           GLubyte *tex10 = tex00 + tbytesline;            \
           GLubyte *tex01 = tex00 + COMP;                  \
           GLubyte *tex11 = tex10 + COMP;                  \
           if (t == tmask) {                               \
              tex10 -= tsize;                              \
              tex11 -= tsize;                              \
           }                                               \
           if (s == smask) {                               \
              tex01 -= tbytesline;                         \
              tex11 -= tbytesline;                         \
           }                                               \
	   zspan[i] = FixedToDepth(ffz);                   \
           DO_TEX;                                         \
	   ffz += fdzdx;                                   \
           ffr += fdrdx;                                   \
	   ffg += fdgdx;                                   \
           ffb += fdbdx;                                   \
	   ffa += fdadx;                                   \
           SS += dSdx;                                     \
           TT += dTdx;                                     \
	   vv += dvdx;                                     \
           dest += 4;                                      \
	}

#define INNER_LOOP( LEFT, RIGHT, Y )	                   \
	{				                   \
	   GLint i, n = RIGHT-LEFT;	                   \
	   GLdepth zspan[MAX_WIDTH];	                   \
	   GLubyte rgba[MAX_WIDTH][4];	                   \
           (void)uu; /* please GCC */                      \
	   if (n>0) {                                      \
              GLfloat SS = ss * sscale;                    \
              GLfloat TT = tt * tscale;                    \
              GLfloat dSdx = dsdx * sscale;                \
              GLfloat dTdx = dtdx * tscale;                \
              GLubyte *dest = rgba[0];                     \
              switch (filter) {                            \
   	      case GL_NEAREST:                             \
		 switch (format) {                         \
                 case GL_RGB:                              \
	            switch (envmode) {                     \
	            case GL_MODULATE:                      \
                       SPAN1(NEAREST_RGB;MODULATE,3);      \
                       break;                              \
	            case GL_DECAL:                         \
                    case GL_REPLACE:                       \
                       SPAN1(NEAREST_RGB_REPLACE,3);       \
                       break;                              \
                    case GL_BLEND:                         \
                       SPAN1(NEAREST_RGB;BLEND,3);         \
                       break;                              \
                    case GL_ADD:                           \
                       SPAN1(NEAREST_RGB;ADD,3);           \
                       break;                              \
	            }                                      \
                    break;                                 \
		 case GL_RGBA:                             \
		    switch(envmode) {                      \
		    case GL_MODULATE:                      \
                       SPAN1(NEAREST_RGBA;MODULATE,4);     \
                       break;                              \
		    case GL_DECAL:                         \
                       SPAN1(NEAREST_RGBA;DECAL,4);        \
                       break;                              \
		    case GL_BLEND:                         \
                       SPAN1(NEAREST_RGBA;BLEND,4);        \
                       break;                              \
		    case GL_REPLACE:                       \
                       SPAN1(NEAREST_RGBA_REPLACE,4);      \
                       break;                              \
		    case GL_ADD:                           \
                       SPAN1(NEAREST_RGBA;ADD,4);          \
                       break;                              \
		    }                                      \
                    break;                                 \
	         }                                         \
                 break;                                    \
	      case GL_LINEAR:                              \
	         SS -= 0.5f * FIXED_SCALE * vv;            \
		 TT -= 0.5f * FIXED_SCALE * vv;            \
		 switch (format) {                         \
		 case GL_RGB:                              \
		    switch (envmode) {                     \
		    case GL_MODULATE:                      \
		       SPAN2(LINEAR_RGB;MODULATE,3);       \
                       break;                              \
		    case GL_DECAL:                         \
		    case GL_REPLACE:                       \
                       SPAN2(LINEAR_RGB;REPLACE,3);        \
                       break;                              \
		    case GL_BLEND:                         \
		       SPAN2(LINEAR_RGB;BLEND,3);          \
		       break;                              \
		    case GL_ADD:                           \
		       SPAN2(LINEAR_RGB;ADD,3);            \
		       break;                              \
		    }                                      \
		    break;                                 \
		 case GL_RGBA:                             \
		    switch (envmode) {                     \
		    case GL_MODULATE:                      \
		       SPAN2(LINEAR_RGBA;MODULATE,4);      \
		       break;                              \
		    case GL_DECAL:                         \
		       SPAN2(LINEAR_RGBA;DECAL,4);         \
		       break;                              \
		    case GL_BLEND:                         \
		       SPAN2(LINEAR_RGBA;BLEND,4);         \
		       break;                              \
		    case GL_REPLACE:                       \
		       SPAN2(LINEAR_RGBA;REPLACE,4);       \
		       break;                              \
		    case GL_ADD:                           \
		       SPAN2(LINEAR_RGBA;ADD,4);           \
		       break;                              \
		    }                                      \
		    break;                                 \
	         }                                         \
                 break;                                    \
	      }                                            \
	      gl_write_rgba_span( ctx, n, LEFT, Y, zspan,  \
				  rgba, GL_POLYGON );      \
              ffr = ffg = ffb = ffa = 0;                   \
	   }                                               \
	}


#include "tritemp.h"
#undef SPAN1
#undef SPAN2
}


/*
 * Render a smooth-shaded, textured, RGBA triangle.
 * Interpolate S,T,U with perspective correction, w/out mipmapping.
 * Note: we use texture coordinates S,T,U,V instead of S,T,R,Q because
 * R is already used for red.
 */
static void general_textured_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                                       GLuint v2, GLuint pv )
{
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_STUV 1
#define SETUP_CODE						\
   GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);	\
   GLint r, g, b, a;						\
   if (flat_shade) {						\
      r = VB->Color[pv][0];					\
      g = VB->Color[pv][1];					\
      b = VB->Color[pv][2];					\
      a = VB->Color[pv][3];					\
   }
#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   GLint i, n = RIGHT-LEFT;				\
	   GLdepth zspan[MAX_WIDTH];				\
	   GLubyte rgba[MAX_WIDTH][4];				\
           GLfloat s[MAX_WIDTH], t[MAX_WIDTH], u[MAX_WIDTH];	\
	   if (n>0) {						\
              if (flat_shade) {					\
                 for (i=0;i<n;i++) {				\
		    GLdouble invQ = 1.0 / vv;			\
		    zspan[i] = FixedToDepth(ffz);		\
		    rgba[i][RCOMP] = r;				\
		    rgba[i][GCOMP] = g;				\
		    rgba[i][BCOMP] = b;				\
		    rgba[i][ACOMP] = a;				\
		    s[i] = ss*invQ;				\
		    t[i] = tt*invQ;				\
		    u[i] = uu*invQ;				\
		    ffz += fdzdx;				\
		    ss += dsdx;					\
		    tt += dtdx;					\
		    uu += dudx;					\
		    vv += dvdx;					\
		 }						\
              }							\
              else {						\
                 for (i=0;i<n;i++) {				\
		    GLdouble invQ = 1.0 / vv;			\
		    zspan[i] = FixedToDepth(ffz);		\
		    rgba[i][RCOMP] = FixedToInt(ffr);		\
		    rgba[i][GCOMP] = FixedToInt(ffg);		\
		    rgba[i][BCOMP] = FixedToInt(ffb);		\
		    rgba[i][ACOMP] = FixedToInt(ffa);		\
		    s[i] = ss*invQ;				\
		    t[i] = tt*invQ;				\
		    u[i] = uu*invQ;				\
		    ffz += fdzdx;				\
		    ffr += fdrdx;				\
		    ffg += fdgdx;				\
		    ffb += fdbdx;				\
		    ffa += fdadx;				\
		    ss += dsdx;					\
		    tt += dtdx;					\
		    uu += dudx;					\
		    vv += dvdx;					\
		 }						\
              }							\
	      gl_write_texture_span( ctx, n, LEFT, Y, zspan,	\
                                     s, t, u, NULL, 		\
	                             rgba, NULL, GL_POLYGON );	\
	   }							\
	}

#include "tritemp.h"
}


/*
 * Render a smooth-shaded, textured, RGBA triangle with separate specular
 * color interpolation.
 * Interpolate S,T,U with perspective correction, w/out mipmapping.
 * Note: we use texture coordinates S,T,U,V instead of S,T,R,Q because
 * R is already used for red.
 */
static void general_textured_spec_triangle1( GLcontext *ctx, GLuint v0,
                                             GLuint v1, GLuint v2, GLuint pv,
                                             GLdepth zspan[MAX_WIDTH],
                                             GLubyte rgba[MAX_WIDTH][4],
                                             GLubyte spec[MAX_WIDTH][4] )
{
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_STUV 1
#define SETUP_CODE						\
   GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);	\
   GLint r, g, b, a, sr, sg, sb;				\
   if (flat_shade) {						\
      r = VB->Color[pv][0];					\
      g = VB->Color[pv][1];					\
      b = VB->Color[pv][2];					\
      a = VB->Color[pv][3];					\
      sr = VB->Specular[pv][0]; 				\
      sg = VB->Specular[pv][1]; 				\
      sb = VB->Specular[pv][2]; 				\
   }
#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   GLint i, n = RIGHT-LEFT;				\
           GLfloat s[MAX_WIDTH], t[MAX_WIDTH], u[MAX_WIDTH];	\
	   if (n>0) {						\
              if (flat_shade) {					\
                 for (i=0;i<n;i++) {				\
		    GLdouble invQ = 1.0 / vv;			\
		    zspan[i] = FixedToDepth(ffz);		\
		    rgba[i][RCOMP] = r;				\
		    rgba[i][GCOMP] = g;				\
		    rgba[i][BCOMP] = b;				\
		    rgba[i][ACOMP] = a;				\
		    spec[i][RCOMP] = sr;			\
		    spec[i][GCOMP] = sg;			\
		    spec[i][BCOMP] = sb;			\
		    s[i] = ss*invQ;				\
		    t[i] = tt*invQ;				\
		    u[i] = uu*invQ;				\
		    ffz += fdzdx;				\
		    ss += dsdx;					\
		    tt += dtdx;					\
		    uu += dudx;					\
		    vv += dvdx;					\
		 }						\
              }							\
              else {						\
                 for (i=0;i<n;i++) {				\
		    GLdouble invQ = 1.0 / vv;			\
		    zspan[i] = FixedToDepth(ffz);		\
		    rgba[i][RCOMP] = FixedToInt(ffr);		\
		    rgba[i][GCOMP] = FixedToInt(ffg);		\
		    rgba[i][BCOMP] = FixedToInt(ffb);		\
		    rgba[i][ACOMP] = FixedToInt(ffa);		\
		    spec[i][RCOMP] = FixedToInt(ffsr);		\
		    spec[i][GCOMP] = FixedToInt(ffsg);		\
		    spec[i][BCOMP] = FixedToInt(ffsb);		\
		    s[i] = ss*invQ;				\
		    t[i] = tt*invQ;				\
		    u[i] = uu*invQ;				\
		    ffz += fdzdx;				\
		    ffr += fdrdx;				\
		    ffg += fdgdx;				\
		    ffb += fdbdx;				\
		    ffa += fdadx;				\
		    ffsr += fdsrdx;				\
		    ffsg += fdsgdx;				\
		    ffsb += fdsbdx;				\
		    ss += dsdx;					\
		    tt += dtdx;					\
		    uu += dudx;					\
		    vv += dvdx;					\
		 }						\
              }							\
	      gl_write_texture_span( ctx, n, LEFT, Y, zspan,	\
                                     s, t, u, NULL, 		\
	                             rgba, spec, GL_POLYGON );	\
	   }							\
	}

#include "tritemp.h"
}



/*
 * Compute the lambda value (texture level value) for a fragment.
 */
static GLfloat compute_lambda( GLfloat s, GLfloat dsdx, GLfloat dsdy,
                               GLfloat t, GLfloat dtdx, GLfloat dtdy,
                               GLfloat invQ, GLfloat dqdx, GLfloat dqdy,
                               GLfloat width, GLfloat height ) 
{
   GLfloat dudx, dudy, dvdx, dvdy;
   GLfloat r1, r2, rho2;
   GLfloat invQ_width = invQ * width;
   GLfloat invQ_height = invQ * height;

   dudx = (dsdx - s*dqdx) * invQ_width;
   dudy = (dsdy - s*dqdy) * invQ_width;
   dvdx = (dtdx - t*dqdx) * invQ_height;
   dvdy = (dtdy - t*dqdy) * invQ_height;

   r1 = dudx * dudx + dudy * dudy;
   r2 = dvdx * dvdx + dvdy * dvdy;

   rho2 = r1 + r2;     /* used to be:  rho2 = MAX2(r1,r2); */
   ASSERT( rho2 >= 0.0 );

   /* return log base 2 of rho */
   return log(rho2) * 1.442695 * 0.5;       /* 1.442695 = 1/log(2) */
}



/*
 * Render a smooth-shaded, textured, RGBA triangle.
 * Interpolate S,T,U with perspective correction and compute lambda for
 * each fragment.  Lambda is used to determine whether to use the
 * minification or magnification filter.  If minification and using
 * mipmaps, lambda is also used to select the texture level of detail.
 */
static void lambda_textured_triangle1( GLcontext *ctx, GLuint v0, GLuint v1,
                                       GLuint v2, GLuint pv,
                                       GLfloat s[MAX_WIDTH],
                                       GLfloat t[MAX_WIDTH],
                                       GLfloat u[MAX_WIDTH] )
{
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_STUV 1

#define SETUP_CODE							\
   const struct gl_texture_object *obj = ctx->Texture.Unit[0].Current;	\
   const GLint baseLevel = obj->BaseLevel;				\
   const struct gl_texture_image *texImage = obj->Image[baseLevel];	\
   const GLfloat twidth = (GLfloat) texImage->Width;			\
   const GLfloat theight = (GLfloat) texImage->Height;			\
   const GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);	\
   GLint r, g, b, a;							\
   if (flat_shade) {							\
      r = VB->Color[pv][0];						\
      g = VB->Color[pv][1];						\
      b = VB->Color[pv][2];						\
      a = VB->Color[pv][3];						\
   }

#define INNER_LOOP( LEFT, RIGHT, Y )					\
	{								\
	   GLint i, n = RIGHT-LEFT;					\
	   GLdepth zspan[MAX_WIDTH];					\
	   GLubyte rgba[MAX_WIDTH][4];					\
	   GLfloat lambda[MAX_WIDTH];					\
	   if (n>0) {							\
	      if (flat_shade) {						\
		 for (i=0;i<n;i++) {					\
		    GLdouble invQ = 1.0 / vv;				\
		    zspan[i] = FixedToDepth(ffz);			\
		    rgba[i][RCOMP] = r;					\
		    rgba[i][GCOMP] = g;					\
		    rgba[i][BCOMP] = b;					\
		    rgba[i][ACOMP] = a;					\
		    s[i] = ss*invQ;					\
		    t[i] = tt*invQ;					\
		    u[i] = uu*invQ;					\
		    lambda[i] = compute_lambda( s[i], dsdx, dsdy,	\
						t[i], dtdx, dtdy,	\
						invQ, dvdx, dvdy,	\
						twidth, theight );	\
		    ffz += fdzdx;					\
		    ss += dsdx;						\
		    tt += dtdx;						\
		    uu += dudx;						\
		    vv += dvdx;						\
		 }							\
              }								\
              else {							\
		 for (i=0;i<n;i++) {					\
		    GLdouble invQ = 1.0 / vv;				\
		    zspan[i] = FixedToDepth(ffz);			\
		    rgba[i][RCOMP] = FixedToInt(ffr);			\
		    rgba[i][GCOMP] = FixedToInt(ffg);			\
		    rgba[i][BCOMP] = FixedToInt(ffb);			\
		    rgba[i][ACOMP] = FixedToInt(ffa);			\
		    s[i] = ss*invQ;					\
		    t[i] = tt*invQ;					\
		    u[i] = uu*invQ;					\
		    lambda[i] = compute_lambda( s[i], dsdx, dsdy,	\
						t[i], dtdx, dtdy,	\
						invQ, dvdx, dvdy,	\
						twidth, theight );	\
		    ffz += fdzdx;					\
		    ffr += fdrdx;					\
		    ffg += fdgdx;					\
		    ffb += fdbdx;					\
		    ffa += fdadx;					\
		    ss += dsdx;						\
		    tt += dtdx;						\
		    uu += dudx;						\
		    vv += dvdx;						\
		 }							\
              }								\
	      gl_write_texture_span( ctx, n, LEFT, Y, zspan,		\
                                     s, t, u, lambda,	 		\
	                             rgba, NULL, GL_POLYGON );		\
	   }								\
	}

#include "tritemp.h"
}



/*
 * Render a smooth-shaded, textured, RGBA triangle with separate specular
 * interpolation.
 * Interpolate S,T,U with perspective correction and compute lambda for
 * each fragment.  Lambda is used to determine whether to use the
 * minification or magnification filter.  If minification and using
 * mipmaps, lambda is also used to select the texture level of detail.
 */
static void lambda_textured_spec_triangle1( GLcontext *ctx, GLuint v0,
                                            GLuint v1, GLuint v2, GLuint pv,
                                            GLfloat s[MAX_WIDTH],
                                            GLfloat t[MAX_WIDTH],
                                            GLfloat u[MAX_WIDTH] )
{
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_STUV 1

#define SETUP_CODE							\
   const struct gl_texture_object *obj = ctx->Texture.Unit[0].Current;	\
   const GLint baseLevel = obj->BaseLevel;				\
   const struct gl_texture_image *texImage = obj->Image[baseLevel];	\
   const GLfloat twidth = (GLfloat) texImage->Width;			\
   const GLfloat theight = (GLfloat) texImage->Height;			\
   const GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);	\
   GLint r, g, b, a, sr, sg, sb;					\
   if (flat_shade) {							\
      r = VB->Color[pv][0];						\
      g = VB->Color[pv][1];						\
      b = VB->Color[pv][2];						\
      a = VB->Color[pv][3];						\
      sr = VB->Specular[pv][0];						\
      sg = VB->Specular[pv][1];						\
      sb = VB->Specular[pv][2];						\
   }

#define INNER_LOOP( LEFT, RIGHT, Y )					\
	{								\
	   GLint i, n = RIGHT-LEFT;					\
	   GLdepth zspan[MAX_WIDTH];					\
	   GLubyte spec[MAX_WIDTH][4];					\
           GLubyte rgba[MAX_WIDTH][4];					\
	   GLfloat lambda[MAX_WIDTH];					\
	   if (n>0) {							\
	      if (flat_shade) {						\
		 for (i=0;i<n;i++) {					\
		    GLdouble invQ = 1.0 / vv;				\
		    zspan[i] = FixedToDepth(ffz);			\
		    rgba[i][RCOMP] = r;					\
		    rgba[i][GCOMP] = g;					\
		    rgba[i][BCOMP] = b;					\
		    rgba[i][ACOMP] = a;					\
		    spec[i][RCOMP] = sr;				\
		    spec[i][GCOMP] = sg;				\
		    spec[i][BCOMP] = sb;				\
		    s[i] = ss*invQ;					\
		    t[i] = tt*invQ;					\
		    u[i] = uu*invQ;					\
		    lambda[i] = compute_lambda( s[i], dsdx, dsdy,	\
						t[i], dtdx, dtdy,	\
						invQ, dvdx, dvdy,	\
						twidth, theight );	\
		    ffz += fdzdx;					\
		    ss += dsdx;						\
		    tt += dtdx;						\
		    uu += dudx;						\
		    vv += dvdx;						\
		 }							\
              }								\
              else {							\
		 for (i=0;i<n;i++) {					\
		    GLdouble invQ = 1.0 / vv;				\
		    zspan[i] = FixedToDepth(ffz);			\
		    rgba[i][RCOMP] = FixedToInt(ffr);			\
		    rgba[i][GCOMP] = FixedToInt(ffg);			\
		    rgba[i][BCOMP] = FixedToInt(ffb);			\
		    rgba[i][ACOMP] = FixedToInt(ffa);			\
		    spec[i][RCOMP] = FixedToInt(ffsr);			\
		    spec[i][GCOMP] = FixedToInt(ffsg);			\
		    spec[i][BCOMP] = FixedToInt(ffsb);			\
		    s[i] = ss*invQ;					\
		    t[i] = tt*invQ;					\
		    u[i] = uu*invQ;					\
		    lambda[i] = compute_lambda( s[i], dsdx, dsdy,	\
						t[i], dtdx, dtdy,	\
						invQ, dvdx, dvdy,	\
						twidth, theight );	\
		    ffz += fdzdx;					\
		    ffr += fdrdx;					\
		    ffg += fdgdx;					\
		    ffb += fdbdx;					\
		    ffa += fdadx;					\
		    ffsr += fdsrdx;					\
		    ffsg += fdsgdx;					\
		    ffsb += fdsbdx;					\
		    ss += dsdx;						\
		    tt += dtdx;						\
		    uu += dudx;						\
		    vv += dvdx;						\
		 }							\
              }								\
	      gl_write_texture_span( ctx, n, LEFT, Y, zspan,		\
                                     s, t, u, lambda,	 		\
	                             rgba, spec, GL_POLYGON );		\
	   }								\
	}

#include "tritemp.h"
}



/*
 * This is the big one!
 * Interpolate Z, RGB, Alpha, and two sets of texture coordinates.
 * Yup, it's slow.
 */
static void lambda_multitextured_triangle1( GLcontext *ctx, GLuint v0,
                                      GLuint v1, GLuint v2, GLuint pv,
                                      GLubyte rgba[MAX_WIDTH][4],
                                      GLfloat s[MAX_TEXTURE_UNITS][MAX_WIDTH],
                                      GLfloat t[MAX_TEXTURE_UNITS][MAX_WIDTH] )
{
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_STUV 1
#define INTERP_STUV1 1

#define SETUP_CODE							\
   const struct gl_texture_object *obj0 = ctx->Texture.Unit[0].Current;	\
   const GLint baseLevel0 = obj0->BaseLevel;				\
   const struct gl_texture_image *texImage0 = obj0->Image[baseLevel0];	\
   const GLfloat twidth0 = (GLfloat) texImage0->Width;			\
   const GLfloat theight0 = (GLfloat) texImage0->Height;		\
   const struct gl_texture_object *obj1 = ctx->Texture.Unit[1].Current;	\
   const GLint baseLevel1 = obj1->BaseLevel;				\
   const struct gl_texture_image *texImage1 = obj1->Image[baseLevel1];	\
   const GLfloat twidth1 = (GLfloat) texImage1->Width;			\
   const GLfloat theight1 = (GLfloat) texImage1->Height;		\
   const GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);	\
   GLint r, g, b, a;							\
   if (flat_shade) {							\
      r = VB->Color[pv][0];						\
      g = VB->Color[pv][1];						\
      b = VB->Color[pv][2];						\
      a = VB->Color[pv][3];						\
   }

#define INNER_LOOP( LEFT, RIGHT, Y )					\
	{								\
	   GLint i, n = RIGHT-LEFT;					\
	   GLdepth zspan[MAX_WIDTH];					\
           GLfloat u[MAX_TEXTURE_UNITS][MAX_WIDTH];			\
           GLfloat lambda[MAX_TEXTURE_UNITS][MAX_WIDTH];		\
	   if (n>0) {							\
	      if (flat_shade) {						\
		 for (i=0;i<n;i++) {					\
		    GLdouble invQ = 1.0 / vv;				\
		    GLdouble invQ1 = 1.0 / vv1;				\
		    zspan[i] = FixedToDepth(ffz);			\
		    rgba[i][RCOMP] = r;					\
		    rgba[i][GCOMP] = g;					\
		    rgba[i][BCOMP] = b;					\
		    rgba[i][ACOMP] = a;					\
		    s[0][i] = ss*invQ;					\
		    t[0][i] = tt*invQ;					\
		    u[0][i] = uu*invQ;					\
		    lambda[0][i] = compute_lambda( s[0][i], dsdx, dsdy,	\
						   t[0][i], dtdx, dtdy,	\
						   invQ, dvdx, dvdy,	\
						   twidth0, theight0 );	\
		    s[1][i] = ss1*invQ1;				\
		    t[1][i] = tt1*invQ1;				\
		    u[1][i] = uu1*invQ1;				\
		    lambda[1][i] = compute_lambda( s[1][i], ds1dx, ds1dy,	\
						   t[1][i], dt1dx, dt1dy,	\
						   invQ1, dvdx, dvdy,	\
						   twidth1, theight1 );	\
		    ffz += fdzdx;					\
		    ss += dsdx;						\
		    tt += dtdx;						\
		    uu += dudx;						\
		    vv += dvdx;						\
		    ss1 += ds1dx;					\
		    tt1 += dt1dx;					\
		    uu1 += du1dx;					\
		    vv1 += dv1dx;					\
		 }							\
              }								\
              else {							\
		 for (i=0;i<n;i++) {					\
		    GLdouble invQ = 1.0 / vv;				\
		    GLdouble invQ1 = 1.0 / vv1;				\
		    zspan[i] = FixedToDepth(ffz);			\
		    rgba[i][RCOMP] = FixedToInt(ffr);			\
		    rgba[i][GCOMP] = FixedToInt(ffg);			\
		    rgba[i][BCOMP] = FixedToInt(ffb);			\
		    rgba[i][ACOMP] = FixedToInt(ffa);			\
		    s[0][i] = ss*invQ;					\
		    t[0][i] = tt*invQ;					\
		    u[0][i] = uu*invQ;					\
		    lambda[0][i] = compute_lambda( s[0][i], dsdx, dsdy,	\
						   t[0][i], dtdx, dtdy,	\
						   invQ, dvdx, dvdy,	\
						   twidth0, theight0 );	\
		    s[1][i] = ss1*invQ1;				\
		    t[1][i] = tt1*invQ1;				\
		    u[1][i] = uu1*invQ1;				\
		    lambda[1][i] = compute_lambda( s[1][i], ds1dx, ds1dy,	\
						   t[1][i], dt1dx, dt1dy,	\
						   invQ1, dvdx, dvdy,	\
						   twidth1, theight1 );	\
		    ffz += fdzdx;					\
		    ffr += fdrdx;					\
		    ffg += fdgdx;					\
		    ffb += fdbdx;					\
		    ffa += fdadx;					\
		    ss += dsdx;						\
		    tt += dtdx;						\
		    uu += dudx;						\
		    vv += dvdx;						\
		    ss1 += ds1dx;					\
		    tt1 += dt1dx;					\
		    uu1 += du1dx;					\
		    vv1 += dv1dx;					\
		 }							\
              }								\
	      gl_write_multitexture_span( ctx, 2, n, LEFT, Y, zspan,	\
                                          s, t, u, lambda,	 	\
	                                  rgba, NULL, GL_POLYGON );	\
	   }								\
	}

#include "tritemp.h"
}


/*
 * These wrappers are needed to deal with the 32KB / stack frame limit
 * on Mac / PowerPC systems.
 */

static void general_textured_spec_triangle(GLcontext *ctx, GLuint v0,
                                           GLuint v1, GLuint v2, GLuint pv)
{
   GLdepth zspan[MAX_WIDTH];
   GLubyte rgba[MAX_WIDTH][4], spec[MAX_WIDTH][4];
   general_textured_spec_triangle1(ctx,v0,v1,v2,pv,zspan,rgba,spec);
}

static void lambda_textured_triangle( GLcontext *ctx, GLuint v0,
                                      GLuint v1, GLuint v2, GLuint pv )
{
   GLfloat s[MAX_WIDTH], t[MAX_WIDTH], u[MAX_WIDTH];
   lambda_textured_triangle1(ctx,v0,v1,v2,pv,s,t,u);
}

static void lambda_textured_spec_triangle( GLcontext *ctx, GLuint v0,
                                           GLuint v1, GLuint v2, GLuint pv )
{
   GLfloat s[MAX_WIDTH];
   GLfloat t[MAX_WIDTH];
   GLfloat u[MAX_WIDTH];
   lambda_textured_spec_triangle1(ctx,v0,v1,v2,pv,s,t,u);
}

static void lambda_multitextured_triangle( GLcontext *ctx, GLuint v0,
                                           GLuint v1, GLuint v2, GLuint pv)
{
   GLubyte rgba[MAX_WIDTH][4];
   GLfloat s[MAX_TEXTURE_UNITS][MAX_WIDTH];
   GLfloat t[MAX_TEXTURE_UNITS][MAX_WIDTH];
   lambda_multitextured_triangle1(ctx,v0,v1,v2,pv,rgba,s,t);
}



/*
 * Null rasterizer for measuring transformation speed.
 */
static void null_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
                           GLuint v2, GLuint pv )
{
   (void) ctx;
   (void) v0;
   (void) v1;
   (void) v2;
   (void) pv;
}


#if 0
# define dputs(s) puts(s)
#else
# define dputs(s)
#endif

/*
 * Determine which triangle rendering function to use given the current
 * rendering context.
 */
void gl_set_triangle_function( GLcontext *ctx )
{
   GLboolean rgbmode = ctx->Visual->RGBAflag;

   if (ctx->RenderMode==GL_RENDER) {
      if (ctx->NoRaster) {
         ctx->Driver.TriangleFunc = null_triangle;
         return;
      }
      if (ctx->Driver.TriangleFunc) {
         /* Device driver will draw triangles. */
      }
      else if (ctx->Texture.ReallyEnabled) {
         /* Ugh, we do a _lot_ of tests to pick the best textured tri func */
	 int format, filter;
	 const struct gl_texture_object *current2Dtex = ctx->Texture.Unit[0].Current2D;
         const struct gl_texture_image *image;
         /* First see if we can used an optimized 2-D texture function */
         if (ctx->Texture.ReallyEnabled==TEXTURE0_2D
             && current2Dtex->WrapS==GL_REPEAT
	     && current2Dtex->WrapT==GL_REPEAT
             && (image = current2Dtex->Image[current2Dtex->BaseLevel])  /* correct? */
             && image->Border==0
             && ((format = image->Format)==GL_RGB || format==GL_RGBA)
	     && (filter = current2Dtex->MinFilter)==current2Dtex->MagFilter
	     && ctx->Light.Model.ColorControl==GL_SINGLE_COLOR) {

	    if (ctx->Hint.PerspectiveCorrection==GL_FASTEST) {
	     
	       if (filter==GL_NEAREST
		   && format==GL_RGB
		   && (ctx->Texture.Unit[0].EnvMode==GL_REPLACE
		       || ctx->Texture.Unit[0].EnvMode==GL_DECAL)
		   && ((ctx->RasterMask==DEPTH_BIT
			&& ctx->Depth.Func==GL_LESS
			&& ctx->Depth.Mask==GL_TRUE)
		       || ctx->RasterMask==0)
		   && ctx->Polygon.StippleFlag==GL_FALSE) {

		  if (ctx->RasterMask==DEPTH_BIT) {
		     ctx->Driver.TriangleFunc = simple_z_textured_triangle;
		     dputs("simple_z_textured_triangle");
		  }
		  else {
		     ctx->Driver.TriangleFunc = simple_textured_triangle;
		     dputs("simple_textured_triangle");
		  }
	       }
	       else {
		  ctx->Driver.TriangleFunc = affine_textured_triangle;
		  dputs("affine_textured_triangle");
	       }
	    }
	    else {
	       ctx->Driver.TriangleFunc = persp_textured_triangle;
	       dputs("persp_textured_triangle");
	    }
	 }
         else {
            /* More complicated textures (mipmap, multi-tex, sep specular) */
            GLboolean needLambda;
            /* if mag filter != min filter we need to compute lambda */
            const struct gl_texture_object *obj0 = ctx->Texture.Unit[0].Current;
            const struct gl_texture_object *obj1 = ctx->Texture.Unit[1].Current;
            if (obj0 && obj0->MinFilter != obj0->MagFilter)
               needLambda = GL_TRUE;
            else if (obj1 && obj1->MinFilter != obj1->MagFilter)
               needLambda = GL_TRUE;
            else
               needLambda = GL_FALSE;
            if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
               /* multi-texture! */
               ctx->Driver.TriangleFunc = lambda_multitextured_triangle;
	       dputs("lambda_multitextured_triangle");
            }
            else if (ctx->Light.Model.ColorControl==GL_SINGLE_COLOR) {
               if (needLambda) {
                  ctx->Driver.TriangleFunc = lambda_textured_triangle;
		  dputs("lambda_textured_triangle");
	       }
               else {
                  ctx->Driver.TriangleFunc = general_textured_triangle;
		  dputs("general_textured_triangle");
	       }
            }
            else {
               /* seprate specular color interpolation */
               if (needLambda) {
                  ctx->Driver.TriangleFunc = lambda_textured_spec_triangle;
		  dputs("lambda_textured_spec_triangle");
	       }
               else {
                  ctx->Driver.TriangleFunc = general_textured_spec_triangle;
		  dputs("general_textured_spec_triangle");
	       }
            }
         }
      }
      else {
	 if (ctx->Light.ShadeModel==GL_SMOOTH) {
	    /* smooth shaded, no texturing, stippled or some raster ops */
            if (rgbmode)
               ctx->Driver.TriangleFunc = smooth_rgba_triangle;
            else
               ctx->Driver.TriangleFunc = smooth_ci_triangle;
	 }
	 else {
	    /* flat shaded, no texturing, stippled or some raster ops */
            if (rgbmode)
               ctx->Driver.TriangleFunc = flat_rgba_triangle;
            else
               ctx->Driver.TriangleFunc = flat_ci_triangle;
	 }
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      ctx->Driver.TriangleFunc = feedback_triangle;
   }
   else {
      /* GL_SELECT mode */
      ctx->Driver.TriangleFunc = select_triangle;
   }
}
