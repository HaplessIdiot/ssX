/* $XConsortium: tables.c,v 1.5 94/03/27 13:56:27 dpw Exp $ */
/*
 * $NCDOr$
 * $NCDId: @(#)tables.c,v 1.13 1994/03/21 21:08:30 lemke Exp $
 *
 * Copyright 1992 Network Computing Devices
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of NCD. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  NCD. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * NCD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL NCD.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, Network Computing Devices
 */

extern int    ProcInitialConnection(), ProcEstablishConnection();

extern int    ProcStandardRequest ();

extern int	ProcLBXInternAtom(),
		ProcLBXGetAtomName(),
                ProcLBXCreateColormap(),
                ProcLBXFreeColormap(),
                ProcLBXCopyColormapAndFree(),
                ProcLBXFreeColors(),
                ProcLBXLookupColor(),
                ProcLBXAllocColor(),
                ProcLBXAllocNamedColor(),
                ProcLBXGetModifierMapping(),
                ProcLBXGetKeyboardMapping(),
                ProcLBXQueryFont(),
                ProcLBXChangeProperty(),
                ProcLBXGetProperty(),
		ProcLBXPolyPoint(),
		ProcLBXPolyLine(),
		ProcLBXPolySegment(),
		ProcLBXPolyRectangle(),
		ProcLBXPolyArc(),
		ProcLBXFillPoly(),
		ProcLBXPolyFillRectangle(),
		ProcLBXPolyFillArc()
                ;

extern int    ProcBadRequest(), ProcCreateWindow(),
    ProcChangeWindowAttributes(), ProcGetWindowAttributes(),
    ProcDestroyWindow(), ProcDestroySubwindows(), ProcChangeSaveSet(),
    ProcReparentWindow(), ProcMapWindow(), ProcMapSubwindows(),
    ProcUnmapWindow(), ProcUnmapSubwindows(), ProcConfigureWindow(),
    ProcCirculateWindow(), ProcGetGeometry(), ProcQueryTree(),
    ProcInternAtom(), ProcGetAtomName(), ProcChangeProperty(),
    ProcDeleteProperty(), ProcGetProperty(), ProcListProperties(),
    ProcSetSelectionOwner(), ProcGetSelectionOwner(), ProcConvertSelection(),
    ProcSendEvent(), ProcGrabPointer(), ProcUngrabPointer(),
    ProcGrabButton(), ProcUngrabButton(), ProcChangeActivePointerGrab(),
    ProcGrabKeyboard(), ProcUngrabKeyboard(), ProcGrabKey(),
    ProcUngrabKey(), ProcAllowEvents(), ProcGrabServer(),
    ProcUngrabServer(), ProcQueryPointer(), ProcGetMotionEvents(),
    ProcTranslateCoords(), ProcWarpPointer(), ProcSetInputFocus(),
    ProcGetInputFocus(), ProcQueryKeymap(), ProcOpenFont(),
    ProcCloseFont(), ProcQueryFont(), ProcQueryTextExtents(),
    ProcListFonts(), ProcListFontsWithInfo(), ProcSetFontPath(),
    ProcGetFontPath(), ProcCreatePixmap(), ProcFreePixmap(),
    ProcCreateGC(), ProcChangeGC(), ProcCopyGC(),
    ProcSetDashes(), ProcSetClipRectangles(), ProcFreeGC(),
    ProcClearToBackground(), ProcCopyArea(), ProcCopyPlane(),
    ProcPolyPoint(), ProcPolyLine(), ProcPolySegment(),
    ProcPolyRectangle(), ProcPolyArc(), ProcFillPoly(),
    ProcPolyFillRectangle(), ProcPolyFillArc(), ProcPutImage(),
    ProcGetImage(), ProcPolyText(),
    ProcImageText8(), ProcImageText16(), ProcCreateColormap(),
    ProcFreeColormap(), ProcCopyColormapAndFree(), ProcInstallColormap(),
    ProcUninstallColormap(), ProcListInstalledColormaps(), ProcAllocColor(),
    ProcAllocNamedColor(), ProcAllocColorCells(), ProcAllocColorPlanes(),
    ProcFreeColors(), ProcStoreColors(), ProcStoreNamedColor(),
    ProcQueryColors(), ProcLookupColor(), ProcCreateCursor(),
    ProcCreateGlyphCursor(), ProcFreeCursor(), ProcRecolorCursor(),
    ProcQueryBestSize(), ProcQueryExtension(), ProcListExtensions(),
    ProcChangeKeyboardMapping(), ProcSetPointerMapping(),
    ProcGetKeyboardMapping(), ProcGetPointerMapping(),
    ProcChangeKeyboardControl(),
    ProcGetKeyboardControl(), ProcBell(), ProcChangePointerControl(),
    ProcGetPointerControl(), ProcSetScreenSaver(), ProcGetScreenSaver(),
    ProcChangeHosts(), ProcListHosts(), ProcChangeAccessControl(),
    ProcChangeCloseDownMode(), ProcKillClient(),
    ProcRotateProperties(), ProcForceScreenSaver(),
    ProcSetModifierMapping(), ProcGetModifierMapping(),
    ProcNoOperation();

