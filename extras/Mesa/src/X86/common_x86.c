
/*
 * Mesa 3-D graphics library
 * Version:  3.4
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 * Check CPU capabilities & initialize optimized funtions for this particular
 * processor.
 *
 * Written by Holger Waechtler <holger@akaflieg.extern.tu-berlin.de>
 * Changed by Andre Werthmann <wertmann@cs.uni-potsdam.de> for using the
 * new Katmai functions.
 */

#include <stdlib.h>
#include <stdio.h>

#include "common_x86_asm.h"


int gl_x86_cpu_features = 0;

/* No reason for this to be public.
 */
extern int gl_identify_x86_cpu_features( void );


static void message( const char *msg )
{
   if ( getenv( "MESA_DEBUG" ) ) {
      fprintf( stderr, "%s\n", msg );
   }
}

#if defined(USE_KATMAI_ASM) && defined(__linux__)
/*
 * GH: This is a hack.  Try to determine at runtime what version of the
 * Linux kernel we're running on, and disable the SSE assembly if the
 * kernel is too old and doesn't correctly support the code.
 *
 * Magic versions tested for are:
 *
 *   - 2.2.18-pre19 and newer
 *   - 2.4.0-test11 and newer
 *
 * If we find a version older than this, or can't correctly determine
 * the version, disable the code to be safe.
 */
static void verify_linux_kernel_version( void )
{
   FILE *fd;
   int major, minor, teeny, extra;
   int use_sse = 0;
   int ret;

   fd = fopen( "/proc/sys/kernel/osrelease", "r" );
   if ( fd == NULL ) {
      /* If we can't test the kernel version, disable the code.
       */
      message( "couldn't open /proc/sys/kernel/osrelease, disabling SSE." );
      gl_x86_cpu_features &= ~(X86_FEATURE_XMM);
      return;
   }

   fscanf( fd, "%d.%d.%d", &major, &minor, &teeny );

   if ( major == 2 ) {
      if ( minor == 2 ) {
	 /* Test 2.2 kernel versions.  2.2.18-pre19 and newer are okay.
	  */
	 if ( teeny == 18 ) {
	    ret = fscanf( fd, "-pre%d", &extra );
	    switch ( ret ) {
	    case 1:
	       if ( extra >= 19 ) {
		  message( "kernel 2.2.18-pre19 or newer, enabling SSE." );
		  use_sse = 1;
	       } else {
		  message( "kernel older than 2.2.18-pre19, disabling SSE." );
	       }
	       break;
	    case 0:
	       message( "kernel 2.2.18 or newer, enabling SSE." );
	       use_sse = 1;
	       break;
	    }
	 } else if ( teeny > 18 ) {
	    message( "kernel 2.2.19 or newer, enabling SSE." );
	    use_sse = 1;
	 } else {
	    message( "kernel older than 2.2.18, disabling SSE." );
	 }

      } else if ( minor == 3 ) {
	 message( "kernel older than 2.4.0-test11, disabling SSE." );
      } else if ( minor == 4 ) {
	 /* Test 2.4 kernel versions.  2.4.0-test11 and newer are okay.
	  */
	 if ( teeny == 0 ) {
	    ret = fscanf( fd, "-test%d", &extra );
	    switch ( ret ) {
	    case 1:
	       if ( extra >= 11 ) {
		  message( "kernel 2.4.0-test11 or newer, enabling SSE." );
		  use_sse = 1;
	       } else {
		  message( "kernel older than 2.4.0-test11, disabling SSE." );
	       }
	       break;
	    case 0:
	       message( "kernel 2.4.0 or newer, enabling SSE." );
	       use_sse = 1;
	       break;
	    }
	 } else if ( teeny > 0 ) {
	    message( "kernel 2.4.1 or newer, enabling SSE." );
	    use_sse = 1;
	 }
      } else if ( minor >= 5 ) {
	 /* This test should be removed eventually, but cover ourselves
	  * just in case.
	  */
	 message( "kernel 2.5.0 or newer, enabling SSE." );
	 use_sse = 1;
      } else {
	 message( "kernel older than 2.2.18-pre19, disabling SSE." );
      }
   }

   fclose( fd );

   if ( !use_sse ) {
      gl_x86_cpu_features &= ~(X86_FEATURE_XMM);
   }
}
#endif

void gl_init_all_x86_transform_asm( void )
{
#ifdef USE_X86_ASM
   gl_x86_cpu_features = gl_identify_x86_cpu_features();

   if ( getenv( "MESA_NO_ASM" ) ) {
      gl_x86_cpu_features = 0;
   }

   if ( gl_x86_cpu_features ) {
      gl_init_x86_transform_asm();
   }

#ifdef USE_MMX_ASM
   if ( cpu_has_mmx ) {
      if ( getenv( "MESA_NO_MMX" ) == 0 ) {
         message( "MMX cpu detected." );
      } else {
         gl_x86_cpu_features &= ~(X86_FEATURE_MMX);
      }
   }
#endif

#ifdef USE_3DNOW_ASM
   if ( cpu_has_3dnow ) {
      if ( getenv( "MESA_NO_3DNOW" ) == 0 ) {
         message( "3Dnow cpu detected." );
         gl_init_3dnow_transform_asm();
      } else {
         gl_x86_cpu_features &= ~(X86_FEATURE_3DNOW);
      }
   }
#endif

#ifdef USE_KATMAI_ASM
#ifdef __linux__
   /* HACK: Check the version of the Linux kernel we're being run on,
    * and disable 'cpu_has_xmm' if user-space SSE is not supported.
    */
   verify_linux_kernel_version();
#endif
   if ( cpu_has_xmm ) {
      if ( getenv( "MESA_NO_KATMAI" ) == 0 ) {
         message( "Katmai cpu detected." );
         gl_init_katmai_transform_asm();
      } else {
         gl_x86_cpu_features &= ~(X86_FEATURE_XMM);
      }
   }
#endif
#endif
}

/* Note: the above function must be called before this one, so that
 * gl_x86_cpu_features gets correctly initialized.
 */
void gl_init_all_x86_shade_asm( void )
{
#ifdef USE_X86_ASM
   if ( gl_x86_cpu_features ) {
      /* Nothing here yet... */
   }

#ifdef USE_3DNOW_ASM
   if ( cpu_has_3dnow && getenv( "MESA_NO_3DNOW" ) == 0 ) {
      gl_init_3dnow_shade_asm();
   }
#endif
#endif
}

void gl_init_all_x86_vertex_asm( void )
{
#ifdef USE_X86_ASM
   if ( gl_x86_cpu_features ) {
      gl_init_x86_vertex_asm();
   }

#ifdef USE_3DNOW_ASM
   if ( cpu_has_3dnow && getenv( "MESA_NO_3DNOW" ) == 0 ) {
      gl_init_3dnow_vertex_asm();
   }
#endif

#ifdef USE_KATMAI_ASM
   if ( cpu_has_xmm && getenv( "MESA_NO_KATMAI" ) == 0 ) {
      gl_init_katmai_vertex_asm();
   }
#endif
#endif
}
