/* Id: varray.c,v 1.3 1999/02/26 08:52:39 martin Exp $ */

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
 * Log: varray.c,v $
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
 * Revision 3.12  1998/11/08 22:34:07  brianp
 * renamed texture sets to texture units
 *
 * Revision 3.11  1998/11/07 02:14:17  brianp
 * bug fixes and optimizations to gl_DrawArrays()
 *
 * Revision 3.10  1998/10/31 17:06:15  brianp
 * variety of multi-texture changes
 *
 * Revision 3.9  1998/10/29 03:57:11  brianp
 * misc clean-up of new vertex transformation code
 *
 * Revision 3.8  1998/10/29 02:28:13  brianp
 * incorporated Keith Whitwell's transformation optimizations
 *
 * Revision 3.7  1998/09/01 03:05:22  brianp
 * fixed interleaved texture coordinate problem
 *
 * Revision 3.6  1998/08/21 01:50:53  brianp
 * added support for texture coordinate set interleave factor
 *
 * Revision 3.5  1998/06/07 22:18:52  brianp
 * implemented GL_EXT_multitexture extension
 *
 * Revision 3.4  1998/03/27 04:26:44  brianp
 * fixed G++ warnings
 *
 * Revision 3.3  1998/02/20 04:53:07  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.2  1998/02/01 20:05:10  brianp
 * added glDrawRangeElements()
 *
 * Revision 3.1  1998/02/01 16:37:19  brianp
 * added GL_EXT_rescale_normal extension
 *
 * Revision 3.0  1998/01/31 21:06:45  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/varray.c,v 1.0tsi Exp $ */


/*
 * NOTE:  At this time, only three vertex array configurations are optimized:
 *  1.  glVertex3fv(), zero stride
 *  2.  glNormal3fv() with glVertex3fv(), zero stride
 *  3.  glNormal3fv() with glVertex4fv(), zero stride
 *
 * More optimized array configurations can be added.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <stdlib.h>
#include <string.h>
#endif
#include "context.h"
#include "enable.h"
#include "dlist.h"
#include "light.h"
#include "macros.h"
#include "texstate.h"
#include "types.h"
#include "varray.h"
#include "vb.h"
#include "vbfill.h"
#include "vbrender.h"
#include "vbxform.h"
#include "xform.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif


void gl_VertexPointer( GLcontext *ctx,
                       GLint size, GLenum type, GLsizei stride,
                       const GLvoid *ptr )
{
   if (size<2 || size>4) {
      gl_error( ctx, GL_INVALID_VALUE, "glVertexPointer(size)" );
      return;
   }
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glVertexPointer(stride)" );
      return;
   }
   switch (type) {
      case GL_SHORT:
         ctx->Array.VertexStrideB = stride ? stride : size*sizeof(GLshort);
         break;
      case GL_INT:
         ctx->Array.VertexStrideB = stride ? stride : size*sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.VertexStrideB = stride ? stride : size*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.VertexStrideB = stride ? stride : size*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glVertexPointer(type)" );
         return;
   }
   ctx->Array.VertexSize = size;
   ctx->Array.VertexType = type;
   ctx->Array.VertexStride = stride;
   ctx->Array.VertexPtr = (void *) ptr;
}




void gl_NormalPointer( GLcontext *ctx,
                       GLenum type, GLsizei stride, const GLvoid *ptr )
{
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glNormalPointer(stride)" );
      return;
   }
   switch (type) {
      case GL_BYTE:
         ctx->Array.NormalStrideB = stride ? stride : 3*sizeof(GLbyte);
         break;
      case GL_SHORT:
         ctx->Array.NormalStrideB = stride ? stride : 3*sizeof(GLshort);
         break;
      case GL_INT:
         ctx->Array.NormalStrideB = stride ? stride : 3*sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.NormalStrideB = stride ? stride : 3*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.NormalStrideB = stride ? stride : 3*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glNormalPointer(type)" );
         return;
   }
   ctx->Array.NormalType = type;
   ctx->Array.NormalStride = stride;
   ctx->Array.NormalPtr = (void *) ptr;
}



void gl_ColorPointer( GLcontext *ctx,
                      GLint size, GLenum type, GLsizei stride,
                      const GLvoid *ptr )
{
   if (size<3 || size>4) {
      gl_error( ctx, GL_INVALID_VALUE, "glColorPointer(size)" );
      return;
   }
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glColorPointer(stride)" );
      return;
   }
   switch (type) {
      case GL_BYTE:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLbyte);
         break;
      case GL_UNSIGNED_BYTE:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLubyte);
         break;
      case GL_SHORT:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLshort);
         break;
      case GL_UNSIGNED_SHORT:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLushort);
         break;
      case GL_INT:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLint);
         break;
      case GL_UNSIGNED_INT:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLuint);
         break;
      case GL_FLOAT:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.ColorStrideB = stride ? stride : size*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glColorPointer(type)" );
         return;
   }
   ctx->Array.ColorSize = size;
   ctx->Array.ColorType = type;
   ctx->Array.ColorStride = stride;
   ctx->Array.ColorPtr = (void *) ptr;
}



void gl_IndexPointer( GLcontext *ctx,
                      GLenum type, GLsizei stride, const GLvoid *ptr )
{
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glIndexPointer(stride)" );
      return;
   }
   switch (type) {
      case GL_SHORT:
         ctx->Array.IndexStrideB = stride ? stride : sizeof(GLbyte);
         break;
      case GL_INT:
         ctx->Array.IndexStrideB = stride ? stride : sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.IndexStrideB = stride ? stride : sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.IndexStrideB = stride ? stride : sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glIndexPointer(type)" );
         return;
   }
   ctx->Array.IndexType = type;
   ctx->Array.IndexStride = stride;
   ctx->Array.IndexPtr = (void *) ptr;
}



void gl_TexCoordPointer( GLcontext *ctx,
                         GLint size, GLenum type, GLsizei stride,
                         const GLvoid *ptr )
{
   GLuint texUnit = ctx->TexCoordUnit;
   if (size<1 || size>4) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexCoordPointer(size)" );
      return;
   }
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexCoordPointer(stride)" );
      return;
   }
   switch (type) {
      case GL_SHORT:
         ctx->Array.TexCoordStrideB[texUnit] = stride ? stride : size*sizeof(GLshort);
         break;
      case GL_INT:
         ctx->Array.TexCoordStrideB[texUnit] = stride ? stride : size*sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.TexCoordStrideB[texUnit] = stride ? stride : size*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.TexCoordStrideB[texUnit] = stride ? stride : size*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexCoordPointer(type)" );
         return;
   }
   ctx->Array.TexCoordSize[texUnit] = size;
   ctx->Array.TexCoordType[texUnit] = type;
   ctx->Array.TexCoordStride[texUnit] = stride;
   ctx->Array.TexCoordPtr[texUnit] = (void *) ptr;
}



/* GL_SGIS_multitexture ONLY!!! */
void gl_MultiTexCoordPointer( GLcontext *ctx, GLenum target,
                              GLint size, GLenum type, GLsizei stride,
                              const GLvoid *ptr )
{
   GLuint texUnit;
   if (target < GL_TEXTURE0_SGIS || target > GL_TEXTURE1_SGIS) {
      gl_error(ctx, GL_INVALID_ENUM, "glMultiTexCoord(target)");
      return;
   }
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glMultiTexCoordPointer(stride)" );
      return;
   }
   if (size<1 || size>4) {
      gl_error( ctx, GL_INVALID_VALUE, "glMultiTexCoordPointer(size)" );
      return;
   }
   texUnit = target - GL_TEXTURE0_SGIS;
   switch (type) {
      case GL_SHORT:
         ctx->Array.TexCoordStrideB[texUnit] = stride ? stride : size*sizeof(GLshort);
         break;
      case GL_INT:
         ctx->Array.TexCoordStrideB[texUnit] = stride ? stride : size*sizeof(GLint);
         break;
      case GL_FLOAT:
         ctx->Array.TexCoordStrideB[texUnit] = stride ? stride : size*sizeof(GLfloat);
         break;
      case GL_DOUBLE:
         ctx->Array.TexCoordStrideB[texUnit] = stride ? stride : size*sizeof(GLdouble);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glMultiTexCoordPointer(type)" );
         return;
   }
   ctx->Array.TexCoordSize[texUnit] = size;
   ctx->Array.TexCoordType[texUnit] = type;
   ctx->Array.TexCoordStride[texUnit] = stride;
   ctx->Array.TexCoordPtr[texUnit] = (void *) ptr;
}



