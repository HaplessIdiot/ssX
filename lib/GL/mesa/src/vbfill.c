/* Id: vbfill.c,v 1.3 1999/02/26 08:52:39 martin Exp $ */

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
 * Log: vbfill.c,v $
 * Revision 1.3  1999/02/26 08:52:39  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.15  1999/02/24 22:48:08  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.14  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.13  1998/11/25 04:18:19  brianp
 * fixed a few IRIX compiler warnings
 *
 * Revision 3.12  1998/11/17 01:52:47  brianp
 * implemented GL_NV_texgen_reflection extension (MJK)
 *
 * Revision 3.11  1998/11/08 22:36:34  brianp
 * renamed texture sets to texture units
 *
 * Revision 3.10  1998/11/03 02:40:40  brianp
 * new ctx->RenderVB function pointer
 *
 * Revision 3.9  1998/10/31 17:06:15  brianp
 * variety of multi-texture changes
 *
 * Revision 3.8  1998/10/29 03:57:11  brianp
 * misc clean-up of new vertex transformation code
 *
 * Revision 3.7  1998/10/29 02:28:13  brianp
 * incorporated Keith Whitwell's transformation optimizations
 *
 * Revision 3.6  1998/06/07 22:18:52  brianp
 * implemented GL_EXT_multitexture extension
 *
 * Revision 3.5  1998/04/18 05:00:42  brianp
 * moved FLOAT_COLOR_TO_UBYTE_COLOR macro to mmath.h
 *
 * Revision 3.4  1998/03/27 04:26:44  brianp
 * fixed G++ warnings
 *
 * Revision 3.3  1998/02/20 04:53:07  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.2  1998/02/08 20:17:42  brianp
 * removed unneeded headers
 *
 * Revision 3.1  1998/02/02 03:09:34  brianp
 * added GL_LIGHT_MODEL_COLOR_CONTROL (separate specular color interpolation)
 *
 * Revision 3.0  1998/01/31 21:06:45  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/vbfill.c,v 1.0tsi Exp $ */

/*
 * This file implements the functions for filling the vertex buffer:
 *   glVertex, glNormal, glColor, glIndex, glEdgeFlag, glTexCoord,
 */


#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <assert.h>
#endif
#include "context.h"
#include "light.h"
#include "macros.h"
#include "matrix.h"
#include "mmath.h"
#include "pb.h"
#include "types.h"
#include "vb.h"
#include "vbfill.h"
#include "vbrender.h"
#include "vbxform.h"
#include "xform.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



/**********************************************************************/
/******                    glNormal functions                     *****/
/**********************************************************************/

/*
 * Caller:  context->API.Normal3f pointer.
 */
void gl_Normal3f( GLcontext *ctx, GLfloat nx, GLfloat ny, GLfloat nz )
{
   ctx->Current.Normal[0] = nx;
   ctx->Current.Normal[1] = ny;
   ctx->Current.Normal[2] = nz;
   ctx->VB->MonoNormal = GL_FALSE;
}


/*
 * Caller:  context->API.Normal3fv pointer.
 */
void gl_Normal3fv( GLcontext *ctx, const GLfloat *n )
{
   ctx->Current.Normal[0] = n[0];
   ctx->Current.Normal[1] = n[1];
   ctx->Current.Normal[2] = n[2];
   ctx->VB->MonoNormal = GL_FALSE;
}



/**********************************************************************/
/******                    glIndex functions                      *****/
/**********************************************************************/

/*
 * Caller:  context->API.Indexf pointer.
 */
void gl_Indexf( GLcontext *ctx, GLfloat c )
{
   ctx->Current.Index = (GLuint) (GLint) c;
   ctx->VB->MonoColor = GL_FALSE;
}


/*
 * Caller:  context->API.Indexi pointer.
 */
void gl_Indexi( GLcontext *ctx, GLint c )
{
   ctx->Current.Index = (GLuint) c;
   ctx->VB->MonoColor = GL_FALSE;
}



/**********************************************************************/
/******                     glColor functions                     *****/
/**********************************************************************/


/*
 * Caller:  context->API.Color3f pointer.
 */
