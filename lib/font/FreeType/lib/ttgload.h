/*******************************************************************
 *
 *  ttgload.h                                                   1.0
 *
 *    TrueType Glyph Loader.                           
 *
 *  Copyright 1996-1998 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT.  By continuing to use, modify, or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 ******************************************************************/

#ifndef TTGLOAD_H
#define TTGLOAD_H

#include "ttcommon.h"
#include "ttobjs.h"

#ifdef __cplusplus
  extern "C" {
#endif

  TT_Error  Load_TrueType_Glyph( PInstance  instance,
                                 PGlyph     glyph,
                                 Int        glyph_index,
                                 Int        load_flags );

#ifdef __cplusplus
  }
#endif


#endif /* TTGLOAD_H */


/* END */
