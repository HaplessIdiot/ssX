/*******************************************************************
 *
 *  gw_amiga.c  graphics utility Intuition Amiga driver.       1.0
 *
 *  This is the driver for windowed display under Amiga WorkBench,
 *  used by the graphics utility of the FreeType test suite.
 *
 *  Copyright 1996-1998 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT. By continuing to use, modify or distribute 
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 ******************************************************************/

/* standard includes */

#include <stdlib.h>
#include <stdio.h>

/* AmigaOS includes */

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>

#ifdef __GNUC__
#include <inline/exec.h>
#include <inline/intuition.h>
#include <inline/graphics.h>
#include <inline/dos.h>
#else
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/dos_protos.h>
#endif

/* FreeType includes */

#include "gdriver.h"
#include "gmain.h"
#include "gevents.h"


/* some screen definitions */

#define MONO_WINDOW_WIDTH   640
#define MONO_WINDOW_HEIGHT  480

#define GRAY_WINDOW_WIDTH   320
#define GRAY_WINDOW_HEIGHT  240

#define DISPLAY_MEM         ( 1024 * 64 )


  /* external variables */
  extern struct Library*  SysBase;
  extern struct Library*  DOSBase;

  extern int    vio_ScanLineWidth;
  extern char*  Vio;
  extern char   gray_palette[5];

  /* global variables */
  struct Library*  IntuitionBase = NULL;
  struct Library*  GfxBase = NULL;

  typedef struct  _Translator
  {
    char    key;
    GEvent  event_class;
    int     event_info;
  } Translator;
                          
