/* Id: points.c,v 1.3 1999/02/26 08:52:36 martin Exp $ */

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
/* $XFree86$ */


/*
 * Log: points.c,v $
 * Revision 1.3  1999/02/26 08:52:36  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.10  1999/02/24 22:48:07  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.9  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.8  1998/12/01 02:42:27  brianp
 * patched per MJK for multi-textured points
 *
 * Revision 3.7  1998/11/08 22:34:07  brianp
 * renamed texture sets to texture units
 *
 * Revision 3.6  1998/11/03 01:16:57  brianp
 * added Texture.ReallyEnabled to simply testing for enabled textures
 *
 * Revision 3.5  1998/07/29 04:08:09  brianp
 * feedback returned wrong point color
 *
 * Revision 3.4  1998/06/07 22:18:52  brianp
 * implemented GL_EXT_multitexture extension
 *
 * Revision 3.3  1998/03/27 04:17:31  brianp
 * fixed G++ warnings
 *
 * Revision 3.2  1998/02/08 20:19:12  brianp
 * removed unneeded headers
 *
 * Revision 3.1  1998/02/04 00:44:29  brianp
 * fixed casts and conditional expression problems for Amiga StormC compiler
 *
 * Revision 3.0  1998/01/31 21:01:27  brianp
 * initial rev
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "mmath.h"
#include "context.h"
#include "feedback.h"
#include "macros.h"
#include "pb.h"
#include "span.h"
#include "texstate.h"
#include "types.h"
#include "vb.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



void gl_PointSize( GLcontext *ctx, GLfloat size )
{
   if (size<=0.0) {
      gl_error( ctx, GL_INVALID_VALUE, "glPointSize" );
      return;
   }
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPointSize" );
      return;
   }
   ctx->Point.Size = size;
   ctx->NewState |= NEW_RASTER_OPS;
}



void gl_PointParameterfvEXT( GLcontext *ctx, GLenum pname,
                                    const GLfloat *params)
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPointParameterfvEXT" );
      return;
   }
   if(pname==GL_DISTANCE_ATTENUATION_EXT) {
         COPY_3V(ctx->Point.Params,params);
   } else {
        if (*params<0.0 ) {
            gl_error( ctx, GL_INVALID_VALUE, "glPointParameterfvEXT" );
            return;
        }
        switch (pname) {
            case GL_POINT_SIZE_MIN_EXT:
                ctx->Point.MinSize=*params;
                break;
            case GL_POINT_SIZE_MAX_EXT:
                ctx->Point.MaxSize=*params;
                break;
            case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
                ctx->Point.Threshold=*params;
                break;
            default:
                gl_error( ctx, GL_INVALID_ENUM, "glPointParameterfvEXT" );
                return;
        }
   }
   ctx->NewState |= NEW_RASTER_OPS;
}


/**********************************************************************/
/*****                    Rasterization                           *****/
/**********************************************************************/


/*
 * There are 3 pairs (RGBA, CI) of point rendering functions:
 *   1. simple:  size=1 and no special rasterization functions (fastest)
 *   2. size1:  size=1 and any rasterization functions
 *   3. general:  any size and rasterization functions (slowest)
 *
 * All point rendering functions take the same two arguments: first and
 * last which specify that the points specified by VB[first] through
 * VB[last] are to be rendered.
 */



/*
 * Put points in feedback buffer.
 */
