/*******************************************************************
 *
 *  ttload.h                                                    1.1
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
 *
 *  Changes between 1.1 and 1.0 :
 *
 *  - add function Load_TrueType_Any used by TT_Get_Font_Data
 *
 ******************************************************************/

/* $XFree86: $ */
  
#ifndef TTLOAD_H
#define TTLOAD_H

#include "ttcommon.h"


#ifdef __cplusplus
  extern "C" {
#endif

  Int  LookUp_TrueType_Table( PFace  face,
                              Long   tag  );

  TT_Error  Load_TrueType_Directory        ( PFace  face, 
                                             int    faceIndex );

  TT_Error  Load_TrueType_MaxProfile       ( PFace  face );
  TT_Error  Load_TrueType_Gasp             ( PFace  face );
  TT_Error  Load_TrueType_Header           ( PFace  face );
  TT_Error  Load_TrueType_Horizontal_Header( PFace  face );
  TT_Error  Load_TrueType_Locations        ( PFace  face );
  TT_Error  Load_TrueType_Names            ( PFace  face );
  TT_Error  Load_TrueType_CVT              ( PFace  face );
  TT_Error  Load_TrueType_CMap             ( PFace  face );
  TT_Error  Load_TrueType_HMTX             ( PFace  face );
  TT_Error  Load_TrueType_Programs         ( PFace  face );
  TT_Error  Load_TrueType_OS2              ( PFace  face );
  TT_Error  Load_TrueType_PostScript       ( PFace  face );
  TT_Error  Load_TrueType_Hdmx             ( PFace  face );

  TT_Error  Load_TrueType_Any( PFace  face,
                               Long   tag,
                               Long   offset,
                               void*  buffer,
                               Long*  length );


  TT_Error  Free_TrueType_Names( PFace  face );
  TT_Error  Free_TrueType_Hdmx ( PFace  face );


/* The following macros are defined to simplify the writing of */
/* the various table and glyph loaders.                        */

/* For examples see the code in ttload.c, ttgload.c etc.       */

#define USE_Stream( original, duplicate ) \
          ( error = TT_Use_Stream( original, &duplicate ) )

#define DONE_Stream( _stream ) \
          TT_Done_Stream( &_stream )

/* Define a file frame -- use it only when needed */
#define DEFINE_A_FRAME   TFileFrame  frame = TT_Null_FileFrame

/* Define a stream -- use it only when needed */
#define DEFINE_A_STREAM  TT_Stream   stream


#ifdef TT_CONFIG_REENTRANT  /* re-entrant implementation */

/* The following macros define the necessary local */
/* variables used to access streams and frames.    */

/* Define stream locals with frame */
#define DEFINE_STREAM_LOCALS  \
          TT_Error  error;    \
          DEFINE_A_STREAM;    \
          DEFINE_A_FRAME

/* Define stream locals without frame */
#define DEFINE_STREAM_LOCALS_WO_FRAME  \
          TT_Error  error;             \
          DEFINE_A_STREAM

/* Define locals with a predefined stream in reentrant mode -- see ttload.c */
#define DEFINE_LOAD_LOCALS( STREAM )  \
          TT_Error  error;            \
          DEFINE_A_STREAM = (STREAM); \
          DEFINE_A_FRAME

/* Define locals without frame with a predefined stream - see ttload.c */
#define DEFINE_LOAD_LOCALS_WO_FRAME( STREAM ) \
          TT_Error      error;                \
          DEFINE_A_STREAM = (STREAM)

/* Define all locals necessary to access a font file */
#define DEFINE_ALL_LOCALS  \
          TT_Error  error; \
          DEFINE_A_STREAM; \
          DEFINE_A_FRAME


#define ACCESS_Frame( _size_ ) \
          ( error = TT_Access_Frame( stream, &frame, _size_ ) )
#define CHECK_ACCESS_Frame( _size_ ) \
          ( error = TT_Check_And_Access_Frame( stream, &frame, _size_ ) )
#define FORGET_Frame() \
          ( error = TT_Forget_Frame( &frame ) )
  
#define GET_Byte()    TT_Get_Byte  ( &frame )
#define GET_Char()    TT_Get_Char  ( &frame )
#define GET_UShort()  TT_Get_UShort( &frame )
#define GET_Short()   TT_Get_Short ( &frame )
#define GET_Long()    TT_Get_Long  ( &frame )
#define GET_ULong()   TT_Get_ULong ( &frame )
#define GET_Tag4()    TT_Get_Long  ( &frame )
  
#define FILE_Pos()    TT_File_Pos ( stream )

#define FILE_Seek( _position_ ) \
          ( error = TT_Seek_File( stream, _position_ ) )
#define FILE_Skip( _distance_ ) \
          ( error = TT_Skip_File( stream, _distance_ ) )
#define FILE_Read( buffer, count ) \
          ( error = TT_Read_File ( stream, buffer, count ) )
#define FILE_Read_At( pos, buffer, count ) \
          ( error = TT_Read_At_File( stream, pos, buffer, count ) )
  
#else   /* thread-safe implementation */

/* Define stream locals with frame -- nothing in thread-safe mode */
#define DEFINE_STREAM_LOCALS  \
          TT_Error  error

/* Define stream locals without frame -- nothing in thread-safe mode */
#define DEFINE_STREAM_LOCALS_WO_FRAME \
          TT_Error  error

/* Define locals with a predefined stream in reentrant mode -- see ttload.c */
#define DEFINE_LOAD_LOCALS( STREAM ) \
          TT_Error  error

/* Define locals without frame with a predefined stream - see ttload.c */
#define DEFINE_LOAD_LOCALS_WO_FRAME( STREAM ) \
          TT_Error  error

/* Define all locals necessary to access a font file */
#define DEFINE_ALL_LOCALS  \
          TT_Error  error; \
          DEFINE_A_STREAM


#define ACCESS_Frame( _size_ ) \
          ( error = TT_Access_Frame( _size_ ) )
#define CHECK_ACCESS_Frame( _size_ ) \
          ( error = TT_Check_And_Access_Frame( _size_ ) )
#define FORGET_Frame() \
          ( error = TT_Forget_Frame() )
  
#define GET_Byte()    TT_Get_Byte  ()
#define GET_Char()    TT_Get_Char  ()
#define GET_UShort()  TT_Get_UShort()
#define GET_Short()   TT_Get_Short ()
#define GET_Long()    TT_Get_Long  ()
#define GET_ULong()   TT_Get_ULong ()
#define GET_Tag4()    TT_Get_Long  ()
  
#define FILE_Pos()    TT_File_Pos()

#define FILE_Seek( _position_ ) \
          ( error = TT_Seek_File( _position_ ) )
#define FILE_Skip( _distance_ ) \
          ( error = TT_Skip_File( _distance_ ) )
#define FILE_Read( buffer, count ) \
          ( error = TT_Read_File ( buffer, count ) )
#define FILE_Read_At( pos, buffer, count ) \
          ( error = TT_Read_At_File( pos, buffer, count ) )
  
#endif /* TT_CONFIG_REENTRANT */

#ifdef __cplusplus
  }
#endif

#endif /* TTLOAD_H */


/* END */