void gl_InterleavedTextureCoordSets( GLcontext *ctx, GLint factor )
{
   if (factor < 1 || factor > ctx->Const.MaxTextureUnits) {
      gl_error( ctx, GL_INVALID_VALUE, "glInterleavedTextureCoordSets" );
      return;
   }
   ctx->Array.TexCoordInterleaveFactor = factor;
}



void gl_EdgeFlagPointer( GLcontext *ctx,
                         GLsizei stride, const GLboolean *ptr )
{
   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glEdgeFlagPointer(stride)" );
      return;
   }
   ctx->Array.EdgeFlagStride = stride;
   ctx->Array.EdgeFlagStrideB = stride ? stride : sizeof(GLboolean);
   ctx->Array.EdgeFlagPtr = (GLboolean *) ptr;
}



/*
 * Execute
 */
void gl_ArrayElement( GLcontext *ctx, GLint i )
{
   struct vertex_buffer *VB = ctx->VB;
   GLint count = VB->Count;
   GLuint texUnit;

   /* copy vertex data into the Vertex Buffer */

   if (ctx->Array.NormalEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.NormalPtr
                  + i * ctx->Array.NormalStrideB;
      switch (ctx->Array.NormalType) {
         case GL_BYTE:
            VB->Normal[count][0] = BYTE_TO_FLOAT( p[0] );
            VB->Normal[count][1] = BYTE_TO_FLOAT( p[1] );
            VB->Normal[count][2] = BYTE_TO_FLOAT( p[2] );
            break;
         case GL_SHORT:
            VB->Normal[count][0] = SHORT_TO_FLOAT( ((GLshort*)p)[0] );
            VB->Normal[count][1] = SHORT_TO_FLOAT( ((GLshort*)p)[1] );
            VB->Normal[count][2] = SHORT_TO_FLOAT( ((GLshort*)p)[2] );
            break;
         case GL_INT:
            VB->Normal[count][0] = INT_TO_FLOAT( ((GLint*)p)[0] );
            VB->Normal[count][1] = INT_TO_FLOAT( ((GLint*)p)[1] );
            VB->Normal[count][2] = INT_TO_FLOAT( ((GLint*)p)[2] );
            break;
         case GL_FLOAT:
            VB->Normal[count][0] = ((GLfloat*)p)[0];
            VB->Normal[count][1] = ((GLfloat*)p)[1];
            VB->Normal[count][2] = ((GLfloat*)p)[2];
            break;
         case GL_DOUBLE:
            VB->Normal[count][0] = ((GLdouble*)p)[0];
            VB->Normal[count][1] = ((GLdouble*)p)[1];
            VB->Normal[count][2] = ((GLdouble*)p)[2];
            break;
         default:
            gl_problem(ctx, "Bad normal type in gl_ArrayElement");
            return;
      }
      VB->MonoNormal = GL_FALSE;
   }
   else {
      VB->Normal[count][0] = ctx->Current.Normal[0];
      VB->Normal[count][1] = ctx->Current.Normal[1];
      VB->Normal[count][2] = ctx->Current.Normal[2];
   } 

   /* TODO: directly set VB->Fcolor instead of calling a glColor command */
   if (ctx->Array.ColorEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.ColorPtr + i * ctx->Array.ColorStrideB;
      switch (ctx->Array.ColorType) {
         case GL_BYTE:
            switch (ctx->Array.ColorSize) {
               case 4:   glColor4bv( (GLbyte*) p );   break;
               case 3:   glColor3bv( (GLbyte*) p );   break;
            }
            break;
         case GL_UNSIGNED_BYTE:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3ubv( (GLubyte*) p );   break;
               case 4:   glColor4ubv( (GLubyte*) p );   break;
            }
            break;
         case GL_SHORT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3sv( (GLshort*) p );   break;
               case 4:   glColor4sv( (GLshort*) p );   break;
            }
            break;
         case GL_UNSIGNED_SHORT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3usv( (GLushort*) p );   break;
               case 4:   glColor4usv( (GLushort*) p );   break;
            }
            break;
         case GL_INT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3iv( (GLint*) p );   break;
               case 4:   glColor4iv( (GLint*) p );   break;
            }
            break;
         case GL_UNSIGNED_INT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3uiv( (GLuint*) p );   break;
               case 4:   glColor4uiv( (GLuint*) p );   break;
            }
            break;
         case GL_FLOAT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3fv( (GLfloat*) p );   break;
               case 4:   glColor4fv( (GLfloat*) p );   break;
            }
            break;
         case GL_DOUBLE:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3dv( (GLdouble*) p );   break;
               case 4:   glColor4dv( (GLdouble*) p );   break;
            }
            break;
         default:
            gl_problem(ctx, "Bad color type in gl_ArrayElement");
            return;
      }
      ctx->VB->MonoColor = GL_FALSE;
   }

   /* current color has been updated. store in vertex buffer now */
   {
      COPY_4UBV( VB->Fcolor[count], ctx->Current.ByteColor );
      if (ctx->Light.ColorMaterialEnabled) {
         GLfloat color[4];
         color[0] = ctx->Current.ByteColor[0] * (1.0F / 255.0F);
         color[1] = ctx->Current.ByteColor[1] * (1.0F / 255.0F);
         color[2] = ctx->Current.ByteColor[2] * (1.0F / 255.0F);
         color[3] = ctx->Current.ByteColor[3] * (1.0F / 255.0F);
         gl_set_material( ctx, ctx->Light.ColorMaterialBitmask, color );
      }
   }

   if (ctx->Array.IndexEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.IndexPtr + i * ctx->Array.IndexStrideB;
      switch (ctx->Array.IndexType) {
         case GL_SHORT:
            VB->Findex[count] = (GLuint) (*((GLshort*) p));
            break;
         case GL_INT:
            VB->Findex[count] = (GLuint) (*((GLint*) p));
            break;
         case GL_FLOAT:
            VB->Findex[count] = (GLuint) (*((GLfloat*) p));
            break;
         case GL_DOUBLE:
            VB->Findex[count] = (GLuint) (*((GLdouble*) p));
            break;
         default:
            gl_problem(ctx, "Bad index type in gl_ArrayElement");
            return;
      }
      ctx->VB->MonoColor = GL_FALSE;
   }
   else {
      VB->Findex[count] = ctx->Current.Index;
   }

   for (texUnit=0; texUnit<ctx->Const.MaxTextureUnits; texUnit++) {
       if (ctx->Array.TexCoordEnabled[texUnit]) {
          GLbyte *p = (GLbyte*) ctx->Array.TexCoordPtr[texUnit]
                      + i * ctx->Array.TexCoordStrideB[texUnit];
          VB->MultiTexCoord[texUnit][count][1] = 0.0F;
          VB->MultiTexCoord[texUnit][count][2] = 0.0F;
          VB->MultiTexCoord[texUnit][count][3] = 1.0F;
          switch (ctx->Array.TexCoordType[texUnit]) {
             case GL_SHORT:
                switch (ctx->Array.TexCoordSize[texUnit]) {
                   /* FALL THROUGH! */
                   case 4:   VB->MultiTexCoord[texUnit][count][3] = ((GLshort*) p)[3];
                   case 3:   VB->MultiTexCoord[texUnit][count][2] = ((GLshort*) p)[2];
                   case 2:   VB->MultiTexCoord[texUnit][count][1] = ((GLshort*) p)[1];
                   case 1:   VB->MultiTexCoord[texUnit][count][0] = ((GLshort*) p)[0];
                }
                break;
             case GL_INT:
                switch (ctx->Array.TexCoordSize[texUnit]) {
                   /* FALL THROUGH! */
                   case 4:   VB->MultiTexCoord[texUnit][count][3] = ((GLint*) p)[3];
                   case 3:   VB->MultiTexCoord[texUnit][count][2] = ((GLint*) p)[2];
                   case 2:   VB->MultiTexCoord[texUnit][count][1] = ((GLint*) p)[1];
                   case 1:   VB->MultiTexCoord[texUnit][count][0] = ((GLint*) p)[0];
                }
                break;
             case GL_FLOAT:
                switch (ctx->Array.TexCoordSize[texUnit]) {
                   /* FALL THROUGH! */
                   case 4:   VB->MultiTexCoord[texUnit][count][3] = ((GLfloat*) p)[3];
                   case 3:   VB->MultiTexCoord[texUnit][count][2] = ((GLfloat*) p)[2];
                   case 2:   VB->MultiTexCoord[texUnit][count][1] = ((GLfloat*) p)[1];
                   case 1:   VB->MultiTexCoord[texUnit][count][0] = ((GLfloat*) p)[0];
                }
                break;
             case GL_DOUBLE:
                switch (ctx->Array.TexCoordSize[texUnit]) {
                   /* FALL THROUGH! */
                   case 4:   VB->MultiTexCoord[texUnit][count][3] = ((GLdouble*) p)[3];
                   case 3:   VB->MultiTexCoord[texUnit][count][2] = ((GLdouble*) p)[2];
                   case 2:   VB->MultiTexCoord[texUnit][count][1] = ((GLdouble*) p)[1];
                   case 1:   VB->MultiTexCoord[texUnit][count][0] = ((GLdouble*) p)[0];
                }
                break;
             default:
                gl_problem(ctx, "Bad texcoord type in gl_ArrayElement");
                return;
          }
       }
       else {
          COPY_4V( VB->MultiTexCoord[texUnit][count], ctx->Current.MultiTexCoord[texUnit] );
       }
   }

   if (ctx->Array.EdgeFlagEnabled) {
      GLbyte *b = (GLbyte*) ctx->Array.EdgeFlagPtr
                  + i * ctx->Array.EdgeFlagStrideB;
      VB->Edgeflag[count] = *((GLboolean*) b);
   }
   else {
      VB->Edgeflag[count] = ctx->Current.EdgeFlag;
   }

   if (ctx->Array.VertexEnabled) {
      GLbyte *b = (GLbyte*) ctx->Array.VertexPtr
                  + i * ctx->Array.VertexStrideB;
      VB->Obj[count][2] = 0.0F;
      VB->Obj[count][3] = 1.0F;
      switch (ctx->Array.VertexType) {
         case GL_SHORT:
            switch (ctx->Array.VertexSize) {
               /* FALL THROUGH */
               case 4:   VB->Obj[count][3] = ((GLshort*) b)[3];
               case 3:   VB->Obj[count][2] = ((GLshort*) b)[2];
               case 2:   VB->Obj[count][1] = ((GLshort*) b)[1];
                         VB->Obj[count][0] = ((GLshort*) b)[0];
            }
            break;
         case GL_INT:
            switch (ctx->Array.VertexSize) {
               /* FALL THROUGH */
               case 4:   VB->Obj[count][3] = ((GLint*) b)[3];
               case 3:   VB->Obj[count][2] = ((GLint*) b)[2];
               case 2:   VB->Obj[count][1] = ((GLint*) b)[1];
                         VB->Obj[count][0] = ((GLint*) b)[0];
            }
            break;
         case GL_FLOAT:
            switch (ctx->Array.VertexSize) {
               /* FALL THROUGH */
               case 4:   VB->Obj[count][3] = ((GLfloat*) b)[3];
               case 3:   VB->Obj[count][2] = ((GLfloat*) b)[2];
               case 2:   VB->Obj[count][1] = ((GLfloat*) b)[1];
                         VB->Obj[count][0] = ((GLfloat*) b)[0];
            }
            break;
         case GL_DOUBLE:
            switch (ctx->Array.VertexSize) {
               /* FALL THROUGH */
               case 4:   VB->Obj[count][3] = ((GLdouble*) b)[3];
               case 3:   VB->Obj[count][2] = ((GLdouble*) b)[2];
               case 2:   VB->Obj[count][1] = ((GLdouble*) b)[1];
                         VB->Obj[count][0] = ((GLdouble*) b)[0];
            }
            break;
         default:
            gl_problem(ctx, "Bad vertex type in gl_ArrayElement");
            return;
      }

      /* Only store vertex if Vertex array pointer is enabled */
      count++;
      VB->Count = count;
      if (count==VB_MAX) {
         gl_transform_vb( ctx, GL_FALSE );
      }

   }
   else {
      /* vertex array pointer not enabled: no vertex to process */
   }
}




