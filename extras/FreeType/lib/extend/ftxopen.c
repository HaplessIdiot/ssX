/*******************************************************************
 *
 *  ftxopen.c
 *
 *    TrueType Open common table support.
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

#include "tttypes.h"
#include "ttload.h"
#include "ttextend.h"
#include "ttmemory.h"
#include "ttfile.h"

#include "ftxopen.h"
#include "ftxopenf.h"


  /***************************
   * Script related functions
   ***************************/


  /* LangSys */

  static TT_Error  Load_LangSys( TTO_LangSys*  ls,
                                 PFace         input )
  {
    DEFINE_LOAD_LOCALS( input->stream );

    UShort  n, count;


    if ( ACCESS_Frame( 6L ) )
      return error;

    ls->LookupOrderOffset    = GET_UShort();    /* should be 0 */
    ls->ReqFeatureIndex      = GET_UShort();
    count = ls->FeatureCount = GET_UShort();

    FORGET_Frame();

    ls->FeatureIndex = NULL;

    if ( ALLOC_ARRAY( ls->FeatureIndex, count, TT_UShort ) )
      return error;

    if ( ACCESS_Frame( count * 2L ) )
      return error;

    for ( n = 0; n < count; n++ )
      ls->FeatureIndex[n] = GET_UShort();

    FORGET_Frame();

    return TT_Err_Ok;
  }


  static void  Free_LangSys( TTO_LangSys*  ls )
  {
    if ( ls->FeatureIndex )
      FREE( ls->FeatureIndex );
  }


  /* Script */  

  static TT_Error  Load_Script( TTO_Script*  s,
                                PFace        input )
  {
    DEFINE_LOAD_LOCALS( input->stream );

    UShort  n, count;
    ULong   cur_offset, new_offset, base_offset;


    base_offset = FILE_Pos();

    if ( ACCESS_Frame( 2L ) )
      return error;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    if ( new_offset != base_offset )        /* not a NULL offset */
    {
      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_LangSys( &s->DefaultLangSys,
                                   input ) ) != TT_Err_Ok )
        return error;
      (void)FILE_Seek( cur_offset );
    }
    else
    {
      /* we create a DefaultLangSys table with no entries */

      s->DefaultLangSys.LookupOrderOffset = 0;
      s->DefaultLangSys.ReqFeatureIndex   = 0xFFFF;
      s->DefaultLangSys.FeatureCount      = 0;
      s->DefaultLangSys.FeatureIndex      = NULL;
    }

    if ( ACCESS_Frame( 2L ) )
      return error;

    count = s->LangSysCount = GET_UShort();

    FORGET_Frame();

    s->LangSysRecord = NULL;

    if ( ALLOC_ARRAY( s->LangSysRecord, count, TTO_LangSysRecord ) )
      return error;

    for ( n = 0; n < count; n++ )
    {
      if ( ACCESS_Frame( 6L ) )
        return error;

      s->LangSysRecord[n].LangSysTag = GET_ULong();
      new_offset = GET_UShort() + base_offset;

      FORGET_Frame();

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_LangSys( &s->LangSysRecord[n].LangSys,
                                   input ) ) != TT_Err_Ok )
        return error;
      (void)FILE_Seek( cur_offset );
    }

    return TT_Err_Ok;
  }


  static void  Free_Script( TTO_Script*  s )
  {
    UShort  n, count;


    Free_LangSys( &s->DefaultLangSys );

    if ( s->LangSysRecord )
    {
      count = s->LangSysCount;

      for ( n = 0; n < count; n++ )
        Free_LangSys( &s->LangSysRecord[n].LangSys );

      FREE( s->LangSysRecord );
    }
  }


  /* ScriptList */

  TT_Error  Load_ScriptList( TTO_ScriptList*  sl,
                             PFace            input )
  {
    DEFINE_LOAD_LOCALS( input->stream );

    UShort  n, count;
    ULong   cur_offset, new_offset, base_offset;


    if ( ACCESS_Frame( 2L ) )
      return error;

    /* get real offset */
    
    base_offset = FILE_Pos() + GET_UShort() - 6L;

    FORGET_Frame();

    if ( FILE_Seek ( base_offset ) ||
         ACCESS_Frame( 2L ) )
      return error;

    count = sl->ScriptCount = GET_UShort();

    FORGET_Frame();

    sl->ScriptRecord = NULL;

    if ( ALLOC_ARRAY( sl->ScriptRecord, count, TTO_ScriptRecord ) )
      return error;

    for ( n = 0; n < count; n++ )
    {
      if ( ACCESS_Frame( 6L ) )
        return error;

      sl->ScriptRecord[n].ScriptTag = GET_ULong();
      new_offset = GET_UShort() + base_offset;

      FORGET_Frame();

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_Script( &sl->ScriptRecord[n].Script,
                                  input ) ) != TT_Err_Ok )
        return error;
      (void)FILE_Seek( cur_offset );
    }

    return TT_Err_Ok;
  }


  void  Free_ScriptList( TTO_ScriptList*  sl )
  {
    UShort  n, count;


    if ( sl->ScriptRecord )
    {
      count = sl->ScriptCount;

      for ( n = 0; n < count; n++ )
        Free_Script( &sl->ScriptRecord[n].Script );

      FREE( sl->ScriptRecord );
    }
  }



  /*********************************
   * Feature List related functions
   *********************************/


  /* Feature */

  static TT_Error  Load_Feature( TTO_Feature*  f,
                                 PFace         input )
  {
    DEFINE_LOAD_LOCALS( input->stream );

    UShort  n, count;


    if ( ACCESS_Frame( 4L ) )
      return error;

    f->FeatureParams           = GET_UShort();    /* should be 0 */
    count = f->LookupListCount = GET_UShort();

    FORGET_Frame();

    f->LookupListIndex = NULL;

    if ( ALLOC_ARRAY( f->LookupListIndex, count, TT_UShort ) )
      return error;

    if ( ACCESS_Frame( count * 2L ) )
      return error;

    for ( n = 0; n < count; n++ )
      f->LookupListIndex[n] = GET_UShort();

    FORGET_Frame();

    return TT_Err_Ok;
  }


  static void  Free_Feature( TTO_Feature*  f )
  {
    if ( f->LookupListIndex )
      FREE( f->LookupListIndex );
  }


  /* FeatureList */

  TT_Error  Load_FeatureList( TTO_FeatureList*  fl,
                              PFace             input )
  {
    DEFINE_LOAD_LOCALS( input->stream );

    UShort  n, count;
    ULong   cur_offset, new_offset, base_offset;


    if ( ACCESS_Frame( 2L ) )
      return error;

    /* get real offset */

    base_offset = FILE_Pos() + GET_UShort() - 8L;

    FORGET_Frame();

    if ( FILE_Seek( base_offset ) ||
         ACCESS_Frame( 2L ) )
      return error;

    count = fl->FeatureCount = GET_UShort();

    FORGET_Frame();

    fl->FeatureRecord = NULL;

    if ( ALLOC_ARRAY( fl->FeatureRecord, count, TTO_FeatureRecord ) )
      return error;

    for ( n = 0; n < count; n++ )
    {
      if ( ACCESS_Frame( 6L ) )
        return error;

      fl->FeatureRecord[n].FeatureTag = GET_ULong();
      new_offset = GET_UShort() + base_offset;

      FORGET_Frame();

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_Feature( &fl->FeatureRecord[n].Feature,
                                   input ) ) != TT_Err_Ok )
        return error;
      (void)FILE_Seek( cur_offset );
    }

    return TT_Err_Ok;
  }


  void  Free_FeatureList( TTO_FeatureList*  fl )
  {
    UShort  n, count;


    if ( fl->FeatureRecord )
    {
      count = fl->FeatureCount;

      for ( n = 0; n < count; n++ )
        Free_Feature( &fl->FeatureRecord[n].Feature );

      FREE( fl->FeatureRecord );
    }
  }



  /********************************
   * Lookup List related functions
   ********************************/

  /* the subroutines of the following two functions are defined in
     ftxgsub.c and ftxgpos.c                                       */


  /* SubTable */

  static TT_Error  Load_SubTable( TTO_SubTable*  st,
                                  PFace          input,
                                  TTO_LoadType   table_type,
                                  UShort         lookup_type )
  {
    if ( table_type == Load_GSUB )
      switch ( lookup_type )
      {
      case LOOKUP_SINGLE:
        return Load_LookupSingle( &st->st.gsub.single, input );
      case LOOKUP_MULTIPLE:
        return Load_LookupMultiple( &st->st.gsub.multiple, input );
      case LOOKUP_ALTERNATE:
        return Load_LookupAlternate( &st->st.gsub.alternate, input );
      case LOOKUP_LIGATURE:
        return Load_LookupLigature( &st->st.gsub.ligature, input );
      case LOOKUP_CONTEXT:
        return Load_LookupContext( &st->st.gsub.context, input );
      default:
        return TTO_Err_Invalid_GSUB_SubTable_Format;
      }
    else
      ;                                 /* not implemented yet */

    return TT_Err_Ok;
  }


  static void  Free_SubTable( TTO_SubTable*  st,
                              TTO_LoadType   table_type,
                              UShort         lookup_type )
  {
    if ( table_type == Load_GSUB )
      switch ( lookup_type )
      {
      case LOOKUP_SINGLE:
        Free_LookupSingle( &st->st.gsub.single );
        break;
      case LOOKUP_MULTIPLE:
        Free_LookupMultiple( &st->st.gsub.multiple );
        break;
      case LOOKUP_ALTERNATE:
        Free_LookupAlternate( &st->st.gsub.alternate );
        break;
      case LOOKUP_LIGATURE:
        Free_LookupLigature( &st->st.gsub.ligature );
        break;
      case LOOKUP_CONTEXT:
        Free_LookupContext( &st->st.gsub.context );
        break;
      default:
        break;
      }
    else
      ;                                 /* not implemented yet */
  }


  /* Lookup */

  static TT_Error  Load_Lookup( TTO_Lookup*   l,
                                PFace         input,
                                TTO_LoadType  type )
  {
    DEFINE_LOAD_LOCALS( input->stream );

    UShort  n, count;
    ULong   cur_offset, new_offset, base_offset;


    base_offset = FILE_Pos();

    if ( ACCESS_Frame( 6L ) )
      return error;

    l->LookupType            = GET_UShort();
    l->LookupFlag            = GET_UShort();
    count = l->SubTableCount = GET_UShort();

    FORGET_Frame();

    l->SubTable = NULL;

    if ( ALLOC_ARRAY( l->SubTable, count, TTO_SubTable ) )
      return error;

    for ( n = 0; n < count; n++ )
    {
      if ( ACCESS_Frame( 2L ) )
        return error;

      new_offset = GET_UShort() + base_offset;

      FORGET_Frame();

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_SubTable( &l->SubTable[n], input,
                                    type, l->LookupType ) ) != TT_Err_Ok )
        return error;
      (void)FILE_Seek( cur_offset );
    }

    return TT_Err_Ok;
  }


  static void  Free_Lookup( TTO_Lookup*   l,
                            TTO_LoadType  type )
  {
    UShort  n, count;


    if ( l->SubTable )
    {
      count = l->SubTableCount;

      for ( n = 0; n < count; n++ )
        Free_SubTable( &l->SubTable[n], type, l->LookupType );

      FREE( l->SubTable );
    }
  }


  /* LookupList */

  TT_Error  Load_LookupList( TTO_LookupList*  ll,
                             PFace            input,
                             TTO_LoadType     type )
  {
    DEFINE_LOAD_LOCALS( input->stream );

    UShort  n, count;
    ULong   cur_offset, new_offset, base_offset;


    if ( ACCESS_Frame( 2L ) )
      return error;

    /* get real offset */

    base_offset = FILE_Pos() + GET_UShort() - 10L;

    FORGET_Frame();

    if ( FILE_Seek( base_offset ) ||
         ACCESS_Frame( 2L ) )
      return error;

    count = ll->LookupCount = GET_UShort();

    FORGET_Frame();

    ll->Lookup = NULL;

    if ( ALLOC_ARRAY( ll->Lookup, count, TTO_Lookup ) )
      return error;

    for ( n = 0; n < count; n++ )
    {
      if ( ACCESS_Frame( 2L ) )
        return error;

      new_offset = GET_UShort() + base_offset;

      FORGET_Frame();

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_Lookup( &ll->Lookup[n],
                                  input, type ) ) != TT_Err_Ok )
        return error;
      (void)FILE_Seek( cur_offset );
    }

    return TT_Err_Ok;
  }


  void  Free_LookupList( TTO_LookupList*  ll,
                         TTO_LoadType     type )
  {
    UShort  n, count;


    if ( ll->Lookup )
    {
      count = ll->LookupCount;

      for ( n = 0; n < count; n++ )
        Free_Lookup( &ll->Lookup[n], type );

      FREE( ll->Lookup );
    }
  }



  /*****************************
   * Coverage related functions
   *****************************/


  /* CoverageFormat1 */

  static TT_Error  Load_Coverage1( TTO_CoverageFormat1*  cf1,
                                   PFace                 input )
  {
    DEFINE_LOAD_LOCALS( input->stream );

    UShort  n, count;


    if ( ACCESS_Frame( 2L ) )
      return error;

    count = cf1->GlyphCount = GET_UShort();

    FORGET_Frame();

    cf1->GlyphArray = NULL;

    if ( ALLOC_ARRAY( cf1->GlyphArray, count, TT_UShort ) )
      return error;

    if ( ACCESS_Frame( count * 2L ) )
      return error;

    for ( n = 0; n < count; n++ )
      cf1->GlyphArray[n] = GET_UShort();

    FORGET_Frame();

    return TT_Err_Ok;
  }


  static void  Free_Coverage1( TTO_CoverageFormat1*  cf1 )
  {
    if ( cf1->GlyphArray )
      FREE( cf1->GlyphArray );
  }


  /* CoverageFormat2 */

  static TT_Error  Load_Coverage2( TTO_CoverageFormat2*  cf2,
                                   PFace                 input )
  {
    DEFINE_LOAD_LOCALS( input->stream );

    UShort  n, count;


    if ( ACCESS_Frame( 2L ) )
      return error;

    count = cf2->RangeCount = GET_UShort();

    FORGET_Frame();

    cf2->RangeRecord = NULL;

    if ( ALLOC_ARRAY( cf2->RangeRecord, count, TTO_RangeRecord ) )
      return error;

    if ( ACCESS_Frame( count * 6L ) )
      return error;

    for ( n = 0; n < count; n++ )
    {
      cf2->RangeRecord[n].Start              = GET_UShort();
      cf2->RangeRecord[n].End                = GET_UShort();
      cf2->RangeRecord[n].StartCoverageIndex = GET_UShort();
    }

    FORGET_Frame();

    return TT_Err_Ok;
  }


  static void  Free_Coverage2( TTO_CoverageFormat2*  cf2 )
  {
    if ( cf2->RangeRecord )
      FREE( cf2->RangeRecord );
  }


  TT_Error  Load_Coverage( TTO_Coverage*  c,
                           PFace          input )
  {
    DEFINE_LOAD_LOCALS( input->stream );


    if ( ACCESS_Frame( 2L ) )
      return error;

    c->CoverageFormat = GET_UShort();

    FORGET_Frame();

    switch ( c->CoverageFormat )
    {
    case 1:
      return Load_Coverage1( &c->cf.cf1, input );
    case 2:
      return Load_Coverage2( &c->cf.cf2, input );
    default:
      return TTO_Err_Invalid_SubTable_Format;
    }

    return TT_Err_Ok;               /* never reached */
  }


  void  Free_Coverage( TTO_Coverage*  c )
  {
    switch ( c->CoverageFormat )
    {
    case 1:
      Free_Coverage1( &c->cf.cf1 );
      break;
    case 2:
      Free_Coverage2( &c->cf.cf2 );
      break;
    default:
      break;
    }
  }



  /*************************************
   * Class Definition related functions
   *************************************/


  /* ClassDefFormat1 */

  static TT_Error  Load_ClassDef1 ( TTO_ClassDefFormat1*  cdf1,
                                    PFace                 input )
  {
    DEFINE_LOAD_LOCALS( input->stream );

    UShort  n, count;


    if ( ACCESS_Frame( 4L ) )
      return error;

    cdf1->StartGlyph         = GET_UShort();
    count = cdf1->GlyphCount = GET_UShort();

    FORGET_Frame();

    cdf1->ClassValueArray = NULL;

    if ( ALLOC_ARRAY( cdf1->ClassValueArray, count, TT_UShort ) )
      return error;

    if ( ACCESS_Frame( count * 2L ) )
      return error;

    for ( n = 0; n < count; n++ )
      cdf1->ClassValueArray[n] = GET_UShort();

    FORGET_Frame();

    return TT_Err_Ok;
  }


  static void  Free_ClassDef1( TTO_ClassDefFormat1*  cdf1 )
  {
    if ( cdf1->ClassValueArray )
      FREE( cdf1->ClassValueArray );
  }


  /* ClassDefFormat2 */

  static TT_Error  Load_ClassDef2 ( TTO_ClassDefFormat2*  cdf2,
                                    PFace                 input )
  {
    DEFINE_LOAD_LOCALS( input->stream );

    UShort  n, count;


    if ( ACCESS_Frame( 2L ) )
      return error;

    count = cdf2->ClassRangeCount = GET_UShort();

    FORGET_Frame();

    cdf2->ClassRangeRecord = NULL;

    if ( ALLOC_ARRAY( cdf2->ClassRangeRecord, count, TTO_ClassRangeRecord ) )
      return error;

    if ( ACCESS_Frame( count * 6L ) )
      return error;

    for ( n = 0; n < count; n++ )
    {
      cdf2->ClassRangeRecord[n].Start = GET_UShort();
      cdf2->ClassRangeRecord[n].End   = GET_UShort();
      cdf2->ClassRangeRecord[n].Class = GET_UShort();
    }

    FORGET_Frame();

    return TT_Err_Ok;
  }


  static void  Free_ClassDef2( TTO_ClassDefFormat2*  cdf2 )
  {
    if ( cdf2->ClassRangeRecord )
      FREE( cdf2->ClassRangeRecord );
  }


  /* ClassDefinition */

  TT_Error  Load_ClassDefinition( TTO_ClassDefinition*  cd,
                                  PFace                 input )
  {
    DEFINE_LOAD_LOCALS( input->stream );


    if ( ACCESS_Frame( 2L ) )
      return error;

    cd->ClassFormat = GET_UShort();

    FORGET_Frame();

    switch ( cd->ClassFormat )
    {
    case 1:
      return Load_ClassDef1( &cd->cd.cd1, input );
    case 2:
      return Load_ClassDef2( &cd->cd.cd2, input );
    default:
      return TTO_Err_Invalid_SubTable_Format;
    }

    return TT_Err_Ok;               /* never reached */
  }


  void  Free_ClassDefinition( TTO_ClassDefinition*  cd )
  {
    switch ( cd->ClassFormat )
    {
    case 1:
      Free_ClassDef1( &cd->cd.cd1 );
      break;
    case 2:
      Free_ClassDef2( &cd->cd.cd2 );
      break;
    default:
      break;
    }
  }


/* END */
