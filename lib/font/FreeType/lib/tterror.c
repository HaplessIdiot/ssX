/*******************************************************************
 *
 *  tterror.c
 *
 *    Error number declaration and handling (body).
 *
 *  Copyright 1996-1998 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT.  By continuing to use, modify, or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 *  NOTE:
 *
 *    This component's body only contains the global variable Error
 *    which will most probably move to the Font Pool in the final
 *    release.
 *
 ******************************************************************/

/* $XFree86: xc/lib/font/FreeType/lib/tterror.c,v 1.2 1998/04/28 13:30:55 robin Exp $ */
  
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ttconfig.h"
#include "tterror.h"

#ifdef XFree86LOADER
#include <xf86_libc.h>
#endif

  /* the Print() function is defined in ttconfig.h */

  void  Message( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    Print( fmt, ap );
    va_end( ap );
  }


  void  Panic( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    Print( fmt, ap );
    va_end( ap );

    exit( 1 );
  }


/* END */
