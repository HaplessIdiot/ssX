/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-1998 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*  ftdump: Simple TrueType font file resource profiler.                    */
/*                                                                          */
/*  This program dumps various properties of a given font file.             */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*  NOTE:  This is just a test program that is used to show off and         */
/*         debug the current engine.                                        */
/*                                                                          */
/****************************************************************************/

#ifdef ARM
#include "std.h"
#include "graflink.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freetype.h"
#include "common.h"    /* for Panic.. */
#include "ttobjs.h"    /* We're going to access internal tables directly */

#ifdef ARM
#include "armsup.c" /* pull in our routines */
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef HAVE_LIBINTL_H
#include "ftxerr18.h"
#include <libintl.h>
#else
#define gettext( x )  ( x )
#endif


  TT_Error     error;

  TT_Engine    engine;
  TT_Face      face;
  TT_Instance  instance;
  TT_Glyph     glyph;

  TT_Instance_Metrics  imetrics;
  TT_Outline           outline;
  TT_Glyph_Metrics     metrics;

  TT_Face_Properties   properties;

  TT_CharMap  cmap;

  int  num_glyphs;
  int  ptsize;

  int  Fail;
  int  Num;


  int  flag_memory    = 1;
  int  flag_names     = 1;
  int  flag_encodings = 1;

  extern long  TTMemory_Allocated;

  long   org_memory, old_memory, cur_memory;

  const char*  Apple_Encodings[33] =
  {
    "Roman", "Japanese", "Chinese", "Korean", "Arabic", "Hebrew",
    "Greek", "Russian", "RSymbol", "Devanagari", "Gurmukhi",
    "Gujarati", "Oriya", "Bengali", "Tamil", "Telugu", "Kannada",
    "Malayalam", "Sinhalese", "Burmese", "Khmer", "Tai", "Laotian",
    "Georgian", "Armenian", "Maldivian/Simplif. Chinese", "Tibetan",
    "Mongolian", "Geez", "Slavic", "Vietnamese", "Sindhi", "Uninterpreted"
  };

  struct
  {
    long  initial_overhead;
    long  face_object;
    long  glyph_object;
    long  first_instance;
    long  second_instance;

  } memory_footprint;


  /* We ignore error message strings with this function */

#ifndef HAVE_LIBINTL_H
  static char*  TT_ErrToString18( TT_Error  error )
  {
    static char  temp[32];


    sprintf( temp, "0x%04lx", error );
    return temp;
  }
#endif


  void  Save_Memory( long*  var )
  {
    *var = TTMemory_Allocated - old_memory;
    old_memory += *var;
  }

#define FOOTPRINT( field )  Save_Memory( &memory_footprint.##field )


  static void
  Print_Mem( long  val, char*  string )
  {
    printf( "%6ld bytes (%4ldkByte): %s\n",
             val,
             (val+1023) / 1024,
             string );
  }

