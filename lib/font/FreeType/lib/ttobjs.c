/*******************************************************************
 *
 *  ttobjs.c                                                     1.0
 *
 *    Objects manager.        
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
  
#include "ttobjs.h"
#include "ttfile.h"
#include "ttcalc.h"
#include "ttmemory.h"
#include "ttload.h"
#include "ttinterp.h"


/* Add extensions definition */
#ifdef TT_EXTEND_ENGINE
#include "ttextend.h"
#endif


/*******************************************************************
 *
 *  Function    :  New_Context
 *
 *  Description :  Creates a new execution context for a given
 *                 face object.
 *
 ******************************************************************/

  PExecution_Context  New_Context( PFace  face )
  {
    PExecution_Context  exec;


    if ( !face )
      return NULL;

    CACHE_New( &face->contexts, exec, face );

    return exec;
  }


/*******************************************************************
 *
 *  Function    :  Done_Context
 *
 *  Description :  Discards an execution context.
 *
 ******************************************************************/

  TT_Error  Done_Context( PExecution_Context  exec )
  {
    if ( !exec )
      return TT_Err_Ok;

    return CACHE_Done( &exec->owner->contexts, exec );
  }


/*******************************************************************
 *
 *  Function    :  New_Instance
 *
 *  Description :  Creates a new instance for a given face object.
 *
 ******************************************************************/

  PInstance  New_Instance( PFace  face )
  {
    PInstance  ins;


    if ( !face )
      return NULL;

    CACHE_New( &face->instances, ins, face );

    return ins;
  }


/*******************************************************************
 *
 *  Function    :  Done_Instance
 *
 *  Description :  Discards an instance.
 *
 ******************************************************************/

  TT_Error  Done_Instance( PInstance  instance )
  {
    return CACHE_Done( &instance->owner->instances, instance );
  }



/*******************************************************************
 *                                                                 *
 *                     CODERANGE FUNCTIONS                         *
 *                                                                 *
 *                                                                 *
 *******************************************************************/

