#include "sym.h"
#include "fntfilst.h"

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
  
  SYMVAR(FontFileBitmapSources)

  { 0, 0 },

};
