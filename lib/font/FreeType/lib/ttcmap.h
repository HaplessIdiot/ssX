/*******************************************************************
 *
 *  ttcmap.h                                                    1.0
 *
 *    TrueType Character Mappings                      
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
 *
 ******************************************************************/

#ifndef TTCMAP_H
#define TTCMAP_H

#include "ttcommon.h"
#include "tttypes.h"


#ifdef __cplusplus
  extern "C" {
#endif

  /* format 0 */

  struct  _TCMap0
  {
    PByte  glyphIdArray;
  };

  typedef struct _TCMap0  TCMap0;
  typedef TCMap0*         PCMap0;


  /* format 2 */

  struct  _TCMap2SubHeader
  {
    UShort  firstCode;      /* first valid low byte         */
    UShort  entryCount;     /* number of valid low bytes    */
    Short   idDelta;        /* delta value to glyphIndex    */
    UShort  idRangeOffset;  /* offset from here to 1st code */
  };

  typedef struct _TCMap2SubHeader  TCMap2SubHeader;
  typedef TCMap2SubHeader*         PCMap2SubHeader;

  struct  _TCMap2
  {
    PUShort subHeaderKeys;
    /* high byte mapping table            */
    /* value = subHeader index * 8        */

    PCMap2SubHeader  subHeaders;
    PUShort          glyphIdArray;
  };

  typedef struct _TCMap2  TCMap2;  
  typedef TCMap2*         PCMap2;


  /* format 4 */

  struct  _TCMap4Segment
  {
    UShort  endCount;
    UShort  startCount;
    Short   idDelta;        /* in the specs defined as UShort but the
                               example there gives negative values... */
    UShort  idRangeOffset;
  };

  typedef struct _TCMap4Segment  TCMap4Segment;
  typedef TCMap4Segment*         PCMap4Segment;

  struct  _TCMap4
  {
    UShort  segCountX2;     /* number of segments * 2       */
    UShort  searchRange;    /* these parameters can be used */
    UShort  entrySelector;  /* for a binary search          */
    UShort  rangeShift;

    PCMap4Segment  segments;
    PUShort        glyphIdArray;
  };

  typedef struct _TCMap4  TCMap4;
  typedef TCMap4*         PCMap4;


  /* format 6 */

  struct  _TCMap6
  {
    UShort   firstCode;      /* first character code of subrange      */
    UShort   entryCount;     /* number of character codes in subrange */

    PUShort  glyphIdArray;
  };

  typedef struct _TCMap6  TCMap6;
  typedef TCMap6*         PCMap6;


  /* charmap table */

  struct  _TCMapTable
  {
    UShort  platformID;
    UShort  platformEncodingID;
    UShort  format;
    UShort  length;
    UShort  version;

    Bool    loaded;
    Long    offset;

    union
    {
      TCMap0  cmap0;
      TCMap2  cmap2;
      TCMap4  cmap4;
      TCMap6  cmap6;
    } c;
  };

  typedef struct _TCMapTable  TCMapTable;
  typedef TCMapTable*         PCMapTable;



  /* Load character mappings directory when face is loaded. */
  /* The mappings themselves are only loaded on demand.     */

  TT_Error  CharMap_Load( PCMapTable  table, TT_Stream  input );


  /* Destroy one character mapping table */

  TT_Error  CharMap_Free( PCMapTable  table );


  /* Use character mapping table to perform mapping */

  Int  CharMap_Index( PCMapTable  cmap,
                      UShort      charcode );

  /* NOTE: The PFace type isn't defined at this point */

#ifdef __cplusplus
  }
#endif

#endif /* TTCMAP_H */


/* END */
