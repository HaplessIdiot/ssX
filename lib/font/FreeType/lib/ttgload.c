/*******************************************************************
 *
 *  ttgload.c                                                   1.0
 *
 *    TrueType Glyph Loader.                           
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
  
#include "tttypes.h"
#include "tterror.h"
#include "ttcalc.h"
#include "ttfile.h"

#include "tttables.h"
#include "ttobjs.h"
#include "ttgload.h"

#include "ttmemory.h"
#include "tttags.h"
#include "ttload.h"


/* composite font flags */
  
#define ARGS_ARE_WORDS       0x001
#define ARGS_ARE_XY_VALUES   0x002
#define ROUND_XY_TO_GRID     0x004
#define WE_HAVE_A_SCALE      0x008
/* reserved                  0x010 */
#define MORE_COMPONENTS      0x020
#define WE_HAVE_AN_XY_SCALE  0x040
#define WE_HAVE_A_2X2        0x080
#define WE_HAVE_INSTR        0x100
#define USE_MY_METRICS       0x200
  

 /********************************************************)
 (* return horizontal metrics in font units for a        *)
 (* given glyph. if "check" is true, take care of        *)
 (* mono-spaced fonts by returning the aw max.           *)
 (*                                                      */
 static void  Get_HMetrics( PFace  face,
                            Int    index,
                            Bool   check,
                            Int*   lsb,
                            Int*   aw    )
 {
   Int  k;

   k = face->horizontalHeader.number_Of_HMetrics;

   if ( index < k )
   {
     *lsb = face->longHMetrics[index].lsb;
     *aw  = face->longHMetrics[index].advance_Width;
   }
   else
   {
     *lsb = face->shortMetrics[index-k];
     *aw  = face->longHMetrics[k-1].advance_Width;
   }
   if ( check && face->postscript.isFixedPitch )
     *aw = face->horizontalHeader.advance_Width_Max;
 }


 /********************************************************)
 (* return advance width table for a given pixel size    *)
 (* if it is found in the font's "hdmx" table (if any)   *)
 (*                                                      */
 static PByte  Get_Advance_Widths( PFace  face,
                                   Int    ppem )
 {
   Int  n;

   for ( n = 0; n < face->hdmx.num_records; n++ )
     if ( face->hdmx.records[n].ppem == ppem )
       return face->hdmx.records[n].widths;

   return NULL;
 }


 /********************************************************)
 (* copy current glyph into original one                 *)
 (*                                                      */
 static void  cur_to_org( Int  n, PGlyph_Zone  zone )
 {
   Int  k;

   for ( k = 0; k < n; k++ )
     zone->org_x[k] = zone->cur_x[k];

   for ( k = 0; k < n; k++ )
     zone->org_y[k] = zone->cur_y[k];
 }

 /********************************************************)
 (* copy original glyph into current one                 *)
 (*                                                      */
 static void  org_to_cur( Int  n, PGlyph_Zone  zone )
 {
   Int  k;

   for ( k = 0; k < n; k++ )
     zone->cur_x[k] = zone->org_x[k];

   for ( k = 0; k < n; k++ )
     zone->cur_y[k] = zone->org_y[k];
 }


 /********************************************************)
 (* translate an array of coordinates                    *)
 (*                                                      */
 static void  translate_array( Int  n, PCoordinates  c, TT_Pos  x )
 {
   Int  k;
   if (x)
     for ( k = 0; k < n; k++ )
       c[k] += x;
 }

 /********************************************************)
 (* mount one zone on top of another one                 *)
 (*                                                      */
 static void  mount_zone( PGlyph_Zone  source,
                          PGlyph_Zone  target )
 {
   Int  np, nc;

   np = source->n_points;
   nc = source->n_contours;

   target->org_x = source->org_x + np;
   target->org_y = source->org_y + np;
   target->cur_x = source->cur_x + np;
   target->cur_y = source->cur_y + np;
   target->touch = source->touch + np;

   target->contours = source->contours + nc;

   target->n_points   = 0;
   target->n_contours = 0;
 }