extern int    SProcSProcBadRequest(), SProcCreateWindow(),
    SProcChangeWindowAttributes(), 
    SProcReparentWindow(), SProcConfigureWindow(),
    SProcInternAtom(), SProcChangeProperty(),
    SProcDeleteProperty(), SProcGetProperty(),
    SProcSetSelectionOwner(),
    SProcConvertSelection(),
    SProcSendEvent(), SProcGrabPointer(),
    SProcGrabButton(), SProcUngrabButton(), SProcChangeActivePointerGrab(),
    SProcGrabKeyboard(), SProcGrabKey(),
    SProcUngrabKey(), SProcGetMotionEvents(),
    SProcTranslateCoords(), SProcWarpPointer(), SProcSetInputFocus(),
    SProcOpenFont(),
    SProcListFonts(), SProcListFontsWithInfo(), SProcSetFontPath(),
    SProcCreatePixmap(),
    SProcCreateGC(), SProcChangeGC(), SProcCopyGC(),
    SProcSetDashes(), SProcSetClipRectangles(),
    SProcClearToBackground(), SProcCopyArea(), SProcCopyPlane(),
    SProcPoly(), SProcFillPoly(), SProcPutImage(),
    SProcGetImage(), SProcPolyText(), 
    SProcImageText(), SProcCreateColormap(),
    SProcCopyColormapAndFree(), SProcAllocColor(),
    SProcAllocNamedColor(), SProcAllocColorCells(), SProcAllocColorPlanes(),
    SProcFreeColors(), SProcStoreColors(), SProcStoreNamedColor(),
    SProcQueryColors(), SProcLookupColor(), SProcCreateCursor(),
    SProcCreateGlyphCursor(), SProcRecolorCursor(),
    SProcQueryBestSize(), SProcQueryExtension(),
    SProcChangeKeyboardMapping(), SProcChangeKeyboardControl(),
    SProcChangePointerControl(),
    SProcSetScreenSaver(),
    SProcChangeHosts(),
    SProcRotateProperties(), 
    SProcNoOperation(), SProcResourceReq(), SProcSimpleReq();

extern void 
    SErrorEvent(), NotImplemented(), SKeyButtonPtrEvent(), SKeyButtonPtrEvent(),
    SKeyButtonPtrEvent(), SKeyButtonPtrEvent(), SKeyButtonPtrEvent(),
    SEnterLeaveEvent(), SEnterLeaveEvent(), SFocusEvent(),
    SFocusEvent(), SKeymapNotifyEvent(), SExposeEvent(),
    SGraphicsExposureEvent(), SNoExposureEvent(), SVisibilityEvent(),
    SCreateNotifyEvent(), SDestroyNotifyEvent(), SUnmapNotifyEvent(),
    SMapNotifyEvent(), SMapRequestEvent(), SReparentEvent(),
    SConfigureNotifyEvent(), SConfigureRequestEvent(), SGravityEvent(),
    SResizeRequestEvent(), SCirculateEvent(), SCirculateEvent(),
    SPropertyEvent(), SSelectionClearEvent(), SSelectionRequestEvent(),
    SSelectionNotifyEvent(), SColormapEvent(), SClientMessageEvent(), SMappingEvent();