static void feedback_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint texUnit = ctx->Texture.CurrentTransformUnit;
   GLuint i;

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLfloat x, y, z, w, invq;
         GLfloat color[4], texcoord[4];
         x = VB->Win[i][0];
         y = VB->Win[i][1];
         z = VB->Win[i][2] / DEPTH_SCALE;
         w = VB->Clip[i][3];

         /* convert color from integer back to a float in [0,1] */
         color[0] = VB->Color[i][0] * (1.0F / 255.0F);
         color[1] = VB->Color[i][1] * (1.0F / 255.0F);
         color[2] = VB->Color[i][2] * (1.0F / 255.0F);
         color[3] = VB->Color[i][3] * (1.0F / 255.0F);

         invq = 1.0F / VB->MultiTexCoord[texUnit][i][3];
         texcoord[0] = VB->MultiTexCoord[texUnit][i][0] * invq;
         texcoord[1] = VB->MultiTexCoord[texUnit][i][1] * invq;
         texcoord[2] = VB->MultiTexCoord[texUnit][i][2] * invq;
         texcoord[3] = VB->MultiTexCoord[texUnit][i][3];

         FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_POINT_TOKEN );
         gl_feedback_vertex( ctx, x, y, z, w, color,
                             (GLfloat) VB->Index[i], texcoord );
      }
   }
}



/*
 * Put points in selection buffer.
 */
static void select_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint i;

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         gl_update_hitflag( ctx, VB->Win[i][2] / DEPTH_SCALE );
      }
   }
}


/*
 * CI points with size == 1.0
 */
void size1_ci_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLfloat *win;
   GLint *pbx = PB->x, *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLuint *pbi = PB->i;
   GLuint pbcount = PB->count;
   GLuint i;

   win = &VB->Win[first][0];
   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         pbx[pbcount] = (GLint)  win[0];
         pby[pbcount] = (GLint)  win[1];
         pbz[pbcount] = (GLint) (win[2] + ctx->PointZoffset);
         pbi[pbcount] = VB->Index[i];
         pbcount++;
      }
      win += 3;
   }
   PB->count = pbcount;
   PB_CHECK_FLUSH(ctx, PB)
}



/*
 * RGBA points with size == 1.0
 */
static void size1_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint red, green, blue, alpha;

         x = (GLint)  VB->Win[i][0];
         y = (GLint)  VB->Win[i][1];
         z = (GLint) (VB->Win[i][2] + ctx->PointZoffset);

         red   = VB->Color[i][0];
         green = VB->Color[i][1];
         blue  = VB->Color[i][2];
         alpha = VB->Color[i][3];

         PB_WRITE_RGBA_PIXEL( PB, x, y, z, red, green, blue, alpha );
      }
   }
   PB_CHECK_FLUSH(ctx,PB)
}



/*
 * General CI points.
 */
static void general_ci_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLint isize;

   isize = (GLint) (CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE) + 0.5F);

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;

         x = (GLint)  VB->Win[i][0];
         y = (GLint)  VB->Win[i][1];
         z = (GLint) (VB->Win[i][2] + ctx->PointZoffset);

         if (isize&1) {
            /* odd size */
            x0 = x - isize/2;
            x1 = x + isize/2;
            y0 = y - isize/2;
            y1 = y + isize/2;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 0.5F) - isize/2;
            x1 = x0 + isize-1;
            y0 = (GLint) (y + 0.5F) - isize/2;
            y1 = y0 + isize-1;
         }

         PB_SET_INDEX( ctx, PB, VB->Index[i] );

         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               PB_WRITE_PIXEL( PB, ix, iy, z );
            }
         }
         PB_CHECK_FLUSH(ctx,PB)
      }
   }
}


/*
 * General RGBA points.
 */
static void general_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLint isize;

   isize = (GLint) (CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE) + 0.5F);

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;

         x = (GLint)  VB->Win[i][0];
         y = (GLint)  VB->Win[i][1];
         z = (GLint) (VB->Win[i][2] + ctx->PointZoffset);

         if (isize&1) {
            /* odd size */
            x0 = x - isize/2;
            x1 = x + isize/2;
            y0 = y - isize/2;
            y1 = y + isize/2;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 0.5F) - isize/2;
            x1 = x0 + isize-1;
            y0 = (GLint) (y + 0.5F) - isize/2;
            y1 = y0 + isize-1;
         }

         PB_SET_COLOR( ctx, PB,
                       VB->Color[i][0],
                       VB->Color[i][1],
                       VB->Color[i][2],
                       VB->Color[i][3] );

         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               PB_WRITE_PIXEL( PB, ix, iy, z );
            }
         }
         PB_CHECK_FLUSH(ctx,PB)
      }
   }
}




