/*******************************************************************
 *
 *  gfs_dos.c  graphics utility fullscreen Dos driver.         1.0
 *
 *  This is the driver for fullscreen Dos display, used by the
 *  graphics utility of the FreeType test suite.
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

#include "gdriver.h"
#include "gmain.h"
#include "gevents.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>


/* The following #ifdef are used to define the following macros :          */
/*                                                                         */
/*  - int86  : function to call an interrupt                               */
/*  - reg_ax : the 'ax' register as stored in the REGS struct              */
/*                                                                         */

/* ---- DJGPP dos compiler support --------------------------------------- */

#if 0                   /* 0 for __DJGPP__ #defines, 1 for DJGPP2 */
#undef __DJGPP__
#define DJGPP2
#else
#undef DJGPP2
#endif

#ifdef __DJGPP__

#undef __STRICT_ANSI__
#include <sys/nearptr.h>
#include <dos.h>
#include <strings.h>
#include <conio.h>

#define reg_ax regs.x.ax

#endif

#ifdef DJGPP2

#include <bios.h>
#include <conio.h>
#include <sys/movedata.h>
#define int86   _int86
#define reg_ax  regs.x.ax

#endif

/* ---- Borland C 3.1 dos support ---------------------------------------- */

#ifdef __BORLANDC__

#include <dos.h>                     /* Includes the declaration of int86  */

#define reg_ax  regs.x.ax

#endif

/* ---- EMX/Dos compiler support ----------------------------------------- */

#ifdef __EMX__

#include <dos.h>
#include <sys/hw.h>
#include <conio.h>       /* for getch                                   */
  extern _read_kbd();    /* to avoid an ANSI warning during compilation */

#define int86   _int86
#define reg_ax  regs.x.ax

#endif

/* ---- WATCOM Dos/16 & Dos/32 support ----------------------------------- */

#ifdef __WATCOMC__               

#include <i86.h>

#define reg_ax  regs.w.ax

#ifdef __386__
#define int86   int386
#endif

#endif


#if !defined( reg_ax ) || !defined( int86 )
#error Your compiler is not (yet) supported.  Check the source file!
#endif


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


  /* Set Graphics Mode */

  int  Driver_Set_Graphics( int  mode ) 
  {
    union REGS  regs;


    switch ( mode )
    {
    case Graphics_Mode_Mono:    /* Standard VGA 640x480x16 mode */
      reg_ax = 0x12;
      int86( 0x10, &regs, &regs );

      vio_ScanLineWidth = 80;
      vio_Width         = 640;
      vio_Height        = 480;
      break;

    case Graphics_Mode_Gray:    /* Standard VGA 320x200x256 mode */
      reg_ax = 0x13;
      int86( 0x10, &regs, &regs );

      vio_ScanLineWidth = 320;
      vio_Width         = 320;
      vio_Height        = 200;

      /* default gray_palette takes the gray levels of the standard VGA */
      /* 256 colors mode                                                */
    
      gray_palette[0] = 0;
      gray_palette[1] = 23;
      gray_palette[2] = 27;
      gray_palette[3] = 29;
      gray_palette[4] = 31;
      break;

    default:
      return 0;                 /* failure */
    }

#ifdef __EMX__
    Vio = _memaccess( 0xA0000, 0xAFFFF, 1 );
#elif defined(DJGPP2) || defined(__DJGPP__)
    Vio = (char *)0xA0000;
#elif defined(__WATCOMC__) && defined(__386__)
    Vio = (char *)0xA0000;
#else
    Vio = (char*)MK_FP( 0xA000, 0 );
#endif

    return 1;                   /* success */
  }


  /* Revert to text mode */

  int  Driver_Restore_Mode()
  {
    union REGS  regs;


    reg_ax = 0x3;
    int86( 0x10, &regs, &regs );

    return 1;                   /* success */
  }


  int  Driver_Display_Bitmap( char*  buffer, int  line, int  col )
  {
    int    y, used_col;
    char*  target;


#ifdef DJGPP2
    char cbuf = 0;
    int i;
    for( i = 0; i < vio_Height*vio_ScanLineWidth; i++ )
         dosmemput( &cbuf, 1, (unsigned long) Vio+i );
#else

#ifdef __DJGPP__
    __djgpp_nearptr_enable();
    Vio += __djgpp_conventional_base;

#else

    memset( Vio, 0, vio_Height * vio_ScanLineWidth ); 

#endif  /* __DJGPP__ */

#endif  /* DJGPP2 */

    if ( line > vio_Height )
      line = vio_Height;
    if ( col > vio_ScanLineWidth )
      used_col = vio_ScanLineWidth;
    else
      used_col = col;

    target = Vio + ( line - 1 ) * vio_ScanLineWidth;

    for ( y = 0; y < line; y++ )
    {
#if defined(DJGPP2)
      dosmemput( buffer, used_col, (unsigned long)target );
#else
      memcpy( target, buffer, used_col );
#endif
      target -= vio_ScanLineWidth;
      buffer += col;
    }


#ifdef __DJGPP__
    __djgpp_nearptr_disable();
    Vio -= __djgpp_conventional_base;
#endif

    return 1;                   /* success */
  }


  void  Get_Event( TEvent*  event )
  {
    int   i;
    char  c;

    
    c = getch();

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