#define NUM_Translators  20

  static const Translator  trans[NUM_Translators] =
  {
    { (char)27, event_Quit,              0 },
    { 'q',      event_Quit,              0 },

    { 'x',      event_Rotate_Glyph,     -1 },
    { 'c',      event_Rotate_Glyph,      1 },
    { 'v',      event_Rotate_Glyph,    -16 },
    { 'b',      event_Rotate_Glyph,     16 },

    { '{',      event_Change_Glyph, -10000 },
    { '}',      event_Change_Glyph,  10000 },
    { '(',      event_Change_Glyph,  -1000 },
    { ')',      event_Change_Glyph,   1000 },
    { '9',      event_Change_Glyph,   -100 },
    { '0',      event_Change_Glyph,    100 },
    { 'i',      event_Change_Glyph,    -10 },
    { 'o',      event_Change_Glyph,     10 },
    { 'k',      event_Change_Glyph,     -1 },
    { 'l',      event_Change_Glyph,      1 },

    { '+',      event_Scale_Glyph,      10 },
    { '-',      event_Scale_Glyph,     -10 },
    { 'u',      event_Scale_Glyph,       1 },
    { 'j',      event_Scale_Glyph,      -1 }
  };


  /* local variables */
  static struct Screen*  fts = NULL;
  static struct Window*  ftw = NULL;

  static int  graphx_mode;



  /* Exit gracefully */
  static void  AmigaCleanUp( void )
  {
    if ( Vio )
      FreeMem( Vio, vio_Height * vio_ScanLineWidth );

    ReleasePen( fts->ViewPort.ColorMap, gray_palette[0] );
    ReleasePen( fts->ViewPort.ColorMap, gray_palette[1] );

    if ( graphx_mode == Graphics_Mode_Gray )
    {
      ReleasePen( fts->ViewPort.ColorMap, gray_palette[2] );
      ReleasePen( fts->ViewPort.ColorMap, gray_palette[3] );
      ReleasePen( fts->ViewPort.ColorMap, gray_palette[4] );
    }

    if ( ftw )
      CloseWindow( ftw );

    if ( GfxBase )
      CloseLibrary( GfxBase );

    if ( IntuitionBase )
      CloseLibrary( IntuitionBase );
  }


  /* open libraries & custom screen */
  static int  AmigaInit( void )
  {
    /* cleanup at exit */
    if ( atexit( AmigaCleanUp ) )
    {
      PutStr( "atexit() failed\n" );
      return -1;
    }

    /* open intuition library */
    IntuitionBase = (struct Library*)OpenLibrary( "intuition.library", 39L );
    if ( IntuitionBase == NULL )
    {
      PutStr( "Could not open intuition library\n" );
      return -1;
    }
  
    /* open graphics library */
    GfxBase = OpenLibrary( "graphics.library", 39L );
    if ( GfxBase == NULL )
    {
      PutStr( "Could not open graphics library\n" );
      return -1;
    }                  

    /* get public screen */
    fts = LockPubScreen( NULL );

    if ( fts == NULL )
    {
      PutStr( "Could not lock public screen\n" );
      return -1;
    }

    if ( graphx_mode == Graphics_Mode_Gray )
    {
      vio_ScanLineWidth = GRAY_WINDOW_WIDTH;
      vio_Width         = GRAY_WINDOW_WIDTH;
      vio_Height        = GRAY_WINDOW_HEIGHT;

      gray_palette[0] = ObtainBestPenA( fts->ViewPort.ColorMap,
                          0x00000000, 0x00000000, 0x00000000, NULL );
      gray_palette[1] = ObtainBestPenA( fts->ViewPort.ColorMap,
                          0x33333300, 0x33333300, 0x33333300, NULL );
      gray_palette[2] = ObtainBestPenA( fts->ViewPort.ColorMap,
                          0x77777700, 0x77777700, 0x77777700, NULL );
      gray_palette[3] = ObtainBestPenA( fts->ViewPort.ColorMap,
                          0xBBBBBB00, 0xBBBBBB00, 0xBBBBBB00, NULL );
      gray_palette[4] = ObtainBestPenA( fts->ViewPort.ColorMap,
                          0xFFFFFF00, 0xFFFFFF00, 0xFFFFFF00, NULL );
    }
    else
    {
      vio_ScanLineWidth = MONO_WINDOW_WIDTH / 8;
      vio_Width         = MONO_WINDOW_WIDTH;
      vio_Height        = MONO_WINDOW_HEIGHT;

      gray_palette[0] = ObtainBestPenA( fts->ViewPort.ColorMap,
                          0xAAAAAA00, 0xAAAAAA00, 0xAAAAAA00, NULL );
      gray_palette[1] = ObtainBestPenA( fts->ViewPort.ColorMap,
                          0x00000000, 0x00000000, 0x00000000, NULL );
    }


    /* open intuition window */
    ftw = OpenWindowTags(
            NULL,
            WA_InnerWidth,    vio_Width,
            WA_InnerHeight,   vio_Height,
            WA_IDCMP,         IDCMP_VANILLAKEY | IDCMP_CLOSEWINDOW,
            /* WA_AutoAdjust, TRUE, */
            WA_CloseGadget,   TRUE,
            WA_Activate,      TRUE,
            WA_DragBar,       TRUE,
            WA_SmartRefresh,  TRUE,
            WA_Gadgets,       NULL,
            WA_Title,         (UBYTE*)"FreeType Project",
            WA_PubScreen,     fts,
            TAG_DONE );

    if ( ftw == NULL )
    {
      PutStr( "Could not open intuition window\n" );
      return -1;
    }

    UnlockPubScreen( NULL, fts );

    Vio = (char*)AllocMem( vio_Height * vio_ScanLineWidth, MEMF_ANY );
    if ( Vio == NULL )
    {
      PutStr( "Could not allocate memory\n" );
      return -1;
    }

    return 0;
  }


  /* get events in the window */
  static char  Get_Intuition_Event( void )
  {
    struct IntuiMessage*  msg;
    ULONG                 class;
    USHORT                code;


    WaitPort( ftw->UserPort );

    while ( ( msg = (struct IntuiMessage*)GetMsg( ftw->UserPort ) ) )
    {
      class = msg->Class;
      code = msg->Code;

      ReplyMsg( (struct Message*)msg );

      switch( class )
      {
      case IDCMP_CLOSEWINDOW:
        return (char)27;

      case IDCMP_VANILLAKEY:
        return (char)code;
      }
    }

    return '\0';
  }


  /* Set Amiga graphics mode */
  int  Driver_Set_Graphics( int  mode )
  {
    graphx_mode = mode;
    
    if ( AmigaInit() == -1 )
      return 0;         /* failure */
  
    return 1;           /* success */
  }


  /* restore screen to its original state */
  int  Driver_Restore_Mode( void )
  {
   /* Do nothing */
        
   return 1;            /* success */
  }


  /* display bitmap */
  int  Driver_Display_Bitmap( char*  buffer, int  line, int  col )
  {
    int    y, z;
    char*  target;
    char   old = 0;


    target = Vio + ( line - 1 ) * vio_ScanLineWidth;

    for ( y = 0; y < line; y++ )
    {
      CopyMem( buffer, target, col );
      target -= vio_ScanLineWidth;
      buffer += col;
    }
    
    /* clear window */
    /* SetRast( ftw->RPort, gray_palette[0] ); */

    SetAPen( ftw->RPort, gray_palette[0] );

    RectFill( ftw->RPort, ftw->BorderLeft, ftw->BorderTop,
                          ftw->Width - ftw->BorderRight - 1, 
                          ftw->Height - ftw->BorderBottom - 1 );

    if ( graphx_mode != Graphics_Mode_Gray )
    {
      SetAPen( ftw->RPort, gray_palette[1] );
      old = 1;
    }

    /* Draw glyph */
    for ( y = 0; y < line; y++ )
    {
      for ( z = 0; z < col; z++ )
      {
        int  c = Vio[y * vio_ScanLineWidth + z];

        if ( graphx_mode == Graphics_Mode_Gray )
        {
          if ( c != 0 && c != gray_palette[0] )
          {
            if ( old != c )
            {
              old = c;
              /* printf("x = %d, y = %d, color = %d\n", z, y, c ); */
              SetAPen( ftw->RPort, c );
            }

            WritePixel( ftw->RPort, ftw->BorderLeft + z, ftw->BorderTop + y );
          }
        }
        else
        {
          int  mask    = 0x80;
          int  counter = 0;

          while ( mask )
          {
            if ( mask & c )
              WritePixel( ftw->RPort, ftw->BorderLeft + z * 8 + counter,
                                      ftw->BorderTop + y );

            counter++;
            mask >>= 1;
          }
        }
      }
    }
    
    return 1;               /* success */
  }


  void  Get_Event( TEvent*  event )
  {
    int   i;
    char  c;

    
    c = Get_Intuition_Event();

    if ( c != '\0' )
      for ( i = 0; i < NUM_Translators; i++ )
      {
        if ( c == trans[i].key )
        {
          event->what = trans[i].event_class;
          event->info = trans[i].event_info;
          return;
        }
      }

    /* unrecognized keystroke */

    event->what = event_Keyboard;
    event->info = (int)c;

    return;
  }


/* End */