/*
 * Textured RGBA points.
 */
static void textured_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;
         GLint isize;
         GLint red, green, blue, alpha;
         GLfloat s, t, u;

         x = (GLint)  VB->Win[i][0];
         y = (GLint)  VB->Win[i][1];
         z = (GLint) (VB->Win[i][2] + ctx->PointZoffset);

         isize = (GLint)
                   (CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE) + 0.5F);
         if (isize<1) {
            isize = 1;
         }

         if (isize&1) {
            /* odd size */
            x0 = x - isize/2;
            x1 = x + isize/2;
            y0 = y - isize/2;
            y1 = y + isize/2;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 0.5F) - isize/2;
            x1 = x0 + isize-1;
            y0 = (GLint) (y + 0.5F) - isize/2;
            y1 = y0 + isize-1;
         }

         red   = VB->Color[i][0];
         green = VB->Color[i][1];
         blue  = VB->Color[i][2];
         alpha = VB->Color[i][3];
         s = VB->MultiTexCoord[0][i][0] / VB->MultiTexCoord[0][i][3];
         t = VB->MultiTexCoord[0][i][1] / VB->MultiTexCoord[0][i][3];
         u = VB->MultiTexCoord[0][i][2] / VB->MultiTexCoord[0][i][3];

/*    don't think this is needed
         PB_SET_COLOR( red, green, blue, alpha );
*/

         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               PB_WRITE_TEX_PIXEL( PB, ix, iy, z, red, green, blue, alpha, s, t, u );
            }
         }
         PB_CHECK_FLUSH(ctx,PB)
      }
   }
}


/*
 * Multitextured RGBA points.
 */
static void multitextured_rgba_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;
         GLint isize;
         GLint red, green, blue, alpha;
         GLfloat s, t, u;
         GLfloat s1, t1, u1;

         x = (GLint)  VB->Win[i][0];
         y = (GLint)  VB->Win[i][1];
         z = (GLint) (VB->Win[i][2] + ctx->PointZoffset);

         isize = (GLint)
                   (CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE) + 0.5F);
         if (isize<1) {
            isize = 1;
         }

         if (isize&1) {
            /* odd size */
            x0 = x - isize/2;
            x1 = x + isize/2;
            y0 = y - isize/2;
            y1 = y + isize/2;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 0.5F) - isize/2;
            x1 = x0 + isize-1;
            y0 = (GLint) (y + 0.5F) - isize/2;
            y1 = y0 + isize-1;
         }

         red   = VB->Color[i][0];
         green = VB->Color[i][1];
         blue  = VB->Color[i][2];
         alpha = VB->Color[i][3];
         s = VB->MultiTexCoord[0][i][0] / VB->MultiTexCoord[0][i][3];
         t = VB->MultiTexCoord[0][i][1] / VB->MultiTexCoord[0][i][3];
         u = VB->MultiTexCoord[0][i][2] / VB->MultiTexCoord[0][i][3];
         s1 = VB->MultiTexCoord[1][i][0] / VB->MultiTexCoord[1][i][3];
         t1 = VB->MultiTexCoord[1][i][1] / VB->MultiTexCoord[1][i][3];
         u1 = VB->MultiTexCoord[1][i][2] / VB->MultiTexCoord[1][i][3];

/*    don't think this is needed
         PB_SET_COLOR( red, green, blue, alpha );
*/

         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               PB_WRITE_MULTITEX_PIXEL( PB, ix, iy, z, red, green, blue, alpha, s, t, u, s1, t1, u1 );
            }
         }
         PB_CHECK_FLUSH(ctx,PB)
      }
   }
}


/*
 * Antialiased points with or without texture mapping.
 */
