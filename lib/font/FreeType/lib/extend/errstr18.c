/****************************************************************************/
/*                                                                          */
/*  Erwin Dieterich,  15. 10. 1997                                          */
/*                  - 21. 01. 1998                                          */
/*                                                                          */
/*  TT_ErrToString: translate error codes to character strings              */
/*                                                                          */
/****************************************************************************/

/* $XFree86: $ */
  
#include "errstr18.h"
#include "freetype.h"
#include "ftxkern.h"
#include "ft_conf.h"
#include <stdio.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif

#ifndef HAVE_LIBINTL
#define gettext(x) (x)
#endif


char* TT_ErrToString18(int error)
{
  /* First we find out the old domain to store it.  Then the domain   */
  /* is set to freetype and the error code is translated to a string. */
  /* Before we leave TT_ErrToString18() we set the domain back again. */

#ifdef HAVE_LIBINTL
  char *newdomain;
  /* char *olddomain; */
  setlocale (LC_ALL, "");
  bindtextdomain ("freetype", LOCALEDIR);
  /* olddomain = textdomain(NULL); */
  newdomain = textdomain ("freetype");
  /* printf ("olddomain %s\n",olddomain); */
  /* printf ("newdomain %s\n",newdomain); */
#endif


  switch (error)
  {
    /* ----- high-level API error codes -----*/
  case TT_Err_Ok:
    return gettext("Successful function call, no error.");

  case TT_Err_Invalid_Face_Handle:
    return gettext("Invalid face handle.");
  case TT_Err_Invalid_Instance_Handle:
    return gettext("Invalid instance handle.");
  case TT_Err_Invalid_Glyph_Handle:
    return gettext("Invalid glyph handle.");
  case TT_Err_Invalid_CharMap_Handle:
    return gettext("Invalid charmap handle.");
  case TT_Err_Invalid_Result_Address:
    return gettext("Invalid result address.");
  case TT_Err_Invalid_Glyph_Index:
    return gettext("Invalid glyph index.");
  case TT_Err_Invalid_Argument:
    return gettext("Invalid argument.");
  case TT_Err_Could_Not_Open_File:
    return gettext("Could not open file.");
  case TT_Err_File_Is_Not_Collection:
    return gettext("File is not a TrueType collection.");

  case TT_Err_Table_Missing:
    return gettext("Mandatory table missing.");
  case TT_Err_Invalid_Horiz_Metrics:
    return gettext("Invalid horizontal metrics (hmtx table broken).");
  case TT_Err_Invalid_CharMap_Format:
    return gettext("Invalid charmap format.");

  case TT_Err_Invalid_File_Format:
    return gettext("Invalid file format.");
      
  case TT_Err_Invalid_Engine:
    return gettext("Invalid engine.");
  case TT_Err_Too_Many_Extensions:
    return gettext("Too many extensions (max: 8).");
  case TT_Err_Extensions_Unsupported:
    return gettext("Extensions unsupported.");
  case TT_Err_Invalid_Extension_Id:
    return gettext("Invalid extension id.");
      
  case TT_Err_Max_Profile_Missing:
    return gettext("Maximum Profile (maxp) table missing.");
  case TT_Err_Header_Table_Missing:
    return gettext("Font Header (head) table missing.");
  case TT_Err_Horiz_Header_Missing:
    return gettext("Horizontal Header (hhea) table missing.");
  case TT_Err_Locations_Missing:
    return gettext("Index to Location (loca) table missing.");
  case TT_Err_Name_Table_Missing:
    return gettext("Naming (name) table missing.");
  case TT_Err_CMap_Table_Missing:
    return gettext("Character to Glyph Index Mapping (cmap) tables missing.");
  case TT_Err_Hmtx_Table_Missing:
    return gettext("Horizontal Metrics (hmtx) table missing.");
  case TT_Err_OS2_Table_Missing:
    return gettext("OS/2 table missing.");
  case TT_Err_Post_Table_Missing:
    return gettext("PostScript (post) table missing.");
      
      
  /* ----- memory component error codes -----*/
  case TT_Err_Out_Of_Memory:
    return gettext("Out of memory.");
      
      
  /* ----- file component error codes -----*/
  case TT_Err_Invalid_File_Offset:
    return gettext("Invalid file offset.");
  case TT_Err_Invalid_File_Read:
    return gettext("Invalid file read.");
  case TT_Err_Invalid_Frame_Access:
    return gettext("Invalid frame access.");
      
      
  /* ----- glyph loader error codes -----*/
  case TT_Err_Too_Many_Points:
    return gettext("Too many points.");
  case TT_Err_Too_Many_Contours:
    return gettext("Too many contours.");
  case TT_Err_Invalid_Composite:
    return gettext("Invalid composite glyph.");
  case TT_Err_Too_Many_Ins:
    return gettext("Too many instructions.");
      
      
  /* ----- byte-code interpreter error codes -----*/
  case TT_Err_Invalid_Opcode:
    return gettext("Invalid opcode.");
  case TT_Err_Too_Few_Arguments:
    return gettext("Too few arguments.");
  case TT_Err_Stack_Overflow:
    return gettext("Stack overflow.");
  case TT_Err_Code_Overflow:
    return gettext("Code overflow.");
  case TT_Err_Bad_Argument:
    return gettext("Bad argument.");
  case TT_Err_Divide_By_Zero:
    return gettext("Divide by zero.");
  case TT_Err_Storage_Overflow:
    return gettext("Storage overflow.");
  case TT_Err_Cvt_Overflow:
    return gettext("Control Value (cvt) table overflow.");
  case TT_Err_Invalid_Reference:
    return gettext("Invalid reference.");
  case TT_Err_Invalid_Distance:
    return gettext("Invalid distance.");
  case TT_Err_Interpolate_Twilight:
    return gettext("Interpolate twilight points.");
  case TT_Err_Debug_OpCode:
    return gettext("`DEBUG' opcode found.");
  case TT_Err_ENDF_In_Exec_Stream:
    return gettext("`ENDF' in byte-code stream.");
  case TT_Err_Out_Of_CodeRanges:
    return gettext("Out of code ranges.");
  case TT_Err_Nested_DEFS:
    return gettext("Nested function definitions.");
  case TT_Err_Invalid_CodeRange:
    return gettext("Invalid code range.");
  case TT_Err_Invalid_Displacement:
    return gettext("Invalid displacement.");
      
      
  /* ----- internal failure error codes -----*/
  case TT_Err_Nested_Frame_Access:
    return gettext("Nested frame access.");
  case TT_Err_Invalid_Cache_List:
    return gettext("Invalid cache list.");
  case TT_Err_Could_Not_Find_Context:
      return gettext("Could not find context.");
  case TT_Err_Unlisted_Object:
    return gettext("Unlisted object.");
      
      
  /* ----- scan-line converter error codes -----*/
  case TT_Err_Raster_Pool_Overflow:
    return gettext("Raster pool overflow.");
  case TT_Err_Raster_Negative_Height:
    return gettext("Raster: negative height encountered.");
  case TT_Err_Raster_Invalid_Value:
    return gettext("Raster: invalid value.");
  case TT_Err_Raster_Not_Initialized:
    return gettext("Raster not initialized.");
      
      
  /* ----- engine extensions error codes -----*/
  case TT_Err_Invalid_Kerning_Table_Format:
    return gettext("Invalid kerning (kern) table format.");
  case TT_Err_Invalid_Kerning_Table:
    return gettext("Invalid kerning (kern) table.");
      
      
  /* ----- unknown error code -----*/
  default:
    return gettext("Invalid Error Number.");
  }


#ifdef HAVE_LIBINTL
  /* newdomain = textdomain (olddomain); */
#endif

  return NULL; /* never reached */
}
