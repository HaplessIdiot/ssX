/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xf86defs.c,v 3.13 1997/09/25 07:31:15 hohndel Exp $ */


#include "windowstr.h"
#include "gcstruct.h"
#include "regionstr.h"

#include "xf86.h"
#include "xf86xaa.h"
#include "xf86cursor.h"

Bool NeedToSync = FALSE;

xf86GCInfoRecType xf86GCInfoRec = {
    NULL, 0,	/* PolyFillRectSolid() */
    NULL, 0,	/* PolyFillRectTiled() */
    NULL, 0,	/* PolyFillRectStippled() */
    0,		/* SecondaryPolyFillRectStippledFlags */
    NULL, 0,	/* PolyFillRectOpaqueStippled() */
    0,		/* SecondaryPolyFillRectOpaqueStippledFlags */
    NULL, 0,	/* PolyFillArcSolid() */
    NULL, 0,	/* FillPolygonSolid() */
    NULL, 0,	/* PolyRectangleSolidZeroWidth() */
    NULL, 0,	/* PolyLineSolidZeroWidth() */
    NULL, 0,	/* PolySegmentSolidZeroWidth() */
    NULL, 0,	/* PolyGlyphBltNonTE() */
    NULL, 0,	/* PolyGlyphBltTE() */
    NULL, 0,	/* ImageGlyphBltNonTE() */
    NULL, 0,	/* ImageGlyphBltTE() */
    NULL, 0,	/* FillSpansSolid() */
    NULL, 0,	/* FillSpansTiled() */
    NULL, 0,	/* FillSpansStippled() */
    NULL, 0,	/* FillSpansOpaqueStippled() */
    NULL, 0,	/* CopyArea() */
    NULL, 0,	/* PolyLineDashedZeroWidth() */
    NULL, 0, 	/* PolySegmentDashedZeroWidth() */
    NULL,	/* FillSpansFallBack */
    NULL,	/* PolyGlyphBltFallBack() */
    NULL,	/* ImageGlyphBltFallBack() */
    NULL,	/* OffScreenCopyWindowFallBack() */
    NULL,	/* CopyAreaFallBack() */
    NULL,	/* cfbBitBltDispatch() */
    NULL,	/* CopyPlane1toNFallBack() */
    NULL,	/* PolyFillRectSolidFallBack() */
/************ start of wrappers ***************/
    NULL,	/* FillPolygonWrapper */
    NULL,	/* PolyArcWrapper */
    NULL,	/* PolyLinesWrapper */
    NULL,	/* PolySegmentWrapper */
    NULL,	/* ImageGlyphBltWrapper */
    NULL,	/* PolyGlyphBltWrapper */
    NULL,	/* FillSpansWrapper */
    NULL,	/* PolyFillArcWrapper */
    NULL,	/* PolyFillRectWrapper */
    NULL,	/* CopyAreaWrapper */
    NULL,	/* PolyPointWrapper */ 
    NULL,	/* PutImageWrapper */
    NULL,	/* SetSpansWrapper */
    NULL,	/* PushPixelsWrapper */
    NULL, 	/* SaveAreasWrapper */
    NULL	/* RestoreAreasWrapper */
};

xf86AccelInfoRecType xf86AccelInfoRec = {
    NULL,	/* FillRectSolid() */
    NULL,	/* FillRectTiled() */
    NULL,	/* FillRectTiledFallBack() */
    NULL,	/* FillRectStippled() */
    NULL,	/* FillRectStippledFallBack() */
    NULL,	/* FillRectOpaqueStippled() */
    NULL,	/* FillRectOpaqueStippledFallBack() */
    NULL,	/* PolyTextTE() */
    NULL,	/* PolyTextNonTE() */
    NULL,	/* ImageTextTE() */
    NULL,	/* ImageTextNonTE() */
    NULL,	/* FillSpansSolid() */
    NULL,	/* ScreenToScreenBitBlt() */
    NULL,	/* WriteBitmap() */
    NULL,	/* WriteBitmapFallBack() */
    NULL,	/* SetupForFillRectSolid() */
    NULL,	/* SubsequentFillRectSolid() */
    NULL,	/* SubsequentFillTrapezoidSolid() */
    NULL,	/* SetupForScreenToScreenCopy() */
    NULL,	/* SubsequentScreenToScreenCopy() */
    NULL,	/* SubsequentScanlineScreenToScreenCopy() */
    NULL,	/* SubsequentBresenhamLine() */
    NULL,	/* SubsequentTwoPointLine() */
    NULL,	/* SetClippingRectangle() */
    NULL,	/* SetupForFill8x8Pattern() */
    NULL,	/* SubsequentFill8x8Pattern() */
    NULL,	/* SetupFor8x8PatternColorExpand() */
    NULL,	/* SetupFor8x8PatternColorExpand() */
    NULL,	/* SetupForCPUToScreenColorExpand() */
    NULL,	/* SubsequentCPUToScreenColorExpand() */
    NULL,	/* SetupForScreenToScreenColorExpand() */
    NULL,	/* SubsequentScreenToScreenColorExpand() */
    NULL,	/* SetupForScanlineCPUToScreenColorExpand() */
    NULL,	/* SubsequentScanlineCPUToScreenColorExpand() */
    NULL,	/* SetupForScanlineScreenToScreenColorExpand() */
    NULL,	/* SubsequentScanlineScreenToScreenColorExpand() */
    NULL,	/* DoImageWrite() */
    NULL,	/* SetupForImageWrite() */
    NULL,	/* SubsequentImageWrite() */
    NULL,	/* ImageWriteFallBack() */
    NULL,	/* VerticalLineGXcopyFallBack() */
    NULL,	/* BresenhamLineFallBack() */
    NULL,	/* xf86GetLongWidthAndPointer() */
    NULL,	/* SetupForDashedLine() */
    NULL,	/* SubsequentDashedBresenhamLine() */
    NULL,	/* SubsequentDashedTwoPointLine() */
    NULL,	/* Sync() */
    0,		/* Flags */
    0,		/* ColorExpandFlags */
    NULL,	/* CPUToScreenColorExpandBase */
    NULL,	/* CPUToScreenColorExpandEndMarker */
    0,		/* CPUToScreenColorExpandRange */
    NULL,	/* FramebufferBase */
    0,		/* FramebufferWidth */
    0,		/* BitsPerPixel */
    0xFF,	/* FullPlanemask */
    0,		/* ScratchBufferAddr */
    NULL,	/* ScratchBufferBase */
    0,		/* ScratchBufferSize */
    2,		/* PingPongBuffers */
    0,		/* ErrorTermBits */
    0,		/* UsingVGA256 */
    NULL,	/* ServerInfoRec */
    0,		/* PixmapCacheMemoryStart */
    0,		/* PixmapCacheMemoryEnd */
    0,		/* LinePatternMaxLength */
    NULL,	/* LinePatternBuffer */
    0, 		/* PatternFlags */
    NULL,	/* ImageWriteBase */
    0,		/* ImageWriteRange */
    0,		/* ImageWriteFlags */
    0		/* ImageWriteOffset */
};

XAACursorInfoRecType XAACursorInfoRec = {
    0,		/* Flags */
    0,		/* MaxWidth */
    0,		/* MaxHeight */
    0,		/* CursorDataX */
    0,		/* CursorDataY */
    NULL,	/* SetCursorColors */
    NULL,	/* SetCursorPosition */
    NULL,	/* LoadCursorImage */
    NULL,	/* HideCursor */
    NULL,	/* ShowCursor */
    NULL 	/* GetInstalledColormaps */
};
