/*
 * ssX Compatibility Header
 * 
 * This header provides a global shim layer that intercepts legacy XFree86 calls
 * and maps them to modern XLibre/DIX signatures. This bridges the 15-year gap
 * between the legacy XFree86 codebase and modern XLibre/DIX headers.
 * 
 * Priority: dix/ and mi/ compilation. Ignore glx and dri3 until core DIX is green.
 */

#ifndef SSX_COMPAT_H
#define SSX_COMPAT_H

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <pixman.h>

/* Include inputstr.h to get device class types */
#include "inputstr.h"

/* NOTE: We don't include xaa-compat.h here because it should be included
 * after other X server headers. Individual source files can include it
 * if needed. The EXTENSION_BASE definition is provided below. */

/* ============================================================================
 * XKB SUPPORT
 * Enable XKB support for this build
 * ============================================================================ */

#ifndef XKB
#define XKB 1
#endif

/* ============================================================================
 * EXTENSION DEFINITIONS
 * EXTENSION_BASE is used for extension major opcode calculations
 * ============================================================================ */

#ifndef EXTENSION_BASE
#define EXTENSION_BASE 128
#endif

/* ============================================================================
 * DIX ACCESS RIGHTS
 * These are access modes for dixLookupWindow and similar functions
 * ============================================================================ */

#ifndef DixAddAccess
#define DixAddAccess 0
#endif

#ifndef DixReceiveAccess
#define DixReceiveAccess 0
#endif

#ifndef DixShowAccess
#define DixShowAccess 0
#endif

#ifndef DixListAccess
#define DixListAccess 0
#endif

#ifndef DixHideAccess
#define DixHideAccess 0
#endif

/* ============================================================================
 * XI2 (XInput2) TYPE FORWARD DECLARATIONS
 * Legacy code uses modern XI2 types that may not be declared
 * ============================================================================ */

#ifndef _XI2_TYPES_DEFINED
#define _XI2_TYPES_DEFINED

/* XI2 Property types - complete struct definition for ssX */
#ifndef XIPropertyValuePtr
typedef struct _XIPropertyValue {
    Atom type;           /* property type */
    int format;          /* 8, 16, or 32 */
    unsigned long size;  /* number of elements */
    void *data;          /* pointer to property data */
} XI2PropertyValue, *XIPropertyValuePtr;
#endif

#ifndef XIPropertyPtr  
typedef struct _XIProperty *XIPropertyPtr;
#endif

/* XI2 Property atoms - provide stubs for legacy code */
#ifndef XI_PROP_ENABLED
#define XI_PROP_ENABLED 0
#endif

#ifndef XI_PROP_TRANSFORM
#define XI_PROP_TRANSFORM 0
#endif

#ifndef XI_PROP_ENABLED
#define XI_PROP_ENABLED 0
#endif

/* Button label property atoms */
#ifndef BTN_LABEL_PROP_BTN_LEFT
#define BTN_LABEL_PROP_BTN_LEFT 0
#endif

#ifndef BTN_LABEL_PROP_BTN_MIDDLE
#define BTN_LABEL_PROP_BTN_MIDDLE 0
#endif

#ifndef BTN_LABEL_PROP_BTN_RIGHT
#define BTN_LABEL_PROP_BTN_RIGHT 0
#endif

#ifndef BTN_LABEL_PROP_BTN_WHEEL_UP
#define BTN_LABEL_PROP_BTN_WHEEL_UP 0
#endif

#ifndef BTN_LABEL_PROP_BTN_WHEEL_DOWN
#define BTN_LABEL_PROP_BTN_WHEEL_DOWN 0
#endif

#ifndef BTN_LABEL_PROP_BTN_HWHEEL_LEFT
#define BTN_LABEL_PROP_BTN_HWHEEL_LEFT 0
#endif

#ifndef BTN_LABEL_PROP_BTN_HWHEEL_RIGHT
#define BTN_LABEL_PROP_BTN_HWHEEL_RIGHT 0
#endif

