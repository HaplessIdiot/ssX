/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/fontsym.c,v 1.2 1998/07/25 16:56:15 dawes Exp $ */

#include "sym.h"
#include "fntfilst.h"
#include "fontenc.h"

extern void TwoByteSwap();
extern void FourByteSwap();
extern Bool FontCouldBeTerminal();
extern int BufFileRead();
extern int BufFileWrite();
extern int CheckFSFormat();
extern int FontFileOpen();
extern Bool FontFileRegisterRenderer();
extern Bool FontParseXLFDName();
extern void FontFileCloseFont();
extern int FontFileOpenBitmap();
extern Bool FontFileCompleteXLFD();
extern int FontFileCountDashes();
extern FontEntryPtr FontFileFindNameInDir();
extern int FontFileClose();
extern Bool FontComputeInfoAccelerators();
extern void FontDefaultFormat();
extern char *NameForAtom();
extern void BitOrderInvert();
extern FontRendererPtr FontFileMatchRenderer();
extern int RepadBitmap();

LOOKUP fontLookupTab[] = {

  SYMFUNC(TwoByteSwap)
  SYMFUNC(FourByteSwap)
  SYMFUNC(FontCouldBeTerminal)
  SYMFUNC(BufFileRead)
  SYMFUNC(BufFileWrite)
  SYMFUNC(CheckFSFormat)
  SYMFUNC(FontFileOpen)
  SYMFUNC(FontFileRegisterRenderer)
  SYMFUNC(FontParseXLFDName)
  SYMFUNC(FontFileCloseFont)
  SYMFUNC(FontFileOpenBitmap)
  SYMFUNC(FontFileCompleteXLFD)
  SYMFUNC(FontFileCountDashes)
  SYMFUNC(FontFileFindNameInDir)
  SYMFUNC(FontFileClose)
  SYMFUNC(FontComputeInfoAccelerators)
  SYMFUNC(FontDefaultFormat)
  SYMFUNC(NameForAtom)
  SYMFUNC(BitOrderInvert)
  SYMFUNC(FontFileMatchRenderer)
  SYMFUNC(RepadBitmap)
  SYMFUNC(font_encoding_name)
  SYMFUNC(font_encoding_recode)
  SYMFUNC(font_encoding_find)
  SYMFUNC(font_encoding_from_xlfd)
  
  SYMVAR(FontFileBitmapSources)

  { 0, 0 },

};