#define PRINT_MEM( field, string ) \
          Print_Mem( memory_footprint.##field, string )


  /* Print the memory footprint */

  void  Print_Memory( void )
  {
    /* create glyph */
    error = TT_New_Glyph( face, &glyph );
    if ( error )
      Panic( gettext( "Could not create glyph container.\n" ) );

    FOOTPRINT( glyph_object );

    /* create instance */
    error = TT_New_Instance( face, &instance );
    if ( error )
      Panic( gettext(" Could not create instance.\n" ) );

    FOOTPRINT( first_instance );

    error = TT_New_Instance( face, &instance );
    if ( error )
      Panic( gettext( "Could not create 2nd instance.\n" ) );

    FOOTPRINT( second_instance );

    printf( gettext( "Memory footprint statistics:\n" ) );
    printf(          "-----------------------------------------------\n" );

    /* NOTE: In our current implementation, the face's execution */
    /*       context object is created lazily with the first     */
    /*       instance.  However, all later instances share the   */
    /*       the same context.                                   */

    PRINT_MEM( face_object,     gettext( "face object" )     );
    PRINT_MEM( glyph_object,    gettext( "glyph_object" )    );
    PRINT_MEM( second_instance, gettext( "instance object" ) );

    Print_Mem( memory_footprint.first_instance -
               memory_footprint.second_instance,
               gettext( "exec. context object" ) );

    printf( "-----------------------------------------------\n" );
    Print_Mem( memory_footprint.face_object  +
               memory_footprint.glyph_object +
               memory_footprint.first_instance,
               gettext( "total memory usage" ) );

    printf( "\n" );
  }


  static char  name_buffer[257];
  static int   name_len = 0;


  static char*  LookUp_Name( int  index )
  {
    unsigned short  i, n;

    unsigned short  platform, encoding, language, id;
    char*           string;
    unsigned short  string_len;

    int             j, found;


    n = properties.num_Names;

    for ( i = 0; i < n; i++ )
    {
      TT_Get_Name_ID( face, i, &platform, &encoding, &language, &id );
      TT_Get_Name_String( face, i, &string, &string_len );

      if ( id == index )
      {

        /* The following code was inspired from Mark Leisher's */
        /* ttf2bdf package                                     */

        found = 0;

        /* Try to find a Microsoft English name */

        if ( platform == 3 )
          for ( j = 1; j >= 0; j-- )
            if ( encoding == j )  /* Microsoft ? */
              if ( (language & 0x3FF) == 0x009 )    /* English language */
              {
                found = 1;
                break;
              }

        if ( !found && platform == 0 && language == 0 )
          found = 1;

        /* Found a Unicode Name. */

        if ( found )
        {
          if ( string_len > 512 )
            string_len = 512;

          name_len = 0;

          for ( i = 1; i < string_len; i += 2 )
            name_buffer[name_len++] = string[i];

          name_buffer[name_len] = '\0';

          return name_buffer;
        }
      }
    }

    /* Not found */
    return NULL;
  }


  static void
  Print_Names( void )
  {
     printf( gettext( "font name table entries\n" ) );
     printf( "-----------------------------------------------\n" );

     if ( LookUp_Name( 4 ) )
       printf( "%s - ", name_buffer );

     if ( LookUp_Name( 5 ) )
       printf( "%s\n\n", name_buffer );

     if ( LookUp_Name( 0 ) )
       printf( "%s\n\n", name_buffer );

     if ( LookUp_Name( 7 ) )
       printf( name_buffer );

     printf( "\n" );
     printf( "-----------------------------------------------\n\n" );
  }


  static void
  Print_Encodings( void )
  {
     unsigned short  n, i;
     unsigned short  platform, encoding;
     char*           platStr, *encoStr;

     char  tempStr[128];

     printf( gettext( "character map encodings\n" ) );
     printf( "-----------------------------------------------\n" );

     n = properties.num_CharMaps;
     if ( n == 0 )
     {
       printf( gettext( "The file doesn't seem to have any encoding table.\n" ) );
       return;
     }

     printf( gettext( "There are %d encodings:\n\n" ), n );

     for ( i = 0; i < n; i++ )
     {
       TT_Get_CharMap_ID( face, i, &platform, &encoding );
       printf( gettext( "encoding %2d: " ), i );

       platStr = encoStr = NULL;

       switch ( platform )
       {
       case TT_PLATFORM_APPLE_UNICODE:
         platStr = "Apple Unicode";
         switch ( encoding )
         {
         case TT_APPLE_ID_DEFAULT:
           encoStr = "";
           break;

         case TT_APPLE_ID_UNICODE_1_1:
           encoStr = "(v.1.1)";
           break;

         case TT_APPLE_ID_ISO_10646:
           encoStr = "(ISO 10646-1:1993)";
           break;

         case TT_APPLE_ID_UNICODE_2_0:
           encoStr = "(v.2.0)";
           break;

         default:
           sprintf( tempStr, gettext( "Unknown %d" ), encoding );
           encoStr = tempStr;
         }
         break;

       case TT_PLATFORM_MACINTOSH:
         platStr = "Apple";
         if ( encoding > 32 )
         {
           sprintf( tempStr, gettext( "Unknown %d" ), encoding );
           encoStr = tempStr;
         }
         else
           encoStr = (char*)Apple_Encodings[encoding];
         break;

       case TT_PLATFORM_ISO:
         platStr = "Iso";
         switch ( encoding )
         {
         case TT_ISO_ID_7BIT_ASCII:
           platStr = "Ascii";
           encoStr = "7-bit";
           break;

         case TT_ISO_ID_10646:
           encoStr = "10646";
           break;

         case TT_ISO_ID_8859_1:
           encoStr = "8859-1";
           break;

         default:
           sprintf( tempStr, "%d", encoding );
           encoStr = tempStr;
         }
         break;

       case TT_PLATFORM_MICROSOFT:
         platStr = "Windows";
         switch ( encoding )
         {
         case TT_MS_ID_SYMBOL_CS:
           encoStr = "Symbol";
           break;

         case TT_MS_ID_UNICODE_CS:
           encoStr = "Unicode";
           break;

         case TT_MS_ID_SJIS:
           encoStr = "Shift-JIS";
           break;

         case TT_MS_ID_GB2312:
           encoStr = "GB2312";
           break;

         case TT_MS_ID_BIG_5:
           encoStr = "Big 5";
           break;

         case TT_MS_ID_WANSUNG:
           encoStr = "WanSung";
           break;

         case TT_MS_ID_JOHAB:
           encoStr = "Johab";
           break;

         default:
           sprintf( tempStr, gettext( "Unknown %d" ), encoding );
           encoStr = tempStr;
         }
         break;

       default:
         sprintf( tempStr, "%d - %d", platform, encoding );
         platStr = gettext( "Unknown" );
         encoStr = tempStr;
       }

       printf( "%s %s\n", platStr, encoStr );
     }

     printf( "\n" );
     printf( "-----------------------------------------------\n\n" );
  }


  int  main( int  argc, char**  argv )
  {
    int    i;
    char   filename[128 + 4];
    char   alt_filename[128 + 4];
    char*  execname;

#ifdef HAVE_LIBINTL_H
    setlocale( LC_ALL, "" );
    bindtextdomain( "freetype", LOCALEDIR );
    textdomain( "freetype" );
#endif

    execname = argv[0];

    if ( argc != 2 )
    {
      printf( gettext( "ftdump: simple TrueType Dumper -- part of the FreeType project\n" ) );
      printf(          "--------------------------------------------------------------\n" );
      printf(          "\n" );
      printf( gettext( "Usage: %s fontname[.ttf|.ttc]\n\n" ), execname );

      exit( EXIT_FAILURE );
    }


    i = strlen( argv[1] );
    while ( i > 0 && argv[1][i] != '\\' )
    {
      if ( argv[1][i] == '.' )
        i = 0;
      i--;
    }

    filename[128] = '\0';
    alt_filename[128] = '\0';

    strncpy( filename, argv[1], 128 );
    strncpy( alt_filename, argv[1], 128 );

    if ( i >= 0 )
    {
      strncpy( filename + strlen( filename ), ".ttf", 4 );
      strncpy( alt_filename + strlen( alt_filename ), ".ttc", 4 );
    }

    /* Initialize engine */

    old_memory = 0;

    if ( (error = TT_Init_FreeType( &engine )) )
      Panic( gettext( "Error while initializing engine: %s\n" ),
             TT_ErrToString18( error ) );

    FOOTPRINT( initial_overhead );

    /* Open and Load face */

    error = TT_Open_Face( engine, filename, &face );
    if ( error == TT_Err_Could_Not_Open_File )
    {
      strcpy( filename, alt_filename );
      error = TT_Open_Face( engine, alt_filename, &face );
    }

    if ( error )
    {
      printf( gettext( "Error while opening %s.\n" ), filename );
      Panic( gettext( "FreeType error message: %s\n" ),
              TT_ErrToString18( error ) );
    }

    FOOTPRINT( face_object );

    /* get face properties and allocate preload arrays */

    TT_Get_Face_Properties( face, &properties );
    num_glyphs = properties.num_Glyphs;

    /* Now do various dumps */

    if ( flag_names )
      Print_Names();

    if ( flag_encodings )
      Print_Encodings();

    if ( flag_memory )
      Print_Memory();

    TT_Close_Face( face );

    TT_Done_FreeType( engine );

    exit( EXIT_SUCCESS );      /* for safety reasons */

    return 0;       /* never reached */
}


/* End */