/* Axis label property atoms */
#ifndef AXIS_LABEL_PROP_REL_X
#define AXIS_LABEL_PROP_REL_X 0
#endif

#ifndef AXIS_LABEL_PROP_REL_Y
#define AXIS_LABEL_PROP_REL_Y 0
#endif

#ifndef AXIS_LABEL_PROP_ABS_X
#define AXIS_LABEL_PROP_ABS_X 0
#endif

#ifndef AXIS_LABEL_PROP_ABS_Y
#define AXIS_LABEL_PROP_ABS_Y 0
#endif

/* XI2 function stubs */
#ifndef XIGetKnownProperty
#define XIGetKnownProperty(x) (0)
#endif

#ifndef XIChangeDeviceProperty
#define XIChangeDeviceProperty(a, b, c, d, e, f, g, h) (FALSE)
#endif

#ifndef XISetDevicePropertyDeletable
#define XISetDevicePropertyDeletable(a, b, c) 
#endif

#ifndef XIRegisterPropertyHandler
#define XIRegisterPropertyHandler(a, b, c, d) 
#endif

#endif /* _XI2_TYPES_DEFINED */

/* ============================================================================
 * ADDITIONAL MISSING FUNCTION STUBS
 * ============================================================================ */

/* Note: EnableDevice and DisableDevice are defined in dix/devices.c
 * with signature: Bool EnableDevice(DeviceIntPtr dev, BOOL sendevent)
 * They take 2 arguments. The 1-arg stubs were causing issues. */

#ifndef IsXTestDevice
#define IsXTestDevice(a, b) (FALSE)
#endif

#ifndef SyncInitDeviceIdleTime
#define SyncInitDeviceIdleTime(a) ((void*)0)
#endif

#ifndef SyncRemoveDeviceIdleTime
#define SyncRemoveDeviceIdleTime(a) 
#endif

#ifndef TouchEndPhysicallyActiveTouches
#define TouchEndPhysicallyActiveTouches(a) 
#endif

#ifndef GestureEndActiveGestures
#define GestureEndActiveGestures(a) 
#endif

#ifndef ReleaseButtonsAndKeys
#define ReleaseButtonsAndKeys(a) 
#endif

#ifndef XkbPushLockedStateToSlaves
#define XkbPushLockedStateToSlaves(a, b, c) 
#endif

/* Note: Function signature mismatches are too deep for macros.
 * They require source-level changes to dix/devices.c to match modern API.
 * The types and stubs above provide basic compatibility.
 */

/* ===== CREATEPIXMAP FIX ===== */
/* Modern CreatePixmap has 5 parameters: (screen, w, h, depth, usage)
 * Legacy had 4 parameters. Don't redefine as macro - let code call directly.
 * Use parentheses to prevent macro expansion when needed. */

/* ===== RENDER/PIXMAN TYPE COMPATIBILITY ===== */
/* Modern X uses pixman types directly; legacy code expects pict_ wrappers */
/* Note: pixman_f_transform is now defined in pixman.h - don't redefine */
/* Provide pict_ prefix only if pixman doesn't already have it */

/* ===== DAMAGE EXTENSION STUBS ===== */
/* Modern X may have moved or renamed these. Provide stubs for legacy builds. */
/* Note: DamagePtr is already defined in pixmap.h, don't redefine */
#ifndef DamageReportLevel
typedef enum { DamageReportNoneVal = 0 } DamageReportLevel;
#define DamageReportNone ((DamageReportLevel)0)
#endif

/* Stub declarations - actual implementations in damage module */
#ifndef DamageCreate
#define DamageCreate(a, b, c, d, e, f) ((void*)0)
#endif
#ifndef DamageRegister
#define DamageRegister(a, b) /* no-op */
#endif
#ifndef DamageDestroy
#define DamageDestroy(a) /* no-op */
#endif
#ifndef DamageRegion
#define DamageRegion(a) ((RegionPtr)0)
#endif

