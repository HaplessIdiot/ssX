/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-1998 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*  ftview:  A simple font viewer.  Now supports hinting and grayscaling    */
/*           with the '-g' option.                                          */
/*                                                                          */
/*                                                                          */
/*  Keys:                                                                   */
/*                                                                          */
/*  x :   fine counter-clockwise rotation                                   */
/*  c :   fine clockwise rotation                                           */
/*                                                                          */
/*  v :   fast counter-clockwise rotation                                   */
/*  b :   fast clockwise rotation                                           */
/*                                                                          */
/*  + :   fast scale up                                                     */
/*  - :   fast scale down                                                   */
/*  u :   fine scale down                                                   */
/*  j :   fine scale up                                                     */
/*                                                                          */
/*  l :   go to next glyph                                                  */
/*  k :   go to previous glyph                                              */
/*                                                                          */
/*  o :   go to tenth next glyph                                            */
/*  i :   go to tenth previous glyph                                        */
/*                                                                          */
/*  0 :   go to hundredth next glyph                                        */
/*  9 :   go to hundredth previous glyph                                    */
/*                                                                          */
/*  ) :   go to 1000th next glyph                                           */
/*  ( :   go to 1000th previous glyph                                       */
/*                                                                          */
/*  } :   go to 10000th next glyph                                          */
/*  { :   go to 10000th previous glyph                                      */
/*                                                                          */
/*  n :   go to next (or last) .ttf file                                    */
/*  p :   go to previous (or first) .ttf file                               */
/*                                                                          */
/*  h :   toggle hinting                                                    */
/*                                                                          */
/*  ESC :   exit                                                            */
/*                                                                          */
/*                                                                          */
/*  NOTE: This is just a test program that is used to show off and          */
/*        debug the current engine.                                         */
/*                                                                          */
/****************************************************************************/

#ifdef ARM
#include "std.h"
#include "graflink.h"
#endif

#include "freetype.h"
#include "common.h"  /* for Panic() only */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gmain.h"
#include "gevents.h"
#include "gdriver.h"
#include "display.h"

#ifdef ARM
#include "armsup.c" /* pull in our routines */
#endif

#include "common.h"

#define  Pi         3.1415926535

