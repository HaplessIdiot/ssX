/*
 * $Id: fbbits.c,v 1.2 2000/01/18 16:35:41 tsi Exp $
 *
 * Copyright © 1998 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $XFree86: xc/programs/Xserver/fb/fbbits.c,v 1.1 1999/11/19 13:53:41 hohndel Exp $ */

#include "fb.h"
#include "miline.h"
#include "mizerarc.h"

#undef BRESSOLID
#undef BRESDASH
#undef DOTS
#undef ARC
#undef GLYPH
#undef BITS
#undef BITS2
#undef BITS4

#define BRESSOLID   fbBresSolid8
#define BRESDASH    fbBresDash8
#define DOTS	    fbDots8
#define ARC	    fbArc8
#define GLYPH	    fbGlyph8
#define BITS	    BYTE
#define BITS2	    CARD16
#define BITS4	    CARD32

#include "fbbits.h"

#undef BRESSOLID
#undef BRESDASH
#undef DOTS
#undef ARC
#undef GLYPH
#undef BITS
#undef BITS2
#undef BITS4

#define BRESSOLID   fbBresSolid16
#define BRESDASH    fbBresDash16
#define DOTS	    fbDots16
#define ARC	    fbArc16
#define GLYPH	    fbGlyph16
#define BITS	    CARD16
#define BITS2	    CARD32
#if FB_SHIFT == 6
#define BITS4	    FbBits
#endif
#include "fbbits.h"

#undef BRESSOLID
#undef BRESDASH
#undef DOTS
#undef ARC
#undef GLYPH
#undef BITS
#undef BITS2
#if FB_SHIFT == 6
#undef BITS4
#endif

#define BRESSOLID   fbBresSolid32
#define BRESDASH    fbBresDash32
#define DOTS	    fbDots32
#define ARC	    fbArc32
#define GLYPH	    fbGlyph32
#define BITS	    CARD32
#if FB_SHIFT == 6
#define BITS2	    FbBits
#endif

#include "fbbits.h"

#undef BRESSOLID
#undef BRESDASH
#undef DOTS
#undef ARC
#undef GLYPH
#undef BITS
#if FB_SHIFT == 6
#undef BITS2
#endif
