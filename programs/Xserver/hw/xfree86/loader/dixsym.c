/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/dixsym.c,v 1.9 1997/11/22 08:17:36 dawes Exp $ */




/*
 *
 * Copyright 1995-1998 by Metro Link, Inc.
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

#undef DBMALLOC
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
#include "inputstr.h"
#include "XIproto.h"
#include "exevents.h"
#include "extinit.h"

extern Bool     Must_have_memory;
extern WindowPtr *WindowTable;
extern int defaultColorVisualClass;
extern int GrabInProgress;
extern int monitorResolution;
extern Bool permitOldBugs;
extern Bool noTestExtensions;

extern void ClientSleepUntil();

/* DIX things */

/* extern void writev(); */

LOOKUP dixLookupTab[] = {

  /* dix */
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
  /* cursor.c */
  SYMFUNC(FreeCursor)
  /* devices.c */
  SYMFUNC(Ones)
  SYMFUNC(InitButtonClassDeviceStruct)
  SYMFUNC(InitFocusClassDeviceStruct)
  SYMFUNC(InitPtrFeedbackClassDeviceStruct)
  SYMFUNC(InitValuatorClassDeviceStruct)
  /* dispatch.c */
  SYMFUNC(SetInputCheck)
  SYMFUNC(SendErrorToClient)
  SYMFUNC(UpdateCurrentTime)
  SYMFUNC(UpdateCurrentTimeIf)
  SYMVAR(dispatchException)
  SYMVAR(isItTimeToYield)
  SYMVAR(ClientStateCallback)
  /* dixutils.c */
  SYMFUNC(AddCallback)
  SYMFUNC(CompareTimeStamps)
  SYMFUNC(CopyISOLatin1Lowered)
  SYMFUNC(DeleteCallback)
  SYMFUNC(LookupClient)
  SYMFUNC(LookupDrawable)
  SYMFUNC(LookupWindow)
  SYMFUNC(NoopDDA)
  SYMFUNC(RegisterBlockAndWakeupHandlers)
  SYMFUNC(RemoveBlockAndWakeupHandlers)
  SYMFUNC(SecurityLookupDrawable)
  SYMFUNC(SecurityLookupWindow)
  /* events.c */
  SYMFUNC(CheckCursorConfinement)
  SYMFUNC(DeliverEvents)
  SYMFUNC(NewCurrentScreen)
  SYMFUNC(PointerConfinedToScreen)
  SYMFUNC(TryClientEvents)
  SYMFUNC(WriteEventsToClient)
  SYMVAR(DeviceEventCallback)
  SYMVAR(EventCallback)
  SYMVAR(inputInfo)
  /* extension.c */
  SYMFUNC(AddExtension)
  SYMFUNC(AddExtensionAlias)
  SYMFUNC(DeclareExtensionSecurity)
  SYMFUNC(MinorOpcodeOfRequest)
  SYMFUNC(StandardMinorOpcode)
  /* gc.c */
  SYMFUNC(CopyGC)
  SYMFUNC(CreateGC)
  SYMFUNC(CreateScratchGC)
  SYMFUNC(ChangeGC)
  SYMFUNC(dixChangeGC)
  SYMFUNC(DoChangeGC)
  SYMFUNC(FreeGC)
  SYMFUNC(FreeScratchGC)
  SYMFUNC(GetScratchGC)
  SYMFUNC(SetClipRects)
  SYMFUNC(ValidateGC)
  SYMFUNC(VerifyRectOrder)
  /* globals.c */
#ifdef DPMSExtension
  SYMVAR(DPMSEnabled)
  SYMVAR(DPMSCapableFlag)
  SYMVAR(DPMSOffTime)
  SYMVAR(DPMSPowerLevel)
  SYMVAR(DPMSStandbyTime)
  SYMVAR(DPMSSuspendTime)
#endif
  SYMVAR(ScreenSaverBlanking)
  SYMVAR(WindowTable)
  SYMVAR(clients)
  SYMVAR(currentMaxClients)
  SYMVAR(currentTime)
  SYMVAR(defaultColorVisualClass)
  SYMVAR(globalSerialNumber)
  SYMVAR(lastDeviceEventTime)
  SYMVAR(monitorResolution)
  SYMVAR(permitOldBugs)
  SYMVAR(screenInfo)
  SYMVAR(serverClient)
  SYMVAR(serverGeneration)
  /* pixmap.c */
  SYMFUNC(AllocatePixmap)
  SYMFUNC(GetScratchPixmapHeader)
  SYMFUNC(FreeScratchPixmapHeader)
  SYMVAR(PixmapWidthPaddingInfo)
  /* privates.c */
  SYMFUNC(AllocateClientPrivate)
  SYMFUNC(AllocateClientPrivateIndex)
  SYMFUNC(AllocateGCPrivate)
  SYMFUNC(AllocateGCPrivateIndex)
  SYMFUNC(AllocateWindowPrivate)
  SYMFUNC(AllocateWindowPrivateIndex)
  SYMFUNC(AllocateScreenPrivateIndex)
  /* resource.c */
  SYMFUNC(AddResource)
  SYMFUNC(ChangeResourceValue)
  SYMFUNC(CreateNewResourceClass)
  SYMFUNC(CreateNewResourceType)
  SYMFUNC(FakeClientID)
  SYMFUNC(FreeResource)
  SYMFUNC(FreeResourceByType)
  SYMFUNC(GetXIDList)
  SYMFUNC(GetXIDRange)
  SYMFUNC(LookupIDByType)
  SYMFUNC(LookupIDByClass)
  SYMFUNC(LegalNewID)
  SYMFUNC(SecurityLookupIDByClass)
  SYMFUNC(SecurityLookupIDByType)
  /* swaprep.c */
  SYMFUNC(CopySwap32Write)
  SYMFUNC(Swap32Write)
  SYMFUNC(SwapConnSetupInfo)
  SYMFUNC(SwapConnSetupPrefix)
  SYMFUNC(SwapShorts)
  SYMFUNC(SwapLongs)
  /* tables.c */
  SYMVAR(EventSwapVector)
  /* window.c */
  SYMFUNC(ChangeWindowAttributes)
  SYMFUNC(CheckWindowOptionalNeed)
  SYMFUNC(CreateUnclippedWinSize)
  SYMFUNC(CreateWindow)
  SYMFUNC(FindWindowWithOptional)
  SYMFUNC(GravityTranslate)
  SYMFUNC(MakeWindowOptional)
  SYMFUNC(MapWindow)
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
  SYMVAR(savedScreenInfo)
  SYMVAR(screenIsSaved)

  /*os/ */
  /* access.c */
  SYMFUNC(LocalClient)
  /* util.c */
  SYMFUNC(Error)
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
  SYMFUNC(SetCriticalOutputPending)
  SYMVAR(FlushCallback)
  SYMVAR(ReplyCallback)
  SYMVAR(SkippedRequestsCallback)
  SYMFUNC(ResetCurrentRequest)
  /* connection.c */
  SYMFUNC(IgnoreClient)
  SYMFUNC(AttendClient)
  SYMFUNC(AddEnabledDevice)
  SYMFUNC(RemoveEnabledDevice)
  SYMVAR(GrabInProgress)
  /* utils.c */
  SYMFUNC(AdjustWaitForDelay)
  SYMVAR(noTestExtensions)

#ifdef XINPUT
  /* Xi */
  /* exevents.c */
  SYMFUNC(InitValuatorAxisStruct)
  SYMFUNC(InitProximityClassDeviceStruct)
  /* extinit.c */
  SYMFUNC(AssignTypeAndName)
#endif

  /* libfont.a */
  SYMFUNC(GetGlyphs)
  SYMFUNC(QueryGlyphExtents)

  /* libXext.a */
  SYMFUNC(ClientSleepUntil)
  

  { 0, 0 },

};
