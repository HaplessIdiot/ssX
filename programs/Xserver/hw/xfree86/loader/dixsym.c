/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/dixsym.c,v 1.3 1997/02/18 10:54:16 hohndel Exp $ */




/*
 *
 * Copyright 1995,96 by Metro Link, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include "sym.h"
#include "colormap.h"
#include "cursor.h"
#include "dix.h"
#include "dixfont.h"
#include "dixstruct.h"
#include "misc.h"
#include "opaque.h"
#include "os.h"
#include "resource.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "extension.h"
#include "extnsionst.h"
#include "swaprep.h"

extern Bool     Must_have_memory;
extern WindowPtr *WindowTable;
extern int defaultColorVisualClass;
extern int monitorResolution;

/* DIX things */

extern void writev () ;

LOOKUP dixLookupTab[] = {

  /*dix/ */
  /* atom.c */
  SYMFUNC(MakeAtom)
  /* colormap.c */
  SYMFUNC(AllocColor)
  SYMFUNC(CreateColormap)
  SYMFUNC(FakeAllocColor)
  SYMFUNC(FakeFreeColor)
  SYMFUNC(FreeColors)
  SYMFUNC(TellLostMap)
  SYMFUNC(TellGainedMap)
  SYMFUNC(QueryColors)
  /* dispatch.c */
  SYMVAR(isItTimeToYield)
  SYMFUNC(SetInputCheck)
  SYMFUNC(SendErrorToClient)
  /* dixutils.c */
  SYMFUNC(CopyISOLatin1Lowered)
  SYMFUNC(NoopDDA)
  /* events.c */
  SYMFUNC(CheckCursorConfinement)
  SYMFUNC(DeliverEvents)
  SYMFUNC(NewCurrentScreen)
  SYMFUNC(PointerConfinedToScreen)
  SYMFUNC(TryClientEvents)
  SYMFUNC(WriteEventsToClient)
  /* extension.c */
  SYMFUNC(AddExtension)
  SYMFUNC(MinorOpcodeOfRequest)
  SYMFUNC(StandardMinorOpcode)
  /* gc.c */
  SYMFUNC(CopyGC)
  SYMFUNC(CreateGC)
  SYMFUNC(CreateScratchGC)
  SYMFUNC(ChangeGC)
  SYMFUNC(DoChangeGC)
  SYMFUNC(FreeGC)
  SYMFUNC(FreeScratchGC)
  SYMFUNC(GetScratchGC)
  SYMFUNC(SetClipRects)
  SYMFUNC(ValidateGC)
  /* globals.c */
  SYMVAR(currentTime)
  SYMVAR(defaultColorVisualClass)
  SYMVAR(globalSerialNumber)
  SYMVAR(screenInfo)
  SYMVAR(serverGeneration)
  SYMVAR(WindowTable)
  SYMVAR(monitorResolution)
  SYMVAR(DPMSEnabled)
  /* pixmap.c */
  SYMFUNC(AllocatePixmap)
  SYMFUNC(GetScratchPixmapHeader)
  SYMFUNC(FreeScratchPixmapHeader)
  SYMVAR(PixmapWidthPaddingInfo)
  /* privates.c */
  SYMFUNC(AllocateGCPrivate)
  SYMFUNC(AllocateGCPrivateIndex)
  SYMFUNC(AllocateWindowPrivate)
  SYMFUNC(AllocateWindowPrivateIndex)
  SYMFUNC(AllocateScreenPrivateIndex)
  /* resource.c */
  SYMFUNC(AddResource)
  SYMFUNC(CreateNewResourceClass)
  SYMFUNC(CreateNewResourceType)
  SYMFUNC(FakeClientID)
  SYMFUNC(FreeResource)
  SYMFUNC(FreeResourceByType)
  SYMFUNC(LookupIDByType)
  SYMFUNC(LookupIDByClass)
  SYMFUNC(LegalNewID)
  /* swaprep.c */
  SYMFUNC(CopySwap32Write)
  SYMFUNC(SwapShorts)
  SYMFUNC(SwapLongs)
  /* tables.c */
  SYMVAR(EventSwapVector)
  /* window.c */
  SYMFUNC(GravityTranslate)
  SYMFUNC(MoveWindowInStack)
  SYMFUNC(NotClippedByChildren)
  SYMFUNC(ResizeChildrenWinSize)
  SYMFUNC(SaveScreens)
  SYMFUNC(SendVisibilityNotify)
  SYMFUNC(SetWinSize)
  SYMFUNC(SetBorderSize)
  SYMFUNC(TraverseTree)
  SYMFUNC(UnmapWindow)
  SYMFUNC(WalkTree)
  SYMFUNC(WindowsRestructured)
  SYMVAR(deltaSaveUndersViewable)
  SYMVAR(numSaveUndersViewable)
  SYMVAR(screenIsSaved)

  /*os/ */
  /* util.c */
  SYMFUNC(ErrorF)
  SYMFUNC(FatalError)
  SYMVAR(Must_have_memory)
  /* xalloc.c */
  SYMFUNC(XNFalloc)
  SYMFUNC(Xalloc)
  SYMFUNC(Xcalloc)
  SYMFUNC(Xfree)
  SYMFUNC(Xrealloc)
  /* WaitFor.c */
  SYMFUNC(ScreenSaverTime)
  SYMVAR(TimerFree)
  SYMVAR(TimerSet)
  /* io.c */
  SYMFUNC(WriteToClient)
  /* connection.c */
  SYMFUNC(IgnoreClient)
  SYMFUNC(AttendClient)

  /* libfont.a */
  SYMFUNC(GetGlyphs)
  SYMFUNC(QueryGlyphExtents)

  { 0, 0 },

};