/*******************************************************************
 *
 *  Function    :  Goto_CodeRange
 *
 *  Description :  Switch to a new code range (updates Code and IP).
 *
 *  Input  :  exec    target execution context
 *            range   new execution code range       
 *            IP      new IP in new code range       
 *
 *  Output :  SUCCESS on success.  FAILURE on error (no code range).
 *
 *****************************************************************/

  TT_Error  Goto_CodeRange( PExecution_Context  exec, Int  range, Int  IP )
  {
    PCodeRange  cr;


    if ( range < 1 || range > 3 )
      return TT_Err_Bad_Argument;

    cr = &exec->codeRangeTable[range - 1];

    if ( cr->Base == NULL )
      return TT_Err_Invalid_CodeRange;

    /* NOTE:  Because the last instruction of a program may be a CALL */
    /*        which will return to the first byte *after* the code    */
    /*        range, we test for IP <= Size, instead of IP < Size.    */

    if ( IP > cr->Size )
      return TT_Err_Code_Overflow;

    exec->code     = cr->Base;
    exec->codeSize = cr->Size;
    exec->IP       = IP;
    exec->curRange = range;

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Get_CodeRange
 *
 *  Description :  Returns a pointer to a given code range.  Should
 *                 be used only by the debugger.  Returns NULL if
 *                 'range' is out of current bounds.
 *
 *  Input  :  exec    target execution context
 *            range   new execution code range       
 *
 *  Output :  Pointer to the code range record.  NULL on failure.
 *
 *****************************************************************/

  PCodeRange  Get_CodeRange( PExecution_Context  exec, Int  range )
  {
    if ( range < 1 || range > 3 )
      return NULL;
    else                /* arrays start with 1 in Pascal, and with 0 in C */
      return &exec->codeRangeTable[range - 1];
  }


/*******************************************************************
 *
 *  Function    :  Set_CodeRange
 *
 *  Description :  Sets a code range.
 *
 *  Input  :  exec    target execution context
 *            range   code range index
 *            base    new code base
 *            length  sange size in bytes
 *
 *  Output :  SUCCESS on success.  FAILURE on error.
 *
 *****************************************************************/

  TT_Error  Set_CodeRange( PExecution_Context  exec,
                           Int                 range,
                           void*               base,
                           Int                 length )
  {
    if ( range < 1 || range > 3 )
      return TT_Err_Bad_Argument;

    exec->codeRangeTable[range - 1].Base = (unsigned char*)base;
    exec->codeRangeTable[range - 1].Size = length;

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Clear_CodeRange
 *
 *  Description :  Clears a code range.
 *
 *  Input  :  exec    target execution context
 *            range   code range index
 *
 *  Output :  SUCCESS on success.  FAILURE on error.
 *
 *  Note   : Does not set the Error variable.
 *
 *****************************************************************/

  TT_Error Clear_CodeRange( PExecution_Context  exec, Int  range )
  {
    if ( range < 1 || range > 3 )
      return TT_Err_Bad_Argument;

    exec->codeRangeTable[range - 1].Base = NULL;
    exec->codeRangeTable[range - 1].Size = 0;

    return TT_Err_Ok;
  }



/*******************************************************************
 *                                                                 *
 *                EXECUTION CONTEXT ROUTINES                       *
 *                                                                 *
 *                                                                 *
 *******************************************************************/

/*******************************************************************
 *
 *  Function    :  Context_Destroy
 *
 *****************************************************************/

 TT_Error  Context_Destroy( void*  _context )
 {
   PExecution_Context  exec = (PExecution_Context)_context;

   if ( !exec )
     return TT_Err_Ok;

   /* free composite load stack */
   FREE( exec->loadStack );

   /* contours array */
   FREE( exec->pts.contours );

   /* points zone */
   FREE( exec->pts.touch );
   FREE( exec->pts.cur_y );
   FREE( exec->pts.cur_x );
   FREE( exec->pts.org_y );
   FREE( exec->pts.org_x );
   exec->pts.n_points   = 0;
   exec->pts.n_contours = 0;

   /* twilight zone */
   FREE( exec->twilight.touch );
   FREE( exec->twilight.cur_y );
   FREE( exec->twilight.cur_x );
   FREE( exec->twilight.org_y );
   FREE( exec->twilight.org_x );
   exec->twilight.n_points   = 0;
   exec->twilight.n_contours = 0;

   /* free stack */
   FREE( exec->stack );
   exec->stackSize = 0;

   /* free storage area */
   FREE( exec->storage );
   exec->storeSize = 0;

   /* free call stack */
   FREE( exec->callStack );
   exec->callSize = 0;
   exec->callTop  = 0;

   /* free glyph code range */
   FREE( exec->glyphIns );
   exec->glyphSize = 0;

   exec->instance = NULL;
   exec->owner    = NULL;

   return TT_Err_Ok;
 }


/*******************************************************************
 *
 *  Function    :  Context_Create
 *
 *****************************************************************/

 TT_Error  Context_Create( void*  _context, void*  _face )
 {
   PExecution_Context  exec = (PExecution_Context)_context;

   PFace        face = (PFace)_face;
   TMaxProfile* maxp = &face->maxProfile;
   TT_Error     error;
   Int          n_points, n_twilight;


   exec->callSize  = 32;
   exec->storeSize = maxp->maxStorage;

   /* reserve a little extra for broken fonts like courbs or timesbs */
   exec->stackSize = maxp->maxStackElements + 32;

   n_points        = face->maxPoints + 2;
   n_twilight      = maxp->maxTwilightPoints;

   if ( ALLOC( exec->glyphIns, maxp->maxSizeOfInstructions )        ||
        /* reserve glyph code range */

        ALLOC_ARRAY( exec->callStack, exec->callSize, TCallRecord ) ||
        /* reserve interpreter call stack */

        ALLOC_ARRAY( exec->stack, exec->stackSize, Long )           ||
        /* reserve interpreter stack */

        ALLOC_ARRAY( exec->pts.org_x, n_points, TT_F26Dot6 )        ||
        ALLOC_ARRAY( exec->pts.org_y, n_points, TT_F26Dot6 )        ||
        ALLOC_ARRAY( exec->pts.cur_x, n_points, TT_F26Dot6 )        ||
        ALLOC_ARRAY( exec->pts.cur_y, n_points, TT_F26Dot6 )        ||

        ALLOC( exec->pts.touch, n_points )                          ||
        /* reserve points zone */

        ALLOC_ARRAY( exec->twilight.org_x, n_twilight, TT_F26Dot6 ) ||
        ALLOC_ARRAY( exec->twilight.org_y, n_twilight, TT_F26Dot6 ) ||
        ALLOC_ARRAY( exec->twilight.cur_x, n_twilight, TT_F26Dot6 ) ||
        ALLOC_ARRAY( exec->twilight.cur_y, n_twilight, TT_F26Dot6 ) ||

        ALLOC( exec->twilight.touch, n_twilight )                   ||
        /* reserve twilight zone */

        ALLOC_ARRAY( exec->pts.contours, face->maxContours, UShort ) ||
        /* reserve contours array */

        ALLOC_ARRAY( exec->loadStack,
                     face->maxComponents + 1, TSubglyph_Record ) )

     goto Fail_Memory;

     exec->twilight.n_points = n_twilight;

     exec->owner    = face;
     exec->instance = NULL;

     return TT_Err_Ok;

  Fail_Memory:
    Context_Destroy( exec );
    return error;
 }


/*******************************************************************
 *
 *  Function    :  Context_Load
 *
 *****************************************************************/

 TT_Error Context_Load( PExecution_Context  exec,
                        PInstance           ins )
 {
   Int  i;


   exec->instance = ins;

   exec->numFDefs = ins->numFDefs;
   exec->numIDefs = ins->numIDefs;
   exec->FDefs    = ins->FDefs;
   exec->IDefs    = ins->IDefs;

   exec->metrics  = ins->metrics;

   for ( i = 0; i < MAX_CODE_RANGES; i++ )
     exec->codeRangeTable[i] = ins->codeRangeTable[i];

   exec->pts.n_points   = 0;
   exec->pts.n_contours = 0;

   exec->instruction_trap = FALSE;

   /* set default graphics state */
   exec->GS = ins->GS;

   exec->cvtSize = ins->cvtSize;
   exec->cvt     = ins->cvt;

   exec->storeSize = ins->storeSize;
   exec->storage   = ins->storage;

   return TT_Err_Ok;
 }


/*******************************************************************
 *
 *  Function    :  Context_Save
 *
 *****************************************************************/

 TT_Error  Context_Save( PExecution_Context  exec,
                         PInstance           ins )
 {
   Int  i;


   for ( i = 0; i < MAX_CODE_RANGES; i++ )
     ins->codeRangeTable[i] = exec->codeRangeTable[i];

   return TT_Err_Ok;
 }


/*******************************************************************
 *
 *  Function    :  Context_Run
 *
 *****************************************************************/

 TT_Error  Context_Run( PExecution_Context  exec,
                        Bool                debug )
 {
   TT_Error  error;


   if ( ( error = Goto_CodeRange( exec, TT_CodeRange_Glyph, 0 ) ) )
     return error;

   exec->zp0 = exec->pts;
   exec->zp1 = exec->pts;
   exec->zp2 = exec->pts;

   exec->GS.gep0 = 1;
   exec->GS.gep1 = 1;
   exec->GS.gep2 = 1;

   exec->GS.projVector.x = 0x4000;
   exec->GS.projVector.y = 0x0000;

   exec->GS.freeVector = exec->GS.projVector;
   exec->GS.dualVector = exec->GS.projVector;

   exec->GS.round_state = 1;
   exec->GS.loop        = 1;

   /* some glyphs leave something on the stack. so we clean it */
   /* before a new execution.                                  */
   exec->top     = 0;
   exec->callTop = 0;

   if ( !debug )
     return RunIns( exec );
   else
     return TT_Err_Ok;
 }


  const TGraphicsState  Default_GraphicsState =
  {
    0, 0, 0, 
    { 0x4000, 0 },
    { 0x4000, 0 },
    { 0x4000, 0 },
    1, 64, 1,
    TRUE, 68, 0, 0, 9, 3,
    0, FALSE, 2, 1, 1, 1
  };



/*******************************************************************
 *                                                                 *
 *                     INSTANCE  FUNCTIONS                         *
 *                                                                 *
 *                                                                 *
 *******************************************************************/

/*******************************************************************
 *
 *  Function    : Instance_Destroy
 *
 *  Description :
 *
 *  Input  :  _instance   the instance object to destroy
 *
 *  Output :  error code.
 *
 ******************************************************************/

  TT_Error  Instance_Destroy( void* _instance )
  {
    PInstance  ins = (PInstance)_instance;


    if ( !_instance )
      return TT_Err_Ok;

    if ( ins->debug )
    {
      /* the debug context must be deleted by the debugger itself */
      ins->context = NULL;
      ins->debug   = FALSE;
    }

    FREE( ins->cvt );
    ins->cvtSize = 0;

    FREE( ins->FDefs );
    FREE( ins->IDefs );
    ins->numFDefs = 0;
    ins->numIDefs = 0;

    ins->owner = NULL;
    ins->valid = FALSE;

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    : Instance_Create
 *
 *  Description :
 *
 *  Input  :  _instance    instance record to initialize
 *            _face        parent face object
 *
 *  Output :  Error code.  All partially built subtables are
 *            released on error.
 *
 ******************************************************************/

  TT_Error  Instance_Create( void*  _instance,
                             void*  _face )
  {
    PInstance ins  = (PInstance)_instance;
    PFace     face = (PFace)_face;
    TT_Error  error;
    Int       i;

    PMaxProfile  maxp = &face->maxProfile;


    ins->owner = face;
    ins->valid = FALSE;

    ins->numFDefs = maxp->maxFunctionDefs;
    ins->numIDefs = maxp->maxInstructionDefs;
    ins->cvtSize  = face->cvtSize;

    ins->metrics.pointSize    = 10 * 64;     /* default pointsize  = 10pts */

    ins->metrics.x_resolution = 96;          /* default resolution = 96dpi */
    ins->metrics.y_resolution = 96;

    ins->metrics.x_ppem = 0;
    ins->metrics.y_ppem = 0;

    ins->metrics.rotated   = FALSE;
    ins->metrics.stretched = FALSE;

    ins->storeSize = maxp->maxStorage;

    for ( i = 0; i < 4; i++ )
      ins->metrics.compensations[i] = 0;     /* Default compensations */

    if ( ALLOC_ARRAY( ins->FDefs, ins->numFDefs, TDefRecord )  ||
         ALLOC_ARRAY( ins->IDefs, ins->numIDefs, TDefRecord )  ||
         ALLOC_ARRAY( ins->cvt, ins->cvtSize, Long )           ||
         ALLOC_ARRAY( ins->storage, ins->storeSize, Long )     )
      goto Fail_Memory;

    ins->GS = Default_GraphicsState;

    return TT_Err_Ok;

  Fail_Memory:
    Instance_Destroy( ins );
    return error;
  }


/*******************************************************************
 *
 *  Function    : Instance_Init
 *
 *  Description : Initialize a fresh new instance.
 *                Executes the font program if any is found.
 *
 *  Input  :  _instance   the instance object to destroy
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Instance_Init( PInstance  ins )
  {
    PExecution_Context  exec;

    TT_Error  error;
    PFace     face = ins->owner;


    if ( ins->debug )
      exec = ins->context;
    else
      exec = New_Context( face );
    /* debugging instances have their own context */

    if ( !exec )
      return TT_Err_Could_Not_Find_Context;

    ins->GS = Default_GraphicsState;

    Context_Load( exec, ins );

    exec->callTop   = 0;
    exec->top       = 0;

    exec->period    = 64;
    exec->phase     = 0;
    exec->threshold = 0;

    exec->metrics.x_ppem    = 0;
    exec->metrics.y_ppem    = 0;
    exec->metrics.pointSize = 0;
    exec->metrics.x_scale1  = 0;
    exec->metrics.x_scale2  = 1;
    exec->metrics.y_scale1  = 0;
    exec->metrics.y_scale2  = 1;

    exec->metrics.ppem      = 0;
    exec->metrics.scale1    = 0;
    exec->metrics.scale2    = 1;
    exec->metrics.ratio     = 1 << 16;

    exec->instruction_trap = FALSE;

    exec->cvtSize = ins->cvtSize;
    exec->cvt     = ins->cvt;

    exec->F_dot_P = 0x10000;

    /* allow font program execution */
    Set_CodeRange( exec,
                   TT_CodeRange_Font,
                   face->fontProgram,
                   face->fontPgmSize );

    /* disable CVT and glyph programs coderange */
    Clear_CodeRange( exec, TT_CodeRange_Cvt );
    Clear_CodeRange( exec, TT_CodeRange_Glyph );

    if ( face->fontPgmSize > 0 )
    {
      error = Goto_CodeRange( exec, TT_CodeRange_Font, 0 );
      if ( error )
        goto Fin;

      error = RunIns( exec );
    }
    else
      error = TT_Err_Ok;

  Fin:
    Context_Save( exec, ins );

    if ( !ins->debug )
      Done_Context( exec );
    /* debugging instances keep their context */

    ins->valid = FALSE;

    return error;
  }


/*******************************************************************
 *
 *  Function    : Instance_Reset
 *
 *  Description : Resets an instance to a new pointsize/transform.
 *                Executes the cvt program if any is found.
 *
 *  Input  :  _instance   the instance object to destroy
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Instance_Reset( PInstance  ins,
                            Bool       debug )
  {
    PExecution_Context  exec;

    TT_Error  error;
    Int       i;
    PFace     face;


    if ( !ins )
      return TT_Err_Invalid_Instance_Handle;

    if ( ins->valid )
      return TT_Err_Ok;

    face = ins->owner;

    if ( ins->metrics.x_ppem < 1 ||
         ins->metrics.y_ppem < 1 )
      return TT_Err_Invalid_PPem;

    /* compute new transformation */
    if ( ins->metrics.x_ppem >= ins->metrics.y_ppem )
    {
      ins->metrics.scale1  = ins->metrics.x_scale1;
      ins->metrics.scale2  = ins->metrics.x_scale2;
      ins->metrics.ppem    = ins->metrics.x_ppem;
      ins->metrics.x_ratio = 1 << 16;
      ins->metrics.y_ratio = MulDiv_Round( ins->metrics.y_ppem,
                                           0x10000,
                                           ins->metrics.x_ppem );
    }
    else
    {
      ins->metrics.scale1  = ins->metrics.y_scale1;
      ins->metrics.scale2  = ins->metrics.y_scale2;
      ins->metrics.ppem    = ins->metrics.y_ppem;
      ins->metrics.x_ratio = MulDiv_Round( ins->metrics.x_ppem,
                                           0x10000,
                                           ins->metrics.y_ppem );
      ins->metrics.y_ratio = 1 << 16;
    }

    /* Scale the cvt values to the new ppem.          */
    /* We use by default the y ppem to scale the CVT. */

    for ( i = 0; i < ins->cvtSize; i++ )
      ins->cvt[i] = MulDiv_Round( face->cvt[i],
                                  ins->metrics.scale1,
                                  ins->metrics.scale2 );

    ins->GS = Default_GraphicsState;

    /* get execution context and run prep program */

    if ( ins->debug )
      exec = ins->context;
    else
      exec = New_Context(face);
    /* debugging instances have their own context */

    if ( !exec )
      return TT_Err_Could_Not_Find_Context;

    Context_Load( exec, ins );

    Set_CodeRange( exec,
                   TT_CodeRange_Cvt,
                   face->cvtProgram,
                   face->cvtPgmSize );

    Clear_CodeRange( exec, TT_CodeRange_Glyph );

    for ( i = 0; i < exec->storeSize; i++ )
      exec->storage[i] = 0;

    exec->instruction_trap = FALSE;

    exec->top     = 0;
    exec->callTop = 0;

    /* All twilight points are originally zero */

    for ( i = 0; i < exec->twilight.n_points; i++ )
    {
      exec->twilight.org_x[i] = 0;
      exec->twilight.org_y[i] = 0;
      exec->twilight.cur_x[i] = 0;
      exec->twilight.cur_y[i] = 0;
    }

    if ( face->cvtPgmSize > 0 )
    {
      error = Goto_CodeRange( exec, TT_CodeRange_Cvt, 0 );
      if (error)
        goto Fin;

      if ( !ins->debug )
        error = RunIns( exec );
    }
    else
      error = TT_Err_Ok;

    ins->GS = exec->GS;
    /* save default graphics state */

  Fin:
    Context_Save( exec, ins );

    if ( !ins->debug )
      Done_Context( exec );
    /* debugging instances keep their context */

    if ( !error )
      ins->valid = TRUE;

    return error;
  }



/*******************************************************************
 *                                                                 *
 *                         FACE  FUNCTIONS                         *
 *                                                                 *
 *                                                                 *
 *******************************************************************/

/*******************************************************************
 *
 *  Function    :  Face_Destroy
 *
 *  Description :  The face object destructor.
 *
 *  Input  :  _face   typeless pointer to the face object to destroy
 *
 *  Output :  Error code.                       
 *
 ******************************************************************/

  TT_Error  Face_Destroy( void*  _face )
  {
    PFace  face = (PFace)_face;
    Int    n;


    if ( !face )
      return TT_Err_Ok;

    /* well, we assume that no other thread is using the face */
    /* at this moment, but one is never sure enough.          */
    MUTEX_Lock( face->lock );

    /* first of all, destroys the cached sub-objects */
    Cache_Destroy( &face->instances );
    Cache_Destroy( &face->contexts );
    Cache_Destroy( &face->glyphs );

    /* destroy the extensions */
#ifdef TT_EXTEND_ENGINE
    Extension_Destroy( face );
#endif

    /* freeing the collection table */
    FREE( face->ttcHeader.TableDirectory );
    face->ttcHeader.DirCount = 0;

    /* freeing table directory */
    FREE( face->dirTables );
    face->numTables = 0;

    /* freeing the locations table */
    FREE( face->glyphLocations );
    face->numLocations = 0;

    /* freeing the character mapping tables */
    for ( n = 0; n < face->numCMaps; n++ )
      CharMap_Free( face->cMaps + n );

    FREE( face->cMaps );
    face->numCMaps = 0;

    /* freeing the CVT */
    FREE( face->cvt );
    face->cvtSize = 0;

    /* freeing the horizontal metrics */
    FREE( face->longHMetrics );
    FREE( face->shortMetrics );

    /* freeing the programs */
    FREE( face->fontProgram );
    FREE( face->cvtProgram );
    face->fontPgmSize = 0;
    face->cvtPgmSize  = 0;

    /* freeing the gasp table */
    FREE( face->gasp.gaspRanges );
    face->gasp.numRanges = 0;

    /* freeing the name table */
    Free_TrueType_Names( face );

    /* freeing the hdmx table */
    Free_TrueType_Hdmx( face );

    /* TT_Close_Stream( &face->stream ); -- this is performed by the API */

    /* destroy the mutex */
    MUTEX_Destroy(face->lock);

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Face_Create
 *
 *  Description :  The face object constructor.
 *
 *  Input  :  _face    face record to build
 *            _input   input stream where to load font data
 *
 *  Output :  Error code.
 *
 *  NOTE : The input stream is kept in the face object.  The
 *         caller shouldn't destroy it after calling Face_Create().
 *
 ******************************************************************/

#define LOAD_( table ) \
          ( error = Load_TrueType_##table (face) )


  TT_Error  Face_Create( void*  _face,
                         void*  _input )
  {
    PEngine_Instance  engine;

    TFont_Input*  input = (TFont_Input*)_input;
    PFace         face  = (PFace)_face;
    TT_Error      error;


    face->stream = input->stream;
    face->engine = input->engine;

    engine = face->engine;

    MUTEX_Create( face->lock );

    Cache_Create( engine,
                  engine->objs_instance_class,  
                  &face->instances, 
                  &face->lock );
    Cache_Create( engine,
                  engine->objs_execution_class, 
                  &face->contexts, 
                  &face->lock );
    Cache_Create( engine,
                  engine->objs_glyph_class,
                  &face->glyphs,
                  &face->lock );

    /* Load collection directory if present, then font directory */

    if ( ( error = Load_TrueType_Directory( face, input->fontIndex ) ) )
      goto Fail;

    /* Load tables */

    if ( LOAD_(Header)                      ||
         LOAD_(MaxProfile)                  ||
         LOAD_(Locations)                   ||
         LOAD_(CMap)                        ||
         LOAD_(CVT)                         ||
         LOAD_(Horizontal_Header)           ||
         LOAD_(Programs)                    ||
         LOAD_(HMTX)                        ||
         LOAD_(Gasp)                        ||
         LOAD_(Names)                       ||
         LOAD_(OS2)                         ||
         LOAD_(PostScript)                  ||
         LOAD_(Hdmx)                        )

      goto Fail;

#ifdef TT_EXTEND_ENGINE
    if ( ( error = Extension_Create( face ) ) )
      return error;
#endif

    return TT_Err_Ok;

  Fail :
    Face_Destroy( face );
    return error;
  }


/*******************************************************************
 *
 *  Function    :  Glyph_Destroy
 *
 *  Description :  The glyph object destructor.
 *
 *  Input  :  _glyph  typeless pointer to the glyph record to destroy
 *
 *  Output :  Error code.                       
 *
 ******************************************************************/

  TT_Error  Glyph_Destroy( void*  _glyph )
  {
    PGlyph  glyph = (PGlyph)_glyph;

    if ( !glyph )
      return TT_Err_Ok;

    glyph->outline.owner = TRUE;
    return TT_Done_Outline( &glyph->outline );
  }


/*******************************************************************
 *
 *  Function    :  Glyph_Create
 *
 *  Description :  The glyph object constructor.
 *
 *  Input  :  _glyph   glyph record to build.
 *            _face    the glyph's parent face.              
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Glyph_Create( void*  _glyph,
                          void*  _face )
  {
    PFace     face  = (PFace)_face;
    PGlyph    glyph = (PGlyph)_glyph;

    if ( !face )
      return TT_Err_Invalid_Face_Handle;

    if ( !glyph )
      return TT_Err_Invalid_Glyph_Handle;

    glyph->face         = face;

    return TT_New_Outline( glyph->face->maxPoints+2,
                           glyph->face->maxContours,
                           &glyph->outline );
  }


/*******************************************************************
 *
 *  Function    :  Scale_X
 *
 *  Description :  scale an horizontal distance from font
 *                 units to 26.6 pixels
 *
 *  Input  :  metrics  pointer to metrics
 *            x        value to scale
 *
 *  Output :  scaled value
 *
 ******************************************************************/

 TT_Pos  Scale_X( PIns_Metrics  metrics, TT_Pos  x )
 {
   return MulDiv_Round( x, metrics->x_scale1, metrics->x_scale2 );
 }

/*******************************************************************
 *
 *  Function    :  Scale_Y
 *
 *  Description :  scale a vertical distance from font
 *                 units to 26.6 pixels
 *
 *  Input  :  metrics  pointer to metrics
 *            y        value to scale
 *
 *  Output :  scaled value
 *
 ******************************************************************/

 TT_Pos  Scale_Y( PIns_Metrics  metrics, TT_Pos  y )
 {
   return MulDiv_Round( y, metrics->y_scale1, metrics->y_scale2 );
 }

/*******************************************************************
 *
 *  Function    :  TTObjs_Init
 *
 *  Description :  The TTObjs component initializer.  Creates the
 *                 object cache classes, as well as the face record
 *                 cache.
 *
 *  Input  :  engine    engine instance
 *
 *  Output :  Error code.                       
 *
 ******************************************************************/

  TT_Error  TTObjs_Init( PEngine_Instance  engine )
  {
    PCache_Class  instances, contexts, faces, glyphs;
    PCache        face_cache, glyph_cache;
    TT_Error      error;


    instances = NULL;
    contexts  = NULL;
    faces     = NULL;
    glyphs    = NULL;

    glyph_cache = NULL;
    face_cache  = NULL;

    if ( !ALLOC( instances,  sizeof ( TCache_Class ) )   && 
         !ALLOC( contexts,   sizeof ( TCache_Class ) )   &&
         !ALLOC( faces,      sizeof ( TCache_Class ) )   &&
         !ALLOC( glyphs,     sizeof ( TCache_Class ) )   &&
         !ALLOC( face_cache, sizeof ( TCache ) ) )
    {
      /* create the face cache class */

      faces->object_size = sizeof(TFace);
      faces->idle_limit  = -1;            /* no recycling of face objects */
      faces->init        = Face_Create;
      faces->done        = Face_Destroy;
      faces->reset       = NULL;
      faces->finalise    = NULL;

      engine->objs_face_class = faces;

      /* create the instance cache class */

      instances->object_size = sizeof ( TInstance );
      instances->idle_limit  = 2;
      instances->init        = Instance_Create;
      instances->done        = Instance_Destroy;
      instances->reset       = NULL;
      instances->finalise    = NULL;

      engine->objs_instance_class = instances;

      /* create the execution context cache class */

      contexts->object_size = sizeof ( TExecution_Context );
      contexts->idle_limit  = 1;
      contexts->init        = Context_Create;
      contexts->done        = Context_Destroy;
      contexts->reset       = NULL;
      contexts->finalise    = NULL;

      engine->objs_execution_class = contexts;

      glyphs->object_size = sizeof ( TGlyph );
      glyphs->idle_limit  = -1;  /* no recycling of glyph objects */
      glyphs->init        = Glyph_Create;
      glyphs->done        = Glyph_Destroy;
      glyphs->reset       = NULL;
      glyphs->finalise    = NULL;

      engine->objs_glyph_class = glyphs;

      /* create face and glyph 'caches'.  These are used only */
      /* for tracking purpose.  No recycling is performed.    */

      Cache_Create( engine, faces, face_cache, &engine->lock );
      engine->objs_face_cache  = face_cache;
    }
    else
    {
      FREE( face_cache );

      FREE( glyphs );
      FREE( faces );
      FREE( contexts );
      FREE( instances );
    }

    return error;
  }


/*******************************************************************
 *
 *  Function    :  TTObjs_Done
 *
 *  Description :  The TTObjs component finalizer.
 *
 *  Input  :  engine    engine instance
 *
 *  Output :  Error code.                       
 *
 ******************************************************************/

  TT_Error  TTObjs_Done( PEngine_Instance  engine )
  {
    /* destroy all active faces and glyphs before releasing the */
    /* caches                                                   */

    Cache_Destroy( (TCache*)engine->objs_face_cache );

    /* Now frees caches and cache classes */

    FREE( engine->objs_face_cache );

    FREE( engine->objs_glyph_class );
    FREE( engine->objs_execution_class );
    FREE( engine->objs_instance_class );
    FREE( engine->objs_face_class );

    return TT_Err_Ok;
  }


/* END */