static void antialiased_rgba_points( GLcontext *ctx,
                                     GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLfloat radius, rmin, rmax, rmin2, rmax2, cscale;

   radius = CLAMP( ctx->Point.Size, MIN_POINT_SIZE, MAX_POINT_SIZE ) * 0.5F;
   rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
   rmax = radius + 0.7071F;
   rmin2 = rmin*rmin;
   rmax2 = rmax*rmax;
   cscale = 256.0F / (rmax2-rmin2);

   if (ctx->Texture.ReallyEnabled) {
      for (i=first;i<=last;i++) {
         if (VB->ClipMask[i]==0) {
            GLint xmin, ymin, xmax, ymax;
            GLint x, y, z;
            GLint red, green, blue, alpha;
            GLfloat s, t, u;
            GLfloat s1, t1, u1;

            xmin = (GLint) (VB->Win[i][0] - radius);
            xmax = (GLint) (VB->Win[i][0] + radius);
            ymin = (GLint) (VB->Win[i][1] - radius);
            ymax = (GLint) (VB->Win[i][1] + radius);
            z = (GLint) (VB->Win[i][2] + ctx->PointZoffset);

            red   = VB->Color[i][0];
            green = VB->Color[i][1];
            blue  = VB->Color[i][2];
            s = VB->MultiTexCoord[0][i][0] / VB->MultiTexCoord[0][i][3];
            t = VB->MultiTexCoord[0][i][1] / VB->MultiTexCoord[0][i][3];
            u = VB->MultiTexCoord[0][i][2] / VB->MultiTexCoord[0][i][3];

            if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
               /* Multitextured!  This is probably a slow enough path that
                  there's no reason to specialize the multitexture case. */
               s1 = VB->MultiTexCoord[1][i][0] / VB->MultiTexCoord[1][i][3];
               t1 = VB->MultiTexCoord[1][i][1] / VB->MultiTexCoord[1][i][3];
               u1 = VB->MultiTexCoord[1][i][2] / VB->MultiTexCoord[1][i][3];
            }

            for (y=ymin;y<=ymax;y++) {
               for (x=xmin;x<=xmax;x++) {
                  GLfloat dx = x/*+0.5F*/ - VB->Win[i][0];
                  GLfloat dy = y/*+0.5F*/ - VB->Win[i][1];
                  GLfloat dist2 = dx*dx + dy*dy;
                  if (dist2<rmax2) {
                     alpha = VB->Color[i][3];
                     if (dist2>=rmin2) {
                        GLint coverage = (GLint) (256.0F-(dist2-rmin2)*cscale);
                        /* coverage is in [0,256] */
                        alpha = (alpha * coverage) >> 8;
                     }
                     if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
                        PB_WRITE_MULTITEX_PIXEL( PB, x,y,z, red, green, blue, alpha, s, t, u, s1, t1, u1 );
                     } else {
                        PB_WRITE_TEX_PIXEL( PB, x,y,z, red, green, blue, alpha, s, t, u );
                     }
                  }
               }
            }

            PB_CHECK_FLUSH(ctx,PB)
         }
      }
   }
   else {
      /* Not texture mapped */
      for (i=first;i<=last;i++) {
         if (VB->ClipMask[i]==0) {
            GLint xmin, ymin, xmax, ymax;
            GLint x, y, z;
            GLint red, green, blue, alpha;

            xmin = (GLint) (VB->Win[i][0] - radius);
            xmax = (GLint) (VB->Win[i][0] + radius);
            ymin = (GLint) (VB->Win[i][1] - radius);
            ymax = (GLint) (VB->Win[i][1] + radius);
            z = (GLint) (VB->Win[i][2] + ctx->PointZoffset);

            red   = VB->Color[i][0];
            green = VB->Color[i][1];
            blue  = VB->Color[i][2];

            for (y=ymin;y<=ymax;y++) {
               for (x=xmin;x<=xmax;x++) {
                  GLfloat dx = x/*+0.5F*/ - VB->Win[i][0];
                  GLfloat dy = y/*+0.5F*/ - VB->Win[i][1];
                  GLfloat dist2 = dx*dx + dy*dy;
                  if (dist2<rmax2) {
                     alpha = VB->Color[i][3];
                     if (dist2>=rmin2) {
                        GLint coverage = (GLint) (256.0F-(dist2-rmin2)*cscale);
                        /* coverage is in [0,256] */
                        alpha = (alpha * coverage) >> 8;
                     }
                     PB_WRITE_RGBA_PIXEL( PB, x, y, z, red, green, blue, alpha );
                  }
               }
            }
            PB_CHECK_FLUSH(ctx,PB)
         }
      }
   }
}



