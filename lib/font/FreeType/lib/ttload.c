/*******************************************************************
 *
 *  ttload.c                                                    1.0
 *
 *    TrueType Tables Loader.                          
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

#include "ttmemory.h"
#include "tttags.h"
#include "ttload.h"


/* In all functions, the stream is taken from the 'face' object */
#define DEFINE_LOCALS           DEFINE_LOAD_LOCALS( face->stream )
#define DEFINE_LOCALS_WO_FRAME  DEFINE_LOAD_LOCALS_WO_FRAME( face->stream )


/*******************************************************************
 *
 *  Function    :  LookUp_TrueType_Table
 *
 *  Description :  Looks for a TrueType table by name.
 *
 *  Input  :  face   face table to look for
 *            tag        searched tag
 *
 *  Output :  Index of table if found, -1 otherwise.
 *
 ******************************************************************/

  Int  LookUp_TrueType_Table( PFace  face,
                              Long   tag  )
  {
    Int  i;


    for ( i = 0; i < face->numTables; i++ )
      if ( face->dirTables[i].Tag == tag )
        return i;

    return -1;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_Collection
 *
 *  Description :  Loads the TTC table directory into face table.
 *
 *  Input  :  face    face record to look for
 *
 *  Output :  Error code.                            
 *
 ******************************************************************/

  static TT_Error  Load_TrueType_Collection( PFace  face )
  {
    DEFINE_LOCALS;

    Int  n;


    if ( FILE_Seek   ( 0L ) ||
         ACCESS_Frame( 12L ) )
      return error;

    face->ttcHeader.Tag      = GET_Tag4();
    face->ttcHeader.version  = GET_Long();
    face->ttcHeader.DirCount = GET_Long();

    FORGET_Frame();

    if ( face->ttcHeader.Tag != TTAG_ttcf )
    {
      face->ttcHeader.Tag      = 0;
      face->ttcHeader.version  = 0;
      face->ttcHeader.DirCount = 0;

      face->ttcHeader.TableDirectory = NULL;

      return TT_Err_File_Is_Not_Collection;
    }

    if ( ALLOC_ARRAY( face->ttcHeader.TableDirectory,
                      face->ttcHeader.DirCount, 
                      ULong )                         ||
         ACCESS_Frame( face->ttcHeader.DirCount * 4L ) )
      return error;

    for ( n = 0; n < face->ttcHeader.DirCount; n++ )
      face->ttcHeader.TableDirectory[n] = GET_ULong();

    FORGET_Frame();

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_Directory
 *
 *  Description :  Loads the table directory into face table.
 *
 *  Input  :  face       face record to look for
 *
 *            faceIndex  the index of the TrueType font, when
 *                       we're opening a collection.
 *
 *  Output :  SUCCESS on success.  FAILURE on error.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_Directory( PFace  face, int  faceIndex )
  {
    DEFINE_LOCALS;

    Int        n, limit;
    TTableDir  tableDir;


    DebugTrace(( "Directory " ));

    error = Load_TrueType_Collection( face );

    if ( error )
    {
      if ( error != TT_Err_File_Is_Not_Collection )
        return error;

      /* the file isn't a collection, exit if we're asking */
      /* for a collected font                              */
      if ( faceIndex != 0 )
        return error;

      /* Now skip to the beginning of the file */
      if ( FILE_Seek( 0 ) )
        return error;
    }
    else
    {
      /* The file is a collection. Check the font index */
      if ( faceIndex < 0 || faceIndex >= face->ttcHeader.DirCount )
        return TT_Err_Bad_Argument;

      /* select a TrueType font in the ttc file   */
      if ( FILE_Seek( face->ttcHeader.TableDirectory[faceIndex] ) )
        return error;
    }

    if ( ACCESS_Frame( 12L ) )
      return error;

    tableDir.version   = GET_Long();
    tableDir.numTables = GET_UShort();

    tableDir.searchRange   = GET_UShort();
    tableDir.entrySelector = GET_UShort();
    tableDir.rangeShift    = GET_UShort();

    DebugTrace(( "Tables number : %12u\n", tableDir.numTables ));

    FORGET_Frame();

    /* Check that we have a 'sfnt' format there */

    if ( tableDir.version != 0x00010000 )
      return TT_Err_Invalid_File_Format;

    face->numTables = tableDir.numTables;

    if ( ALLOC_ARRAY( face->dirTables,
                      face->numTables, 
                      TTableDirEntry ) )
      return error;

    if ( ACCESS_Frame( face->numTables * 16L ) )
      return error;

    limit = face->numTables;
    for ( n = 0; n < limit; n++ )
    {                      /* loop through the tables and get all entries */
      face->dirTables[n].Tag      = GET_Tag4();
      face->dirTables[n].CheckSum = GET_ULong();
      face->dirTables[n].Offset   = GET_Long();
      face->dirTables[n].Length   = GET_Long();
    }

    FORGET_Frame();

    DebugTrace(( "loaded\n" ));

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_MaxProfile
 *
 *  Description :  Loads the maxp table into face table.
 *
 *  Input  :  face     face table to look for
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_MaxProfile( PFace  face )
  {
    DEFINE_LOCALS;

    Int          i;
    PMaxProfile  maxProfile = &face->maxProfile;


    DebugTrace(( "MaxProfile " ));

    if ( ( i = LookUp_TrueType_Table( face, TTAG_maxp ) ) < 0 )
      return TT_Err_Max_Profile_Missing;

    if ( FILE_Seek( face->dirTables[i].Offset ) )   /* seek to maxprofile */
      return error;

    if ( ACCESS_Frame( 32L ) )  /* read into frame */
      return error;

    /* read frame data into face table */
    maxProfile->version               = GET_ULong();

    maxProfile->numGlyphs             = GET_UShort();

    maxProfile->maxPoints             = GET_UShort();
    maxProfile->maxContours           = GET_UShort();
    maxProfile->maxCompositePoints    = GET_UShort();
    maxProfile->maxCompositeContours  = GET_UShort();

    maxProfile->maxZones              = GET_UShort();
    maxProfile->maxTwilightPoints     = GET_UShort();

    maxProfile->maxStorage            = GET_UShort();
    maxProfile->maxFunctionDefs       = GET_UShort();
    maxProfile->maxInstructionDefs    = GET_UShort();
    maxProfile->maxStackElements      = GET_UShort();
    maxProfile->maxSizeOfInstructions = GET_UShort();
    maxProfile->maxComponentElements  = GET_UShort();
    maxProfile->maxComponentDepth     = GET_UShort();

    FORGET_Frame();

    face->numGlyphs     = maxProfile->numGlyphs;

    face->maxPoints     = MAX( maxProfile->maxCompositePoints, 
                               maxProfile->maxPoints );
    face->maxContours   = MAX( maxProfile->maxCompositeContours,
                               maxProfile->maxContours );
    face->maxComponents = maxProfile->maxComponentElements +
                          maxProfile->maxComponentDepth;

    DebugTrace(( "loaded\n" ));
  
    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_Gasp
 *
 *  Description :  Loads the TrueType Gasp table into the face
 *                 table.
 *
 *  Input  :  face     face table to look for
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_Gasp( PFace  face )
  {
    DEFINE_LOCALS;

    Int         i;
    TGasp*      gas;
    GaspRange*  gaspranges;


    DebugTrace(( "Gasp " ));

    if ( ( i = LookUp_TrueType_Table( face, TTAG_gasp ) ) < 0 )
      return TT_Err_Ok; /* gasp table is not required */

    if ( FILE_Seek( face->dirTables[i].Offset ) ||
         ACCESS_Frame( 4L ) )
      return error;

    gas = &face->gasp;

    gas->version   = GET_UShort();
    gas->numRanges = GET_UShort();

    FORGET_Frame();

    DebugTrace(( "numRanges=%d\n" ,gas->numRanges ));

    if ( ALLOC_ARRAY( gaspranges, gas->numRanges, GaspRange ) ||
         ACCESS_Frame( gas->numRanges * 4L ) )
      goto Fail;

    face->gasp.gaspRanges = gaspranges;

    for ( i = 0; i < gas->numRanges; i++ )
    {
      gaspranges[i].maxPPEM  = GET_UShort();
      gaspranges[i].gaspFlag = GET_UShort();
      DebugTrace(( "%d %d\n",
                   gaspranges[i].maxPPEM, gaspranges[i].gaspFlag ));
    }

    FORGET_Frame();

    return TT_Err_Ok;

  Fail:
    FREE( gaspranges );
    gas->numRanges = 0;
    return error;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_Header
 *
 *  Description :  Loads the TrueType header table into the face
 *                 table.
 *
 *  Input  :  face     face table to look for
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_Header( PFace  face )
  {
    DEFINE_LOCALS;

    Int         i;
    TT_Header*  header;


    DebugTrace(( "Header " ));

    if ( ( i = LookUp_TrueType_Table( face, TTAG_head ) ) < 0 )
      return TT_Err_Header_Table_Missing;

    if ( FILE_Seek( face->dirTables[i].Offset ) ||
         ACCESS_Frame( 54L ) )
      return error;

    header = &face->fontHeader;

    header->Table_Version = GET_ULong();
    header->Font_Revision = GET_ULong();

    header->CheckSum_Adjust = GET_Long();
    header->Magic_Number    = GET_Long();

    header->Flags        = GET_UShort();
    header->Units_Per_EM = GET_UShort();

    header->Created [0] = GET_Long();
    header->Created [1] = GET_Long();
    header->Modified[0] = GET_Long();
    header->Modified[1] = GET_Long();

    header->xMin = GET_Short();
    header->yMin = GET_Short();
    header->xMax = GET_Short();
    header->yMax = GET_Short();

    header->Mac_Style       = GET_UShort();
    header->Lowest_Rec_PPEM = GET_UShort();

    header->Font_Direction      = GET_Short();
    header->Index_To_Loc_Format = GET_Short();
    header->Glyph_Data_Format   = GET_Short();

    DebugTrace(( "loaded\n" ));

    DebugTrace(( "Units per EM       : %12u\n", 
                 face->fontHeader.Units_Per_EM ));

    DebugTrace(( "IndexToLocFormat   : %12d\n",
                 face->fontHeader.Index_To_Loc_Format ));

    FORGET_Frame();

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    : Load_TrueType_Horizontal_Header
 *
 *  Description : Loads the hhea table into face table.
 *
 *  Input  :  face   face table to look for
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_Horizontal_Header( PFace  face )
  {
    DEFINE_LOCALS;

    Int  i;

    TT_Horizontal_Header*  hheader;


    DebugTrace(( "Horizontal header " ));

    if ( ( i = LookUp_TrueType_Table( face, TTAG_hhea ) ) < 0 )
      return TT_Err_Horiz_Header_Missing;

    if ( FILE_Seek( face->dirTables[i].Offset ) ||
         ACCESS_Frame( 36L ) )
      return error;

    hheader = &face->horizontalHeader;

    hheader->Version   = GET_ULong();
    hheader->Ascender  = GET_Short();
    hheader->Descender = GET_Short();
    hheader->Line_Gap  = GET_Short();

    hheader->advance_Width_Max = GET_UShort();

    hheader->min_Left_Side_Bearing  = GET_Short();
    hheader->min_Right_Side_Bearing = GET_Short();
    hheader->xMax_Extent            = GET_Short();
    hheader->caret_Slope_Rise       = GET_Short();
    hheader->caret_Slope_Run        = GET_Short();

    hheader->Reserved[0] = GET_Short();
    hheader->Reserved[1] = GET_Short();
    hheader->Reserved[2] = GET_Short();
    hheader->Reserved[3] = GET_Short();
    hheader->Reserved[4] = GET_Short();

    hheader->metric_Data_Format = GET_Short();
    hheader->number_Of_HMetrics = GET_UShort();

    FORGET_Frame();

    DebugTrace(( "loaded\n" ));

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_Locations
 *
 *  Description :  Loads the location table into face table.
 *
 *  Input  :  face     face table to look for
 *
 *  Output :  Error code.
 *
 *  NOTE:
 *    The Font Header *must* be loaded in the leading segment
 *    calling this function.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_Locations( PFace  face )
  {
    DEFINE_LOCALS;

    Int  n, limit, LongOffsets;


    DebugTrace(( "Locations " ));

    LongOffsets = face->fontHeader.Index_To_Loc_Format;

    if ( ( n = LookUp_TrueType_Table( face, TTAG_loca ) ) < 0 )
      return TT_Err_Locations_Missing;

    if ( FILE_Seek( face->dirTables[n].Offset ) )
      return error;

    if ( LongOffsets != 0 )
    {
      face->numLocations =
        (unsigned long)face->dirTables[n].Length >> 2;

      DebugTrace(( "(32 bits offsets): %12d ",
                   face->numLocations ));

      if ( ALLOC_ARRAY( face->glyphLocations,
                        face->numLocations,
                        Long ) )
        return error;

      if ( ACCESS_Frame( face->numLocations * 4L ) )
        return error;

      limit = face->numLocations;

      for ( n = 0; n < limit; n++ )
        face->glyphLocations[n] = GET_Long();

      FORGET_Frame();
    }
    else
    {
      face->numLocations =
        (unsigned long)face->dirTables[n].Length >> 1;

      DebugTrace(( "(16 bits offsets): %12d ",
                   face->numLocations ));

      if ( ALLOC_ARRAY( face->glyphLocations,
                        face->numLocations,
                        Long ) )
        return error;

      if ( ACCESS_Frame( face->numLocations * 2L ) )
        return error;   

      limit = face->numLocations;

      for ( n = 0; n < limit; n++ )
        face->glyphLocations[n] =
          (Long)((unsigned long)GET_UShort() * 2);

      FORGET_Frame();
    }

    DebugTrace(( "loaded\n" ));

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_Names
 *
 *  Description :  Loads the name table into face table.
 *
 *  Input  :  face     face table to look for
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_Names( PFace  face )
  {
    DEFINE_LOCALS;

    int    i, bytes, n;
    PByte  storage;

    TName_Table*  names;
    TNameRec*     namerec;


    DebugTrace(( "Names " ));

    if ( ( n = LookUp_TrueType_Table( face, TTAG_name ) ) < 0 )
    {
      /* The name table is required so indicate failure. */
      DebugTrace(( "is missing!\n" ));

      return TT_Err_Name_Table_Missing;
    }

    /* Seek to the beginning of the table and check the frame access. */
    /* The names table has a 6 byte header.                           */
    if ( FILE_Seek( face->dirTables[n].Offset ) ||
         ACCESS_Frame( 6L ) )
      return error;

    names = &face->nameTable;

    /* Load the initial names data. */
    names->format         = GET_UShort();
    names->numNameRecords = GET_UShort();
    names->storageOffset  = GET_UShort();

    FORGET_Frame();

    /* Allocate the array of name records. */
    if ( ALLOC_ARRAY( names->names,
                      names->numNameRecords,
                      TNameRec )                    ||
         ACCESS_Frame( names->numNameRecords * 12L ) )
    {
      names->numNameRecords = 0;
      goto Fail;
    }

    /* Load the name records and determine how much storage is needed */
    /* to hold the strings themselves.                                */

    for ( i = bytes = 0; i < names->numNameRecords; i++ )
    {
      namerec = names->names + i;
      namerec->platformID   = GET_UShort();
      namerec->encodingID   = GET_UShort();
      namerec->languageID   = GET_UShort();
      namerec->nameID       = GET_UShort();
      namerec->stringLength = GET_UShort();
      namerec->stringOffset = GET_UShort();

      /* check the ids */
      if ( namerec->platformID <= 3 )
      {
        /* this test takes care of 'holes' in the names tables, as */
        /* reported by Erwin                                       */
        if ( (namerec->stringOffset + namerec->stringLength) > bytes )
          bytes = namerec->stringOffset + namerec->stringLength;
      }
    }

    FORGET_Frame();

    /* Allocate storage for the strings if they exist. */

    names->storage = NULL;

    if ( bytes > 0 )
    {
      if ( ALLOC( storage, bytes ) ||
           FILE_Read_At( face->dirTables[n].Offset + names->storageOffset,
                         (void*)storage,
                         bytes ) )
        goto Fail_Storage;

      names->storage = storage;

      /* Go through and assign the string pointers to the name records. */

      for ( i = 0; i < names->numNameRecords; i++ )
      {
        namerec = names->names + i;

        if ( namerec->platformID <= 3 )
          namerec->string = storage + names->names[i].stringOffset;
        else
        {
          namerec->string       = NULL;
          namerec->stringLength = 0;
        }
      }
    }

#ifdef DEBUG

    for ( i = 0; i < names->numNameRecords; i++ ) 
    {
      int  j;


      DebugTrace(( "%d %d %x %d ",
                   names->names[i].platformID,
                   names->names[i].encodingID,
                   names->names[i].languageID,
                   names->names[i].nameID ));

      /* I know that M$ encoded strings are Unicode,            */
      /* but this works reasonable well for debugging purposes. */ 
      for ( j = 0; j < names->names[i].stringLength; j++ )
      {
        if (names->names[i].string)
        {
          char  c = *(names->names[i].string + j);
     
          if ( (unsigned char)c < 128 )
            DebugTrace(( "%c", c ));
        }
      }

      DebugTrace(( "\n" ));
    }

#endif

    DebugTrace(( "loaded\n" ));
    return TT_Err_Ok;

  Fail_Storage:
    FREE( storage );

  Fail:
    Free_TrueType_Names( face );
    return error;
  }


/*******************************************************************
 *
 *  Function    :  Free_TrueType_Names
 *
 *  Description :  Frees a name table.
 *
 *  Input  :  face     face table to look for
 *
 *  Output :  TT_Err_Ok.
 *
 ******************************************************************/

  TT_Error  Free_TrueType_Names( PFace  face )
  {
    TName_Table*  names = &face->nameTable;


    /* free strings table */
    FREE( names->names );

    /* free strings storage */
    FREE( names->storage );

    names->numNameRecords = 0;
    names->format         = 0;
    names->storageOffset  = 0;

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_CVT
 *
 *  Description :  Loads cvt table into resident table.
 *
 *  Input  :  face     face table to look for
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_CVT( PFace  face )
  {
    DEFINE_LOCALS;

    long  n;
    Int  limit;


    DebugTrace(( "CVT " ));

    if ( ( n = LookUp_TrueType_Table( face, TTAG_cvt ) ) < 0 )
    {
      DebugTrace(( "is missing!\n" ));

      face->cvtSize = 0;
      face->cvt     = NULL;
      return TT_Err_Ok;
    }

    face->cvtSize = face->dirTables[n].Length / 2;

    if ( ALLOC_ARRAY( face->cvt,
                      face->cvtSize,
                      Short ) )
      return error;

    if ( FILE_Seek( face->dirTables[n].Offset ) ||
         ACCESS_Frame( face->cvtSize * 2 ) )
      return error;

    limit = face->cvtSize;

    for ( n = 0; n < limit; n++ )
      face->cvt[n] = GET_Short();

    FORGET_Frame();

    DebugTrace(( "loaded\n" ));

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_CMap
 *
 *  Description :  Loads the cmap directory in memory.
 *                 The cmaps themselves are loaded in ttcmap.c .
 *
 *  Input  :  face
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_CMap( PFace  face )
  {
    DEFINE_LOCALS;

    long  off, table_start;
    int   n, limit;

    TCMapDir         cmap_dir;
    TCMapDirEntry    entry_;
    PCMapTable       cmap;

    DebugTrace(( "CMaps " ));

    if ( ( n = LookUp_TrueType_Table( face, TTAG_cmap ) ) < 0 )
      return TT_Err_CMap_Table_Missing;

    table_start = face->dirTables[n].Offset;

    if ( ( FILE_Seek( table_start ) ) ||
         ( ACCESS_Frame ( 4L ) ) )           /* 4 bytes cmap header */
      return error;

    cmap_dir.tableVersionNumber = GET_UShort();
    cmap_dir.numCMaps           = GET_UShort();

    FORGET_Frame();

    off = FILE_Pos();  /* save offset to cmapdir[] which follows */

    /* save space in face table for cmap tables */
    if ( ALLOC_ARRAY( face->cMaps,
                      cmap_dir.numCMaps,
                      TCMapTable ) )
      return error;

    face->numCMaps = cmap_dir.numCMaps;

    limit = face->numCMaps;
    cmap  = face->cMaps;

    for ( n = 0; n < limit; n++ )
    {
      if ( FILE_Seek( off )  ||
           ACCESS_Frame( 8L ) )
        return error;

      /* extra code using entry_ for platxxx could be cleaned up later */
      cmap->loaded             = FALSE;
      cmap->platformID         = entry_.platformID         = GET_UShort();
      cmap->platformEncodingID = entry_.platformEncodingID = GET_UShort();

      entry_.offset = GET_Long();

      FORGET_Frame();

      off = FILE_Pos();

      if ( FILE_Seek( table_start + entry_.offset ) ||
           ACCESS_Frame( 6L ) )
        return error;
      
      cmap->format  = GET_UShort();
      cmap->length  = GET_UShort();
      cmap->version = GET_UShort();

      FORGET_Frame();

      cmap->offset = FILE_Pos();

      cmap++;
    }

    DebugTrace(( "loaded\n" ));

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_HMTX
 *
 *  Description :  Loads the horizontal metrics table into face
 *                 table.
 *
 *  Input  :  face
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_HMTX( PFace  face )
  {
    DEFINE_LOCALS;

    Long              n, nsmetrics, nhmetrics;
    PTableHorMetrics  plhm;


    DebugTrace(( "HMTX " ));

    if ( ( n = LookUp_TrueType_Table( face, TTAG_hmtx ) ) < 0 )
      return TT_Err_Hmtx_Table_Missing;

    nhmetrics = face->horizontalHeader.number_Of_HMetrics;
    nsmetrics = face->maxProfile.numGlyphs - nhmetrics;

    if ( nsmetrics < 0 )            /* sanity check */
    {
      DebugTrace(( "more hmetrics than glyphs!\n" ));

      return TT_Err_Invalid_Horiz_Metrics;
    }

    if ( ALLOC_ARRAY( face->longHMetrics,
                      nhmetrics,
                      TLongHorMetric ) )
      return error;

    if ( ALLOC_ARRAY( face->shortMetrics,
                      nsmetrics,
                      Long ) )
      return error;

    if ( FILE_Seek( face->dirTables[n].Offset )   ||
         ACCESS_Frame( face->dirTables[n].Length ) )
      return error;

    for ( n = 0; n < nhmetrics; n++ )
    {
      plhm = face->longHMetrics + n;

      plhm->advance_Width = GET_Short();
      plhm->lsb           = GET_Short();
    }

    for ( n = 0; n < nsmetrics; n++ )
      face->shortMetrics[n] = GET_Short();

    FORGET_Frame();

    DebugTrace(( "loaded\n" ));

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_Programs
 *
 *  Description :  Loads the font (fpgm) and cvt programs into the
 *                 face table.
 *
 *  Input  :  face
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_Programs( PFace  face )
  {
    DEFINE_LOCALS_WO_FRAME;

    int  n;


    DebugTrace(( "Font program " ));

    /* The font program is optional */
    if ( ( n = LookUp_TrueType_Table( face, TTAG_fpgm ) ) < 0 )
    {
      face->fontProgram = NULL;
      face->fontPgmSize = 0;

      DebugTrace(( "is missing!\n" ));
    }
    else
    {
      face->fontPgmSize = face->dirTables[n].Length;

      if ( ALLOC( face->fontProgram,
                  face->fontPgmSize )              ||
           FILE_Read_At( face->dirTables[n].Offset,
                         (void*)face->fontProgram,
                         face->fontPgmSize )       )
        return error;

      DebugTrace(( "loaded, %12d bytes\n", face->fontPgmSize ));
    }

    DebugTrace(( "Prep program " ));

    if ( ( n = LookUp_TrueType_Table( face, TTAG_prep ) ) < 0 )
    {
      face->cvtProgram = NULL;
      face->cvtPgmSize = 0;

      DebugTrace(( "is missing!\n" ));
    }
    else
    {
      face->cvtPgmSize = face->dirTables[n].Length;
  
      if ( ALLOC( face->cvtProgram,
                  face->cvtPgmSize )               ||
           FILE_Read_At( face->dirTables[n].Offset,
                         (void*)face->cvtProgram,
                         face->cvtPgmSize )        )
        return error;
  
      DebugTrace(( "loaded, %12d bytes\n", face->cvtPgmSize ));
    }

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_OS2
 *
 *  Description :  Loads the OS2 Table.
 *
 *  Input  :  face
 *
 *  Output :  Error code.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_OS2( PFace  face )
  {
    DEFINE_LOCALS;

    Int      i;
    TT_OS2*  os2;

    DebugTrace(( "OS/2 Table " ));

    if ( ( i = LookUp_TrueType_Table( face, TTAG_OS2 ) ) < 0 )
    {
      DebugTrace(( "is missing\n!" ));
      return TT_Err_OS2_Table_Missing;
    }

    if ( FILE_Seek( face->dirTables[i].Offset ) ||
         ACCESS_Frame( 78L ) )
      return error;

    os2 = &face->os2;

    os2->version             = GET_UShort();
    os2->xAvgCharWidth       = GET_Short();
    os2->usWeightClass       = GET_UShort();
    os2->usWidthClass        = GET_UShort();
    os2->fsType              = GET_Short();
    os2->ySubscriptXSize     = GET_Short();
    os2->ySubscriptYSize     = GET_Short();
    os2->ySubscriptXOffset   = GET_Short();
    os2->ySubscriptYOffset   = GET_Short();
    os2->ySuperscriptXSize   = GET_Short();
    os2->ySuperscriptYSize   = GET_Short();
    os2->ySuperscriptXOffset = GET_Short();
    os2->ySuperscriptYOffset = GET_Short();
    os2->yStrikeoutSize      = GET_Short();
    os2->yStrikeoutPosition  = GET_Short();
    os2->sFamilyClass        = GET_Short();

    for ( i = 0; i < 10; i++ )
      os2->panose[i] = GET_Byte();

    os2->ulUnicodeRange1     = GET_ULong();
    os2->ulUnicodeRange2     = GET_ULong();
    os2->ulUnicodeRange3     = GET_ULong();
    os2->ulUnicodeRange4     = GET_ULong();

    for ( i = 0; i < 4; i++ )
      os2->achVendID[i] = GET_Byte();

    os2->fsSelection         = GET_UShort();
    os2->usFirstCharIndex    = GET_UShort();
    os2->usLastCharIndex     = GET_UShort();
    os2->sTypoAscender       = GET_UShort();
    os2->sTypoDescender      = GET_UShort();
    os2->sTypoLineGap        = GET_UShort();
    os2->usWinAscent         = GET_UShort();
    os2->usWinDescent        = GET_UShort();

    FORGET_Frame();

    if ( os2->version >= 0x0001 )
    {
      /* only version 1 tables */

      if ( ACCESS_Frame( 8L ) )  /* read into frame */
        return error;

      os2->ulCodePageRange1 = GET_ULong();
      os2->ulCodePageRange2 = GET_ULong();

      FORGET_Frame();
    }
    else 
    {
      os2->ulCodePageRange1 = 0;
      os2->ulCodePageRange2 = 0;
    }

    DebugTrace(( "loaded\n" ));

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_PostScript
 *
 *  Description :  Loads the post table into face table.
 *
 *  Input  :  face         face table to look for
 *
 *  Output :  SUCCESS on success.  FAILURE on error.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_PostScript( PFace  face )
  {
    DEFINE_LOCALS;

    Int  i;

    TT_Postscript*  post = &face->postscript;


    DebugTrace(( "PostScript " ));

    if ( ( i = LookUp_TrueType_Table( face, TTAG_post ) ) < 0 )
      return TT_Err_Post_Table_Missing;

    if ( FILE_Seek( face->dirTables[i].Offset ) ||
         ACCESS_Frame( 32L ) )                       
      return error;

    /* read frame data into face table */

    post->FormatType         = GET_ULong();
    post->italicAngle        = GET_ULong();
    post->underlinePosition  = GET_Short();
    post->underlineThickness = GET_Short();
    post->isFixedPitch       = GET_ULong();
    post->minMemType42       = GET_ULong();
    post->maxMemType42       = GET_ULong();
    post->minMemType1        = GET_ULong();
    post->maxMemType1        = GET_ULong();

    FORGET_Frame();

    /* we don't load the glyph names, we'd rather do that in a */
    /* library extension.                                      */

    DebugTrace(( "loaded\n" ));

    return TT_Err_Ok;
  }


/*******************************************************************
 *
 *  Function    :  Load_TrueType_Hdmx
 *
 *  Description :  Loads the horizontal device metrics table.
 *
 *  Input  :  face         face object to look for
 *
 *  Output :  SUCCESS on success.  FAILURE on error.
 *
 ******************************************************************/

  TT_Error  Load_TrueType_Hdmx( PFace  face )
  {
    DEFINE_LOCALS;

    TT_Hdmx_Record*  rec;
    TT_Hdmx          hdmx;
    Int              table, n, num_glyphs;
    Long             record_size;


    hdmx.version     = 0;
    hdmx.num_records = 0;
    hdmx.records     = 0;

    face->hdmx = hdmx;

    if ( ( table = LookUp_TrueType_Table( face, TTAG_hdmx ) ) < 0 )
      return TT_Err_Ok;

    if ( FILE_Seek( face->dirTables[table].Offset )  ||
         ACCESS_Frame( 8 ) )
      return error;

    hdmx.version     = GET_UShort();
    hdmx.num_records = GET_Short();
    record_size      = GET_Long();

    FORGET_Frame();

    /* Only recognize format 0 */

    if ( hdmx.version != 0 )
      return TT_Err_Ok;

    if ( ALLOC( hdmx.records, sizeof ( TT_Hdmx_Record ) * hdmx.num_records ) )
      return error;

    num_glyphs   = face->numGlyphs;
    record_size -= num_glyphs+2;
    rec          = hdmx.records;
 
    for ( n = 0; n < hdmx.num_records; n++ )
    {
      /* read record */
 
      if ( ACCESS_Frame( 2 ) )
        goto Fail;
 
      rec->ppem      = GET_Byte();
      rec->max_width = GET_Byte();
 
      FORGET_Frame();
 
      if ( ALLOC( rec->widths, num_glyphs )  ||
           FILE_Read( rec->widths, num_glyphs ) )
        goto Fail;
 
      /* skip padding bytes */
      if ( record_size > 0 )
        if ( FILE_Skip( record_size ) )
          goto Fail;
 
      rec++;
    }

    face->hdmx = hdmx;

    return TT_Err_Ok;

  Fail:
    for ( n = 0; n < hdmx.num_records; n++ )
      FREE( hdmx.records[n].widths );

    FREE( hdmx.records );
    return error;
  }


/*******************************************************************
 *
 *  Function    :  Free_TrueType_Hdmx
 *
 *  Description :  Frees the horizontal device metrics table.
 *
 *  Input  :  face         face object to look for
 *
 *  Output :  TT_Err_Ok.
 *
 ******************************************************************/

  TT_Error  Free_TrueType_Hdmx( PFace  face )
  {
    Int  n;


    if ( !face )
      return TT_Err_Ok;

    for ( n = 0; n < face->hdmx.num_records; n++ )
      FREE( face->hdmx.records[n].widths );

    FREE( face->hdmx.records );
    face->hdmx.num_records = 0;

    return TT_Err_Ok;
  }

/*******************************************************************
 *
 *  Function    :  Load_TrueType_Any
 *
 *  Description :  Loads any font table in clinet memory. Used by
 *                 the TT_Get_Font_Data API
 *
 *  Input  :  face     face object to look for
 *            tag      tag of table to load, the whole font file if 0 
 *            offset   starting offset in the table
 *            buffer   address of target buffer
 *
 *            length   on input  : number of bytes to read
 *                     on output : if input is 0, returns the
 *                                 table or file's size in bytes
 *  Output :  Error condition
 *
 ******************************************************************/

  TT_Error  Load_TrueType_Any( PFace  face,
                               Long   tag,
                               Long   offset,
                               void*  buffer,
                               Long*  length )
  {
    TT_Stream  stream;
    TT_Error   error;
    Int        found, i;

    if (tag != 0)
    {
      /* look for tag in font directory */
      found = -1;
      i     = 0;
      while ( i < face->numTables )
      {
        if ( face->dirTables[i].Tag == tag )
        {
          found = i;
          i = face->numTables;
        }
        else
          i++;
      }

      if (found < 0)
        return TT_Err_Table_Missing;

      offset += face->dirTables[found].Offset;

      /* If length == 0, the use requested the table's size */
      if ( *length == 0 )
      {
        *length = face->dirTables[found].Length;
        return TT_Err_Ok;
      }
    }
    else
      /* tag = 0 -- the use want to access the font file directly */
      if ( *length == 0 )
      {
        *length = TT_Stream_Size( face->stream );
        return TT_Err_Ok;
      }

    TT_Use_Stream( face->stream, &stream );
    error = TT_Read_At_File( offset, buffer, *length );
    TT_Done_Stream( &stream );

    return error;
  }

/* END */
