/*******************************************************************
 *
 *  tterror.h
 *
 *    Error number declaration and handling (specification).
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
 ******************************************************************/

/* $XFree86: $ */
  
#ifndef TTERROR_H
#define TTERROR_H

#include "ttcommon.h"

/***************************************************************************/
/*                                                                         */
/*  various tracing macros                                                 */
/*                                                                         */
/***************************************************************************/

#ifdef CHECK_ASSERTIONS

#define TT_Assert( condition, action )  if ( !(condition) ) ( action )

#else

#define TT_Assert( condition, action )  /* void, no check */

#endif /* CHECK_ASSERTIONS */


/* you must say                                                            */
/*                                                                         */
/*  DebugTrace(( fmt, arg1, arg2, ... ))                                   */
/*                                                                         */
/* in the code (note the double parentheses).                              */

#ifdef DEBUG

#define DebugTrace( x )  Message##x

#else

#define DebugTrace( x )  do { } while ( 0 )

#endif /* DEBUG */


#ifdef __cplusplus
  extern "C" {
#endif

  void  Message( const char*  fmt, ... );
  void  Panic( const char*  fmt, ... );   /* print a message and exit */

#ifdef __cplusplus
  }
#endif

#endif /* TTERROR_H */


/* END */