/*
 * Null rasterizer for measuring transformation speed.
 */
static void null_points( GLcontext *ctx, GLuint first, GLuint last )
{
   (void) ctx;
   (void) first;
   (void) last;
}



/* Definition of the functions for GL_EXT_point_parameters */

/*
 * Calculates the distance attenuation formula of a point in eye space
 * coordinates
 */
static GLfloat dist_attenuation(GLcontext *ctx, const GLfloat p[3])
{
 GLfloat dist;
 dist=GL_SQRT(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);
 return (1/(ctx->Point.Params[0]+ ctx->Point.Params[1]*dist +
             ctx->Point.Params[2]*dist*dist));
}

/*
 * Distance Attenuated General CI points.
 */
static void dist_atten_general_ci_points( GLcontext *ctx, GLuint first, 
					GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLint isize;
   GLfloat psize,dsize;
   psize=CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE);

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;

         x = (GLint)  VB->Win[i][0];
         y = (GLint)  VB->Win[i][1];
         z = (GLint) (VB->Win[i][2] + ctx->PointZoffset);

         dsize=psize*dist_attenuation(ctx,VB->Eye[i]);
         if(dsize>=ctx->Point.Threshold) {
            isize=(GLint) (MIN2(dsize,ctx->Point.MaxSize)+0.5F);
         } else {
            isize=(GLint) (MAX2(ctx->Point.Threshold,ctx->Point.MinSize)+0.5F);
         }

         if (isize&1) {
            /* odd size */
            x0 = x - isize/2;
            x1 = x + isize/2;
            y0 = y - isize/2;
            y1 = y + isize/2;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 0.5F) - isize/2;
            x1 = x0 + isize-1;
            y0 = (GLint) (y + 0.5F) - isize/2;
            y1 = y0 + isize-1;
         }

         PB_SET_INDEX( ctx, PB, VB->Index[i] );

         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               PB_WRITE_PIXEL( PB, ix, iy, z );
            }
         }
         PB_CHECK_FLUSH(ctx,PB)
      }
   }
}

/*
 * Distance Attenuated General RGBA points.
 */
static void dist_atten_general_rgba_points( GLcontext *ctx, GLuint first, 
				GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLubyte alpha;
   GLint isize;
   GLfloat psize,dsize;
   psize=CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE);

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;

         x = (GLint)  VB->Win[i][0];
         y = (GLint)  VB->Win[i][1];
         z = (GLint) (VB->Win[i][2] + ctx->PointZoffset);
         dsize=psize*dist_attenuation(ctx,VB->Eye[i]);
         if (dsize >= ctx->Point.Threshold) {
            isize = (GLint) (MIN2(dsize,ctx->Point.MaxSize)+0.5F);
            alpha = VB->Color[i][3];
         }
         else {
            isize = (GLint) (MAX2(ctx->Point.Threshold,ctx->Point.MinSize)+0.5F);
            dsize /= ctx->Point.Threshold;
            alpha = (GLint) (VB->Color[i][3]* (dsize*dsize));
         }
         if (isize & 1) {
            /* odd size */
            x0 = x - isize/2;
            x1 = x + isize/2;
            y0 = y - isize/2;
            y1 = y + isize/2;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 0.5F) - isize/2;
            x1 = x0 + isize-1;
            y0 = (GLint) (y + 0.5F) - isize/2;
            y1 = y0 + isize-1;
         }
         PB_SET_COLOR( ctx, PB,
                       VB->Color[i][0],
                       VB->Color[i][1],
                       VB->Color[i][2],
                       alpha );

         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               PB_WRITE_PIXEL( PB, ix, iy, z );
            }
         }
         PB_CHECK_FLUSH(ctx,PB)
      }
   }
}

