/*******************************************************************
 *
 *  ftxgasp.h                                                   1.0
 *
 *    Gasp table support API extension
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
 *
 *  The gasp table is currently loaded by the core engine, but the 
 *  standard API doesn't give access to it.  This file is used to
 *  demonstrate the use of a simple API extension.            
 *
 ******************************************************************/

/* $XFree86: $ */
  
 /* You must include this file after "freetype.h" */

#ifndef FTXGASP_H
#define FTXGASP_H

#include "freetype.h"

#ifdef __cplusplus
extern "C" {
#endif


  /* This function returns for a given 'point_size' the values of the */
  /* gasp flags 'grid_fit' and 'smooth_font'.  The returned values    */
  /* arebooleans (where 0 = NO, and 1 = YES).                         */

  /* Note that this function will returned TT_Err_Table_Missing if    */
  /* the font file doesn't contain any gasp table.                    */

  TT_Error  TT_Get_Face_Gasp_Flags( TT_Face  face,
                                    int      point_size,
                                    int*     grid_fit,
                                    int*     smooth_font );

#ifdef __cplusplus
}
#endif

#endif /* FTXGASP_H */


/* END */