/*
 * Save into display list
 * Use external API entry points since speed isn't too important here
 * and makes the code simpler.  Also, if GL_COMPILE_AND_EXECUTE then
 * execute will happen too.
 */
void gl_save_ArrayElement( GLcontext *ctx, GLint i )
{
   GLuint texUnit;
   if (ctx->Array.NormalEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.NormalPtr
                  + i * ctx->Array.NormalStrideB;
      switch (ctx->Array.NormalType) {
         case GL_BYTE:
            glNormal3bv( (GLbyte*) p );
            break;
         case GL_SHORT:
            glNormal3sv( (GLshort*) p );
            break;
         case GL_INT:
            glNormal3iv( (GLint*) p );
            break;
         case GL_FLOAT:
            glNormal3fv( (GLfloat*) p );
            break;
         case GL_DOUBLE:
            glNormal3dv( (GLdouble*) p );
            break;
         default:
            gl_problem(ctx, "Bad normal type in gl_save_ArrayElement");
            return;
      }
   }

   if (ctx->Array.ColorEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.ColorPtr + i * ctx->Array.ColorStrideB;
      switch (ctx->Array.ColorType) {
         case GL_BYTE:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3bv( (GLbyte*) p );   break;
               case 4:   glColor4bv( (GLbyte*) p );   break;
            }
            break;
         case GL_UNSIGNED_BYTE:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3ubv( (GLubyte*) p );   break;
               case 4:   glColor4ubv( (GLubyte*) p );   break;
            }
            break;
         case GL_SHORT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3sv( (GLshort*) p );   break;
               case 4:   glColor4sv( (GLshort*) p );   break;
            }
            break;
         case GL_UNSIGNED_SHORT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3usv( (GLushort*) p );   break;
               case 4:   glColor4usv( (GLushort*) p );   break;
            }
            break;
         case GL_INT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3iv( (GLint*) p );   break;
               case 4:   glColor4iv( (GLint*) p );   break;
            }
            break;
         case GL_UNSIGNED_INT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3uiv( (GLuint*) p );   break;
               case 4:   glColor4uiv( (GLuint*) p );   break;
            }
            break;
         case GL_FLOAT:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3fv( (GLfloat*) p );   break;
               case 4:   glColor4fv( (GLfloat*) p );   break;
            }
            break;
         case GL_DOUBLE:
            switch (ctx->Array.ColorSize) {
               case 3:   glColor3dv( (GLdouble*) p );   break;
               case 4:   glColor4dv( (GLdouble*) p );   break;
            }
            break;
         default:
            gl_problem(ctx, "Bad color type in gl_save_ArrayElement");
            return;
      }
   }

   if (ctx->Array.IndexEnabled) {
      GLbyte *p = (GLbyte*) ctx->Array.IndexPtr + i * ctx->Array.IndexStrideB;
      switch (ctx->Array.IndexType) {
         case GL_SHORT:
            glIndexsv( (GLshort*) p );
            break;
         case GL_INT:
            glIndexiv( (GLint*) p );
            break;
         case GL_FLOAT:
            glIndexfv( (GLfloat*) p );
            break;
         case GL_DOUBLE:
            glIndexdv( (GLdouble*) p );
            break;
         default:
            gl_problem(ctx, "Bad index type in gl_save_ArrayElement");
            return;
      }
   }

   for (texUnit = 0; texUnit < ctx->Const.MaxTextureUnits; texUnit++) {
      if (ctx->Array.TexCoordEnabled[texUnit]) {
         GLbyte *p = (GLbyte*) ctx->Array.TexCoordPtr[texUnit]
                     + i * ctx->Array.TexCoordStrideB[texUnit];
         GLenum target = (GLenum) (GL_TEXTURE0_SGIS + texUnit);
         switch (ctx->Array.TexCoordType[texUnit]) {
            case GL_SHORT:
               switch (ctx->Array.TexCoordSize[texUnit]) {
                  case 1:   glMultiTexCoord1svSGIS( target, (GLshort*) p );   break;
                  case 2:   glMultiTexCoord2svSGIS( target, (GLshort*) p );   break;
                  case 3:   glMultiTexCoord3svSGIS( target, (GLshort*) p );   break;
                  case 4:   glMultiTexCoord4svSGIS( target, (GLshort*) p );   break;
               }
               break;
            case GL_INT:
               switch (ctx->Array.TexCoordSize[texUnit]) {
                  case 1:   glMultiTexCoord1ivSGIS( target, (GLint*) p );   break;
                  case 2:   glMultiTexCoord2ivSGIS( target, (GLint*) p );   break;
                  case 3:   glMultiTexCoord3ivSGIS( target, (GLint*) p );   break;
                  case 4:   glMultiTexCoord4ivSGIS( target, (GLint*) p );   break;
               }
               break;
            case GL_FLOAT:
               switch (ctx->Array.TexCoordSize[texUnit]) {
                  case 1:   glMultiTexCoord1fvSGIS( target, (GLfloat*) p );   break;
                  case 2:   glMultiTexCoord2fvSGIS( target, (GLfloat*) p );   break;
                  case 3:   glMultiTexCoord3fvSGIS( target, (GLfloat*) p );   break;
                  case 4:   glMultiTexCoord4fvSGIS( target, (GLfloat*) p );   break;
               }
               break;
            case GL_DOUBLE:
               switch (ctx->Array.TexCoordSize[texUnit]) {
                  case 1:   glMultiTexCoord1dvSGIS( target, (GLdouble*) p );   break;
                  case 2:   glMultiTexCoord2dvSGIS( target, (GLdouble*) p );   break;
                  case 3:   glMultiTexCoord3dvSGIS( target, (GLdouble*) p );   break;
                  case 4:   glMultiTexCoord4dvSGIS( target, (GLdouble*) p );   break;
               }
               break;
            default:
               gl_problem(ctx, "Bad texcoord type in gl_save_ArrayElement");
               return;
         }
      }
   }

   if (ctx->Array.EdgeFlagEnabled) {
      GLbyte *b = (GLbyte*) ctx->Array.EdgeFlagPtr + i * ctx->Array.EdgeFlagStrideB;
      glEdgeFlagv( (GLboolean*) b );
   }

   if (ctx->Array.VertexEnabled) {
      GLbyte *b = (GLbyte*) ctx->Array.VertexPtr
                  + i * ctx->Array.VertexStrideB;
      switch (ctx->Array.VertexType) {
         case GL_SHORT:
            switch (ctx->Array.VertexSize) {
               case 2:   glVertex2sv( (GLshort*) b );   break;
               case 3:   glVertex3sv( (GLshort*) b );   break;
               case 4:   glVertex4sv( (GLshort*) b );   break;
            }
            break;
         case GL_INT:
            switch (ctx->Array.VertexSize) {
               case 2:   glVertex2iv( (GLint*) b );   break;
               case 3:   glVertex3iv( (GLint*) b );   break;
               case 4:   glVertex4iv( (GLint*) b );   break;
            }
            break;
         case GL_FLOAT:
            switch (ctx->Array.VertexSize) {
               case 2:   glVertex2fv( (GLfloat*) b );   break;
               case 3:   glVertex3fv( (GLfloat*) b );   break;
               case 4:   glVertex4fv( (GLfloat*) b );   break;
            }
            break;
         case GL_DOUBLE:
            switch (ctx->Array.VertexSize) {
               case 2:   glVertex2dv( (GLdouble*) b );   break;
               case 3:   glVertex3dv( (GLdouble*) b );   break;
               case 4:   glVertex4dv( (GLdouble*) b );   break;
            }
            break;
         default:
            gl_problem(ctx, "Bad vertex type in gl_save_ArrayElement");
            return;
      }
   }
}