extern void
    SGetWindowAttributesReply(), SGetGeometryReply(), SQueryTreeReply(),
    SInternAtomReply(), SGetAtomNameReply(), SGetPropertyReply(),
    SListPropertiesReply(), 
    SGetSelectionOwnerReply(),
    SQueryPointerReply(), SGetMotionEventsReply(), STranslateCoordsReply(),
    SGetInputFocusReply(), SQueryKeymapReply(), SQueryFontReply(),
    SQueryTextExtentsReply(), SListFontsReply(), SListFontsWithInfoReply(),
    SGetFontPathReply(), SGetImageReply(), SListInstalledColormapsReply(),
    SAllocColorReply(), SAllocNamedColorReply(), SAllocColorCellsReply(),
    SAllocColorPlanesReply(), SQueryColorsReply(), SLookupColorReply(),
    SQueryBestSizeReply(), SListExtensionsReply(),
    SGetKeyboardMappingReply(), SGetKeyboardControlReply(), 
    SGetPointerControlReply(), SGetScreenSaverReply(), 
    SListHostsReply(), SGetPointerMappingReply(),
    SGetModifierMappingReply(), SGenericReply();

int (* InitialVector[3]) () =
{
    0,
    ProcInitialConnection,
    ProcEstablishConnection
};

int (* ProcVector[256]) () =
{
    ProcBadRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,		/* 5 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,			/* 10 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,			/* 15 */
    ProcLBXInternAtom,
    ProcLBXGetAtomName,
    ProcLBXChangeProperty,
    ProcStandardRequest,
    ProcLBXGetProperty,			/* 20 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,			/* 25 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,	/* 30 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,			/* 35 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,		/* 40 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,			/* 45 */
    ProcStandardRequest,
    ProcLBXQueryFont,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,		/* 50 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,			/* 55 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,			/* 60 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcLBXPolyPoint,
    ProcLBXPolyLine,				/* 65 */
    ProcLBXPolySegment,
    ProcLBXPolyRectangle,
    ProcLBXPolyArc,
    ProcLBXFillPoly,
    ProcLBXPolyFillRectangle,			/* 70 */
    ProcLBXPolyFillArc,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,			/* 75 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcLBXCreateColormap,
    ProcLBXFreeColormap,
    ProcLBXCopyColormapAndFree,		/* 80 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcLBXAllocColor,
    ProcLBXAllocNamedColor,		/* 85 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcLBXFreeColors,
    ProcStandardRequest,
    ProcStandardRequest,		/* 90 */
    ProcStandardRequest,
    ProcLBXLookupColor,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,			/* 95 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,		/* 100 */
    ProcLBXGetKeyboardMapping,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,		/* 105 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,			/* 110 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,		/* 115 */
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcLBXGetModifierMapping,
    0,					/* 120 */
    0,
    0,
    0,
    0,
    0,					/* 125 */
    0,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
    ProcStandardRequest,
};