void gl_Color3f( GLcontext *ctx, GLfloat red, GLfloat green, GLfloat blue )
{
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[0], red);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[1], green);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[2], blue);
   ctx->Current.ByteColor[3] = 255;
   ASSERT( !ctx->Light.ColorMaterialEnabled );
   ctx->VB->MonoColor = GL_FALSE;
}


/*
 * Caller:  context->API.Color3fv pointer.
 */
void gl_Color3fv( GLcontext *ctx, const GLfloat *c )
{
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[0], c[0]);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[1], c[1]);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[2], c[2]);
   ctx->Current.ByteColor[3] = 255;
   ASSERT( !ctx->Light.ColorMaterialEnabled );
   ctx->VB->MonoColor = GL_FALSE;
}



/*
 * Caller:  context->API.Color4f pointer.
 */
void gl_Color4f( GLcontext *ctx,
                 GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[0], red);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[1], green);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[2], blue);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[3], alpha);
   ASSERT( !ctx->Light.ColorMaterialEnabled );
   ctx->VB->MonoColor = GL_FALSE;
}


/*
 * Caller:  context->API.Color4fv pointer.
 */
void gl_Color4fv( GLcontext *ctx, const GLfloat *c )
{
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[0], c[0]);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[1], c[1]);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[2], c[2]);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[3], c[3]);
   ASSERT( !ctx->Light.ColorMaterialEnabled );
   ctx->VB->MonoColor = GL_FALSE;
}


/*
 * Caller:  context->API.Color4ub pointer.
 */
void gl_Color4ub( GLcontext *ctx,
                  GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
   ASSIGN_4V( ctx->Current.ByteColor, red, green, blue, alpha );
   ASSERT( !ctx->Light.ColorMaterialEnabled );
   ctx->VB->MonoColor = GL_FALSE;
}


/*
 * Caller:  context->API.Color4ub pointer.
 */
void gl_Color4ubv( GLcontext *ctx, const GLubyte *c )
{
   COPY_4UBV( ctx->Current.ByteColor, c );
   ASSERT( !ctx->Light.ColorMaterialEnabled );
   ctx->VB->MonoColor = GL_FALSE;
}


/*
 * glColor() which modifies material(s).
 * Caller:  context->API.Color3f pointer.
 */
void gl_ColorMat3f( GLcontext *ctx, GLfloat red, GLfloat green, GLfloat blue )
{
   GLfloat color[4];
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[0], red);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[1], green);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[2], blue);
   ctx->Current.ByteColor[3] = 255;
   /* update material */
   ASSERT( ctx->Light.ColorMaterialEnabled );
   ASSIGN_4V( color, red, green, blue, 1.0F );
   gl_set_material( ctx, ctx->Light.ColorMaterialBitmask, color );
   ctx->VB->MonoColor = GL_FALSE;
}


/*
 * glColor() which modifies material(s).
 * Caller:  context->API.Color3fv pointer.
 */
void gl_ColorMat3fv( GLcontext *ctx, const GLfloat *c )
{
   GLfloat color[4];
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[0], c[0]);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[1], c[1]);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[2], c[2]);
   ctx->Current.ByteColor[3] = 255;
   /* update material */
   ASSERT( ctx->Light.ColorMaterialEnabled );
   ASSIGN_4V( color, c[0], c[1], c[2], 1.0F );
   gl_set_material( ctx, ctx->Light.ColorMaterialBitmask, color );
   ctx->VB->MonoColor = GL_FALSE;
}


/*
 * glColor() which modifies material(s).
 * Caller:  context->API.Color4f pointer.
 */
void gl_ColorMat4f( GLcontext *ctx,
                    GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
   GLfloat color[4];
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[0], red);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[1], green);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[2], blue);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[3], alpha);
   /* update material */
   ASSERT( ctx->Light.ColorMaterialEnabled );
   ASSIGN_4V( color, red, green, blue, alpha );
   gl_set_material( ctx, ctx->Light.ColorMaterialBitmask, color );
   ctx->VB->MonoColor = GL_FALSE;
}


/*
 * glColor() which modifies material(s).
 * Caller:  context->API.Color4fv pointer.
 */