/* ===== MI LAYER STUBS ===== */
#ifndef miSourceValidate
/* Stub for miSourceValidate - used in pixmap.c */
#define miSourceValidate(pDraw, x, y, w, h, subWindowMode) /* no-op */
#endif

/* ============================================================================
 * STRUCT ALIGNMENT FIXES FOR _SPRITE
 * Legacy 2009 code used hotPhys and spriteTrace. Modern uses hot and win.
 * ============================================================================ */

/* The Sprite struct in include/cursorstr.h already has both legacy and 
 * modern members. Provide accessor macros for code that might use either. */

/* Accessor for physical hot spot (legacy) */
#ifndef sprite_phys_hot
#define sprite_phys_hot(s) ((s)->hotPhys)
#endif

/* Accessor for current hot spot (modern) */
#ifndef sprite_hot
#define sprite_hot(s) ((s)->hot)
#endif

/* Accessor for window tracking (both legacy and modern have win) */
#ifndef sprite_win
#define sprite_win(s) ((s)->win)
#endif

/* For legacy code that accessed spriteTrace array - map to current win 
 * The spriteTrace was an array of windows for XI2 multi-pointer */
#ifndef sprite_current_win
#define sprite_current_win(s) ((s)->win)
#endif

/* ============================================================================
 * XKB FALLBACK
 * Modern code expects direct xkbInfo access. Legacy may need fallback.
 * ============================================================================ */

/* If xkbInfo is not available (opaque), provide fallback macros that use
 * curKeySyms min/max keycodes instead */
#ifndef XKB_INFO_AVAILABLE
#define XKB_GET_KEYCODES(k) ((k)->curKeySyms.minKeyCode)
#define XKB_GET_KEYCODE_MAX(k) ((k)->curKeySyms.maxKeyCode)
#define XKB_GET_MODS_STATE(k) (0)
#define XKB_GET_GROUP_STATE(k) (0)
#else
/* xkbInfo is available - use direct access */
#define XKB_GET_KEYCODES(k) (0)
#define XKB_GET_KEYCODE_MAX(k) (0)
#define XKB_GET_MODS_STATE(k) ((k)->xkbInfo->state.mods)
#define XKB_GET_GROUP_STATE(k) ((k)->xkbInfo->state.group)
#endif

/* ============================================================================
 * BUG RETURN HELPER
 * Common debugging macro for enterleave.c and similar
 * ============================================================================ */

#ifndef BUG_RETURN
#define BUG_RETURN(cond) if (cond) return
#endif

/* ============================================================================
 * ADDITIONAL COMMON COMPATIBILITY FIXES
 * ============================================================================ */

/* bits_to_bytes helper - used in enterleave.c */
#ifndef bits_to_bytes
#define bits_to_bytes(n) (((n) + 7) >> 3)
#endif

/* Common typedefs for legacy code - removed to avoid redefinition conflicts
 * These types are already defined in their respective headers */

/* ============================================================================
 * FLOATING POINT HELPER FOR XI2 EVENTS
 * ============================================================================ */

/* Helper for converting double to fp1616 fixed point format 
 * Note: This is a simple implementation - actual implementation may be in eventconvert.c */
#ifndef double_to_fp1616
#define double_to_fp1616(d) ((int)((d) * 65536.0))
#endif

/* ============================================================================
 * LASTDEVICEINFO COMPATIBILITY
 * Modern API uses fixed array, legacy uses pointer. Provide wrapper macros.
 * ============================================================================ */

/* For code that assigns to dev->last.valuators - use a wrapper 
 * This is a no-op since the array is fixed-size in modern API */
#ifndef LAST_VALUATORS_ASSIGN
#define LAST_VALUATORS_ASSIGN(dest, src) /* no-op: modern API uses fixed array */
#endif

/* ============================================================================
 * EVENT COMPATIBILITY STUBS
 * ============================================================================ */

