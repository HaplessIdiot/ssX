/* $XFree86$ */
/*
 * Copyright 1994 by Henry A. Worth, Sunnyvale, California.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Henry A. Worth not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Henry A. Worth makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 * HENRY A. WORTH DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY 
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, 
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF 
 * THIS SOFTWARE.
 *
 */

#ifndef AGX_H
#define AGX_H

#define AGX_PATCHLEVEL "0"

#define AGX_CURSBYTES		1024
#define AGX_CURSMAX		64

#include "X.h"
#include "misc.h"
#include "xf86.h"
#include "regionstr.h"
#include "regagx.h"

extern Bool agxProbe();
extern Bool agxInitialize();
extern void agxEnterLeaveVT();
extern void agxAdjustFrame();
extern Bool agxSwitchMode();
extern void agxPrintIdent();
extern Bool agxClockSelect();
extern Bool xgaNiClockSelect();
extern void agxResetCRTC();

extern agxSaveBlock *agxHWSave();
extern void         agxHWRestore();

extern void agxSetRead();
extern void agxSetReadWrite();
extern void agxSetWrite();

extern void agxDline();
extern void agxDsegment();

extern Bool agxSaveScreen();
extern Bool agxCloseScreen();

extern Bool agxInit();
extern void agxInitDisplay();
extern void agxInitAperture();
extern void agxInitEnvironment();
extern void agxCalcCRTCRegs();
extern void agxSetCRTCRegs();
extern void agxInitLUT();
extern void agxSaveLUT();
extern void agxRestoreLUT();
extern void agxCleanUp();
extern void agxSetRamdac();

extern int agxListInstalledColormaps();
extern int agxGetInstalledColormaps();
extern void agxStoreColors();
extern void agxInstallColormap();
extern void agxUninstallColormap();
extern void agxRestoreColor0();

extern Bool agxRealizeFont();
extern Bool agxUnrealizeFont();

extern void (*agxImageReadFunc)();
extern void (*agxImageWriteFunc)();
extern void (*agxImageFillFunc)();
extern void agxImageStipple();
extern void agxImageOpStipple();
extern int  agxBytePadScratchMapPow2();

extern void agxCacheInit();
extern void agxFontCache8Init();
extern void agxImageInit();

extern int agxCacheTile();
extern int agxCacheStipple();
extern int agxCacheOpStipple();
extern Bool agxCachableTile();
extern Bool agxCachableStipple();
extern Bool agxCachableOpStipple();
extern void agxCImageFill();
extern void agxCImageStipple();
extern void agxCImageOpStipple();
extern void agxCacheFreeSlot();

extern void agxUnCacheFont8();
extern int agxCPolyText8();
extern int agxCImageText8();

extern void agxPolyPoint();
extern void agxLine();
extern void agxLine1Rect();
extern void agxSegment();

extern void agxSetSpans();
extern void agxGetSpans();

extern void agxSolidFSpans();
extern void agxTiledFSpans();
extern void agxStipFSpans();
extern void agxOStipFSpans();

extern void agxPolyFillRect();

extern int agxPolyText8();
extern void agxImageText8();
extern int agxPolyText16();
extern void agxImageText16();

extern void agxFindOrdering();
extern RegionPtr agxCopyArea();
extern RegionPtr agxCopyPlane();
extern void agxCopyWindow();
extern void agxGetImage();

extern void agxPaintWindow();
extern void agxFillBoxSolid();

extern void agxSaveAreas();
extern void agxRestoreAreas();

extern Bool agxCreateGC();

extern Bool agxCursorInit();
extern void agxRestoreCursor();
extern void agxCursorOff();
extern void agxRenewCursorColor();
extern void agxQueryBestSize();
extern void agxWarpCursor();

extern int agxGetMemSize();

extern void agxMapSetSrc();
extern void agxMapSetDst();
extern void agxMapSetSrcDst();

extern pointer agxVideoMem;
extern pointer agxPhysVidMem;
extern unsigned int  agxMemBase;
extern unsigned char agxPOSMemBase;
extern unsigned int  agxFontCacheOffset;
extern unsigned int  agxFontCacheSize;
extern unsigned int  agxScratchOffset;
extern unsigned int  agxScratchSize;
extern unsigned int  agxHWCursorOffset;
extern Bool     agxHWCursor;


extern pointer vgaBase;
extern pointer vgaVirtBase;
extern pointer vgaPhysBase;
extern int     vgaIOBASE;
extern int     vgaCRIndex;
extern int     vgaCRReg;
extern Bool    vgaUse2Banks;
extern Bool    useSpeedUp;

extern Bool xf86VTSema;
extern short agxMaxX, agxMaxY;
extern short agxVirtX, agxVirtY;
extern short agxAdjustedVirtX;
extern Bool  agx128WidthAdjust;
extern Bool  agx256WidthAdjust;
extern Bool  agx288WidthAdjust;
extern Bool agxUse4MbAperture;
extern Bool (*agxClockSelectFunc)();


extern Bool agxSaveVGA;
extern Bool agxInited;

extern pointer      agxGEBase;
extern pointer      agxGEPhysBase;
extern unsigned int agxIdxReg;
extern unsigned int agxDAReg;
extern unsigned int agxApIdxReg;
extern unsigned int agxByteData;
extern unsigned int agxChipId;
extern unsigned int xf86RamDacBase;
extern unsigned int vgaBankSize;
extern unsigned int agxBankSize;

extern int agxDac8Bit;
extern int agxPixMux;
extern int agxBusType;

extern agxCRTCRegRec agxCRTCRegs;
extern agxSaveBlock  *agxSavedState;

extern ScrnInfoRec agxInfoRec;
extern int agxValidTokens[];

extern short agxalu[];

#endif
