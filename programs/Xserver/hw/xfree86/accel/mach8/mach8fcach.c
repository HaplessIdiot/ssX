/* $XConsortium: mach8fcach.c,v 1.1 94/03/28 21:10:58 dpw Exp $ */
/*
 * Copyright 1992 by Kevin E. Martin, Chapel Hill, North Carolina.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Kevin E. Martin not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Kevin E. Martin makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 * KEVIN E. MARTIN AND RICKARD E. FAITH DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL KEVIN E. MARTIN OR RICKARD E. FAITH BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Modified for the Mach-8 by Rickard E. Faith (faith@cs.unc.edu)
 *
 * Modified to use a dynamic font cache.
 * Accelerated text display when no font cache is available added.
 * Now using a linked list to store the CacheFont8Rec structs instead
 * of a pointer array.
 * Hans Nasten. (nasten@everyware.se)
 */


#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"cfb.h"
#include	"misc.h"
/*
#include	"windowstr.h"
#include	"gcstruct.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
*/
#include	"regmach8.h"
#include	"mach8.h"
#include	"xf86bcache.h"
#include	"xf86fcache.h"
#include	"xf86text.h"
#include	"mach8cache.h"

#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
extern Bool xf86Verbose;

void
mach8FontCache8Init()
{
    int first = 1;
    int x, y, w, h;
    unsigned int BitPlane;
    CachePool FontPool;

    x = ( mach8InfoRec.virtualX < 1024 ? 0 : 256 );
    y = mach8InfoRec.virtualY;
    w = ( mach8InfoRec.virtualX < 1024 ? mach8InfoRec.virtualX : 768 );
    h = 1024 - mach8InfoRec.virtualY;
    if( w > 0 && h > 0 ) {
      if( first ) {
        FontPool = xf86CreateCachePool( 8 );
        for( BitPlane = 0; BitPlane < 7; BitPlane++ )
	  xf86AddToCachePool( FontPool, x, y, w, h, BitPlane );

        xf86InitFontCache( FontPool, w, h, mach8ImageOpStipple, mach8alu );
        xf86InitText( mach8GlyphWrite, mach8NoCPolyText,
		      mach8NoCImageText );

        ErrorF( "%s %s: Using %dx%d at %dx%d aligned %d as font cache\n",
	        XCONFIG_PROBED, mach8InfoRec.name,
	        w, h, x, y, 8 );
        first = 0;
      }
      else
        xf86ReleaseFontCache();
    }
    else if( first ) {
      ErrorF( "%s %s: No font cache available\n",
	      XCONFIG_PROBED, mach8InfoRec.name );
      first = 1;
    }

}