int (* SwappedProcVector[256]) () =
{
    ProcStandardRequest,
    SProcCreateWindow,
    SProcChangeWindowAttributes,
    SProcResourceReq,			/* GetWindowAttributes */
    SProcResourceReq,			/* DestroyWindow */
    SProcResourceReq,			/* 5 DestroySubwindows */
    SProcResourceReq,			/* SProcChangeSaveSet, */
    SProcReparentWindow,
    SProcResourceReq,			/* MapWindow */
    SProcResourceReq,			/* MapSubwindows */
    SProcResourceReq,			/* 10 UnmapWindow */
    SProcResourceReq,			/* UnmapSubwindows */
    SProcConfigureWindow,
    SProcResourceReq,			/* SProcCirculateWindow, */
    SProcResourceReq,			/* GetGeometry */
    SProcResourceReq,			/* 15 QueryTree */
    SProcInternAtom,
    SProcResourceReq,			/* SProcGetAtomName, */
    SProcChangeProperty,
    SProcDeleteProperty,
    SProcGetProperty,			/* 20 */
    SProcResourceReq,			/* SProcListProperties, */
    SProcSetSelectionOwner,
    SProcResourceReq, 			/* SProcGetSelectionOwner, */
    SProcConvertSelection,
    SProcSendEvent,			/* 25 */
    SProcGrabPointer,
    SProcResourceReq, 			/* SProcUngrabPointer, */
    SProcGrabButton,
    SProcUngrabButton,
    SProcChangeActivePointerGrab,	/* 30 */
    SProcGrabKeyboard,
    SProcResourceReq,			/* SProcUngrabKeyboard, */
    SProcGrabKey,
    SProcUngrabKey,
    SProcResourceReq,			/* 35 SProcAllowEvents, */
    SProcSimpleReq,			/* SProcGrabServer, */
    SProcSimpleReq,			/* SProcUngrabServer, */
    SProcResourceReq,			/* SProcQueryPointer, */
    SProcGetMotionEvents,
    SProcTranslateCoords,		/*40 */
    SProcWarpPointer,
    SProcSetInputFocus,
    SProcSimpleReq,			/* SProcGetInputFocus, */
    SProcSimpleReq,			/* QueryKeymap, */
    SProcOpenFont,			/* 45 */
    SProcResourceReq,			/* SProcCloseFont, */
    SProcResourceReq, 			/* SProcQueryFont, */
    SProcResourceReq,			/* SProcQueryTextExtents,  */
    SProcListFonts,
    SProcListFontsWithInfo,		/* 50 */
    SProcSetFontPath,
    SProcSimpleReq,			/* GetFontPath, */
    SProcCreatePixmap,
    SProcResourceReq,			/* SProcFreePixmap, */
    SProcCreateGC,			/* 55 */
    SProcChangeGC,
    SProcCopyGC,
    SProcSetDashes,
    SProcSetClipRectangles,
    SProcResourceReq,			/* 60 SProcFreeGC, */
    SProcClearToBackground,
    SProcCopyArea,
    SProcCopyPlane,
    SProcPoly,				/* PolyPoint, */
    SProcPoly,				/* 65 PolyLine */
    SProcPoly,				/* PolySegment, */
    SProcPoly,				/* PolyRectangle, */
    SProcPoly,				/* PolyArc, */
    SProcFillPoly,
    SProcPoly,				/* 70 PolyFillRectangle */
    SProcPoly,				/* PolyFillArc, */
    SProcPutImage,
    SProcGetImage,
    SProcPolyText,
    SProcPolyText,			/* 75 */
    SProcImageText,
    SProcImageText,
    SProcCreateColormap,
    SProcResourceReq,			/* SProcFreeColormap, */
    SProcCopyColormapAndFree,		/* 80 */
    SProcResourceReq,			/* SProcInstallColormap, */
    SProcResourceReq,			/* SProcUninstallColormap, */
    SProcResourceReq, 			/* SProcListInstalledColormaps, */
    SProcAllocColor,
    SProcAllocNamedColor,		/* 85 */
    SProcAllocColorCells,
    SProcAllocColorPlanes,
    SProcFreeColors,
    SProcStoreColors,
    SProcStoreNamedColor,		/* 90 */
    SProcQueryColors,
    SProcLookupColor,
    SProcCreateCursor,
    SProcCreateGlyphCursor,
    SProcResourceReq,			/* 95 SProcFreeCursor, */
    SProcRecolorCursor,
    SProcQueryBestSize,
    SProcQueryExtension,
    SProcSimpleReq,			/* ListExtensions, */
    SProcChangeKeyboardMapping,		/* 100 */
    SProcSimpleReq,			/* GetKeyboardMapping, */
    SProcChangeKeyboardControl,
    SProcSimpleReq,			/* GetKeyboardControl, */
    SProcSimpleReq,			/* Bell, */
    SProcChangePointerControl,		/* 105 */
    SProcSimpleReq,			/* GetPointerControl, */
    SProcSetScreenSaver,
    SProcSimpleReq,			/* GetScreenSaver, */
    SProcChangeHosts,
    SProcSimpleReq,			/* 110 ListHosts, */
    SProcSimpleReq,			/* SProcChangeAccessControl, */
    SProcSimpleReq,			/* SProcChangeCloseDownMode, */
    SProcResourceReq,			/* SProcKillClient, */
    SProcRotateProperties,
    SProcSimpleReq,			/* 115 ForceScreenSaver */
    SProcSimpleReq,			/* SetPointerMapping, */
    SProcSimpleReq,			/* GetPointerMapping, */
    SProcSimpleReq,			/* SetModifierMapping, */
    SProcSimpleReq,			/* GetModifierMapping, */
    0,					/* 120 */
    0,
    0,
    0,
    0,
    0,					/* 125 */
    0,
    SProcNoOperation
};

