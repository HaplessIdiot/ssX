/*******************************************************************
 *
 *  tttags.h
 *
 *    tags for TrueType tables (specification only).
 *
 *  Copyright 1996-1998 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT. By continuing to use, modify, or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 ******************************************************************/

/* $XFree86: $ */
  
#ifndef TTAGS_H
#define TTAGS_H

#define __MAKE_TT_TAG( _x1, _x2, _x3, _x4 ) \
          (_x1 << 24 | _x2 << 16 | _x3 << 8 | _x4)

#define TTAG_cmap __MAKE_TT_TAG( 'c', 'm', 'a', 'p' )
#define TTAG_cvt  __MAKE_TT_TAG( 'c', 'v', 't', ' ' )
#define TTAG_EBDT __MAKE_TT_TAG( 'E', 'B', 'D', 'T' )
#define TTAG_EBLC __MAKE_TT_TAG( 'E', 'B', 'L', 'C' )
#define TTAG_EBSC __MAKE_TT_TAG( 'E', 'B', 'S', 'C' )
#define TTAG_fpgm __MAKE_TT_TAG( 'f', 'p', 'g', 'm' )
#define TTAG_gasp __MAKE_TT_TAG( 'g', 'a', 's', 'p' )
#define TTAG_glyf __MAKE_TT_TAG( 'g', 'l', 'y', 'f' )
#define TTAG_hdmx __MAKE_TT_TAG( 'h', 'd', 'm', 'x' )
#define TTAG_head __MAKE_TT_TAG( 'h', 'e', 'a', 'd' )
#define TTAG_hhea __MAKE_TT_TAG( 'h', 'h', 'e', 'a' )
#define TTAG_hmtx __MAKE_TT_TAG( 'h', 'm', 't', 'x' )
#define TTAG_kern __MAKE_TT_TAG( 'k', 'e', 'r', 'n' )
#define TTAG_loca __MAKE_TT_TAG( 'l', 'o', 'c', 'a' )
#define TTAG_LTSH __MAKE_TT_TAG( 'L', 'T', 'S', 'H' )
#define TTAG_maxp __MAKE_TT_TAG( 'm', 'a', 'x', 'p' )
#define TTAG_name __MAKE_TT_TAG( 'n', 'a', 'm', 'e' )
#define TTAG_OS2  __MAKE_TT_TAG( 'O', 'S', '/', '2' )
#define TTAG_PCLT __MAKE_TT_TAG( 'P', 'C', 'L', 'T' )
#define TTAG_post __MAKE_TT_TAG( 'p', 'o', 's', 't' )
#define TTAG_prep __MAKE_TT_TAG( 'p', 'r', 'e', 'p' )
#define TTAG_ttc  __MAKE_TT_TAG( 't', 't', 'c', ' ' )
#define TTAG_ttcf __MAKE_TT_TAG( 't', 't', 'c', 'f' )
#define TTAG_VDMX __MAKE_TT_TAG( 'V', 'D', 'M', 'X' )
#define TTAG_vhea __MAKE_TT_TAG( 'v', 'h', 'e', 'a' )
#define TTAG_vmtx __MAKE_TT_TAG( 'v', 'm', 't', 'x' )

#endif /* TTAGS_H */


/* END */