/*
 * Execute
 */
void gl_DrawArrays( GLcontext *ctx, GLenum mode, GLint first, GLsizei count )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint texUnit = ctx->Texture.CurrentUnit;
   GLint i;
   GLboolean need_edges;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDrawArrays" );
      return;
   }
   if (count<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glDrawArrays(count)" );
      return;
   }

   if (ctx->Primitive==GL_TRIANGLES || 
       ctx->Primitive==GL_QUADS || 
       ctx->Primitive==GL_POLYGON) {
      need_edges = GL_TRUE;
   }
   else {
      need_edges = GL_FALSE;
   }

   if (!ctx->CompileFlag
       && !ctx->Texture.Enabled
       && ctx->Array.VertexEnabled && ctx->Array.VertexType==GL_FLOAT
       && (!ctx->Array.NormalEnabled || ctx->Array.NormalType==GL_FLOAT)
       && !ctx->Array.ColorEnabled
       && !ctx->Array.IndexEnabled
       && !ctx->Array.TexCoordEnabled[texUnit]
       && !ctx->Array.EdgeFlagEnabled) 
   {
      GLint remaining = count;
      GLuint vstride = ctx->Array.VertexStrideB / sizeof(GLfloat);
      GLuint nstride = ctx->Array.NormalStrideB / sizeof(GLfloat);
      GLfloat *vptr = (GLfloat *) ctx->Array.VertexPtr + vstride * first;
      GLfloat *nptr = (GLfloat *) ctx->Array.NormalPtr + nstride * first;

      ASSERT(ctx->Array.VertexStrideB % 4 == 0); /* unaligned data is bad */
      ASSERT(ctx->Array.NormalStrideB % 4 == 0);

      /* setup constant values */
      {
         GLint i, n = MIN2(VB_MAX, count);
         if (!ctx->Light.Enabled) {
            for (i=0;i<n;i++) {
               COPY_4UBV( VB->Fcolor[i], ctx->Current.ByteColor );
            }
         }
         if (need_edges) {
            for (i=0;i<n;i++) {
               VB->Edgeflag[i] = ctx->Current.EdgeFlag;
            }
         }
      }

      gl_Begin( ctx, mode );

      while (remaining > 0) {
         GLint vbspace = VB_MAX - VB->Start;
         GLint n = MIN2( vbspace, remaining );

         /* Vertexes */
	 VB->ObjPtr = vptr - VB->Start * vstride;
	 VB->VertexSizeMask = VERTEX_SZ_BIT(ctx->Array.VertexSize);
	 VB->ObjStride = vstride;
	 	 
         /* Normals */
	 if (ctx->Array.NormalEnabled) {
	    VB->NormalPtr = nptr - VB->Start * nstride;
	    VB->NormalStride = nstride;
	    VB->MonoNormal = GL_FALSE;
	 }
	 else {
	    VB->MonoNormal = GL_TRUE;
         }

	 /* update the lighting pointers */
	 gl_update_shade_context(ctx, &ctx->shade_context);

         remaining -= n;
         VB->Count = VB->Start + n;

         /* Transform and render */
         gl_transform_vb( ctx, remaining==0 ? GL_TRUE : GL_FALSE );

         vptr += vstride * n;
         nptr += nstride * n;
      }
      
      gl_End( ctx );

      /* put the pointers back where they were */
      VB->ObjPtr = (GLfloat *)VB->Obj;
      VB->ObjCoordSize = 4;
      VB->ObjStride = 4;
      VB->NormalPtr = (GLfloat *)VB->Normal;
      VB->NormalStride = 3;
           
      /* reset the lighting pointers */
      gl_update_shade_context( ctx, &ctx->shade_context );
      
   }
   else {
      /*
       * GENERAL CASE:
       */
      gl_Begin( ctx, mode );
      for (i=0;i<count;i++) {
         gl_ArrayElement( ctx, first+i );
      }
      gl_End( ctx );
   }
}



