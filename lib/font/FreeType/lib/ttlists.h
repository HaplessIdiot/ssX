/*******************************************************************
 *
 *  ttlists.h                                                   1.0
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

/* $XFree86: $ */
  
#ifndef TTLISTS_H
#define TTLISTS_H

#include "ttcommon.h"
#include "tttypes.h"
#include "tterror.h"


#ifdef __cplusplus
  extern "C" {
#endif

  /*                                                                   */
  /* A 'generic' list is made of a series of linked nodes called       */
  /* 'List_Element's.  Each node contains a 'data' field that is a     */
  /* pointer to some listed object of any kind.  The nature of the     */
  /* objects of the list isn't important for now.                      */
  /*                                                                   */
  /*    ______      ______       ______                                */
  /*   |      |    |      |     |      |                               */
  /* ->| next----->| next------>| next----> NIL                        */
  /*   | data |    | data |     | data |                               */
  /*   |__|___|    |__|___|     |__|___|                               */
  /*      |           |            |                                   */
  /*      |           |            |                                   */
  /*      v           v            v                                   */
  /*  __________   ________    ________                                */
  /* |          | |        |  |        |                               */
  /* | Object A | | Obj. B |  | Obj. C |                               */
  /* |__________| |        |  |________|                               */
  /*              |________|                                           */
  /*                                                                   */
  /*                                                                   */
  /* The listed objects do not necessarily contain pointers to their   */
  /* own list element.                                                 */
  /*                                                                   */
  /* Note that the structure of TList_Element may change in the        */
  /* future.  For example, we may decide to include the address        */
  /* of a destructor for the listed object.  For this reason, only     */
  /* the code present in this component and in the central object      */
  /* manager should access directly the fields of TList_Element.       */
  /*                                                                   */
  /* IMPLEMENTATION NOTE:                                             */
  /*                                                                   */
  /* Discarded nodes are recycled in a simple internal list called     */
  /* 'Free_Elements'.                                                  */
  /*                                                                   */

  struct  _TList_Element;

  typedef struct _TList_Element  TList_Element;
  typedef TList_Element*         PList_Element;

  /* Simple list node record.  A list element is said to be 'unlinked' */
  /* when it doesn't belong to any list.                               */

  struct  _TList_Element
  {
    PList_Element  next;
    void*          data;
  };


  /* Simple singly-linked list record */

  struct  _TSingle_List
  {
    PList_Element  head;
    PList_Element  tail;
  };

  typedef struct _TSingle_List  TSingle_List;

#define ZERO_List( list )  { (list).head = NULL; (list).tail = NULL; }


  /********************************************************/
  /*                                                      */
  /* Two functions used to manage list elements.          */
  /*                                                      */
  /* The elements are extracted or recycled from/into     */
  /* a given engine instance.                             */
  /*                                                      */

  /* Returns a new list element, either fresh or recycled. */
  /* Note: the returned element is unlinked.               */

  PList_Element  Element_New( PEngine_Instance  engine );


  /* Recycles or discards an element.                     */
  /* Note: The element must be unlinked!                  */

  void Element_Done( PEngine_Instance  engine,
                     PList_Element     element );



  /********************************************************/
  /*                                                      */
  /* Several functions to manage single linked list.      */
  /*                                                      */
  /* Note that all these functions assume that the lists  */
  /* are already protected against concurrent operations  */
  /* (parsing, modification etc).                         */
  /*                                                      */

  /* Adds one element to the end of a single list.        */
  /* Note : The element must be unlinked!                 */
  /*        Always returns True.                          */

  void List_Add( TSingle_List*  list,
                 PList_Element  element );


  /* Removes one element from one list.                        */
  /* Note:  The element must be in the argument list before    */
  /*        the call.  It will be unlinked after the call,     */
  /*        and may be discarded through 'Done_List_Element()' */
  /*                                                           */
  /* Returns NULL in case of failure, otherwise returns        */
  /* simply 'element'.                                         */

  Bool  List_Remove( TSingle_List*  list,
                     PList_Element  element );


  /* Finds in a list the element corresponding to the object  */
  /* pointed by 'data'.                                       */

  PList_Element  List_Find( TSingle_List*  list,
                            void*          data );


  /* Returns and extracts the first element of a list, if any.          */
  /* Useful for recycle lists where any element can be taken for reuse. */

  PList_Element  List_Extract( TSingle_List*  list );



  /********************************************************/
  /*                                                      */
  /* The component's initializer and finalizer.           */
  /*                                                      */

  /* Initializes the lists component */
  TT_Error  TTLists_Init( PEngine_Instance  engine );

  /* Finalizes the lists component */
  TT_Error  TTLists_Done( PEngine_Instance  engine );


#ifdef __cplusplus
  }
#endif

#endif /* TTLISTS_H */


/* END */
