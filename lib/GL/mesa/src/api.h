/* Id: api.h,v 1.2 1999/02/26 08:52:30 martin Exp $ */

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
 * Log: api.h,v $
 * Revision 1.2  1999/02/26 08:52:30  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.5  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.4  1999/01/03 03:26:06  brianp
 * removed some Windows cruft (Ted Jump)
 *
 * Revision 3.3  1998/02/20 04:48:57  brianp
 * updated comments
 *
 * Revision 3.2  1998/02/04 00:44:02  brianp
 * WIN32 patch from Oleg Letsinsky
 *
 * Revision 3.1  1998/02/04 00:14:18  brianp
 * updated for Cygnus (Stephane Rehel)
 *
 * Revision 3.0  1998/01/31 20:41:55  brianp
 * initial rev
 *
 */


/*
 * This header contains stuff only included by api1.c, api2.c and apiext.c
 */


#ifndef API_H
#define API_H


/*
 * Single/multiple thread context selection.
 */
#ifdef THREADS

/* Get the context associated with the calling thread */
#define GET_CONTEXT	GLcontext *CC = gl_get_thread_context()

#else

/* CC is a global pointer for all threads in the address space */
#define GET_CONTEXT

#endif /* THREADS */


/*
 * Make sure there's a rendering context.
 */
#define CHECK_CONTEXT							\
   if (!CC) {								\
      if (getenv("MESA_DEBUG")) {					\
	 fprintf(stderr,"Mesa user error: no rendering context.\n");	\
      }									\
      return;								\
   }

#define CHECK_CONTEXT_RETURN(R)						\
   if (!CC) {								\
      if (getenv("MESA_DEBUG")) {					\
         fprintf(stderr,"Mesa user error: no rendering context.\n");	\
      }									\
      return (R);							\
   }


/*
 * An optimization in a few performance-critical functions.
 */
#define SHORTCUT


#endif