/*
 * Save into a display list
 */
void gl_save_DrawArrays( GLcontext *ctx,
                         GLenum mode, GLint first, GLsizei count )
{
   GLint i;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDrawArrays" );
      return;
   }
   if (count<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glDrawArrays(count)" );
      return;
   }
   switch (mode) {
      case GL_POINTS:
      case GL_LINES:
      case GL_LINE_STRIP:
      case GL_LINE_LOOP:
      case GL_TRIANGLES:
      case GL_TRIANGLE_STRIP:
      case GL_TRIANGLE_FAN:
      case GL_QUADS:
      case GL_QUAD_STRIP:
      case GL_POLYGON:
         /* OK */
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDrawArrays(mode)" );
         return;
   }

   /* Note: this will do compile AND execute if needed */
   gl_save_Begin( ctx, mode );
   for (i=0;i<count;i++) {
      gl_save_ArrayElement( ctx, first+i );
   }
   gl_save_End( ctx );
}




/*
 * Execute only
 */
void gl_DrawElements( GLcontext *ctx,
                      GLenum mode, GLsizei count,
                      GLenum type, const GLvoid *indices )
{
   
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDrawElements" );
      return;
   }
   if (count<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glDrawElements(count)" );
      return;
   }
   switch (mode) {
      case GL_POINTS:
      case GL_LINES:
      case GL_LINE_STRIP:
      case GL_LINE_LOOP:
      case GL_TRIANGLES:
      case GL_TRIANGLE_STRIP:
      case GL_TRIANGLE_FAN:
      case GL_QUADS:
      case GL_QUAD_STRIP:
      case GL_POLYGON:
         /* OK */
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDrawArrays(mode)" );
         return;
   }
   switch (type) {
      case GL_UNSIGNED_BYTE:
         {
            GLubyte *ub_indices = (GLubyte *) indices;
            GLint i;
            gl_Begin( ctx, mode );
            for (i=0;i<count;i++) {
               gl_ArrayElement( ctx, (GLint) ub_indices[i] );
            }
            gl_End( ctx );
         }
         break;
      case GL_UNSIGNED_SHORT:
         {
            GLushort *us_indices = (GLushort *) indices;
            GLint i;
            gl_Begin( ctx, mode );
            for (i=0;i<count;i++) {
               gl_ArrayElement( ctx, (GLint) us_indices[i] );
            }
            gl_End( ctx );
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint *ui_indices = (GLuint *) indices;
            GLint i;
            gl_Begin( ctx, mode );
            for (i=0;i<count;i++) {
               gl_ArrayElement( ctx, (GLint) ui_indices[i] );
            }
            gl_End( ctx );
         }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDrawElements(type)" );
         return;
   }
}




