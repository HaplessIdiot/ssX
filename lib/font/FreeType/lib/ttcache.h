/*******************************************************************
 *
 *  ttcache.h                                                   1.1
 *
 *    Generic object cache     
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
 *  This component defines and implement object caches.
 *
 *  An object class is a structure layout that encapsulate one
 *  given type of data used by the FreeType engine.  Each object
 *  class is completely described by:
 *
 *    - a 'root' or 'leading' structure containing the first
 *      important fields of the class.  The root structure is
 *      always of fixed size.
 *
 *      It is implemented as a simple C structure, and may
 *      contain several pointers to sub-tables that can be
 *      sized and allocated dynamically.
 *
 *      examples:  TFace, TInstance, TGlyph & TExecution_Context
 *                 (defined in 'ttobjs.h')
 *
 *    - we make a difference between 'child' pointers and 'peer'
 *      pointers.  A 'child' pointer points to a sub-table that is
 *      owned by the object, while a 'peer' pointer points to any
 *      other kind of data the object isn't responsible for.
 *
 *      An object class is thus usually a 'tree' of 'child' tables.
 *
 *    - each object class needs a constructor and a destructor.
 *
 *      A constructor is a function which receives the address of
 *      freshly allocated and zeroed object root structure and
 *      'builds' all the valid child data that must be associated
 *      to the object before it becomes 'valid'.
 *
 *      A destructor does the inverse job: given the address of
 *      a valid object, it must discard all its child data and
 *      zero its main fields (essentially the pointers and array
 *      sizes found in the root fields).
 *
 * Changes between 1.1 and 1.0:
 *
 *  - introduced the refreshed and finalizer class definition/implementation
 *  - inserted an engine instance pointer in the cache structure
 *
 ******************************************************************/

/* $XFree86: $ */
  
#ifndef TTCACHE_H
#define TTCACHE_H

#include "ttmutex.h"
#include "tterror.h"
#include "ttlists.h"

#ifdef __cplusplus
  extern "C" {
#endif

  typedef TT_Error  TConstructor( void*  object,
                                  void*  parent );

  typedef TT_Error  TDestructor ( void*  object );

  typedef TConstructor  TRefresher;
  typedef TDestructor   TFinaliser;

  typedef TConstructor*  PConstructor;
  typedef TDestructor*   PDestructor;
  typedef TRefresher*    PRefresher;
  typedef TFinaliser*    PFinaliser;


  struct  _TCache_Class
  {
    int           object_size;
    int           idle_limit;
    PConstructor  init;
    PDestructor   done;
    PRefresher    reset;
    PFinaliser    finalise;
  };

  typedef struct _TCache_Class  TCache_Class;
  typedef TCache_Class          *PCache_Class;


  /* A Cache class record holds the data necessary to define */
  /* a cache kind.                                           */

  struct  _TCache
  {
    PEngine_Instance  engine;
    PCache_Class      clazz;      /* 'class' is a reserved word in C++ */
    TMutex*           lock;
    TSingle_List      active;
    TSingle_List      idle;
    int               idle_count;
  };

  typedef struct _TCache  TCache;
  typedef TCache          *PCache;


  /* An object cache holds two lists tracking the active and */
  /* idle objects that are currently created and used by the */
  /* engine.  It can also be 'protected' by a mutex.         */

  /* Initializes a new cache, of class 'clazz', pointed by 'cache', */
  /* protected by the 'lock' mutex. Set 'lock' to NULL if the cache */
  /* doesn't need protection                                        */

  TT_Error  Cache_Create( PEngine_Instance  engine,
                          PCache_Class      clazz, 
                          TCache*           cache,
                          TMutex*           lock );


  /* Destroys a cache and all its listed objects */

  TT_Error  Cache_Destroy( TCache*  cache );


  /* Extracts a new object from the cache */

  TT_Error Cache_New( TCache*  cache,
                      void**   new_object,
                      void*    parent_object );


  /* Returns an object to the cache, or discards it depending */
  /* on the cache class' 'idle_limit' field                   */

  TT_Error  Cache_Done( TCache*  cache, void*  data );

#define CACHE_New( _cache, _newobj, _parent ) \
          Cache_New( (TCache*)_cache, (void**)&_newobj, (void*)_parent )

#define CACHE_Done( _cache, _obj ) \
          Cache_Done( (TCache*)_cache, (void*)_obj )

#ifdef __cplusplus
  }
#endif

#endif /* TTCACHE_H */


/* END */