/* Stub for FixUpEventFromWindow if not provided by DIX */
#ifndef FixUpEventFromWindow
static inline void ssx_FixUpEventFromWindow(void *sprite, void *event, 
                                            void *win, int filter, int force) {
    /* No-op stub - actual implementation in eventconvert.c */
}
#define FixUpEventFromWindow(s, e, w, f, f2) ssx_FixUpEventFromWindow(s, e, w, f, f2)
#endif

/* Stub for GetEventFilter if not provided by DIX */
#ifndef GetEventFilter
static inline int ssx_GetEventFilter(void *dev, void *event) {
    return 0;  /* No filter by default */
}
#define GetEventFilter(d, e) ssx_GetEventFilter(d, e)
#endif

/* ===== FUNCTION SIGNATURE BRIDGES ===== */
/* Note: The EnqueueEvent shim is defined in dix/devices.c since it needs types
 * that aren't available when ssx_compat.h is prepended to all files.
 * SSX_ENQUEUE_SHIM is defined there. */

/* ===== PRIVATE DATA COMPATIBILITY ===== */
/* Modern X uses PrivateRec*, legacy uses DevUnion*. */
/* Modern dixFreePrivates takes only one argument (PrivateRec*) */
#define SSX_FREE_DEV_PRIVATES(dev) \
    dixFreePrivates((PrivateRec*)(dev)->devPrivates)

/* ===== CLASSES PTR FORWARD DECLARATION ===== */
/* Legacy XFree86 used ClassesRec to hold all device classes.
 * Modern X moved these to DeviceIntRec directly. Provide compatibility struct. */
#ifndef _CLASSES_REC_DEFINED
#define _CLASSES_REC_DEFINED

typedef struct _ClassesRec {
    KeyClassPtr key;
    ValuatorClassPtr valuator;
    ButtonClassPtr button;
    TouchClassPtr touch;
    FocusClassPtr focus;
    ProximityClassPtr proximity;
    KbdFeedbackPtr kbdfeed;
    PtrFeedbackPtr ptrfeed;
    IntegerFeedbackPtr intfeed;
    StringFeedbackPtr stringfeed;
    BellFeedbackPtr bell;
    LedFeedbackPtr leds;
} ClassesRec, *ClassesPtr;

#endif

/* ===== CLIENT MINOR/MAJOR OP STUBS ===== */
/* Modern ClientRec no longer has minorOp/majorOp tracking.
 * Use requestBuffer to get minor opcode: byte[1] of request */
#define SSX_CLIENT_MINOR_OP(client) ((client)->requestBuffer[1])
#define SSX_CLIENT_MAJOR_OP(client) ((client)->requestBuffer[0])

/* For writes to minorOp - use no-op since modern API derives from request buffer */
#define SSX_SET_CLIENT_MINOR_OP(client, val) /* no-op: minorOp not stored in modern ClientRec */
#define SSX_SET_CLIENT_MAJOR_OP(client, val) /* no-op: majorOp not stored in modern ClientRec */

/* ===== PRIVATE DATA TYPE INDICES ===== */
/* PrivateRec type indices - used in dixAllocatePrivates/dixFreePrivates */
#ifndef PRIVATE_CLIENT
#define PRIVATE_CLIENT      0
#endif
#ifndef PRIVATE_WINDOW
#define PRIVATE_WINDOW      1
#endif
#ifndef PRIVATE_PIXMAP
#define PRIVATE_PIXMAP      2
#endif
#ifndef PRIVATE_GC
#define PRIVATE_GC           3
#endif
#ifndef PRIVATE_DEVICE
#define PRIVATE_DEVICE      4
#endif
#ifndef PRIVATE_EXTENSION
#define PRIVATE_EXTENSION   5
#endif
#ifndef PRIVATE_COLORMAP
#define PRIVATE_COLORMAP    6
#endif
#ifndef PRIVATE_CURSOR
#define PRIVATE_CURSOR      7
#endif
#ifndef PRIVATE_SCREEN
#define PRIVATE_SCREEN      8
#endif