void gl_ColorMat4fv( GLcontext *ctx, const GLfloat *c )
{
   GLfloat color[4];
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[0], c[0]);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[1], c[1]);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[2], c[2]);
   FLOAT_COLOR_TO_UBYTE_COLOR(ctx->Current.ByteColor[3], c[3]);
   /* update material */
   ASSERT( ctx->Light.ColorMaterialEnabled );
   ASSIGN_4V( color, c[0], c[1], c[2], c[3] );
   gl_set_material( ctx, ctx->Light.ColorMaterialBitmask, color );
   ctx->VB->MonoColor = GL_FALSE;
}


/*
 * glColor which modifies material(s).
 * Caller:  context->API.Color4ub pointer.
 */
void gl_ColorMat4ub( GLcontext *ctx,
                     GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
   GLfloat color[4];
   ASSIGN_4V( ctx->Current.ByteColor, red, green, blue, alpha );
   /* update material */
   ASSERT( ctx->Light.ColorMaterialEnabled );
   color[0] = red   * (1.0F/255.0F);
   color[1] = green * (1.0F/255.0F);
   color[2] = blue  * (1.0F/255.0F);
   color[3] = alpha * (1.0F/255.0F);
   gl_set_material( ctx, ctx->Light.ColorMaterialBitmask, color );
   ctx->VB->MonoColor = GL_FALSE;
}


/*
 * glColor which modifies material(s).
 * Caller:  context->API.Color4ub pointer.
 */
void gl_ColorMat4ubv( GLcontext *ctx, const GLubyte *c )
{
   gl_ColorMat4ub( ctx, c[0], c[1], c[2], c[3] );
}



/**********************************************************************/
/******                  glEdgeFlag functions                     *****/
/**********************************************************************/

/*
 * Caller:  context->API.EdgeFlag pointer.
 */
void gl_EdgeFlag( GLcontext *ctx, GLboolean flag )
{
   ctx->Current.EdgeFlag = flag;
}



/**********************************************************************/
/*****                    glVertex functions                      *****/
/**********************************************************************/

/*
 * Used when in feedback mode.
 * Caller:  context->API.Vertex4f pointer.
 */
static void vertex4f_feedback( GLcontext *ctx,
                               GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   /* vertex */
   ASSIGN_4V( VB->Obj[count], x, y, z, w );

   /* color */
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );

   /* index */
   VB->Findex[count] = ctx->Current.Index;

   /* normal */
   COPY_3V( VB->Normal[count], ctx->Current.Normal );

   /* texcoord */
   COPY_4V( VB->TexCoord[count], ctx->Current.TexCoord );

   /* edgeflag */
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


static void vertex3f_feedback( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z )
{
   vertex4f_feedback(ctx, x, y, z, 1.0F);
}


static void vertex2f_feedback( GLcontext *ctx, GLfloat x, GLfloat y )
{
   vertex4f_feedback(ctx, x, y, 0.0F, 1.0F);
}


static void vertex3fv_feedback( GLcontext *ctx, const GLfloat v[3] )
{
   vertex4f_feedback(ctx, v[0], v[1], v[2], 1.0F);
}



/*
 * Only one glVertex4 function since it's not too popular.
 * Caller:  context->API.Vertex4f pointer.
 */
static void vertex4( GLcontext *ctx,
                     GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_4V( VB->Obj[count], x, y, z, w );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_3V( VB->Normal[count], ctx->Current.Normal );
   COPY_4V( VB->TexCoord[count], ctx->Current.TexCoord );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;
   VB->VertexSizeMask = VERTEX4_BIT;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}



/*
 * XYZ vertex, RGB color, normal, ST texture coords.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3f_normal_color_tex2( GLcontext *ctx,
                                        GLfloat x, GLfloat y, GLfloat z )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, z );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_3V( VB->Normal[count], ctx->Current.Normal );
   COPY_2V( VB->TexCoord[count], ctx->Current.TexCoord );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XYZ vertex, RGB color, normal, STRQ texture coords.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3f_normal_color_tex4( GLcontext *ctx,
                                        GLfloat x, GLfloat y, GLfloat z )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, z );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_3V( VB->Normal[count], ctx->Current.Normal );
   /* GL_ARB_multitexture */
   COPY_4V( VB->MultiTexCoord[0][count], ctx->Current.MultiTexCoord[0] );
   COPY_4V( VB->MultiTexCoord[1][count], ctx->Current.MultiTexCoord[1] );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XYZ vertex, normal.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3f_normal( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, z );
   COPY_3V( VB->Normal[count], ctx->Current.Normal );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XYZ vertex, ST texture coords.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3f_color_tex2( GLcontext *ctx,
                                 GLfloat x, GLfloat y, GLfloat z )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, z );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_2V( VB->TexCoord[count], ctx->Current.TexCoord );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XYZ vertex, STRQ texture coords.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3f_color_tex4( GLcontext *ctx,
                                 GLfloat x, GLfloat y, GLfloat z )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, z );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_4V( VB->TexCoord[count], ctx->Current.TexCoord );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XYZ vertex, RGB color.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3f_color( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, z );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XYZ vertex, color index.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3f_index( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, z );
   VB->Findex[count] = ctx->Current.Index;
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}



