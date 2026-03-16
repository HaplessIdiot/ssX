/*
 * XAA Compatibility Adapter Layer
 * 
 * This header provides backward compatibility definitions for XAA code
 * that was written for XFree86 to work with modern Xorg headers.
 */

#ifndef XAA_COMPAT_H
#define XAA_COMPAT_H

#include "inputstr.h"
#include "cursorstr.h"

/* Define MAXDEVICES if not already defined */
#ifndef MAXDEVICES
#define MAXDEVICES 20
#endif

/* Define MAXEVENTS if not already defined */
#ifndef MAXEVENTS
#define MAXEVENTS 128
#endif

/* Event sync info for synchronous event processing */
typedef struct _EventSyncInfo {
    struct xorg_list pending;
    Bool playingEvents;
    TimeStamp time;
    DeviceIntPtr replayDev;
    WindowPtr replayWin;
} EventSyncInfoRec, *EventSyncInfoPtr;

#define syncEventsPlayingEvents (syncEvents.playingEvents)
#define syncEventsTime (syncEvents.time)

/* Screen position - for older multi-screen XAA code */
#ifndef ScreenGetPage
#define screenInfoNumScreens screenInfo.numScreens
#define screenInfoScreens(i) screenInfo.screens[i]
#endif

/* Define EXTENSION_BASE for extensions */
#ifndef EXTENSION_BASE
#define EXTENSION_BASE 128
#endif

/* Define EXTENSION_EVENT_BASE for extension events */
#ifndef EXTENSION_EVENT_BASE  
#define EXTENSION_EVENT_BASE 64
#endif

/* Define DevPrivateKeyRec for compatibility */
typedef DevPrivateKey DevPrivateKeyRec;

/* XInput 2 types for compatibility */
#ifndef XI2Mask
typedef struct _XI2Mask {
    unsigned char *mask;
    int len;
} XI2Mask;
#endif

/* Touch event types */
#ifndef ET_TouchBegin
#define ET_TouchBegin 35
#define ET_TouchUpdate 36
#define ET_TouchEnd 37
#endif

#ifndef ET_RawTouchBegin
#define ET_RawTouchBegin 38
#define ET_RawTouchUpdate 39
#define ET_RawTouchEnd 40
#endif

#ifndef ET_TouchOwnership
#define ET_TouchOwnership 41
#endif

/* Compatibility for callback structures */
#ifndef _CallbackFuncs
#include "callback.h"
#endif

#endif /* XAA_COMPAT_H */