/*******************************************************************
 *
 *  Function    :  Load_Simple_Glyph
 *
 *
 ******************************************************************/

 static TT_Error  Load_Simple_Glyph( PExecution_Context  exec,
                                     TT_Stream           input,
                                     Int                 n_contours,
                                     Int                 left_contours,
                                     Int                 left_points,
                                     Int                 load_flags,
                                     PSubglyph_Record    subg )
  {
    DEFINE_LOAD_LOCALS( input );

    Int   k, n_points, n_ins;
    Byte  c, cnt;

    PFace  face;

    PCoordinates  xs, ys;
    Byte*         flag;

    TT_F26Dot6  x, y, xmin, ymin, xmax, ymax;

    PGlyph_Zone  pts;


    face = exec->owner;

    /* simple check */
    if ( n_contours > left_contours )
    {
      DebugTrace(( "ERROR: Glyph index %d has %d contours > left %d\n",
                   subg->index, n_contours, left_contours ));
      return TT_Err_Too_Many_Contours;
    }


    /* preparing the execution context */
    mount_zone( &subg->zone, &exec->pts );

    /* reading the contours endpoints */
    if ( ACCESS_Frame( (n_contours + 1) * 2L ) )
      return error;

    n_points = 0;

    for ( k = 0; k < n_contours; k++ )
    {
      DebugTrace(( "%d ", n_points ));
      exec->pts.contours[k] = n_points = GET_Short();
      n_points++;
    }

    n_ins = GET_Short();

    FORGET_Frame();

    if ( n_points > left_points )
    {
      DebugTrace(( "ERROR: Too many points in glyph %d\n", subg->index ));
      return TT_Err_Too_Many_Points;
    }

    /* loading instructions */

    DebugTrace(( "Instructions size : %d\n", n_ins ));

    if ( n_ins > face->maxProfile.maxSizeOfInstructions )
    {
      DebugTrace(( "ERROR: Too many instructions!\n" ));
      return TT_Err_Too_Many_Ins;
    }

    if ( FILE_Read( exec->glyphIns,  n_ins ) )
      return error;

    exec->glyphSize = n_ins;

    if ( ( error = Set_CodeRange( exec, 
                                  TT_CodeRange_Glyph, 
                                  exec->glyphIns, 
                                  exec->glyphSize ) ) )
      return error;  


    /* read the flags */

    if ( CHECK_ACCESS_Frame( n_points * 5L ) )
      return error;

    k    = 0;
    flag = exec->pts.touch;

    while ( k < n_points )
    {
      flag[k] = c = GET_Byte();
      k++;

      if ( c & 8 )
      {
        cnt = GET_Byte();
        while( cnt > 0 )
        {
          flag[k++] = c;
          cnt--;
        }
      }
    }


    /* read the X */

    x    = 0;
    xmin = 0;
    xmax = 0;
    xs   = exec->pts.org_x;

    for ( k = 0; k < n_points; k++ )
    {
      if ( flag[k] & 2 )
      {
        if ( flag[k] & 16 )
          x += GET_Byte();
        else
          x -= GET_Byte();
      }
      else
      {
        if ( (flag[k] & 16) == 0 )
          x += GET_Short();
      }

      xs[k] = x;

      if ( x < xmin ) xmin = x;
      if ( x > xmax ) xmax = x;
    }


   /* read the Y */

    y    = 0;
    ymin = 0;
    ymax = 0;
    ys   = exec->pts.org_y;

    for ( k = 0; k < n_points; k++ )
    {
      if ( flag[k] & 4 )
      {
        if ( flag[k] & 32 )
          y += GET_Byte();
        else
          y -= GET_Byte();
      }
      else
      {
        if ( (flag[k] & 32) == 0 )
          y += GET_Short();
      }

      ys[k] = y;

      if ( y < ymin ) ymin = y;
      if ( y > ymax ) ymax = y;
    }

    FORGET_Frame();

    /* Now adds the two shadow points at n and n + 1.   */
    /* We need the left side bearing and advance width. */

    /* pp1 = xMin - lsb */
    xs[n_points] = subg->bbox.xMin - subg->leftBearing;
    ys[n_points] = 0;

    /* pp2 = pp1 + aw */
    xs[n_points+1] = xs[n_points] + subg->advanceWidth;
    ys[n_points+1] = 0;

    /* clear the touch flags */

    for ( k = 0; k < n_points; k++ )
      exec->pts.touch[k] &= TT_Flag_On_Curve;

    exec->pts.touch[n_points    ] = 0;
    exec->pts.touch[n_points + 1] = 0;

    /* Note that we return two more points that are not */
    /* part of the glyph outline.                       */

    n_points += 2;

    /* now eventually scale and hint the glyph */

    pts = &exec->pts;
    pts->n_points   = n_points;
    pts->n_contours = n_contours;

    if ( (load_flags & TTLOAD_SCALE_GLYPH) == 0 )
    {
      /* no scaling, just copy the orig arrays into the cur ones */
      org_to_cur( n_points, pts );
    }
    else
    {
     /* first scale the glyph points */

      for ( k = 0; k < n_points; k++ )
      {
        pts->org_x[k] = Scale_X( &exec->metrics, pts->org_x[k] );
        pts->org_y[k] = Scale_Y( &exec->metrics, pts->org_y[k] );
      }

      /* if hinting, round pp1, and shift the glyph accordingly */
      if (subg->is_hinted)
      {
        x = pts->org_x[n_points-2];
        x = ((x+32) & -64) - x;
        translate_array( n_points, pts->org_x, x );

        org_to_cur( n_points, pts );

        pts->cur_x[n_points-1] = (pts->cur_x[n_points-1]+32) & -64;

        /* now consider hinting */
        if (n_ins > 0)
        {
          exec->is_composite = FALSE;
          if ( ( error = Context_Run( exec, FALSE ) ) )
            return error;
        }
      }
      else
        org_to_cur( n_points, pts );
    }

    /* save glyph phantom points */
    if (!subg->preserve_pps)
    {
      subg->pp1.x = pts->cur_x[n_points-2];
      subg->pp1.y = pts->cur_y[n_points-2];
      subg->pp2.x = pts->cur_x[n_points-1];
      subg->pp2.y = pts->cur_y[n_points-1];
    }

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_Composite_End
 *
 *
 ******************************************************************/

  static
  TT_Error  Load_Composite_End( Int                 n_points,
                                Int                 n_contours,
                                PExecution_Context  exec,
                                PSubglyph_Record    subg )
  {
    Int          k, n_ins;
    TT_Pos       x, y;
    PGlyph_Zone  pts;
    TT_Error     error;


    if ( subg->is_hinted    &&
         subg->element_flag & WE_HAVE_INSTR )
    {
      if ( ACCESS_Frame( 2 ) )
        return error;
  
      n_ins = GET_UShort();     /* read size of instructions */
      FORGET_Frame();
  
      DebugTrace(( "Instructions size = %d\n", k ));
  
      if ( n_ins > exec->owner->maxProfile.maxSizeOfInstructions )
      {
        DebugTrace(( "Too many instructions\n" ));
        return TT_Err_Too_Many_Ins;
      }
  
      if ( FILE_Read( exec->glyphIns, n_ins ) )
        return error;
  
      exec->glyphSize = n_ins;

      error = Set_CodeRange( exec, 
                             TT_CodeRange_Glyph, 
                             exec->glyphIns, 
                             exec->glyphSize );

      if (error) return error;
    }
    else
      n_ins = 0;


    /* prepare the execution context */
    n_points += 2;
    exec->pts = subg->zone;
    pts       = &exec->pts;

    pts->n_points   = n_points;
    pts->n_contours = n_contours;

    /* add phantom points */
    pts->cur_x[n_points-2] = subg->pp1.x;
    pts->cur_y[n_points-2] = subg->pp1.y;
    pts->cur_x[n_points-1] = subg->pp2.x;
    pts->cur_y[n_points-1] = subg->pp2.y;
    pts->touch[n_points-1] = 0;
    pts->touch[n_points-2] = 0;

    /* if hinting, round the phantom points */
    if (subg->is_hinted)
    {
      y = ((subg->pp1.x+32) & -64);
      pts->cur_x[n_points-2] = y;

      x = ((subg->pp2.x+32) & -64);
      pts->cur_x[n_points-1] = x;
    }

    for ( k = 0; k < n_points; k++ )
      pts->touch[k] = pts->touch[k] & TT_Flag_On_Curve;

    cur_to_org( n_points, pts );

    /* now consider hinting */
    if ( subg->is_hinted && n_ins > 0 )
    {
      exec->is_composite = TRUE;
      if ((error = Context_Run( exec, FALSE )))
        return error;
    }

    /* save glyph origin and advance points */
    subg->pp1.x = pts->cur_x[n_points-2];
    subg->pp1.y = pts->cur_y[n_points-2];
    subg->pp2.x = pts->cur_x[n_points-1];
    subg->pp2.y = pts->cur_y[n_points-1];

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Init_Glyph_Component
 *
 *
 ******************************************************************/

  static
  void  Init_Glyph_Component( PSubglyph_Record    element,
                              PSubglyph_Record    original,
                              PExecution_Context  exec )
  {
    element->index     = -1;
    element->is_scaled = FALSE;
    element->is_hinted = FALSE;

    if (original)
      mount_zone( &original->zone, &element->zone );
    else
      element->zone = exec->pts;

    element->zone.n_contours = 0;
    element->zone.n_points   = 0;

    element->arg1 = 0;
    element->arg2 = 0;

    element->element_flag = 0;
    element->preserve_pps = FALSE;

    element->transform.xx = 1 << 16;
    element->transform.xy = 0;
    element->transform.yx = 0;
    element->transform.yy = 1 << 16;

    element->transform.ox = 0;
    element->transform.oy = 0;

    element->leftBearing  = 0;
    element->advanceWidth = 0;
  }



  TT_Error  Load_TrueType_Glyph(  PInstance   instance,
                                  PGlyph      glyph,
                                  Int         glyph_index,
                                  Int         load_flags )
  {
    enum _TPhases
    {
      Load_Exit,
      Load_Glyph,
      Load_Header,
      Load_Simple,
      Load_Composite,
      Load_End
    };

    typedef enum _TPhases  TPhases;

    DEFINE_ALL_LOCALS;

    PFace   face;

    Int  num_points;
    Int  num_contours;
    Int  left_points;
    Int  left_contours;

    Int  table, index, load_top, new_flags, k, l;

    Long  glyph_offset, offset;

    TT_F26Dot6  x, y, nx, ny;

    Fixed  xx, xy, yx, yy;

    PExecution_Context  exec;

    PSubglyph_Record  subglyph, subglyph2;

    TGlyph_Zone base_pts;

    TPhases     phase;
    PByte       widths;

    TT_Glyph_Loader_Callback  cacheCb;
    TT_Outline                cached_outline;

    /* first of all, check arguments */
    if ( !glyph )
      return TT_Err_Invalid_Glyph_Handle;

    face = glyph->face;
    if ( !face )
      return TT_Err_Invalid_Glyph_Handle;

    if (glyph_index < 0 || glyph_index >= face->numGlyphs )
      return TT_Err_Invalid_Glyph_Index;

    if ( instance && (load_flags & TTLOAD_SCALE_GLYPH) == 0)
    {
      instance    = 0;
      load_flags &= ~( TTLOAD_SCALE_GLYPH | TTLOAD_HINT_GLYPH );
    }

    table = LookUp_TrueType_Table( face, TTAG_glyf );
    if ( table < 0 )
    {
      DebugTrace(( "ERROR: there is no glyph table in this font file!\n" ));
      return TT_Err_Table_Missing;
    }

    glyph_offset = face->dirTables[table].Offset;

    /* query new execution context */

    if ( instance && instance->debug )
      exec = instance->context;
    else
      exec = New_Context( face );

    if ( !exec )
      return TT_Err_Out_Of_Memory;

    if (instance)
    {
      Context_Load( exec, instance );
  
      if ( instance->GS.instruct_control & 2 )
        exec->GS = Default_GraphicsState;
      else
        exec->GS = instance->GS;
      /* load default graphics state */

      glyph->outline.high_precision = (instance->metrics.y_ppem < 24 );
    }

    /* save its critical pointers, as they'll be modified during load */
    base_pts = exec->pts;

    /* init variables */
    left_points   = face->maxPoints;
    left_contours = face->maxContours;

    num_points   = 0;
    num_contours = 0;

    load_top = 0;
    subglyph = exec->loadStack;

    Init_Glyph_Component( subglyph, NULL, exec );

    subglyph->index     = glyph_index;
    subglyph->is_hinted = load_flags & TTLOAD_HINT_GLYPH;

    /* when the cvt program has disabled hinting, the argument */
    /* is ignored.                                             */
    if ( instance && instance->GS.instruct_control & 1 )
      subglyph->is_hinted = FALSE;


    /* now access stream */

    if ( USE_Stream( face->stream, stream ) )
      goto Fin;

    /* Main loading loop */

    phase = Load_Glyph;
    index = 0;

    while ( phase != Load_Exit )
    {
      subglyph = exec->loadStack + load_top;

      switch (phase)
      {
        /************************************************************/
        /*                                                          */
        /* Load_Glyph state                                         */
        /*                                                          */
        /*   reading a glyph's generic header to determine          */
        /*   wether it's simple or composite                        */
        /*                                                          */
        /* exit states: Load_Header and Load_End                    */
  
      case Load_Glyph:
        /* check glyph index and table */

        index = subglyph->index;
        if ( index < 0 || index >= face->numGlyphs )
        {
          error = TT_Err_Invalid_Glyph_Index;
          goto Fail;
        }

        /* get horizontal metrics */

        Get_HMetrics( face, index, TRUE, 
                      &subglyph->leftBearing, 
                      &subglyph->advanceWidth );

        phase = Load_Header;

        if (instance)
        {
          /* is the glyph in an outline cache ? */
          cacheCb = instance->owner->engine->glCallback;
          if (cacheCb && 0)   /* disabled */
          {
            /* we have a callback */
            error = cacheCb( instance->generic,
                             index, &cached_outline, &x, &y );
            if (!error)
            {
              /* no error, then append the outline to the current subglyph */
              /* error = Append_Outline( subglyph, 
                                         &left_points, 
                                         &left_contours,
                                         &cached_outline ); */
              phase = Load_End;
            }
          }
        }
        break;

        /************************************************************/
        /*                                                          */
        /* Load_Header state                                        */
        /*                                                          */
        /*   reading a glyph's generic header to determine          */
        /*   wether it's simple or composite                        */
        /*                                                          */
        /* exit states: Load_Simple and Load_Composite              */

      case Load_Header : /* load glyph */

        if ( index+1 < face->numLocations &&
             face->glyphLocations[index] ==
               face->glyphLocations[index + 1] )
        {
          /* as described by Frederic Loyer, these are spaces, and */
          /* not the unknown glyph.                                */

          num_contours = 0;
          num_points   = 0;

          subglyph->bbox.xMin = 0;
          subglyph->bbox.xMax = 0;
          subglyph->bbox.yMin = 0;
          subglyph->bbox.yMax = 0;

          subglyph->pp1.x = 0;
          subglyph->pp2.x = subglyph->advanceWidth;
          if (load_flags & TTLOAD_SCALE_GLYPH)
            subglyph->pp2.x = Scale_X( &exec->metrics, subglyph->pp2.x );

          exec->glyphSize = 0;
          phase = Load_End;
          break;
        }

        offset = glyph_offset + face->glyphLocations[index];

        /* read first glyph header */
        if ( FILE_Seek( offset ) ||
             ACCESS_Frame( 10L ) )
          goto Fail_File;

        num_contours = GET_Short();

        subglyph->bbox.xMin = GET_Short();
        subglyph->bbox.yMin = GET_Short();
        subglyph->bbox.xMax = GET_Short();
        subglyph->bbox.yMax = GET_Short();

        FORGET_Frame();

        DebugTrace(( "Glyph %d\n", index ));
        DebugTrace(( " # of contours : %d\n", num_contours ));
        DebugTrace(( " xMin: %4d  xMax: %4d\n", 
                     subglyph->xMin, 
                     subglyph->xMax ));
        DebugTrace(( " yMin: %4d  yMax: %4d\n", 
                     subglyph->yMin, 
                     subglyph->yMax ));
        DebugTrace(( "-" ));

        if ( num_contours > left_contours )
        {
          DebugTrace(( "ERROR: Too many contours for glyph %d\n", index ));
          error = TT_Err_Too_Many_Contours;
          goto Fail;
        }

        subglyph->pp1.x = subglyph->bbox.xMin - subglyph->leftBearing;
        subglyph->pp1.y = 0;
        subglyph->pp2.x = subglyph->pp1.x + subglyph->advanceWidth;
        if (load_flags & TTLOAD_SCALE_GLYPH)
        {
          subglyph->pp1.x = Scale_X( &exec->metrics, subglyph->pp1.x );
          subglyph->pp2.x = Scale_X( &exec->metrics, subglyph->pp2.x );
        }

        /* is it a simple glyph ? */
        if ( num_contours > 0 )
          phase = Load_Simple;
        else
          phase = Load_Composite;

        break;


        /************************************************************/
        /*                                                          */
        /* Load_Simple state                                        */
        /*                                                          */
        /*   reading a simple glyph (num_contours must be set to    */
        /*   the glyph's number of contours.)                       */
        /*                                                          */
        /* exit states : Load_End                                   */
        /*                                                          */
 
      case Load_Simple:
        new_flags = load_flags;

        /* disable hinting when scaling */
        if ( !subglyph->is_hinted )
          new_flags &= ~TTLOAD_HINT_GLYPH;

        error = Load_Simple_Glyph( exec,
                                   stream,
                                   num_contours,
                                   left_contours,
                                   left_points,
                                   new_flags,
                                   subglyph );
        if ( error )
          goto Fail;

        /* Note: We could have put the simple loader source there */
        /*       but the code is fat enough already :-)           */

        num_points = exec->pts.n_points - 2;

        phase = Load_End;

        break;


        /************************************************************/
        /*                                                          */
        /* Load_Composite state                                     */
        /*                                                          */
        /*   reading a composite glyph header a pushing a new       */
        /*   load element on the stack.                             */
        /*                                                          */
        /* exit states: Load_Glyph                                  */
        /*                                                          */
 
      case Load_Composite:

        /* create a new element on the stack */
        load_top++;

        if ( load_top > face->maxComponents )
        {
          error = TT_Err_Invalid_Composite;
          goto Fail;
        }

        subglyph2 = exec->loadStack + load_top;

        Init_Glyph_Component( subglyph2, subglyph, NULL );
        subglyph2->is_hinted = subglyph->is_hinted;

        /* now read composite header */

        if ( ACCESS_Frame( 4 ) )
          goto Fail_File;

        subglyph->element_flag = new_flags = GET_UShort();

        subglyph2->index = GET_UShort();

        FORGET_Frame();

        k = 2;

        if ( new_flags & ARGS_ARE_WORDS )
          k += 2;

        if ( new_flags & WE_HAVE_A_SCALE )
          k += 2;

        if ( new_flags & WE_HAVE_AN_XY_SCALE )
          k += 4;

        if ( new_flags & WE_HAVE_A_2X2 )
          k += 8;

        if ( ACCESS_Frame( k ) )
          goto Fail_File;

        if ( new_flags & ARGS_ARE_WORDS )
        {
          k = GET_Short();
          l = GET_Short();
        }
        else
        {
          l = GET_UShort();
          k = (signed char)(l >> 8);
          l = (signed char)(l & 0xFF);
        }

        subglyph->arg1 = k;
        subglyph->arg2 = l;

        if ( new_flags & ARGS_ARE_XY_VALUES )
        {
          subglyph->transform.ox = k;
          subglyph->transform.oy = l;
        }

        xx = 1 << 16;
        xy = 0;
        yx = 0;
        yy = 1 << 16;

        if ( new_flags & WE_HAVE_A_SCALE )
        {
          xx = (Fixed)GET_Short() << 2;
          yy = xx;
          subglyph2->is_scaled = TRUE;
        }
        else if ( new_flags & WE_HAVE_AN_XY_SCALE )
        {
          xx = (Fixed)GET_Short() << 2;
          yy = (Fixed)GET_Short() << 2;
          subglyph2->is_scaled = TRUE;
        }
        else if ( new_flags & WE_HAVE_A_2X2 )
        {
          xx = (Fixed)GET_Short() << 2;
          xy = (Fixed)GET_Short() << 2;
          yx = (Fixed)GET_Short() << 2;
          yy = (Fixed)GET_Short() << 2;
          subglyph2->is_scaled = TRUE;
        }

        FORGET_Frame();

        subglyph->transform.xx = xx;
        subglyph->transform.xy = xy;
        subglyph->transform.yx = yx;
        subglyph->transform.yy = yy;

        k = MulDiv_Round( xx, yy, 1 << 16 ) -
            MulDiv_Round( xy, yx, 1 << 16 );

 
        /* disable hinting in case of scaling/slanting */
        if ( k < -(1 << 16) || k > (1 << 16) )
          subglyph2->is_hinted = FALSE;
 
        subglyph->file_offset = FILE_Pos();

        phase = Load_Glyph;

        break;


        /************************************************************/
        /*                                                          */
        /* Load_End state                                           */
        /*                                                          */
        /*   after loading a glyph, apply transformation and offset */
        /*   where necessary, pops element and continue or          */
        /*   stop process.                                          */
        /*                                                          */
        /* exit states : Load_Composite and Load_Exit               */
        /*                                                          */
 
      case Load_End:
        if ( load_top > 0 )
        {
          subglyph2 = subglyph;

          load_top--;
          subglyph = exec->loadStack + load_top;

          /* check advance width and left side bearing */

          if ( !subglyph->preserve_pps &&
               subglyph->element_flag & USE_MY_METRICS )
          {
            subglyph->leftBearing  = subglyph2->leftBearing;
            subglyph->advanceWidth = subglyph2->advanceWidth;

            subglyph->pp1 = subglyph2->pp1;
            subglyph->pp2 = subglyph2->pp2;

            subglyph->preserve_pps = TRUE;
          }

          /* apply scale */

          if ( subglyph2->is_scaled )
          {
            for ( k = 0; k < num_points; k++ )
            {
              x = subglyph2->zone.cur_x[k];
              y = subglyph2->zone.cur_y[k];

              nx = MulDiv_Round( x, subglyph->transform.xx, 1 << 16 ) +
                   MulDiv_Round( y, subglyph->transform.yx, 1 << 16 );

              ny = MulDiv_Round( x, subglyph->transform.xy, 1 << 16 ) +
                   MulDiv_Round( y, subglyph->transform.yy, 1 << 16 );

              subglyph2->zone.cur_x[k] = nx;
              subglyph2->zone.cur_y[k] = ny;

              x = subglyph2->zone.org_x[k];
              y = subglyph2->zone.org_y[k];

              nx = MulDiv_Round( x, subglyph->transform.xx, 1 << 16 ) +
                   MulDiv_Round( y, subglyph->transform.yx, 1 << 16 );

              ny = MulDiv_Round( x, subglyph->transform.xy, 1 << 16 ) +
                   MulDiv_Round( y, subglyph->transform.yy, 1 << 16 );

              subglyph2->zone.org_x[k] = nx;
              subglyph2->zone.org_y[k] = ny;
            }
          }

          /* adjust counts */

          for ( k = 0; k < num_contours; k++ )
            subglyph2->zone.contours[k] += subglyph->zone.n_points;

          subglyph->zone.n_points   += num_points;
          subglyph->zone.n_contours += num_contours;

          left_points   -= num_points;
          left_contours -= num_contours;

          /* apply offset */

          if ( !(subglyph->element_flag & ARGS_ARE_XY_VALUES) )
          {
            k = subglyph->arg1;
            l = subglyph->arg2;

            if ( k < 0 || k >= subglyph->zone.n_points ||
                 l < 0 || l >= subglyph->zone.n_points )
            {
              error = TT_Err_Invalid_Composite;
              goto Fail;
            }

            x = subglyph->zone.org_x[l] - subglyph->zone.org_x[k];
            y = subglyph->zone.org_y[l] - subglyph->zone.org_y[k];
          }
          else
          {
            x = subglyph->transform.ox;
            y = subglyph->transform.oy;

            if (load_flags & TTLOAD_SCALE_GLYPH)
            {
              x = Scale_X( &exec->metrics, x );
              y = Scale_Y( &exec->metrics, y );

              if (subglyph->element_flag & ROUND_XY_TO_GRID)
              {
                x = (x+32) & -64;
                y = (y+32) & -64;
              }
            }
          }

          translate_array( num_points, subglyph2->zone.cur_x, x );
          translate_array( num_points, subglyph2->zone.cur_y, y );

          cur_to_org( num_points, &subglyph2->zone );

          num_points   = subglyph->zone.n_points;
          num_contours = subglyph->zone.n_contours;

          /* check for last component */

          if ( FILE_Seek( subglyph->file_offset ) )
            goto Fail_File;

          if ( subglyph->element_flag & MORE_COMPONENTS )
            phase = Load_Composite;
          else
          {
            error = Load_Composite_End( num_points,
                                        num_contours,
                                        exec,
                                        subglyph );
            if ( error )
              goto Fail;

            phase = Load_End;
          }
        }
        else
          phase = Load_Exit;

        break;


      case Load_Exit:
        break;
      } 
    }

    /* finally, copy the points arrays to the glyph object */

    exec->pts = base_pts;

    num_points += 2;

    for ( k = 0; k < num_points - 1; k++ )
    {
      glyph->outline.xCoord[k] = exec->pts.cur_x[k];
      glyph->outline.yCoord[k] = exec->pts.cur_y[k];
      glyph->outline.flag  [k] = exec->pts.touch[k];
    }

    for ( k = 0; k < num_contours; k++ )
      glyph->outline.conEnds[k] = exec->pts.contours[k];

    glyph->outline.points      = num_points-2;
    glyph->outline.contours    = num_contours;
    glyph->outline.second_pass = TRUE;

    /* translate array so that (0,0) is the glyph's origin */
    translate_array( num_points, glyph->outline.xCoord, -subglyph->pp1.x );

    TT_Get_Outline_BBox( &glyph->outline, &glyph->metrics.bbox );

    if (subglyph->is_hinted)
    {
      /* grid-fit the bounding box */
      glyph->metrics.bbox.xMin &= -64;
      glyph->metrics.bbox.yMin &= -64;
      glyph->metrics.bbox.xMax  = (glyph->metrics.bbox.xMax+63) & -64;
      glyph->metrics.bbox.yMax  = (glyph->metrics.bbox.yMax+63) & -64;
      glyph->metrics.bbox.xMax &= -64;
      glyph->metrics.bbox.yMax &= -64;
    }

    glyph->metrics.bearingX = glyph->metrics.bbox.xMin;
    glyph->metrics.bearingY = glyph->metrics.bbox.yMax;
    glyph->metrics.advance  = subglyph->pp2.x - subglyph->pp1.x;

    /* Adjust advance width to the value contained in the hdmx table. */
    if ( !exec->owner->postscript.isFixedPitch && instance &&
         subglyph->is_hinted )
    {
      widths = Get_Advance_Widths( exec->owner,
                                   exec->instance->metrics.x_ppem );
      if ( widths )
        subglyph->advanceWidth = widths[glyph_index] << 6;
    }

    glyph->outline.dropout_mode = exec->GS.scan_type;

    error = TT_Err_Ok;

  Fail_File:
  Fail:
    TT_Done_Stream( &stream );

  Fin:

    /* reset the execution context */

    exec->pts = base_pts;

    if ( !instance || !instance->debug )
      Done_Context( exec );

    return error;
  }


/* END */