#ifdef notyet
void (* EventSwapVector[128]) () =
{
    SErrorEvent,
    NotImplemented,
    SKeyButtonPtrEvent,
    SKeyButtonPtrEvent,
    SKeyButtonPtrEvent,
    SKeyButtonPtrEvent,			/* 5 */
    SKeyButtonPtrEvent,
    SEnterLeaveEvent,
    SEnterLeaveEvent,
    SFocusEvent,
    SFocusEvent,			/* 10 */
    SKeymapNotifyEvent,
    SExposeEvent,
    SGraphicsExposureEvent,
    SNoExposureEvent,
    SVisibilityEvent,			/* 15 */
    SCreateNotifyEvent,
    SDestroyNotifyEvent,
    SUnmapNotifyEvent,
    SMapNotifyEvent,
    SMapRequestEvent,			/* 20 */
    SReparentEvent,
    SConfigureNotifyEvent,
    SConfigureRequestEvent,
    SGravityEvent,
    SResizeRequestEvent,		/* 25 */
    SCirculateEvent,
    SCirculateEvent,
    SPropertyEvent,
    SSelectionClearEvent,
    SSelectionRequestEvent,		/* 30 */
    SSelectionNotifyEvent,
    SColormapEvent,
    SClientMessageEvent,
    SMappingEvent,
};

void (* ReplySwapVector[256]) () =
{
    NotImplemented,
    NotImplemented,
    NotImplemented,
    SGetWindowAttributesReply,
    NotImplemented,
    NotImplemented,			/* 5 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 10 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    SGetGeometryReply,
    SQueryTreeReply,			/* 15 */
    SInternAtomReply,
    SGetAtomNameReply,
    NotImplemented,
    NotImplemented,
    SGetPropertyReply,			/* 20 */
    SListPropertiesReply,
    NotImplemented,
    SGetSelectionOwnerReply,
    NotImplemented,
    NotImplemented,			/* 25 */
    SGenericReply,			/* SGrabPointerReply, */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 30 */
    SGenericReply,			/* SGrabKeyboardReply, */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 35 */
    NotImplemented,
    NotImplemented,
    SQueryPointerReply,
    SGetMotionEventsReply,
    STranslateCoordsReply,		/* 40 */
    NotImplemented,
    NotImplemented,
    SGetInputFocusReply,
    SQueryKeymapReply,
    NotImplemented,			/* 45 */
    NotImplemented,
    SQueryFontReply,
    SQueryTextExtentsReply,
    SListFontsReply,
    SListFontsWithInfoReply,		/* 50 */
    NotImplemented,
    SGetFontPathReply,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 55 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 60 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 65 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 70 */
    NotImplemented,
    NotImplemented,
    SGetImageReply,
    NotImplemented,
    NotImplemented,			/* 75 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 80 */
    NotImplemented,
    NotImplemented,
    SListInstalledColormapsReply,
    SAllocColorReply,
    SAllocNamedColorReply,		/* 85 */
    SAllocColorCellsReply,
    SAllocColorPlanesReply,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 90 */
    SQueryColorsReply,
    SLookupColorReply,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 95 */
    NotImplemented,
    SQueryBestSizeReply,
    SGenericReply,			/* SQueryExtensionReply, */
    SListExtensionsReply,
    NotImplemented,			/* 100 */
    SGetKeyboardMappingReply,
    NotImplemented,
    SGetKeyboardControlReply,
    NotImplemented,
    NotImplemented,			/* 105 */
    SGetPointerControlReply,
    NotImplemented,
    SGetScreenSaverReply,
    NotImplemented,
    SListHostsReply,			/* 110 */
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,
    NotImplemented,			/* 115 */
    SGenericReply,			/* SetPointerMapping */
    SGetPointerMappingReply,
    SGenericReply,			/* SetModifierMapping */
    SGetModifierMappingReply,		/* 119 */
    NotImplemented,			/* 120 */
    NotImplemented,			/* 121 */
    NotImplemented,			/* 122 */
    NotImplemented,			/* 123 */
    NotImplemented,			/* 124 */
    NotImplemented,			/* 125 */
    NotImplemented,			/* 126 */
    NotImplemented,			/* NoOperation */
    NotImplemented
};

#endif


extern int  ServerProcError (), ServerProcReply (),
	    ServerProcStandardEvent ();
	    
int (*ServerVector[256])() = {
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
    ServerProcStandardEvent,
};