/*
 * Save (and perhaps execute)
 */
void gl_save_DrawElements( GLcontext *ctx,
                           GLenum mode, GLsizei count,
                           GLenum type, const GLvoid *indices )
{
   switch (type) {
      case GL_UNSIGNED_BYTE:
         {
            GLubyte *ub_indices = (GLubyte *) indices;
            GLint i;
            gl_save_Begin( ctx, mode );
            for (i=0;i<count;i++) {
               gl_save_ArrayElement( ctx, (GLint) ub_indices[i] );
            }
            gl_save_End( ctx );
         }
         break;
      case GL_UNSIGNED_SHORT:
         {
            GLushort *us_indices = (GLushort *) indices;
            GLint i;
            gl_save_Begin( ctx, mode );
            for (i=0;i<count;i++) {
               gl_save_ArrayElement( ctx, (GLint) us_indices[i] );
            }
            gl_save_End( ctx );
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint *ui_indices = (GLuint *) indices;
            GLint i;
            gl_save_Begin( ctx, mode );
            for (i=0;i<count;i++) {
               gl_save_ArrayElement( ctx, (GLint) ui_indices[i] );
            }
            gl_save_End( ctx );
         }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDrawElements(type)" );
         return;
   }
}



void gl_InterleavedArrays( GLcontext *ctx,
                           GLenum format, GLsizei stride,
                           const GLvoid *pointer )
{
   GLboolean tflag, cflag, nflag;  /* enable/disable flags */
   GLint tcomps, ccomps, vcomps;   /* components per texcoord, color, vertex */

   GLenum ctype;                   /* color type */
   GLint coffset, noffset, voffset;/* color, normal, vertex offsets */
   GLint defstride;                /* default stride */
   GLint c, f;
   GLint coordUnitSave;

   f = sizeof(GLfloat);
   c = f * ((4*sizeof(GLubyte) + (f-1)) / f);

   if (stride<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glInterleavedArrays(stride)" );
      return;
   }

   switch (format) {
      case GL_V2F:
         tflag = GL_FALSE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 0;  vcomps = 2;
         voffset = 0;
         defstride = 2*f;
         break;
      case GL_V3F:
         tflag = GL_FALSE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 0;  vcomps = 3;
         voffset = 0;
         defstride = 3*f;
         break;
      case GL_C4UB_V2F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 4;  vcomps = 2;
         ctype = GL_UNSIGNED_BYTE;
         coffset = 0;
         voffset = c;
         defstride = c + 2*f;
         break;
      case GL_C4UB_V3F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 4;  vcomps = 3;
         ctype = GL_UNSIGNED_BYTE;
         coffset = 0;
         voffset = c;
         defstride = c + 3*f;
         break;
      case GL_C3F_V3F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 3;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 0;
         voffset = 3*f;
         defstride = 6*f;
         break;
      case GL_N3F_V3F:
         tflag = GL_FALSE;  cflag = GL_FALSE;  nflag = GL_TRUE;
         tcomps = 0;  ccomps = 0;  vcomps = 3;
         noffset = 0;
         voffset = 3*f;
         defstride = 6*f;
         break;
      case GL_C4F_N3F_V3F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_TRUE;
         tcomps = 0;  ccomps = 4;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 0;
         noffset = 4*f;
         voffset = 7*f;
         defstride = 10*f;
         break;
      case GL_T2F_V3F:
         tflag = GL_TRUE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 2;  ccomps = 0;  vcomps = 3;
         voffset = 2*f;
         defstride = 5*f;
         break;
      case GL_T4F_V4F:
         tflag = GL_TRUE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 4;  ccomps = 0;  vcomps = 4;
         voffset = 4*f;
         defstride = 8*f;
         break;
      case GL_T2F_C4UB_V3F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 2;  ccomps = 4;  vcomps = 3;
         ctype = GL_UNSIGNED_BYTE;
         coffset = 2*f;
         voffset = c+2*f;
         defstride = c+5*f;
         break;
      case GL_T2F_C3F_V3F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 2;  ccomps = 3;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 2*f;
         voffset = 5*f;
         defstride = 8*f;
         break;
      case GL_T2F_N3F_V3F:
         tflag = GL_TRUE;  cflag = GL_FALSE;  nflag = GL_TRUE;
         tcomps = 2;  ccomps = 0;  vcomps = 3;
         noffset = 2*f;
         voffset = 5*f;
         defstride = 8*f;
         break;
      case GL_T2F_C4F_N3F_V3F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_TRUE;
         tcomps = 2;  ccomps = 4;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 2*f;
         noffset = 6*f;
         voffset = 9*f;
         defstride = 12*f;
         break;
      case GL_T4F_C4F_N3F_V4F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_TRUE;
         tcomps = 4;  ccomps = 4;  vcomps = 4;
         ctype = GL_FLOAT;
         coffset = 4*f;
         noffset = 8*f;
         voffset = 11*f;
         defstride = 15*f;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glInterleavedArrays(format)" );
         return;
   }

   if (stride==0) {
      stride = defstride;
   }

   gl_DisableClientState( ctx, GL_EDGE_FLAG_ARRAY );
   gl_DisableClientState( ctx, GL_INDEX_ARRAY );

   /* Texcoords */
   coordUnitSave = ctx->TexCoordUnit;
   if (tflag) {
      GLint i;
      GLint factor = ctx->Array.TexCoordInterleaveFactor;
      for (i = 0; i < factor; i++) {
         gl_SelectTextureCoordSet( ctx, (GLenum) (GL_TEXTURE0_SGIS + i) );
         gl_EnableClientState( ctx, GL_TEXTURE_COORD_ARRAY );
         gl_TexCoordPointer( ctx, tcomps, GL_FLOAT, stride,
                             (GLubyte *) pointer + i * coffset );
      }
      for (i = factor; i < ctx->Const.MaxTextureUnits; i++) {
         gl_SelectTextureCoordSet( ctx, (GLenum) (GL_TEXTURE0_SGIS + i) );
         gl_DisableClientState( ctx, GL_TEXTURE_COORD_ARRAY );
      }
   }
   else {
      GLint i;
      for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
         gl_SelectTextureCoordSet( ctx, (GLenum) (GL_TEXTURE0_SGIS + i) );
         gl_DisableClientState( ctx, GL_TEXTURE_COORD_ARRAY );
      }
   }
   /* Restore texture coordinate unit index */
   gl_SelectTextureCoordSet( ctx, (GLenum) (GL_TEXTURE0_SGIS + coordUnitSave) );


   /* Color */
   if (cflag) {
      gl_EnableClientState( ctx, GL_COLOR_ARRAY );
      gl_ColorPointer( ctx, ccomps, ctype, stride,
                       (GLubyte*) pointer + coffset );
   }
   else {
      gl_DisableClientState( ctx, GL_COLOR_ARRAY );
   }


   /* Normals */
   if (nflag) {
      gl_EnableClientState( ctx, GL_NORMAL_ARRAY );
      gl_NormalPointer( ctx, GL_FLOAT, stride,
                        (GLubyte*) pointer + noffset );
   }
   else {
      gl_DisableClientState( ctx, GL_NORMAL_ARRAY );
   }

   gl_EnableClientState( ctx, GL_VERTEX_ARRAY );
   gl_VertexPointer( ctx, vcomps, GL_FLOAT, stride,
                     (GLubyte *) pointer + voffset );
}



void gl_save_InterleavedArrays( GLcontext *ctx,
                                GLenum format, GLsizei stride,
                                const GLvoid *pointer )
{
   /* Just execute since client-side state changes aren't put in
    * display lists.
    */
   gl_InterleavedArrays( ctx, format, stride, pointer );
}



void gl_DrawRangeElements( GLcontext *ctx, GLenum mode, GLuint start,
                           GLuint end, GLsizei count, GLenum type,
                           const GLvoid *indices )
{
   /* XXX TODO optimize this function someday- it would be worthwhile */
   if (end < start) {
      gl_error(ctx, GL_INVALID_VALUE, "glDrawRangeElements( end < start )");
      return;
   }
   (void) start;
   (void) end;
   gl_DrawElements( ctx, mode, count, type, indices );
}


void gl_save_DrawRangeElements( GLcontext *ctx, GLenum mode,
                                GLuint start, GLuint end, GLsizei count,
                                GLenum type, const GLvoid *indices )
{
   /* XXX TODO optimize this function someday- it would be worthwhile */
   (void) start;
   (void) end;
   gl_save_DrawElements( ctx, mode, count, type, indices );
}
