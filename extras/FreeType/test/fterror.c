/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  E. Dieterich                                                            */
/*                                                                          */
/*  fterror: test errstr functionality.                                     */
/*                                                                          */
/****************************************************************************/


#include "freetype.h"
#include "ftxerr18.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif

#ifndef HAVE_LIBINTL_H
#define gettext( x )  ( x )
#endif


  int  main( void )
  {
    int    i;
#ifdef HAVE_LIBINTL_H
    char*  domain;


    setlocale( LC_ALL, "" );
    bindtextdomain( "freetype", LOCALEDIR );
    domain = textdomain( "freetype" );
#endif

#if 0
    printf( "domain: %s\n", domain = textdomain( "" ) );
#endif
    printf( gettext("Start of fterrtest.\n" ) );

    for ( i = 0; i < 10; i++ )
      printf( "Code: %i, %s\n", i, TT_ErrToString18( i ) );

#if 0
    printf( "domain: %s\n", domain = textdomain( "" ) );
#endif
    printf( gettext( "End of fterrtest.\n" ) );

    exit( EXIT_SUCCESS );      /* for safety reasons */

    return 0;       /* never reached */
  }


/* End */
