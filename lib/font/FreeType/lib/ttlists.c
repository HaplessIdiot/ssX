/*******************************************************************
 *
 *  ttlists.c                                                   1.0
 *
 *    Generic lists routines.
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
 *  IMPORTANT NOTE:
 *
 *    These routines should only be used within managers.  As a
 *    consequence, they do not provide support for thread-safety
 *    or re-entrancy.
 *
 ******************************************************************/

#include "ttengine.h"
#include "ttlists.h"
#include "tterror.h"
#include "ttmutex.h"
#include "ttmemory.h"

/* The macro FREE_Elements aliases the current engine instance's */
/* free list_elements recycle list.                              */
#define FREE_Elements  (engine->list_free_elements)
  
#define LOCK()    MUTEX_Lock   ( engine->lock )
#define UNLOCK()  MUTEX_Release( engine->lock )



/*******************************************************************
 *
 *  Function    :  Element_New
 *
 *  Description :  Gets a new (either fresh or recycled) list
 *                 element.  The element is unlisted.
 *
 *  Input  :  None
 *
 *  Output :  List element address.  NULL if out of memory.
 *
 ******************************************************************/

  PList_Element  Element_New( PEngine_Instance  engine )
  {
    PList_Element  element;


    LOCK();

    if ( FREE_Elements )
    {
      element       = (PList_Element)FREE_Elements;
      FREE_Elements = element->next;
    }
    else
    {
      if ( !MEM_Alloc( element, sizeof ( TList_Element ) ) )
      {
        element->next = NULL;
        element->data = NULL;
      }
    }

    /* Note: in case of failure, Alloc sets the pointer to NULL */

    UNLOCK();

    return element;
  }


/*******************************************************************
 *
 *  Function    :  Element_Done
 *
 *  Description :  Recycles an unlinked list element.
 *
 *  Input  :  The list element to recycle.  It _must_ be unlisted.
 *
 *  Output :  none.
 *
 *  Note   :  This function doesn't check the element.
 *
 ******************************************************************/

  void  Element_Done( PEngine_Instance  engine,
                      PList_Element     element )
  {
    LOCK();

    /* Simply add the list element to the recycle list */

    element->next = (PList_Element)FREE_Elements;
    FREE_Elements = element;

    UNLOCK();
  }


/*******************************************************************
 *
 *  Function    :  List_Add
 *
 *  Description :  Adds a new list element at the tail of a
 *                 given list.
 *
 *  Input  :  list     the list
 *            element  the element to add
 *
 ******************************************************************/

  void  List_Add( TSingle_List*  list,
                  PList_Element  element )
  {
    TT_Assert( element, Panic( "TTLists.List_Add: void element" ) );

    element->next = NULL;

    if ( !list->head )
    {
      TT_Assert( list->tail,
                 Panic( "TTLists.List_Add: incoherent list tail" ) );
      list->head = element;
      list->tail = element;
    }
    else
    {
      TT_Assert( !list->tail,
                 Panic( "TTLists.List_Add: incoherent list head" ) );
      list->tail->next = element;
      list->tail       = element;
    }
  }


/*******************************************************************
 *
 *  Function    :  List_Remove
 *
 *  Description :  Removes an element from a given list.  The
 *                 element must be part of the list.
 *
 *  Input  :  list     the list
 *            element  the element to remove
 *
 *  Output :  Returns TRUE on success.  FALSE on failure (when
 *            the element wasn't part of the list).
 *
 ******************************************************************/

  Bool  List_Remove( TSingle_List*  list,
                     PList_Element  element )
  {
    PList_Element  old, current;


    TT_Assert( list && list->head && list->tail,
               Panic( "TTLists.List_Remove: void or incoherent list" ) );

    old     = NULL;
    current = list->head;

    while ( current )
    {
      if ( current == element )
      {
        if ( old ) 
          old->next = current->next;
        else
          list->head = current->next;

        if ( list->tail == current )
          list->tail = old;

        return SUCCESS;
      }

      old     = current;
      current = current->next;
    }

    return FAILURE;
  }


/*******************************************************************
 *
 *  Function    :  List_Find
 *
 *  Description :  Finds the first list element that matches
 *                 the 'data' argument in a given list.
 *
 *  Input  :  list     the list
 *            data     the data field to match
 *
 *  Output :  The found list element.  NULL if none.              
 *
 ******************************************************************/

  PList_Element  List_Find( TSingle_List*  list,
                            void*          data )
  {
    PList_Element  current;


    TT_Assert( list, Panic( "TTLists.List_Find: invalid list" ) );

    current = list->head;

    while ( current )
    {
      if ( current->data==data )
        return current;

      current = current->next;
    }

    return NULL;
  }


/*******************************************************************
 *
 *  Function    :  List_Extract
 *
 *  Description :  Extracts the first list element of a given
 *                 list.  This is useful for recycling lists.
 *
 *  Input  :  list     the list
 *
 *  Output :  The list element.  Returns NULL if the list was  
 *            empty.
 *
 ******************************************************************/

  PList_Element  List_Extract( TSingle_List*  list )
  {
    PList_Element  element;


    TT_Assert( list, Panic( "TTLists.List_Extract: void list argument" ) );

    element = list->head;

    if ( element )
    {
      list->head = element->next;

      if ( list->tail==element )
        list->tail = NULL;
    }

    return element;
  }


/*******************************************************************
 *
 *  Function    :  TTLists_Init
 *
 *  Description :  The component's initializer.  Creates the  
 *                 list mutex and initializes the free list_elements
 *                 list.
 *
 *  Input  :  engine instance
 *
 ******************************************************************/

  TT_Error  TTLists_Init( PEngine_Instance  engine )
  {
    /* init the free list_elements list */

    FREE_Elements = NULL;
    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  TTLists_Done
 *
 *  Description :  The component's finalizer.  Releases the   
 *                 recycled list elements and destroys the mutex.
 *
 *  Input  :  engine instance (for re-entrant builds only)
 *
 ******************************************************************/

  TT_Error  TTLists_Done( PEngine_Instance  engine )
  {
    PList_Element  element, next;


    /* frees the recycled list elements */

    element = FREE_Elements;
    while ( element )
    {
      next = element->next;
      FREE( element );
      element = next;
    }

    return TT_Err_Ok;
  }


/* END */