/*
 * XY vertex, RGB color, normal, ST texture coords.
 * Caller:  context->API.Vertex2f pointer.
 */
static void vertex2f_normal_color_tex2( GLcontext *ctx, GLfloat x, GLfloat y )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, 0.0F );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_3V( VB->Normal[count], ctx->Current.Normal );
   COPY_2V( VB->TexCoord[count], ctx->Current.TexCoord );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XY vertex, RGB color, normal, STRQ texture coords.
 * Caller:  context->API.Vertex2f pointer.
 */
static void vertex2f_normal_color_tex4( GLcontext *ctx, GLfloat x, GLfloat y )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, 0.0F );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_3V( VB->Normal[count], ctx->Current.Normal );
   /* GL_ARB_multitexture */
   COPY_4V( VB->MultiTexCoord[0][count], ctx->Current.MultiTexCoord[0] );
   COPY_4V( VB->MultiTexCoord[1][count], ctx->Current.MultiTexCoord[1] );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XY vertex, normal.
 * Caller:  context->API.Vertex2f pointer.
 */
static void vertex2f_normal( GLcontext *ctx, GLfloat x, GLfloat y )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, 0.0F );
   COPY_3V( VB->Normal[count], ctx->Current.Normal );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XY vertex, ST texture coords.
 * Caller:  context->API.Vertex2f pointer.
 */
static void vertex2f_color_tex2( GLcontext *ctx, GLfloat x, GLfloat y )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, 0.0F );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_2V( VB->TexCoord[count], ctx->Current.TexCoord );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XY vertex, STRQ texture coords.
 * Caller:  context->API.Vertex2f pointer.
 */
static void vertex2f_color_tex4( GLcontext *ctx, GLfloat x, GLfloat y )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, 0.0F );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_4V( VB->TexCoord[count], ctx->Current.TexCoord );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XY vertex, RGB color.
 * Caller:  context->API.Vertex2f pointer.
 */
static void vertex2f_color( GLcontext *ctx, GLfloat x, GLfloat y )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, 0.0F );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XY vertex, color index.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex2f_index( GLcontext *ctx, GLfloat x, GLfloat y )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   ASSIGN_3V( VB->Obj[count], x, y, 0.0F );
   VB->Findex[count] = ctx->Current.Index;
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}




/*
 * XYZ vertex, RGB color, normal, ST texture coords.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3fv_normal_color_tex2( GLcontext *ctx, const GLfloat v[3] )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   COPY_3V( VB->Obj[count], v );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_3V( VB->Normal[count], ctx->Current.Normal );
   COPY_2V( VB->TexCoord[count], ctx->Current.TexCoord );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XYZ vertex, RGB color, normal, STRQ texture coords.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3fv_normal_color_tex4( GLcontext *ctx, const GLfloat v[3] )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   COPY_3V( VB->Obj[count], v );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_3V( VB->Normal[count], ctx->Current.Normal );
   /* GL_ARB_multitexture */
   COPY_4V( VB->MultiTexCoord[0][count], ctx->Current.MultiTexCoord[0] );
   COPY_4V( VB->MultiTexCoord[1][count], ctx->Current.MultiTexCoord[1] );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XYZ vertex, normal.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3fv_normal( GLcontext *ctx, const GLfloat v[3] )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   COPY_3V( VB->Obj[count], v );
   COPY_3V( VB->Normal[count], ctx->Current.Normal );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XYZ vertex, ST texture coords.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3fv_color_tex2( GLcontext *ctx, const GLfloat v[3] )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   COPY_3V( VB->Obj[count], v );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_2V( VB->TexCoord[count], ctx->Current.TexCoord );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XYZ vertex, STRQ texture coords.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3fv_color_tex4( GLcontext *ctx, const GLfloat v[3] )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   COPY_3V( VB->Obj[count], v );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   COPY_4V( VB->TexCoord[count], ctx->Current.TexCoord );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XYZ vertex, RGB color.
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3fv_color( GLcontext *ctx, const GLfloat v[3] )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   COPY_3V( VB->Obj[count], v );
   COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}