/* ===== XACE (X Access Control Extension) COMPATIBILITY ===== */
/* Modern XACE moved constants. Preserve legacy security hooks. */
#ifndef XACE_SCREENSAVER_ACCESS
#define XACE_SCREENSAVER_ACCESS    (1 << 0)
#endif
#ifndef XACE_CLIENT_ACCESS
#define XACE_CLIENT_ACCESS         (1 << 1)
#endif
#ifndef XACE_CORE_DISPATCH
#define XACE_CORE_DISPATCH         (1 << 2)
#endif
#ifndef XACE_EXT_DISPATCH
#define XACE_EXT_DISPATCH          (1 << 3)
#endif
#ifndef XACE_PROPERTY_ACCESS
#define XACE_PROPERTY_ACCESS       (1 << 4)
#endif
#ifndef XACE_DRAWABLE_ACCESS
#define XACE_DRAWABLE_ACCESS      (1 << 5)
#endif
#ifndef XACE_WINDOW_ACCESS
#define XACE_WINDOW_ACCESS         (1 << 6)
#endif
#ifndef XACE_RESOURCE_ACCESS
#define XACE_RESOURCE_ACCESS       (1 << 7)
#endif

/* XACE hook function - preserve call sites but make permissive by default */
#ifndef XaceHook
#define XaceHook(hook, ...) Success
#endif

/* ===== LIMIT CLIENTS ===== */
/* Legacy client limit configuration */
extern int LimitClients;

/* ===== CONNECTION INFO STUB ===== */
/* Legacy global ConnectionInfo - modern X uses per-client model */
/* Use extern pointer - can be reassigned by xalloc/xrealloc */
extern char *ConnectionInfo;

/* ===== GC ALL BITS ===== */
/* GCAllBits was removed - it's the OR of all GC value masks */
#ifndef GCAllBits
#define GCAllBits ((1L << (GCLastBit + 1)) - 1)
#endif

/* ===== DIX ACCESS RIGHTS ===== */
#ifndef DixInstallAccess
#define DixInstallAccess (1L << 16)
#endif

#ifndef DixUninstallAccess
#define DixUninstallAccess (1L << 17)
#endif

/* ===== INPUT LOCKING MACRO COLLISION FIX ===== */
/* Modern headers define input_lock/input_unlock as macros with args.
 * Legacy XFree86 declares them as void functions.
 * Force-undefine the macros LAST to win the collision. */

#ifdef input_lock
#undef input_lock
#endif
#ifdef input_unlock
#undef input_unlock
#endif
#ifdef input_force_unlock
#undef input_force_unlock
#endif

/* Now provide the legacy function declarations */
extern void input_lock(void);
extern void input_unlock(void);
extern void input_force_unlock(void);

/* ============================================================================
 * BUG RETURN VAL MSG STUB
 * Debugging macro used in devices.c
 * ============================================================================ */

#ifndef BUG_RETURN_VAL_MSG
#define BUG_RETURN_VAL_MSG(cond, retval, msg, ...) do { if (cond) return retval; } while(0)
#endif

/* ============================================================================
 * SCREEN X/Y ACCESSOR STUBS
 * Legacy code accesses scr->x and scr->y which may not exist in modern ScreenRec
 * ============================================================================ */

#ifndef GET_SCREEN_X
#define GET_SCREEN_X(s) (0)
#endif

#ifndef GET_SCREEN_Y
#define GET_SCREEN_Y(s) (0)
#endif

/* ============================================================================
 * DEVICE CURSOR STUBS
 * Modern ScreenRec has DeviceCursorInitialize/DeviceCursorCleanup
 * ============================================================================ */

#ifndef DeviceCursorInitialize
#define DeviceCursorInitialize(d, s) (TRUE)
#endif

#ifndef DeviceCursorCleanup
#define DeviceCursorCleanup(d, s) /* no-op */
#endif

