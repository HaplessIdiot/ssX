/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-1998 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*  ftlint: a simple TrueType instruction tester.                           */
/*                                                                          */
/*  NOTE:  This is just a test program that is used to show off and         */
/*         debug the current engine.                                        */
/*                                                                          */
/****************************************************************************/

#ifdef ARM
#include "std.h"
#include "graflink.h"
#endif

#include "freetype.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ARM
#include "armsup.c" /* pull in our routines */
#endif

#ifdef HAVE_LIBINTL_H

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <libintl.h>
#include "ftxerr18.h"

#else /* !HAVE_LIBINTL */

#define gettext( x )  ( x )

  /* We ignore error message strings with this function */

  static char*  TT_ErrToString18( TT_Error  error )
  {
    static char  temp[32];

    sprintf( temp, "0x%04lx", error );
    return temp;
  }

#endif /* !HAVE_LIBINTL */

  TT_Error     error;

  TT_Engine    engine;

  TT_Face      face;
  TT_Instance  instance;
  TT_Glyph     glyph;

  TT_Outline        outline;
  TT_Glyph_Metrics  metrics;

  TT_Face_Properties  properties;

  unsigned int  num_glyphs;
  int           ptsize;

  int  Fail;
  int  Num;


  static TT_Error  LoadTrueTypeChar( int  idx )
  {
    return TT_Load_Glyph( instance, glyph, idx, TTLOAD_DEFAULT );
  }


  static void  Usage( char*  name )
  {
    printf( gettext( "ftlint: simple TrueType instruction tester -- part of the FreeType project\n" ) );
    printf(          "--------------------------------------------------------------------------\n" );
    printf(          "\n" );
    printf( gettext( "Usage: %s ppem fontname[.ttf|.ttc] [fontname2..]\n" ), name );
    printf(          "\n" );

    exit( EXIT_FAILURE );
  }


  int  main( int  argc, char**  argv )
  {
    int           i, file_index;
    unsigned int  id;
    char          filename[128 + 4];
    char          alt_filename[128 + 4];
    char*         execname;
    char*         fname;


#ifdef HAVE_LIBINTL_H
    setlocale( LC_ALL, "" );
    bindtextdomain( "freetype", LOCALEDIR );
    textdomain( "freetype" );
#endif

    execname = argv[0];

    if ( argc < 3 )
      Usage( execname );

    if ( sscanf( argv[1], "%d", &ptsize ) != 1 )
      Usage( execname );

    /* Initialize engine */
    if ( (error = TT_Init_FreeType( &engine )) )
    {
      fprintf( stderr,
               gettext( "Error while initializing engine, code = %d.\n" ),
               (int)error );
      exit( EXIT_FAILURE );
    }

    /* Now check all files */
    for ( file_index = 2; file_index < argc; file_index++ )
    {
      fname = argv[file_index];
      i     = strlen( fname );
      while ( i > 0 && fname[i] != '\\' && fname[i] != '/' )
      {
        if ( fname[i] == '.' )
          i = 0;
        i--;
      }

      filename[128] = '\0';
      alt_filename[128] = '\0';

      strncpy( filename, fname, 128 );
      strncpy( alt_filename, fname, 128 );

      if ( i >= 0 )
      {
        strncpy( filename + strlen( filename ), ".ttf", 4 );
        strncpy( alt_filename + strlen( alt_filename ), ".ttc", 4 );
      }

      /* Load face */
      error = TT_Open_Face( engine, filename, &face );

      if ( error == TT_Err_Could_Not_Open_File )
      {
        strcpy( filename, alt_filename );
        error = TT_Open_Face( engine, alt_filename, &face );
      }

      i     = strlen( filename );
      fname = filename;

      while ( i >= 0 )
        if ( filename[i] == '/' || filename[i] == '\\' )
        {
          fname = filename + i + 1;
          i = -1;
        }
        else
          i--;

      printf( "%s: ", fname );

      if ( error )
      {
        printf( gettext( "Could not find or open file.\n" ) );
        printf( gettext( "FreeType error message: %s\n" ),
                 TT_ErrToString18( error ) );
        goto Fail_Face;
      }

      /* get face properties */

      TT_Get_Face_Properties( face, &properties );
      num_glyphs = properties.num_Glyphs;

      /* create glyph */
      error = TT_New_Glyph( face, &glyph );
      if ( error )
      {
        printf( gettext( "Could not create glyph container.\n" ) );
        printf( gettext( "FreeType error message: %s\n" ),
                 TT_ErrToString18( error ) );
        goto Fail_Glyph;
      }

      /* create instance */
      error = TT_New_Instance( face, &instance );
      if ( error )
      {
        printf( gettext( "Could not create instance.\n" ) );
        printf( gettext( "FreeType error message: %s\n" ),
                 TT_ErrToString18( error ) );
        goto Fail_Instance;
      }

      error = TT_Set_Instance_PixelSizes( instance,
                                          ptsize,
                                          ptsize,
                                          ptsize*3/4 );
      if ( error )
      {
        printf( gettext( "Could not set point size to %d.\n" ), ptsize );
        printf( gettext( "FreeType error message: %s\n" ),
                 TT_ErrToString18( error ) );
        goto Fail_Set;
      }

      Fail = 0;

      for ( id = 0; id < num_glyphs; id++ )
      {
        if ( (error = LoadTrueTypeChar( id )) )
        {
          if ( Fail < 10 )
            printf( gettext( "glyph %4u: %s\n" ),
                     id, TT_ErrToString18( error ) );
          Fail++;
        }
      }

      if ( Fail == 0 )
        printf( "OK.\n" );
      else
        if ( Fail == 1 )
          printf( gettext( "1 fail.\n" ) );
        else
          printf( gettext( "%d fails.\n" ), Fail );

    Fail_Set:
      TT_Done_Instance( instance );
    Fail_Instance:
      TT_Done_Glyph( glyph );
    Fail_Glyph:
      TT_Close_Face( face );
    Fail_Face:
      ;
    }

    TT_Done_FreeType( engine );

    exit( EXIT_SUCCESS );      /* for safety reasons */

    return 0;       /* never reached */
  }


/* End */