/*
 * XYZ vertex, Color index
 * Caller:  context->API.Vertex3f pointer.
 */
static void vertex3fv_index( GLcontext *ctx, const GLfloat v[3] )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;

   COPY_3V( VB->Obj[count], v );
   VB->Findex[count] = ctx->Current.Index;
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}



/*
 * Called when outside glBegin/glEnd, raises an error.
 * Caller:  context->API.Vertex4f pointer.
 */
void gl_vertex4f_nop( GLcontext *ctx,
                      GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   (void) x;
   (void) y;
   (void) z;
   (void) w;
   gl_error( ctx, GL_INVALID_OPERATION, "glVertex4" );
}

void gl_vertex3f_nop( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z )
{
   (void) x;
   (void) y;
   (void) z;
   gl_error( ctx, GL_INVALID_OPERATION, "glVertex3" );
}

void gl_vertex2f_nop( GLcontext *ctx, GLfloat x, GLfloat y )
{
   (void) x;
   (void) y;
   gl_error( ctx, GL_INVALID_OPERATION, "glVertex2" );
}

void gl_vertex3fv_nop( GLcontext *ctx, const GLfloat v[3] )
{
   (void) v;
   gl_error( ctx, GL_INVALID_OPERATION, "glVertex3v" );
}




/**********************************************************************/
/******                   glTexCoord functions                    *****/
/**********************************************************************/

/*
 * Caller:  context->API.TexCoord2f pointer.
 */
void gl_TexCoord2f( GLcontext *ctx, GLfloat s, GLfloat t )
{
   ctx->Current.TexCoord[0] = s;
   ctx->Current.TexCoord[1] = t;
}


/*
 * Caller:  context->API.TexCoord2f pointer.
 * This version of glTexCoord2 is called if glTexCoord[34] was a predecessor.
 */
void gl_TexCoord2f4( GLcontext *ctx, GLfloat s, GLfloat t )
{
   ctx->Current.TexCoord[0] = s;
   ctx->Current.TexCoord[1] = t;
   ctx->Current.TexCoord[2] = 0.0F;
   ctx->Current.TexCoord[3] = 1.0F;
}


/*
 * Caller:  context->API.TexCoord4f pointer.
 */
void gl_TexCoord4f( GLcontext *ctx, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   ctx->Current.TexCoord[0] = s;
   ctx->Current.TexCoord[1] = t;
   ctx->Current.TexCoord[2] = r;
   ctx->Current.TexCoord[3] = q;
   if (ctx->VB->TexCoordSize==2) {
      /* Have to switch to 4-component texture mode now */
      ctx->VB->TexCoordSize = 4;
      gl_set_vertex_function( ctx );
      ctx->Exec.TexCoord2f = ctx->API.TexCoord2f = gl_TexCoord2f4;
   }
}


/* GL_ARB_multitexture */
void gl_MultiTexCoord4f( GLcontext *ctx, GLenum target,
                         GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
   GLint texSet;
   if (target >= GL_TEXTURE0_SGIS && target <= GL_TEXTURE1_SGIS) {
      texSet = target - GL_TEXTURE0_SGIS;
   }
   else if (target >= GL_TEXTURE0_ARB && target <= GL_TEXTURE1_ARB) {
      texSet = target - GL_TEXTURE0_ARB;
   }
   else {
      gl_error(ctx, GL_INVALID_ENUM, "glMultiTexCoord(target)");
      return;
   }

   ctx->Current.MultiTexCoord[texSet][0] = s;
   ctx->Current.MultiTexCoord[texSet][1] = t;
   ctx->Current.MultiTexCoord[texSet][2] = r;
   ctx->Current.MultiTexCoord[texSet][3] = q;
}