#ifndef DisplayCursor
#define DisplayCursor(d, s, c) (TRUE)
#endif

/* ============================================================================
 * LEAVEWINDOW STUB
 * Used in DisableDevice
 * ============================================================================ */

/* LeaveWindow is now defined in dix/enterleave.c - removed stub macro */

/* ============================================================================
 * INITXTESTDEVICES STUB
 * Used in InitCoreDevices
 * ============================================================================ */

#ifndef InitXTestDevices
#define InitXTestDevices() /* no-op stub */
#endif

/* ============================================================================
 * TOUCH POINT INFO STUB FIXES
 * Fix for: member reference type 'TouchPointInfoPtr' is a pointer; did you mean to use '->'?
 * ============================================================================ */

/* Provide a stub for TouchInitTouchPoint if not defined */
#ifndef TouchInitTouchPoint
#define TouchInitTouchPoint(t, v, i) /* no-op stub */
#endif

/* Provide a stub for TouchInitDDXTouchPoint if not defined */
#ifndef TouchInitDDXTouchPoint
#define TouchInitDDXTouchPoint(d, t) /* no-op stub */
#endif

/* Provide a stub for TouchFreeTouchPoint if not defined */
#ifndef TouchFreeTouchPoint
#define TouchFreeTouchPoint(d, i) /* no-op stub */
#endif

/* ============================================================================
 * DIX ALLOCATE PRIVATES STUB
#endif

#ifndef dixAllocatePrivates
#define dixAllocatePrivates(p, id) (TRUE)
#endif
/* ============================================================================
 * VALUATOR MASK STUBS
 * Used for scroll handling
 * ============================================================================ */

#ifndef valuator_mask_new
#define valuator_mask_new(n) ((void*)0)
#endif

/* Note: ClassesPtr is defined in inputstr.h - no need to redefine here */

/* NOTE: We don't include inputstr.h or eventstr.h here because ssx_compat.h
 * is prepended to every compilation unit via -include flag, causing include
 * order issues. Instead, we rely on the individual source files to include
 * headers in the correct order.
 * 
 * The ClassesPtr typedef has been removed from here because it depends on
 * types from inputstr.h which aren't available when ssx_compat.h is prepended.
 * Source files that need ClassesPtr should include inputstr.h directly. */

/* ============================================================================
 * SYNC EVENT STUB
 * Used in FreePendingFrozenDeviceEvents
 * Note: syncEvents is defined in events.c - don't define here to avoid conflicts
 * ============================================================================ */

/* ============================================================================
 * PROCESS INPUT PROC TYPE FIX
 * Legacy code expects xEvent*, modern uses InternalEvent*
 * ============================================================================ */

/* The ProcessInputProc typedef in input.h takes xEvent*, but modern code uses InternalEvent*
 * We need to cast the function pointers appropriately in the code */

/* ============================================================================
 * DEVICE INTREC COMPATIBILITY
 * Modern DeviceIntRec may not have unused_classes member
 * ============================================================================ */

/* Stub for unused_classes access */
#ifndef SSX_HAS_UNUSED_CLASSES
#define SSX_UNUSED_CLASSES(dev) /* no-op */
#else
#define SSX_UNUSED_CLASSES(dev) (dev)->unused_classes
#endif

/* ============================================================================
 * CLIENT COMPATIBILITY
 * Modern ClientRec may not have clientPtr member
 * ============================================================================ */

/* Stub for clientPtr access - use alternative method */
#ifndef SSX_HAS_CLIENT_PTR
#define SSX_CLIENT_PTR(c, dev) /* no-op: no clientPtr member */
#else
#define SSX_CLIENT_PTR(c, dev) (c)->clientPtr = (dev)
#endif

/* ============================================================================
 * PRIVATE REC COMPATIBILITY
 * DevUnion vs PrivateRec type mismatch
 * ============================================================================ */