/*
 *  Distance Attenuated Textured RGBA points.
 */
static void dist_atten_textured_rgba_points( GLcontext *ctx, GLuint first, 
					GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLfloat psize,dsize;
   psize=CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE);

   for (i=first;i<=last;i++) {
      if (VB->ClipMask[i]==0) {
         GLint x, y, z;
         GLint x0, x1, y0, y1;
         GLint ix, iy;
         GLint isize;
         GLint red, green, blue, alpha;
         GLfloat s, t, u;
         GLfloat s1, t1, u1;

         x = (GLint)  VB->Win[i][0];
         y = (GLint)  VB->Win[i][1];
         z = (GLint) (VB->Win[i][2] + ctx->PointZoffset);

         dsize=psize*dist_attenuation(ctx,VB->Eye[i]);
         if(dsize>=ctx->Point.Threshold) {
            isize=(GLint) (MIN2(dsize,ctx->Point.MaxSize)+0.5F);
            alpha=VB->Color[i][3];
         } else {
            isize=(GLint) (MAX2(ctx->Point.Threshold,ctx->Point.MinSize)+0.5F);
            dsize/=ctx->Point.Threshold;
            alpha = (GLint) (VB->Color[i][3]* (dsize*dsize));
         }

         if (isize<1) {
            isize = 1;
         }

         if (isize&1) {
            /* odd size */
            x0 = x - isize/2;
            x1 = x + isize/2;
            y0 = y - isize/2;
            y1 = y + isize/2;
         }
         else {
            /* even size */
            x0 = (GLint) (x + 0.5F) - isize/2;
            x1 = x0 + isize-1;
            y0 = (GLint) (y + 0.5F) - isize/2;
            y1 = y0 + isize-1;
         }

         red   = VB->Color[i][0];
         green = VB->Color[i][1];
         blue  = VB->Color[i][2];
         s = VB->MultiTexCoord[0][i][0] / VB->MultiTexCoord[0][i][3];
         t = VB->MultiTexCoord[0][i][1] / VB->MultiTexCoord[0][i][3];
         u = VB->MultiTexCoord[0][i][2] / VB->MultiTexCoord[0][i][3];
         if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
            /* Multitextured!  This is probably a slow enough path that
               there's no reason to specialize the multitexture case. */
            s1 = VB->MultiTexCoord[1][i][0] / VB->MultiTexCoord[1][i][3];
            t1 = VB->MultiTexCoord[1][i][1] / VB->MultiTexCoord[1][i][3];
            u1 = VB->MultiTexCoord[1][i][2] / VB->MultiTexCoord[1][i][3];
         }

/*    don't think this is needed
         PB_SET_COLOR( red, green, blue, alpha );
*/
  
         for (iy=y0;iy<=y1;iy++) {
            for (ix=x0;ix<=x1;ix++) {
               if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
                  PB_WRITE_MULTITEX_PIXEL( PB, ix, iy, z, red, green, blue, alpha, s, t, u, s1, t1, u1 );
               } else {
                  PB_WRITE_TEX_PIXEL( PB, ix, iy, z, red, green, blue, alpha, s, t, u );
               }
            }
         }
         PB_CHECK_FLUSH(ctx,PB)
      }
   }
}

/*
 * Distance Attenuated Antialiased points with or without texture mapping.
 */