/*
 * This function examines the current GL state and sets the
 * ctx->Exec.Vertex[34]f pointers to point at the appropriate vertex
 * processing functions.
 */
void gl_set_vertex_function( GLcontext *ctx )
{
   if (ctx->RenderMode==GL_FEEDBACK) {
      ctx->Exec.Vertex4f = vertex4f_feedback;
      ctx->Exec.Vertex3f = vertex3f_feedback;
      ctx->Exec.Vertex2f = vertex2f_feedback;
      ctx->Exec.Vertex3fv = vertex3fv_feedback;
   }
   else {
      ctx->Exec.Vertex4f = vertex4;
      if (ctx->Visual->RGBAflag) {
         if (ctx->Texture.Enabled >= TEXTURE1_1D) {
            /* Multi texture coords */
            ctx->Exec.Vertex2f = vertex2f_normal_color_tex4;
            ctx->Exec.Vertex3f = vertex3f_normal_color_tex4;
            ctx->Exec.Vertex3fv = vertex3fv_normal_color_tex4;
         }
         else if (ctx->NeedNormals) {
            /* lighting enabled, need normal vectors */
            if (ctx->Texture.Enabled) {
               if (ctx->VB->TexCoordSize==2) {
                  ctx->Exec.Vertex2f = vertex2f_normal_color_tex2;
                  ctx->Exec.Vertex3f = vertex3f_normal_color_tex2;
                  ctx->Exec.Vertex3fv = vertex3fv_normal_color_tex2;
               }
               else {
                  ctx->Exec.Vertex2f = vertex2f_normal_color_tex4;
                  ctx->Exec.Vertex3f = vertex3f_normal_color_tex4;
                  ctx->Exec.Vertex3fv = vertex3fv_normal_color_tex4;
               }
            }
            else {
               ctx->Exec.Vertex2f = vertex2f_normal;
               ctx->Exec.Vertex3f = vertex3f_normal;
               ctx->Exec.Vertex3fv = vertex3fv_normal;
            }
         }
         else {
            /* not lighting, need vertex color */
            if (ctx->Texture.Enabled) {
               if (ctx->VB->TexCoordSize==2) {
                  ctx->Exec.Vertex2f = vertex2f_color_tex2;
                  ctx->Exec.Vertex3f = vertex3f_color_tex2;
                  ctx->Exec.Vertex3fv = vertex3fv_color_tex2;
               }
               else {
                  ctx->Exec.Vertex2f = vertex2f_color_tex4;
                  ctx->Exec.Vertex3f = vertex3f_color_tex4;
                  ctx->Exec.Vertex3fv = vertex3fv_color_tex4;
               }
            }
            else {
               ctx->Exec.Vertex2f = vertex2f_color;
               ctx->Exec.Vertex3f = vertex3f_color;
               ctx->Exec.Vertex3fv = vertex3fv_color;
            }
         }
      }
      else {
         /* color index mode */
         if (ctx->Light.Enabled) {
            ctx->Exec.Vertex2f = vertex2f_normal;
            ctx->Exec.Vertex3f = vertex3f_normal;
            ctx->Exec.Vertex3fv = vertex3fv_normal;
         }
         else {
            ctx->Exec.Vertex2f = vertex2f_index;
            ctx->Exec.Vertex3f = vertex3f_index;
            ctx->Exec.Vertex3fv = vertex3fv_index;
         }
      }
   }

   if (!ctx->CompileFlag) {
      ctx->API.Vertex2f = ctx->Exec.Vertex2f;
      ctx->API.Vertex3f = ctx->Exec.Vertex3f;
      ctx->API.Vertex4f = ctx->Exec.Vertex4f;
      ctx->API.Vertex3fv = ctx->Exec.Vertex3fv;
   }
}



/*
 * This function examines the current GL state and sets the
 * ctx->Exec.Color[34]* pointers to point at the appropriate vertex
 * processing functions.
 */
