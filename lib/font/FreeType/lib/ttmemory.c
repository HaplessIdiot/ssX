/*******************************************************************
 *
 *  ttmemory.c                                               1.2
 *
 *    Memory management component (body).
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
 *
 *  Changes between 1.1 and 1.2:
 *
 *  - the font pool is gone.
 *
 *  - introduced the FREE macro and the Free function for
 *    future use in destructors.
 *
 *  - Init_FontPool() is now a macro to allow the compilation of
 *    'legacy' applications (all four test programs have been updated).
 *
 ******************************************************************/

/* $XFree86: xc/lib/font/FreeType/lib/ttmemory.c,v 1.2 1998/04/28 13:30:59 robin Exp $ */
  
#include "tterror.h"
#include "ttmemory.h"
#include "ttengine.h"
#include <stdlib.h>

#ifdef DEBUG_MEMORY

#include <stdio.h>

#ifdef XFree86LOADER
#include <xf86_libc.h>
#endif

  #define MAX_TRACKED_BLOCKS  1024

  struct  _TMemRec
  {
    void*  base;
    long   size;
  };

  typedef struct _TMemRec  TMemRec;

  static TMemRec  pointers[MAX_TRACKED_BLOCKS + 1];

  static int  num_alloc;
  static int  num_free;
  static int  fail_alloc;
  static int  fail_free;

#endif /* DEBUG_MEMORY */


#ifndef TT_CONFIG_REENTRANT
  long  TTMemory_Allocated;
  long  TTMemory_MaxAllocated;
#endif


/*******************************************************************
 *
 *  Function    :  TT_Alloc
 *
 *  Description :  Allocates memory from the heap buffer.
 *
 *  Input  :  Size      size of the memory to be allocated
 *            P         pointer to a buffer pointer
 *
 *  Output :  Error code.  
 *
 *  NOTE:  The newly allocated block should _always_ be zeroed
 *         on return.  Many parts of the engine rely on this to
 *         work properly.
 *
 ******************************************************************/

  TT_Error  TT_Alloc( long  Size, void**  P ) 
  {
#ifdef DEBUG_MEMORY
    Int  i;
#endif


    if ( Size )
    {
      *P = malloc( Size );
      if ( !*P )
        return TT_Err_Out_Of_Memory;

#ifndef TT_CONFIG_REENTRANT
      TTMemory_Allocated    += Size;
      TTMemory_MaxAllocated += Size;
#endif

#ifdef DEBUG_MEMORY

      num_alloc++;

      i = 0;
      while ( i < MAX_TRACKED_BLOCKS && pointers[i].base != NULL )
        i++;

      if ( i >= MAX_TRACKED_BLOCKS )
        fail_alloc++;
      else
      {
        pointers[i].base = *P;
        pointers[i].size = Size;
      }
  
#endif /* DEBUG_MEMORY */

      MEM_Set( *P, 0, Size );
    }
    else
      *P = NULL;

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  TT_Free
 *
 *  Description :  Releases a previously allocated block of memory.
 *
 *  Input  :  P    pointer to memory block
 *
 *  Output :  Always SUCCESS.
 *
 *  Note : The pointer must _always_ be set to NULL by this function.
 *
 ******************************************************************/

  TT_Error  TT_Free( void**  P )
  {
#ifdef DEBUG_MEMORY
    Int  i;
#endif /* DEBUG_MEMORY */


    if ( !P || !*P )
      return TT_Err_Ok;

#ifdef DEBUG_MEMORY

    num_free++;

    i = 0;
    while ( i < MAX_TRACKED_BLOCKS && pointers[i].base != *P )
      i++;

    if ( i >= MAX_TRACKED_BLOCKS )
      fail_free++;
    else
    {
#ifndef TT_CONFIG_REENTRANT
      TTMemory_Allocated -= pointers[i].size;
#endif

      pointers[i].base = NULL;
      pointers[i].size = 0;
    }
#endif /* DEBUG_MEMORY */

    free( *P );

    *P = NULL;

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  TTMemory_Init
 *
 *  Description :  Initializes the memory.
 *
 *  Output :  Always SUCCESS.
 *
 ******************************************************************/

  TT_Error  TTMemory_Init()
  {

#ifdef DEBUG_MEMORY
    Int i;
    for ( i = 0; i < MAX_TRACKED_BLOCKS; i++ )
    {
      pointers[i].base = NULL;
      pointers[i].size = 0;
    }

    num_alloc = 0;
    num_free  = 0;

    fail_alloc = 0;
    fail_free  = 0;
#endif


#ifndef TT_CONFIG_REENTRANT
    TTMemory_Allocated    = 0;
    TTMemory_MaxAllocated = 0;
#endif

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  TTMemory_Done
 *
 *  Description :  Finalizes memory usage.
 *
 *  Output :  Always SUCCESS.
 *
 ******************************************************************/

  TT_Error  TTMemory_Done()
  {

#ifdef DEBUG_MEMORY
    Int  i, num_leaked, tot_leaked;


    num_leaked = 0;
    tot_leaked = 0;

    for ( i = 0; i < MAX_TRACKED_BLOCKS; i++ )
    {
      if ( pointers[i].base )
      {
        num_leaked ++;
        tot_leaked += pointers[i].size;
      }
    }

    fprintf( stderr, 
             "%d memory allocations, of which %d failed\n",
             num_alloc,
             fail_alloc );

    fprintf( stderr,
             "%d memory frees, of which %d failed\n",
             num_free,
             fail_free );
               
    if ( num_leaked > 0 )
    {
      fprintf( stderr, 
               "There are %d leaked memory blocks, totalizing %d bytes\n",
               num_leaked, tot_leaked );

      for ( i = 0; i < MAX_TRACKED_BLOCKS; i++ )
      {
        if ( pointers[i].base )
        {
          fprintf( stderr, 
                   "index: %4d (base: $%08lx, size: %08ld)\n",
                   i,
                   (long)pointers[i].base,
                   pointers[i].size );
        }
      }
    }
    else
      fprintf( stderr, "No memory leaks !\n" );

#endif /* DEBUG_MEMORY */

    return TT_Err_Ok;
  }


/* END */
