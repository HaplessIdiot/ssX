/****************************************************************************}
{*                                                                          *}
{*  The FreeType project - a Free and Portable Quality TrueType Renderer.   *}
{*                                                                          *}
{*  Copyright 1996-1998 by                                                  *}
{*  D. Turner, R.Wilhelm, and W. Lemberg                                    *}
{*                                                                          *}
{*  fdebug : A very simple TrueType bytecode debugger.                      *}
{*                                                                          *}
{*  NOTE : You must compile the interpreter with the DEBUG macro            *}
{*         defined in order to link this program !!                         *}
{*                                                                          *}
{****************************************************************************/

#ifdef HAVE_CONIO_H
#include <conio.h>
#endif

#ifdef ARM
#include "std.h"
#include "graflink.h"
#endif

#include "freetype.h"
#include "tttypes.h"
#include "tterror.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>    /* libc ANSI */


#ifdef ARM
#include "armsup.c" /* pull in our routines */
#endif

#define  Pi          3.1415926535

#define  MAXPTSIZE   500           /* dtp */
#define  Center_X   (Bit.width/2)  /* dtp */
#define  Center_Y   (Bit.rows/2)   /* dtp */

#define  Profile_Buff_Size  64000      /* Size of the render pool   */
                                       /* Minimum is around 4 Kb    */
                                       /* experiment to see how its */
                                       /* size impacts on raster    */

                                       /* performance..             */
#define  Font_Buff_Size  256000        /* this buffer holds all     */
                                       /* font specific data.       */

#define  Border 10

  TT_Engine    engine;
  TT_Face      face;
  TT_Instance  instance;
  TT_Glyph     glyph;

  TT_Face_Properties  properties;

  int   num_glyphs;
  int   num_pts;
  int   num_ctr;

  int   glyfArray;

  PShort  epts_ctr;

  /* DTP important font metrics */
  Long   cur_LeftSB,
         cur_Ascent,
         cur_Descent,
         cur_AdvanceWidth;

  Int    ptsize;

  float  ymin,
         ymax,
         xmax,
         xmin,
         xsize;    /* DTP testing precision */

  float  resR;

  Int           Rotation;
  Int           Fail;
  Int           Num;
  unsigned char autorun;

  char          GrayLines[1024];
  int           gray_render;


  Bool  LoadTrueTypeChar( Int idx )
  {
    Short       numP, numC;

    Int i;

    /* Reset_Context( instance->exec );  */

    if ( TT_Load_Glyph( instance, glyph, idx, TTLOAD_DEFAULT ) )
      return FAILURE;


    return SUCCESS;
  }



  int  main( int  argc, char**  argv )
  {

   int     i;
   char    filename[128+4];
   char*   execname;

   TT_Init_FreeType(&engine);

    num_pts = 0;
    num_ctr = 0;

    execname = argv[0];

    if ( argc != 4 )
    {
      Message( "Test: simple TrueType interpreter tester - part of the FreeType project\n" );
      Message( "-----------------------------------------------------------------------\n\n");
      Message( "Usage: %s glyphnum ppem fontname[.ttf]\n\n", execname );
      exit( EXIT_FAILURE );
    }

    if ( sscanf( argv[1], "%d", &Num ) == EOF )
      Num = 0;

    if ( sscanf( argv[2], "%d", &ptsize ) == EOF )
      ptsize = 64;

    i = strlen( argv[3] );
    while ( i > 0 && argv[3][i] != '\\' )
    {
      if ( argv[3][i] == '.' )
        i = 0;
      i--;
    }

    filename[128] = 0;

    strncpy( filename, argv[3], 128 );
    if ( i >= 0 )
      strncpy( filename + strlen(filename), ".ttf", 4 );

    if ( TT_Open_Face( engine, filename , &face) )
    {
      Message( "Error, could not find/open %s\n\n", filename );
      exit( EXIT_FAILURE );
    }

    TT_Get_Face_Properties( face, &properties );

/*
    FOut = fopen( "h:\\work\\freetype\\p2.txt", "wt" );

    fprintf( FOut, "Original CVT\n" );
    Dump_CVT( resident->cvt, resident->cvtSize );
*/
    num_glyphs = properties.num_Glyphs;

    if ( TT_New_Instance( face, &instance ) )
    {
      Message( "ERROR: Could not open instance from %s\n", filename );
      exit( EXIT_FAILURE );
    }

    if ( TT_New_Glyph( face, &glyph ) )
      Panic( "ERROR: could not create glyph container\n" );

    TT_Set_Instance_Resolutions( instance, 96, 96 );

    if ( TT_Set_Instance_CharSize( instance, ptsize*64 ) )
      Panic( "ERROR: could not reset execution context\n" );
/*
    fprintf( FOut, "Scaled CVT\n" );
    Dump_CVT( instance->cvt, instance->cvtSize );
*/
    LoadTrueTypeChar(Num);

/*
    fprintf( FOut, "Hinted CVT\n" );
    Dump_CVT( instance->cvt, instance->cvtSize );

    fclose( FOut );
*/
    TT_Close_Face(face);

    exit( EXIT_SUCCESS );      /* for safety reasons */

    return 0;       /* never reached */
  }


/* End */
