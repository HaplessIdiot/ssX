/*******************************************************************
 *
 *  ttmemory.h                                               1.2
 *
 *    Memory management component (specification).
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
 *  Changes between 1.2 and 1.1:
 *
 *  - the font pool is gone!  All allocations are now performed
 *    with malloc() and free().
 *
 *  - introduced the FREE() macro and the Free() function for
 *    future use in destructors.
 *
 *  - Init_FontPool() is now a macro to allow the compilation of
 *    'legacy' applications (all four test programs have been updated).
 *
 ******************************************************************/

/* $XFree86: $ */
  
#ifndef TTMEMORY_H
#define TTMEMORY_H

#include "tttypes.h"
#include "string.h"


#ifdef __cplusplus
  extern "C" {
#endif

#define MEM_Set( dest, byte, count )  memset( dest, byte, count )

#ifdef HAVE_MEMCPY
#define MEM_Copy( dest, source, count )  memcpy( dest, source, count )
#else
#define MEM_Copy( dest, source, count )  bcopy( source, dest, count )
#endif

#define MEM_Alloc( _pointer_, _size_ ) \
          TT_Alloc( _size_, (void**)&(_pointer_) )
#define ALLOC( _pointer_, _size_ ) \
          ( error = MEM_Alloc( _pointer_, _size_ ) )
#define ALLOC_ARRAY( _pointer_, _count_, _type_ ) \
          ( error = MEM_Alloc( _pointer_, (_count_)*sizeof(_type_) ) )
#define FREE( _pointer_ )  TT_Free( (void**)&(_pointer_) )

  
  /* Allocate a block of memory of 'Size' bytes from the heap, and */
  /* sets the pointer '*P' to its address.  If 'Size' is 0, or in  */
  /* case of error, the pointer is always set to NULL.             */

  TT_Error  TT_Alloc( long  Size, void**  P );


  /* Releases a block that was previously allocated through Alloc. */
  /* Note that the function returns successfully when P or *P are  */
  /* already NULL.  The pointer '*P' is set to NULL on exit in     */
  /* case of success.                                              */

  TT_Error  TT_Free( void**  P );


  /* For "legacy" applications, that should be re-coded.              */
  /* Note that this won't release the previously allocated font pool. */

#define Init_FontPool( x, y )  while( 0 ) { }
  

  TT_Error  TTMemory_Init();
  TT_Error  TTMemory_Done();


#ifdef __cplusplus
  }
#endif

#endif /* TTMEMORY_H */


/* END */
