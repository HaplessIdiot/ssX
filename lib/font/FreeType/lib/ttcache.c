/*******************************************************************
 *
 *  ttcache.c                                                   1.1
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
 * Changes between 1.1 and 1.0:
 *
 *  - introduced the refresher and finalizer in the cache class
 *    definition/implementation.
 *
 ******************************************************************/

#include "ttengine.h"
#include "ttcache.h"
#include "ttmemory.h"
#include "ttobjs.h"


/*******************************************************************
 *
 *  Function    :  Cache_Create
 *
 *  Description :  Creates a new cache that will be used to list
 *                 and recycle several objects of the same class.
 *
 *  Input  :  clazz       a pointer to the cache's class.  This is
 *                        a simple structure that describes the
 *                        the cache's object types and recycling
 *                        limits.
 *
 *            cache       address of cache to create
 *
 *            lock        address of the mutex to use for this
 *                        cache.  The mutex will be used to protect
 *                        the cache's lists.  Use NULL for unprotected
 *                        cache.
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Cache_Create( PEngine_Instance  engine,
                          PCache_Class      clazz,
                          TCache*           cache,
                          TMutex*           lock )
  {
    MUTEX_Create( cache->lock );

    cache->engine     = engine;
    cache->clazz      = clazz;
    cache->lock       = lock;
    cache->idle_count = 0;

    ZERO_List( cache->active );
    ZERO_List( cache->idle );

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Cache_Destroy
 *
 *  Description :  Destroys a cache and all its idle and active
 *                 objects.  This will call each object's destructor
 *                 before freeing it.
 *
 *  Input  :  cache   address of cache to destroy
 *
 *  Output :  error code.
 *
 ******************************************************************/

  TT_Error  Cache_Destroy( TCache*  cache )
  {
    PDestructor    destroy;
    PList_Element  current;


    /* now destroy all active and idle listed objects */

    destroy = cache->clazz->done;

    /* active list */

    current = List_Extract( &cache->active );
    while ( current )
    {
      destroy( current->data );
      FREE( current->data );

      Element_Done( cache->engine, current );

      current = List_Extract( &cache->active );
    }

    /* idle list */

    current = List_Extract( &cache->idle );
    while ( current )
    {
      destroy( current->data );
      FREE( current->data );

      Element_Done( cache->engine, current );

      current = List_Extract( &cache->idle );
    }

    cache->clazz      = NULL;
    cache->idle_count = 0;

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Cache_New
 *
 *  Description :  Extracts a new object from a cache.  This will
 *                 try to recycle an idle object, if any is found.
 *                 Otherwise, a new object will be allocated and
 *                 built (by calling its constructor).
 *
 *  Input  :   cache          address of cache to use
 *             new_object     address of target pointer to the 'new'
 *                            object
 *             parent_object  this pointer is passed to a new object
 *                            constructor (unused if object is
 *                            recycled)
 *
 *  Output :   error code.
 *
 ******************************************************************/

  TT_Error  Cache_New( TCache*  cache,
                       void**   new_object,
                       void*    parent_object )
  {
    TT_Error       error;
    PList_Element  current;
    PConstructor   build;
    PRefresher     reset;
    void*          object;


    MUTEX_Lock( *cache->lock );

    current = List_Extract( &cache->idle );
    if ( current )
    {
      object = current->data;
      reset  = cache->clazz->reset;
      if ( reset )
      {
        error = reset( object, parent_object );
        if ( error )
        {
          List_Add( &cache->idle, current );
          goto Exit;
        }
      }
      cache->idle_count--;
    }
    else
    {
      /* if no object was found in the cache, create a new one */

      build  = cache->clazz->init;

      if ( MEM_Alloc( object, cache->clazz->object_size ) )
        goto Memory_Fail;

      current = Element_New( cache->engine );
      if ( !current )
        goto Memory_Fail;

      current->data = object;

      error = build( object, parent_object );
      if ( error )
      {
        Element_Done( cache->engine, current );
        goto Fail;
      }
    }

    List_Add( &cache->active, current );
    *new_object = current->data;

    error = TT_Err_Ok;

  Exit:
    MUTEX_Release( *cache->lock );
    return error;

  Memory_Fail:
    error = TT_Err_Out_Of_Memory;

  Fail:
    FREE( object );
    goto Exit;
  }


/*******************************************************************
 *
 *  Function    :  Cache_Done
 *
 *  Description :  Releases an object to the cache.  This will either
 *                 recycle or destroy the object, based on the cache's
 *                 class and state.
 *
 *  Input  :  cache   the cache to use
 *            data    the object to recycle/discard
 *
 *  Output :  error code.
 *
 *  Notes  :  The object's destructor is called only when
 *            the objectwill be effectively destroyed by this
 *            function.  This will not happen during recycling.
 *
 ******************************************************************/

  TT_Error  Cache_Done( TCache*  cache, void*  data )
  {
    TT_Error       error;
    PList_Element  element;
    PFinaliser     finalise;
    int            limit;


    MUTEX_Lock( *cache->lock );

    element = List_Find( &cache->active, data );
    if ( !element )
    {
      error = TT_Err_Unlisted_Object;
      goto Exit;
    }

    List_Remove( &cache->active, element );

    limit = cache->clazz->idle_limit;
    if ( cache->idle_count >= limit )
    {
      /* destroy the object when the cache is full */

      cache->clazz->done( element->data );
      FREE( element->data );

      Element_Done( cache->engine, element );
    }
    else
    {
      /* Finalise the object before adding it to the   */
      /* idle list.  Return the error if any is found. */

      finalise = cache->clazz->finalise;
      if ( finalise )
      {
        error = finalise( element->data );
        if ( error )
          goto Exit;

        /* Note: a failure at finalise time is a severe bug in     */
        /*       the engine, which is why we allow ourselves to    */
        /*       lose the object in this case.  A finaliser should */
        /*       have its own error codes to spot this kind of     */
        /*       problems easily.                                  */
      }

      List_Add( &cache->idle, element );
      cache->idle_count++;
    }

    error = TT_Err_Ok;

  Exit:
    MUTEX_Release( *cache_lock );
    return error;
  }


/* END */