static void dist_atten_antialiased_rgba_points( GLcontext *ctx,
                                     GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLuint i;
   GLfloat radius, rmin, rmax, rmin2, rmax2, cscale;
   GLfloat psize,dsize,alphaf;
   psize=CLAMP(ctx->Point.Size,MIN_POINT_SIZE,MAX_POINT_SIZE);

   if (ctx->Texture.ReallyEnabled) {
      for (i=first;i<=last;i++) {
         if (VB->ClipMask[i]==0) {
            GLint xmin, ymin, xmax, ymax;
            GLint x, y, z;
            GLint red, green, blue, alpha;
            GLfloat s, t, u;
            GLfloat s1, t1, u1;

            dsize=psize*dist_attenuation(ctx,VB->Eye[i]);
            if(dsize>=ctx->Point.Threshold) {
               radius=(MIN2(dsize,ctx->Point.MaxSize)*0.5F);
               alphaf=1.0;
            } else {
               radius=(MAX2(ctx->Point.Threshold,ctx->Point.MinSize)*0.5F);
               dsize/=ctx->Point.Threshold;
               alphaf=(dsize*dsize);
            }
            rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
            rmax = radius + 0.7071F;
            rmin2 = rmin*rmin;
            rmax2 = rmax*rmax;
            cscale = 256.0F / (rmax2-rmin2);

            xmin = (GLint) (VB->Win[i][0] - radius);
            xmax = (GLint) (VB->Win[i][0] + radius);
            ymin = (GLint) (VB->Win[i][1] - radius);
            ymax = (GLint) (VB->Win[i][1] + radius);
            z = (GLint) (VB->Win[i][2] + ctx->PointZoffset);

            red   = VB->Color[i][0];
            green = VB->Color[i][1];
            blue  = VB->Color[i][2];
            s = VB->MultiTexCoord[0][i][0] / VB->MultiTexCoord[0][i][3];
            t = VB->MultiTexCoord[0][i][1] / VB->MultiTexCoord[0][i][3];
            u = VB->MultiTexCoord[0][i][2] / VB->MultiTexCoord[0][i][3];

            if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
               /* Multitextured!  This is probably a slow enough path that
                  there's no reason to specialize the multitexture case. */
               s1 = VB->MultiTexCoord[1][i][0] / VB->MultiTexCoord[1][i][3];
               t1 = VB->MultiTexCoord[1][i][1] / VB->MultiTexCoord[1][i][3];
               u1 = VB->MultiTexCoord[1][i][2] / VB->MultiTexCoord[1][i][3];
            }

            for (y=ymin;y<=ymax;y++) {
               for (x=xmin;x<=xmax;x++) {
                  GLfloat dx = x/*+0.5F*/ - VB->Win[i][0];
                  GLfloat dy = y/*+0.5F*/ - VB->Win[i][1];
                  GLfloat dist2 = dx*dx + dy*dy;
                  if (dist2<rmax2) {
                     alpha = VB->Color[i][3];
                     if (dist2>=rmin2) {
                        GLint coverage = (GLint) (256.0F-(dist2-rmin2)*cscale);
                        /* coverage is in [0,256] */
                        alpha = (alpha * coverage) >> 8;
                     }
                     alpha = (GLint) (alpha * alphaf);
                     if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
                        PB_WRITE_MULTITEX_PIXEL( PB, x,y,z, red, green, blue, alpha, s, t, u, s1, t1, u1 );
                     } else {
                        PB_WRITE_TEX_PIXEL( PB, x,y,z, red, green, blue, alpha, s, t, u );
                     }
                  }
               }
            }
            PB_CHECK_FLUSH(ctx,PB)
         }
      }
   }
   else {
      /* Not texture mapped */
      for (i=first;i<=last;i++) {
         if (VB->ClipMask[i]==0) {
            GLint xmin, ymin, xmax, ymax;
            GLint x, y, z;
            GLint red, green, blue, alpha;

            dsize=psize*dist_attenuation(ctx,VB->Eye[i]);
            if(dsize>=ctx->Point.Threshold) {
               radius=(MIN2(dsize,ctx->Point.MaxSize)*0.5F);
               alphaf=1.0;
            } else {
               radius=(MAX2(ctx->Point.Threshold,ctx->Point.MinSize)*0.5F);
               dsize/=ctx->Point.Threshold;
               alphaf=(dsize*dsize);
            }
            rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
            rmax = radius + 0.7071F;
            rmin2 = rmin*rmin;
            rmax2 = rmax*rmax;
            cscale = 256.0F / (rmax2-rmin2);

            xmin = (GLint) (VB->Win[i][0] - radius);
            xmax = (GLint) (VB->Win[i][0] + radius);
            ymin = (GLint) (VB->Win[i][1] - radius);
            ymax = (GLint) (VB->Win[i][1] + radius);
            z = (GLint) (VB->Win[i][2] + ctx->PointZoffset);

            red   = VB->Color[i][0];
            green = VB->Color[i][1];
            blue  = VB->Color[i][2];

            for (y=ymin;y<=ymax;y++) {
               for (x=xmin;x<=xmax;x++) {
                  GLfloat dx = x/*+0.5F*/ - VB->Win[i][0];
                  GLfloat dy = y/*+0.5F*/ - VB->Win[i][1];
                  GLfloat dist2 = dx*dx + dy*dy;
                  if (dist2<rmax2) {
                      alpha = VB->Color[i][3];
                     if (dist2>=rmin2) {
                        GLint coverage = (GLint) (256.0F-(dist2-rmin2)*cscale);
                        /* coverage is in [0,256] */
                        alpha = (alpha * coverage) >> 8;
                     }
                     alpha = (GLint) (alpha * alphaf);
                     PB_WRITE_RGBA_PIXEL( PB, x, y, z, red, green, blue, alpha )
;
                  }
               }
            }
            PB_CHECK_FLUSH(ctx,PB)
         }
      }
   }
}


