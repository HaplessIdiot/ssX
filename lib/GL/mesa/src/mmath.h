/* Id: mmath.h,v 1.2 1999/02/26 08:52:36 martin Exp $ */

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
 * Log: mmath.h,v $
 * Revision 1.2  1999/02/26 08:52:36  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.4  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.3  1998/10/23 00:26:55  brianp
 * added FloatToInt macro for Win32 (Eero Pajarre)
 *
 * Revision 3.2  1998/10/23 00:21:55  brianp
 * patched with Keith Whitwell's changes
 *
 * Revision 3.1  1998/04/18 05:00:14  brianp
 * added FLOAT_COLOR_TO_UBYTE_COLOR macro
 *
 * Revision 3.0  1998/01/31 20:59:27  brianp
 * initial rev
 *
 */


/*
 * Faster arithmetic functions.  If the FAST_MATH preprocessor symbol is
 * defined on the command line (-DFAST_MATH) then we'll use some (hopefully)
 * faster functions for sqrt(), etc.
 */


#ifndef MMATH_H
#define MMATH_H


#include <math.h>



/*
 * Set the x86 FPU control word to less precision for more speed:
 */
#if defined(__linux__) && defined(__i386__) && defined(FAST_MATH)
#include <i386/fpu_control.h>
#define START_FAST_MATH  __setfpucw(_FPU_SINGLE | _FPU_MASK_IM | _FPU_MASK_DM \
            | _FPU_MASK_ZM | _FPU_MASK_OM | _FPU_MASK_UM | _FPU_MASK_PM);
#define END_FAST_MATH  __setfpucw(_FPU_EXTENDED | _FPU_MASK_IM | _FPU_MASK_DM \
            | _FPU_MASK_ZM | _FPU_MASK_OM | _FPU_MASK_UM | _FPU_MASK_PM);
#else
#define START_FAST_MATH
#define END_FAST_MATH
#endif



/*
 * Float -> Int conversion
 */

#if defined(USE_X86_ASM)
static __inline__ int FloatToInt(float f)
{
   int r;
   __asm__ ("fistpl %0" : "=m" (r) : "t" (f) : "st");
   return r;
}
#elif  defined(__MSC__) && defined(__WIN32__)
static __inline int FloatToInt(float f)
{
   int r;
   _asm {
     fld f
     fistp r
    }
   return r;
}
#else
#define FloatToInt(F) ((int) (F))
#endif



/*
 * Square root
 */

extern float gl_sqrt(float x);

#ifdef FAST_MATH
#  define GL_SQRT(X)  gl_sqrt(X)
#else
#  define GL_SQRT(X)  sqrt(X)
#endif



/*
 * Normalize a 3-element vector to unit length.
 */
#define NORMALIZE_3FV( V )			\
do {						\
   GLdouble len = LEN_SQUARED_3FV(V);		\
   if (len > 1e-50) {				\
      len = 1.0 / GL_SQRT(len);			\
      V[0] *= len;				\
      V[1] *= len;				\
      V[2] *= len;				\
   }						\
} while(0)

#define LEN_3FV( V ) (GL_SQRT(V[0]*V[0]+V[1]*V[1]+V[2]*V[2]))

#define LEN_SQUARED_3FV( V ) (V[0]*V[0]+V[1]*V[1]+V[2]*V[2])

/*
 * Optimization for:
 * GLfloat f;
 * GLubyte b = FloatToInt(CLAMP(f, 0, 1) * 255)
 */

#if defined(__i386__)
#define USE_IEEE
#endif

#if defined(USE_IEEE) && !defined(DEBUG)

#define IEEE_ONE 0x3f7f0000
#define FLOAT_COLOR_TO_UBYTE_COLOR(b, f)			\
	{							\
	   GLfloat tmp = f + 32768.0F;				\
	   b = ((*(GLuint *)&f >= IEEE_ONE)			\
	       ? (*(GLint *)&f < 0) ? (GLubyte)0 : (GLubyte)255	\
	       : (GLubyte)*(GLuint *)&tmp);			\
	}

#else

#define FLOAT_COLOR_TO_UBYTE_COLOR(b, f)			\
	b = FloatToInt(CLAMP(f, 0.0F, 1.0F) * 255.0F)

#endif



extern void gl_init_math(void);


#endif