#ifndef SSX_PRIVATE_ALLOC
#define ssx_allocate_privates(p, id) dixAllocatePrivates((PrivateRec*)(p), id)
#else
#define ssx_allocate_privates(p, id) dixAllocatePrivates(p, id)
#endif

/* ============================================================================
 * TOUCH CLASS STUB
 * Touch class initialization for devices without full XI2 support
 * ============================================================================ */

#ifndef SSX_HAS_TOUCH_CLASS
#define SSX_INIT_TOUCH_CLASS(dev) /* no-op: touch not supported */
#else
#define SSX_INIT_TOUCH_CLASS(dev) InitTouchClassDeviceStruct(dev, 0, 0, 0)
#endif

/* ============================================================================
 * POINTER_SCREEN DEFINITION
 * Used in getevents.c - XFree86 had this flag
 * ============================================================================ */

#ifndef POINTER_SCREEN
#define POINTER_SCREEN  (1 << 1)   /* XFree86 value */
#endif

/* ============================================================================
 * XKB STRUCT COMPATIBILITY
 * XFree86 code references xkb_acts and xkb_sli members that were removed
 * ============================================================================ */

/* Button class XKB acts - provide empty definition if not available */
#ifndef SSX_BUTTON_XKB_ACTS
#define SSX_BUTTON_XKB_ACTS(b, action) /* no-op: xkb_acts not available */
#endif

/* Feedback class XKB LED info - provide empty definition if not available */
#ifndef SSX_KBD_FEEDBACK_XKB_SLI
#define SSX_KBD_FEEDBACK_XKB_SLI(kf) ((void*)0)
#endif

#ifndef SSX_LED_FEEDBACK_XKB_SLI
#define SSX_LED_FEEDBACK_XKB_SLI(lf) ((void*)0)
#endif

/* ============================================================================
 * XI2MASK STUBS
 * XFree86 predates XI2 - provide compatibility definitions
 * ============================================================================ */

#ifndef XI2LASTEVENT
#define XI2LASTEVENT  (32)   /* XI2 event type count */
#endif

#ifndef XI2MASK_ISSET
/* Compatibility stubs for XI2Mask - XFree86 predates XI2 */
typedef struct { unsigned char *masks[XI2LASTEVENT]; } XI2MaskCompat;
#  define XI2MaskIsset(mask, dev, ev)  (0)
#  define XI2MaskSet(mask, dev, ev)    do {} while(0)
#endif

/* ============================================================================
 * EVENT SOURCE NORMAL
 * Used in getevents.c for event source classification
 * ============================================================================ */

#ifndef EVENT_SOURCE_NORMAL
#define EVENT_SOURCE_NORMAL  0
#endif

/* ============================================================================
 * SYNC EVENTS DEFINITION
 * Define HAVE_SYNC_EVENTS if Xlibre provides syncEvents
 * ============================================================================ */

/* EventSyncInfoRec - structure for sync event handling */
/* This is defined in events.c but we need the declaration for devices.c */
#ifndef _EVENTSYNCINFO_DEFINED
#define _EVENTSYNCINFO_DEFINED

#include <X11/X.h>

/* Forward declare xorg_list */
struct xorg_list;

typedef struct _EventSyncInfo {
    struct xorg_list pending;
    BOOL playingEvents;
    TimeStamp time;
    DeviceIntPtr replayDev;
    WindowPtr replayWin;
} EventSyncInfoRec;

#endif

#ifndef HAVE_SYNC_EVENTS
/* XFree86 provides syncEvents - define if not provided by Xlibre */
extern EventSyncInfoRec syncEvents;
#endif

/* ============================================================================
 * GET MOTION HISTORY DEFINITION
 * Define HAVE_GET_MOTION_HISTORY if Xlibre provides GetMotionHistory
 * ============================================================================ */

#ifndef HAVE_GET_MOTION_HISTORY
/* XFree86 provides its own GetMotionHistory - wrap if not provided by Xlibre */
#endif

#endif /* SSX_COMPAT_H */
