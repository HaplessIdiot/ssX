/*******************************************************************
 *
 *  ttextend.h                                                   2.0
 *
 *    Extensions Interface.
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
 *  This is an updated version of the extension component, now
 *  located in the main library's source directory.  It allows
 *  the dynamic registration/use of various face object extensions
 *  through a simple API.
 *
 ******************************************************************/

/* $XFree86: $ */
  
#ifndef TTEXTEND_H
#define TTEXTEND_H

#include "ttcommon.h"
#include "tttypes.h"
#include "ttobjs.h"


#ifdef __cplusplus
  extern "C" {
#endif

  /* The extensions don't need to be integrated at compile time into */
  /* the engine, only at link time.                                  */


  /* When a new face object is created, the face constructor calls */
  /* the extension constructor with the following arguments:       */
  /*                                                               */
  /*   ext  : typeless pointer to the face's extension block.      */
  /*          Its size is the one given at registration time       */
  /*          in the extension class's 'size' field.               */
  /*                                                               */
  /*   face : the parent face object.  Note that the extension     */
  /*          constructor is called when the face object is        */
  /*          built.                                               */

  typedef TT_Error  TExt_Constructor( void*  ext, PFace  face );


  /* When a face object is destroyed, the face destructor calls    */
  /* the extension destructor with the follozing arguments.        */
  /*                                                               */
  /*   ext  : typeless pointer to the face's extension block.      */
  /*          Its size is the one given at registration time       */
  /*          in the extension class's 'size' field.               */
  /*                                                               */
  /*   face : the parent face object.  Note that the extension     */
  /*          destructor is called before the actual face object   */
  /*          is destroyed.                                        */

  typedef TT_Error  TExt_Destructor ( void*  ext, PFace  face );

  typedef TExt_Constructor*  PExt_Constructor;
  typedef TExt_Destructor*   PExt_Destructor;


  struct _TExtension_Class
  {
    Long              id;      /* extension id                      */
    Int               size;    /* size in bytes of extension record */
    PExt_Constructor  build;   /* the extension's class constructor */
    PExt_Destructor   destroy; /* the extension's class destructor  */

    Int               offset;  /* offset of ext. record in face obj */
                               /* (set by the engine)               */
  };

  typedef struct _TExtension_Class  TExtension_Class;
  typedef TExtension_Class*         PExtension_Class;


#define Build_Extension_ID( a, b, c, d ) \
           ( ((unsigned)(a) << 24) |     \
             ((unsigned)(b) << 16) |     \
             ((unsigned)(c) << 8 ) |     \
              (unsigned)(d) )


  /* Initialize the extension component */
  TT_Error  TTExtend_Init( PEngine_Instance  engine );

  /* Finalize the extension component */
  TT_Error  TTExtend_Done( PEngine_Instance  engine );

  /* Register a new extension.  Called by extension */
  /* service initialisers.                          */
  TT_Error  Extension_Register( PEngine_Instance  engine,
                                Long              id,
                                Int               size,
                                PExt_Constructor  create,
                                PExt_Destructor   destroy );

  /* Create an extension within a face object.  Called by the */
  /* face object constructor.                                 */
  TT_Error  Extension_Create( PFace  face );

  /* Destroy all extensions within a face object.  Called by the */
  /* face object destructor.                                     */
  TT_Error  Extension_Destroy( PFace  face );

  /* Query an extension block by extension_ID.  Called by extension */
  /* service routines.                                              */
  TT_Error  Extension_Get( PFace   face,
                           Long    extension_id,
                           void**  extension_block );

#ifdef __cplusplus
  }
#endif


#endif /* TTEXTEND_H */


/* END */
