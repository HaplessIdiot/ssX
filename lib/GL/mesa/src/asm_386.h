/* Id: asm_386.h,v 1.2 1999/02/26 08:52:30 martin Exp $ */

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
 * Log: asm_386.h,v $
 * Revision 1.2  1999/02/26 08:52:30  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.3  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.2  1998/10/31 17:07:09  brianp
 * Josh's patches for 3.1
 *
 * Revision 3.1  1998/03/28 04:05:52  brianp
 * added CONST macro
 *
 * Revision 3.0  1998/01/31 20:45:56  brianp
 * initial rev
 *
 */


#ifndef ASM_386_H
#define ASM_386_H


#include "GL/gl.h"
#include "macros.h"
#include "types.h"

/*
 * Prototypes for assembly functions.
 */


extern void asm_transform_points3_general( CONST GLmatrix *matrix,
                                           GLuint count,
                                           CONST GLfloat *source,
                                           GLfloat dest[][4],
                                           GLuint stride );

extern void asm_transform_points3_identity( CONST GLmatrix *matrix,
                                            GLuint count,
                                            CONST GLfloat *source,
                                            GLfloat dest[][4],
                                            GLuint stride );

extern void asm_transform_points3_2d( CONST GLmatrix *matrix,
                                      GLuint count,
                                      CONST GLfloat *source,
                                      GLfloat dest[][4],
                                      GLuint stride );

extern void asm_transform_points3_2d_no_rot( CONST GLmatrix *matrix,
                                             GLuint count,
                                             CONST GLfloat *source,
                                             GLfloat dest[][4],
                                             GLuint stride );

extern void asm_transform_points3_3d( CONST GLmatrix *matrix,
                                      GLuint count,
                                      CONST GLfloat *source,
                                      GLfloat dest[][4],
                                      GLuint stride );

extern void asm_transform_points4_general( CONST GLmatrix *matrix,
                                           GLuint count,
                                           CONST GLfloat *source,
                                           GLfloat dest[][4],
                                           GLuint stride );

extern void asm_transform_points4_identity( CONST GLmatrix *matrix,
                                            GLuint count,
                                            CONST GLfloat *source,
                                            GLfloat dest[][4],
                                            GLuint stride );

extern void asm_transform_points4_2d( CONST GLmatrix *matrix,
                                      GLuint count,
                                      CONST GLfloat *source,
                                      GLfloat dest[][4],
                                      GLuint stride );

extern void asm_transform_points4_2d_no_rot( CONST GLmatrix *matrix,
                                             GLuint count,
                                             CONST GLfloat *source,
                                             GLfloat dest[][4],
                                             GLuint stride );

extern void asm_transform_points4_3d( CONST GLmatrix *matrix,
                                      GLuint count,
                                      CONST GLfloat *source,
                                      GLfloat dest[][4],
                                      GLuint stride );

extern void asm_transform_points4_3d_no_rot( CONST GLmatrix *matrix,
                                             GLuint count,
                                             CONST GLfloat *source,
                                             GLfloat dest[][4],
                                             GLuint stride );

extern void asm_transform_points4_perspective( CONST GLmatrix *matrix,
                                               GLuint count,
                                               CONST GLfloat *source,
                                               GLfloat dest[][4],
                                               GLuint stride );

extern void asm_cliptest_points4( GLuint n,
                                  CONST GLfloat vClip[][4],
                                  GLubyte clipMask[],
                                  GLubyte *orMask,
                                  GLubyte *andMask );
#endif