void gl_set_color_function( GLcontext *ctx )
{
   if (ctx->Light.ColorMaterialEnabled) {
      ctx->Exec.Color3f = gl_ColorMat3f;
      ctx->Exec.Color3fv = gl_ColorMat3fv;
      ctx->Exec.Color4f = gl_ColorMat4f;
      ctx->Exec.Color4fv = gl_ColorMat4fv;
      ctx->Exec.Color4ub = gl_ColorMat4ub;
      ctx->Exec.Color4ubv = gl_ColorMat4ubv;
   }
   else {
      ctx->Exec.Color3f = gl_Color3f;
      ctx->Exec.Color3fv = gl_Color3fv;
      ctx->Exec.Color4f = gl_Color4f;
      ctx->Exec.Color4fv = gl_Color4fv;
      ctx->Exec.Color4ub = gl_Color4ub;
      ctx->Exec.Color4ubv = gl_Color4ubv;
   }
   if (!ctx->CompileFlag) {
      ctx->API.Color3f = ctx->Exec.Color3f;
      ctx->API.Color3fv = ctx->Exec.Color3fv;
      ctx->API.Color4f = ctx->Exec.Color4f;
      ctx->API.Color4fv = ctx->Exec.Color4fv;
      ctx->API.Color4ub = ctx->Exec.Color4ub;
      ctx->API.Color4ubv = ctx->Exec.Color4ubv;
   }
}



/**********************************************************************/
/*****                    Evaluator vertices                      *****/
/**********************************************************************/


/*
 * Process a vertex produced by an evaluator.
 * Caller:  eval.c
 * Input:  vertex - the X,Y,Z,W vertex
 *         normal - normal vector
 *         color - 4 integer color components
 *         index - color index
 *         texcoord - texture coordinate
 */
void gl_eval_vertex( GLcontext *ctx,
                     const GLfloat vertex[4], const GLfloat normal[3],
		     const GLubyte color[4],
                     GLuint index,
                     const GLfloat texcoord[4] )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint count = VB->Count;  /* copy to local var to encourage optimization */
   GLuint texUnit = ctx->Texture.CurrentTransformUnit;
   GLuint j;

   VB->VertexSizeMask = VERTEX4_BIT;
   VB->MonoNormal = GL_FALSE;
   COPY_4V( VB->Obj[count], vertex );
   COPY_3V( VB->Normal[count], normal );
   COPY_4UBV( VB->Fcolor[count], color );
   
#ifdef GL_VERSION_1_1
   if (ctx->Light.ColorMaterialEnabled
       && (ctx->Eval.Map1Color4 || ctx->Eval.Map2Color4)) {
      GLfloat fcolor[4];
      fcolor[0] = color[0] * (1.0F / 255.0F);
      fcolor[1] = color[1] * (1.0F / 255.0F);
      fcolor[2] = color[2] * (1.0F / 255.0F);
      fcolor[3] = color[3] * (1.0F / 255.0F);
      gl_set_material( ctx, ctx->Light.ColorMaterialBitmask, fcolor );
   }
#endif
   VB->Findex[count] = index;
   /* copy the current texture coordinates */
   for (j=0;j<MAX_TEXTURE_UNITS;j++) {
      COPY_4V( VB->MultiTexCoord[j][count], ctx->Current.MultiTexCoord[j] );
   }
   /* overwrite one of the texture coordinates with the evaluated
    * texture coordinate
    * NOTE: ARB_multitexture does not allow texSet to be anything but zero
    */
   COPY_4V( VB->MultiTexCoord[texUnit][count], texcoord );
   VB->Edgeflag[count] = ctx->Current.EdgeFlag;

   count++;
   VB->Count = count;
   if (count==VB_MAX) {
      gl_transform_vb( ctx, GL_FALSE );
   }
}



/**********************************************************************/
/*****                    glBegin / glEnd                         *****/
/**********************************************************************/


#ifdef PROFILE
static GLdouble begin_time;
#endif


void gl_Begin( GLcontext *ctx, GLenum p )
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *PB = ctx->PB;
   GLenum reduced_primitive = 0;

#ifdef PROFILE
   begin_time = gl_time();