/*
 * Examine the current context to determine which point drawing function
 * should be used.
 */
void gl_set_point_function( GLcontext *ctx )
{
   GLboolean rgbmode = ctx->Visual->RGBAflag;

   if (ctx->RenderMode==GL_RENDER) {
      if (ctx->NoRaster) {
         ctx->Driver.PointsFunc = null_points;
         return;
      }
      if (ctx->Driver.PointsFunc) {
         /* Device driver will draw points. */
         ctx->Driver.PointsFunc = ctx->Driver.PointsFunc;
      }
      else if (ctx->Point.Params[0]==1.0 && ctx->Point.Params[1]==0.0 && 
               ctx->Point.Params[2]==0.0) { 
         if (ctx->Point.SmoothFlag && rgbmode) {
            ctx->Driver.PointsFunc = antialiased_rgba_points;
         }
         else if (ctx->Texture.ReallyEnabled) {
            if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D) {
	       ctx->Driver.PointsFunc = multitextured_rgba_points;
            }
            else {
               ctx->Driver.PointsFunc = textured_rgba_points;
            }
         }
         else if (ctx->Point.Size==1.0) {
            /* size=1, any raster ops */
            if (rgbmode)
               ctx->Driver.PointsFunc = size1_rgba_points;
            else
               ctx->Driver.PointsFunc = size1_ci_points;
         }
         else {
	    /* every other kind of point rendering */
            if (rgbmode)
               ctx->Driver.PointsFunc = general_rgba_points;
            else
               ctx->Driver.PointsFunc = general_ci_points;
         }
      } 
      else if(ctx->Point.SmoothFlag && rgbmode) {
         ctx->Driver.PointsFunc = dist_atten_antialiased_rgba_points;
      }
      else if (ctx->Texture.ReallyEnabled) {
         ctx->Driver.PointsFunc = dist_atten_textured_rgba_points;
      } 
      else {
         /* every other kind of point rendering */
         if (rgbmode)
            ctx->Driver.PointsFunc = dist_atten_general_rgba_points;
         else
            ctx->Driver.PointsFunc = dist_atten_general_ci_points;
     }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      ctx->Driver.PointsFunc = feedback_points;
   }
   else {
      /* GL_SELECT mode */
      ctx->Driver.PointsFunc = select_points;
   }

}

