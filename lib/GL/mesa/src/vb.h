/* Id: vb.h,v 1.2 1999/02/26 08:52:39 martin Exp $ */

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
 * Log: vb.h,v $
 * Revision 1.2  1999/02/26 08:52:39  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.8  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.7  1998/10/31 17:06:15  brianp
 * variety of multi-texture changes
 *
 * Revision 3.6  1998/10/29 03:57:11  brianp
 * misc clean-up of new vertex transformation code
 *
 * Revision 3.5  1998/10/29 02:28:13  brianp
 * incorporated Keith Whitwell's transformation optimizations
 *
 * Revision 3.4  1998/06/22 03:16:00  brianp
 * moved VB_MAX definition to config.h
 *
 * Revision 3.3  1998/06/21 02:02:23  brianp
 * added another comment
 *
 * Revision 3.2  1998/02/20 04:53:07  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.1  1998/02/02 03:09:34  brianp
 * added GL_LIGHT_MODEL_COLOR_CONTROL (separate specular color interpolation)
 *
 * Revision 3.0  1998/01/31 21:06:45  brianp
 * initial rev
 *
 */


/*
 * OVERVIEW:  The vertices defined between glBegin() and glEnd()
 * are accumulated in the vertex buffer.  When either the
 * vertex buffer becomes filled or glEnd() is called, we must flush
 * the buffer.  That is, we apply the vertex transformations, compute
 * lighting, fog, texture coordinates etc.  Then, we can render the
 * vertices as points, lines or polygons by calling the gl_render_vb()
 * function in render.c
 *
 * When we're outside of a glBegin/glEnd pair the information in this
 * structure is irrelevant.
 */


#ifndef VB_H
#define VB_H


#include "types.h"



/*
 * Actual vertex buffer size.
 * Arrays must also accomodate new vertices from clipping.
 */
#define VB_SIZE  (VB_MAX + 2 * (6 + MAX_CLIP_PLANES))


/* Bit values for VertexSizeMask, below. */
#define VERTEX2_BIT  1
#define VERTEX3_BIT  2
#define VERTEX4_BIT  4
#define VERTEX_SZ_BIT(n) (1<<((n)-2))


struct vertex_buffer {
   const GLfloat *ObjPtr;	/* Pointer to source of Object-space coords */
   GLuint ObjCoordSize;		/* Coordinate size: 2, 3, or 4 */
   GLuint ObjStride;		/* Stride between coordinates (in floats) */
   
   const GLfloat *NormalPtr;	/* Pointer to source of vertex normals */
   GLuint NormalStride;		/* Stride between normals (in floats) */

   GLfloat Obj[VB_SIZE][4];	/* Object coords */
   GLfloat Eye[VB_SIZE][4];	/* Eye coords */
   GLfloat Clip[VB_SIZE][4];	/* Clip coords */
   GLfloat Win[VB_SIZE][3];	/* Window coords */

   GLfloat Normal[VB_SIZE][3];	/* Normal vectors */

   GLubyte Fcolor[VB_SIZE][4];	/* Front colors (RGBA) */
   GLubyte Bcolor[VB_SIZE][4];	/* Back colors (RGBA) */
   GLubyte (*Color)[4];		/* == Fcolor or Bcolor */

   GLubyte Fspec[VB_SIZE][4];	/* Front specular color (RGBA) */
   GLubyte Bspec[VB_SIZE][4];	/* Back specular color (RGBA) */
   GLubyte (*Specular)[4];	/* == Fspec or Bspec */

   GLuint Findex[VB_SIZE];	/* Front color indexes */
   GLuint Bindex[VB_SIZE];	/* Back color indexes */
   GLuint *Index;		/* == Findex or Bindex */

   GLboolean Edgeflag[VB_SIZE];	/* Polygon edge flags */

   GLfloat (*TexCoord)[4];	/* Points to one of the MultiTexCoord sets */
   GLfloat MultiTexCoord[MAX_TEXTURE_UNITS][VB_SIZE][4];/* Texture coords */

   GLubyte ClipMask[VB_SIZE];	/* bitwise-OR of CLIP_* values, below */
   GLubyte ClipOrMask;		/* bitwise-OR of all ClipMask[] values */
   GLubyte ClipAndMask;		/* bitwise-AND of all ClipMask[] values */

   GLuint Start;		/* First vertex to process */
   GLuint Count;		/* Number of vertexes in buffer */
   GLuint Free;			/* Next empty position for clipping */

   GLuint VertexSizeMask;	/* Bitwise-or of VERTEX[234]_BIT */
   GLuint TexCoordSize;		/* Either 2 or 4 */

   GLboolean MonoColor;		/* Do all vertices have same color? */
   GLboolean MonoNormal;	/* Do all vertices have same normal? */

   /* to handle glMaterial calls inside glBegin/glEnd: */
   GLuint MaterialMask[VB_SIZE];       /* Which material values to change */
   GLuint NextMaterial[VB_SIZE];       /* Till when does it hold? */
   GLuint LastMaterial;		       /* Scratch. */
   struct gl_material Material[VB_SIZE][2]; /* New material values */
};



/* Vertex buffer clipping flags */
#define CLIP_RIGHT_BIT   0x01
#define CLIP_LEFT_BIT    0x02
#define CLIP_TOP_BIT     0x04
#define CLIP_BOTTOM_BIT  0x08
#define CLIP_NEAR_BIT    0x10
#define CLIP_FAR_BIT     0x20
#define CLIP_USER_BIT    0x40
#define CLIP_ALL_BITS    0x3f

#define CLIP_ALL   1
#define CLIP_NONE  2
#define CLIP_SOME  3


extern struct vertex_buffer *gl_alloc_vb(void);


#endif