#endif

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glBegin" );
      return;
   }

   ctx->Primitive = p;

   switch (ctx->Primitive) 
   {
       case GL_POINTS:
          ctx->LightTwoSide = GL_FALSE;
          reduced_primitive = GL_POINT;
          ctx->RenderVB = gl_render_vb_points;
          break;
       case GL_LINES:
          ctx->LightTwoSide = GL_FALSE;
          ctx->StippleCounter = 0;
          reduced_primitive = GL_LINE;
          ctx->RenderVB = gl_render_vb_lines;
          break;
       case GL_LINE_STRIP:
          ctx->LightTwoSide = GL_FALSE;
          ctx->StippleCounter = 0;
          reduced_primitive = GL_LINE;
          ctx->RenderVB = gl_render_vb_line_strip;
          break;
       case GL_LINE_LOOP:
          ctx->LightTwoSide = GL_FALSE;
          ctx->StippleCounter = 0;
          reduced_primitive = GL_LINE;
          ctx->RenderVB = gl_render_vb_line_loop;
          break;
       case GL_TRIANGLES:
          reduced_primitive = GL_POLYGON;
          ctx->RenderVB = gl_render_vb_triangles;
          break;
       case GL_TRIANGLE_STRIP:
          reduced_primitive = GL_POLYGON;
          ctx->RenderVB = gl_render_vb_triangle_strip;
          break;
       case GL_TRIANGLE_FAN:
          reduced_primitive = GL_POLYGON;
          ctx->RenderVB = gl_render_vb_triangle_fan;
          break;
       case GL_QUADS:
          reduced_primitive = GL_POLYGON;
          ctx->RenderVB = gl_render_vb_quads;
          break;
       case GL_QUAD_STRIP:
          reduced_primitive = GL_POLYGON;
          ctx->RenderVB = gl_render_vb_quad_strip;
          break;
       case GL_POLYGON:
          reduced_primitive = GL_POLYGON;
          ctx->RenderVB = gl_render_vb_polygon;
          break;
       default:
          gl_error( ctx, GL_INVALID_ENUM, "glBegin" );
          ctx->RenderVB = gl_render_vb_no_op;
          ctx->Primitive = GL_BITMAP;
          break;
   }

   if (reduced_primitive == GL_POLYGON) {
      GLboolean tmp = (ctx->Light.Enabled && ctx->Light.Model.TwoSide);
      if (tmp && !ctx->LightTwoSide) 
         ctx->NewState |= NEW_LIGHTING; /* need backface precomputes */
      ctx->LightTwoSide = tmp;
   }

   VB->Start = VB->Count = 0;
   VB->LastMaterial = 0;

   if (ctx->NewState) {
      gl_update_state( ctx );
   }
   else if (ctx->Exec.Vertex3f==gl_vertex3f_nop) {
      gl_set_vertex_function(ctx);
   }

   if (ctx->Driver.Begin) {
      (*ctx->Driver.Begin)( ctx, p );
   }

   VB->MonoColor = ctx->MonoPixels;
   VB->MonoNormal = GL_TRUE;
   if (VB->MonoColor) {
      /* All pixels generated are likely to be the same color so have
       * the device driver set the "monocolor" now.
       */
      if (ctx->Visual->RGBAflag) {
         GLubyte r = ctx->Current.ByteColor[0];
         GLubyte g = ctx->Current.ByteColor[1];
         GLubyte b = ctx->Current.ByteColor[2];
         GLubyte a = ctx->Current.ByteColor[3];
         (*ctx->Driver.Color)( ctx, r, g, b, a );
      }
      else {
         (*ctx->Driver.Index)( ctx, ctx->Current.Index );
      }
   }

   /* By default use front color/index.  Two-sided lighting may override. */
   VB->Color = VB->Fcolor;
   VB->Index = VB->Findex;
   VB->Specular = VB->Fspec;

   PB_INIT( PB, reduced_primitive );
}



void gl_End( GLcontext *ctx )
{
   struct pixel_buffer *PB = ctx->PB;
   struct vertex_buffer *VB = ctx->VB;

   if (ctx->Primitive==GL_BITMAP) {
      /* glEnd without glBegin */
      gl_error( ctx, GL_INVALID_OPERATION, "glEnd" );
      return;
   }

   if (VB->Count > VB->Start) {
      gl_transform_vb( ctx, GL_TRUE );
   }

   if (PB->count>0) {
      gl_flush_pb(ctx);
   }

   if (ctx->Driver.End) {
      (*ctx->Driver.End)(ctx);
   }

   PB->primitive = ctx->Primitive = GL_BITMAP;  /* Default mode */

#ifdef PROFILE
   ctx->BeginEndTime += gl_time() - begin_time;
   ctx->BeginEndCount++;
#endif
}