#define  MAXPTSIZE  500                 /* dtp */
#define  Center_X   ( Bit.width / 2 )   /* dtp */
#define  Center_Y   ( Bit.rows  / 2 )   /* dtp */

  char  Header[128];

  TT_Engine    engine;

  TT_Face      face;
  TT_Instance  instance;
  TT_Glyph     glyph;
  TT_CharMap   char_map;

  TT_Big_Glyph_Metrics metrics;
  TT_Outline           outline;
  TT_Face_Properties   properties;
  TT_Instance_Metrics  imetrics;

  int  num_glyphs;

  int  ptsize;
  int  hinted;

  int            Rotation;
  int            Fail;
  int            Num;
  unsigned char  autorun;

  int            gray_render;


  static TT_Error  Reset_Scale( int  pointSize )
  {
    TT_Error  error;


    if ( (error = TT_Set_Instance_CharSize( instance, pointSize*64 )) )
    {
      RestoreScreen();
      fprintf( stderr, "Error = %d.\n", (int)error );
      Panic( "Could not reset instance.\n" );
    }
    TT_Get_Instance_Metrics( instance, &imetrics );

    /* now re-allocates the small bitmap */
    if ( gray_render )
    {
      Init_Small( imetrics.x_ppem, imetrics.y_ppem );
      Clear_Small();
    }

    return TT_Err_Ok;
  }


  static TT_Error  LoadTrueTypeChar( int  idx, int  hint )
  {
    int  flags;

    flags = TTLOAD_SCALE_GLYPH;
    if ( hint )
      flags |= TTLOAD_HINT_GLYPH;

    return TT_Load_Glyph( instance, glyph, idx, flags );
  }


  static TT_Error  Render_All( int  first_glyph, int  ptsize )
  {
    TT_F26Dot6  start_x, start_y, step_x, step_y, x, y;
    int         i;

    TT_Error    error;


    start_x = 4;
    start_y = vio_Height - ( ( ptsize * 96 + 36 ) / 72 + 10 );

    step_x = imetrics.x_ppem + 4;
    step_y = imetrics.y_ppem + 10;

    x = start_x;
    y = start_y;

    i = first_glyph;

    while ( i < num_glyphs )
    {
      if ( !(error = LoadTrueTypeChar( i, hinted )) )
      {
        TT_Get_Glyph_Outline( glyph, &outline );
        TT_Get_Glyph_Big_Metrics( glyph, &metrics );

        Render_Single_Glyph( gray_render, glyph, x, y );

        x += ( metrics.horiAdvance / 64 ) + 1;

        if ( x + imetrics.x_ppem > vio_Width )
        {
          x  = start_x;
          y -= step_y;

          if ( y < 10 )
            return TT_Err_Ok;
        }
      }
      else
        Fail++;

      i++;
    }

    return TT_Err_Ok;
  }


  static int  Process_Event( TEvent*  event )
  {
    switch ( event->what )
    {
    case event_Quit:            /* ESC or q */
      return 0;

    case event_Keyboard:
      if ( event->info == 'n' ) /* Next file */
        return 'n';
      if ( event->info == 'p' ) /* Previous file */
        return 'p';
      if ( event->info == 'h' ) /* Toggle hinting */
        hinted = !hinted;
      break;

    case event_Rotate_Glyph:
      Rotation = ( Rotation + event->info ) & 1023;
      break;

    case event_Scale_Glyph:
      ptsize += event->info;
      if ( ptsize < 1 )         ptsize = 1;
      if ( ptsize > MAXPTSIZE ) ptsize = MAXPTSIZE;
      break;

    case event_Change_Glyph:
      Num += event->info;
      if ( Num < 0 )           Num = 0;
      if ( Num >= num_glyphs ) Num = num_glyphs - 1;
      break;
    }

    return 1;
  }


  static void  usage( char*  execname )
  {
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "ftview: simple TrueType interpreter tester -- part of the FreeType project\n" );
    fprintf( stderr,  "--------------------------------------------------------------------------\n" );
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "Usage: %s [options below] ppem fontname[.ttf|.ttc] ...\n",
             execname );
    fprintf( stderr,  "\n" );
    fprintf( stderr,  "  -g     gray-level rendering (default: none)\n" );
    fprintf( stderr,  "  -r R   use resolution R dpi (default: 96 dpi)\n" );
    fprintf( stderr,  "\n" );

    exit( EXIT_FAILURE );
  }


  int  main( int  argc, char**  argv )
  {
    int    i, old_ptsize, orig_ptsize, file;
    int    XisSetup = 0;
    char   filename[128 + 4];
    char   alt_filename[128 + 4];
    char*  execname;
    int    option;
    int    res = 96;

    TT_Error  error;
    TEvent    event;


    execname = ft_basename( argv[0] );

    while ( 1 )
    {
      option = ft_getopt( argc, argv, "gr:" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'g':
        gray_render = 1;
        break;

      case 'r':
        res = atoi( ft_optarg );
        if ( res < 1 )
          usage( execname );
        break;

      default:
        usage( execname );
        break;
      }
    }

    argc -= ft_optind;
    argv += ft_optind;

    if ( argc <= 1 )
      usage( execname );

    if ( sscanf( argv[0], "%d", &orig_ptsize ) != 1 )
      orig_ptsize = 64;

    file = 1;

    /* Initialize engine */

    if ( (error = TT_Init_FreeType( &engine )) )
      Panic( "Error while initializing engine, code = %d.\n", error );

  NewFile:
    ptsize = orig_ptsize;
    hinted = 1;

    i = strlen( argv[file] );
    while ( i > 0 && argv[file][i] != '\\' && argv[file][i] != '/' )
    {
      if ( argv[file][i] == '.' )
        i = 0;
        i--;
    }

    filename[128] = '\0';
    alt_filename[128] = '\0';

    strncpy( filename, argv[file], 128 );
    strncpy( alt_filename, argv[file], 128 );

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

    if ( error == TT_Err_Could_Not_Open_File )
      Panic( "Could not find/open %s.\n", filename );
    else if ( error )
      Panic( "Error while opening %s, code = %x.\n", filename, error );

    /* get face properties and allocate preload arrays */

    TT_Get_Face_Properties( face, &properties );

    num_glyphs = properties.num_Glyphs;

    /* create glyph */

    error = TT_New_Glyph( face, &glyph );
    if ( error )
      Panic( "Could not create glyph container.\n" );

    /* create instance */

    error = TT_New_Instance( face, &instance );
    if ( error )
      Panic( "Could not create instance for %s.\n", filename );

    error = TT_Set_Instance_Resolutions( instance, res, res );
    if ( error )
      Panic( "Could not set device resolutions." );

    if ( !XisSetup )
    {
      XisSetup = 1;

      if ( gray_render )
      {
        if ( !SetGraphScreen( Graphics_Mode_Gray ) )
          Panic( "Could not set up grayscale graphics mode.\n" );

        TT_Set_Raster_Gray_Palette( engine, virtual_palette );
      }
      else
      {
        if ( !SetGraphScreen( Graphics_Mode_Mono ) )
          Panic( "Could not set up mono graphics mode.\n" );
      }
    }

    Init_Display( gray_render );

    Reset_Scale( ptsize );

    old_ptsize = ptsize;

    Fail = 0;
    Num  = 0;

    for ( ;; )
    {
      int  key;


      Clear_Display();
      Render_All( Num, ptsize );
      if ( gray_render )
        Convert_To_Display_Palette();

      sprintf( Header, "%s:   Glyph: %4d  ptsize: %4d  hinting: %s",
                       ft_basename( filename ), Num, ptsize,
                       hinted ? "on" : "off" );
      Display_Bitmap_On_Screen( Bit.bitmap, Bit.rows, Bit.cols );

#ifndef X11
#ifndef OS2
      Print_XY( 0, 0, Header );
#endif
#endif

      Get_Event( &event );
      if ( !( key = Process_Event( &event ) ) )
        goto Fin;

      if ( key == 'n' )
      {
        TT_Close_Face( face );

        if ( file < argc - 1 )
          file++;

        goto NewFile;
      }

      if ( key == 'p' )
      {
        TT_Close_Face( face );

        if ( file > 1 )
          file--;

        goto NewFile;
      }

      if ( ptsize != old_ptsize )
      {
        if ( Reset_Scale( ptsize ) )
          Panic( "Could not resize font.\n" );

        old_ptsize = ptsize;
      }
    }

  Fin:
    RestoreScreen();

    TT_Done_FreeType( engine );

    printf( "Execution completed successfully.\n" );
    printf( "Fails = %d\n", Fail );

    exit( EXIT_SUCCESS );      /* for safety reasons */

    return 0;       /* never reached */
}


/* End */
