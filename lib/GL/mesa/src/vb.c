/* Id: vb.c,v 1.3 1999/02/26 08:52:39 martin Exp $ */

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
 * Log: vb.c,v $
 * Revision 1.3  1999/02/26 08:52:39  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.6  1999/02/24 22:48:08  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.5  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.4  1998/10/31 17:06:15  brianp
 * variety of multi-texture changes
 *
 * Revision 3.3  1998/10/29 03:57:11  brianp
 * misc clean-up of new vertex transformation code
 *
 * Revision 3.2  1998/10/29 02:28:13  brianp
 * incorporated Keith Whitwell's transformation optimizations
 *
 * Revision 3.1  1998/02/20 04:53:07  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.0  1998/01/31 21:06:45  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/vb.c,v 1.2 1999/03/14 03:20:54 dawes Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <stdlib.h>
#else
#include "GL/xf86glx.h"
#endif
#include "types.h"
#include "vb.h"
#endif


/*
 * Allocate and initialize a vertex buffer.
 */
struct vertex_buffer *gl_alloc_vb(void)
{
   struct vertex_buffer *vb;
   vb = (struct vertex_buffer *) calloc(sizeof(struct vertex_buffer), 1);
   if (vb) {
      /* set non-zero fields */
      GLuint i, j;
      for (i=0;i<VB_SIZE;i++) {
         vb->ClipMask[i] = 0;
         vb->Obj[i][3] = 1.0F;
         for (j=0;j<MAX_TEXTURE_UNITS;j++) {
            vb->MultiTexCoord[j][i][2] = 0.0F;
            vb->MultiTexCoord[j][i][3] = 1.0F;
         }
      }
      vb->LastMaterial = 0;
      vb->NextMaterial[0] = 0;
      vb->MaterialMask[0] = 0;

      vb->TexCoord = vb->MultiTexCoord[0];
      vb->VertexSizeMask = VERTEX3_BIT;
      vb->TexCoordSize = 2;
      vb->MonoColor = GL_TRUE;
      vb->MonoNormal = GL_TRUE;
      vb->ClipOrMask = 0;
      vb->ClipAndMask = CLIP_ALL_BITS;

      /* Indirection for vertex array shortcuts */
      vb->ObjPtr = (GLfloat *)vb->Obj;
      vb->ObjCoordSize = 4;
      vb->ObjStride = 4;

      vb->NormalPtr = (GLfloat *)vb->Normal;
      vb->NormalStride = 3;
   }
   return vb;
}
