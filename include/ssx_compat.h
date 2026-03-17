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

/* Include input.h to get basic types - this is safe because input.h doesn't
 * create circular dependencies like inputstr.h does. Source files that need
 * inputstr.h can include it directly after ssx_compat.h. */
#include "input.h"

/* ============================================================================
 * BASIC TYPE DEFINITIONS NEEDED BY SSX_COMPAT
 * These are typically defined in input.h and inputstr.h
 * ============================================================================ */

/* MAX_VALUATORS is defined in input.h */

/* Valuator mask for XI2 compatibility - defined here to avoid circular deps */
/* Note: input.h already defines ValuatorMask, so we check first */
#ifndef _VALUATOR_MASK_DEFINED
#define _VALUATOR_MASK_DEFINED
/* ValuatorMask may already be defined in input.h */
#endif

/* Forward declarations for device classes - these are defined in inputstr.h
 * but we need them here because inputstr.h can't be included due to circular deps */
typedef struct _KeyClassRec *KeyClassPtr;
typedef struct _ValuatorClassRec *ValuatorClassPtr;
typedef struct _ButtonClassRec *ButtonClassPtr;
typedef struct _TouchClassRec *TouchClassPtr;
typedef struct _FocusClassRec *FocusClassPtr;
typedef struct _ProximityClassRec *ProximityClassPtr;
typedef struct _KbdFeedbackClassRec *KbdFeedbackPtr;
typedef struct _PtrFeedbackClassRec *PtrFeedbackPtr;
typedef struct _IntegerFeedbackClassRec *IntegerFeedbackPtr;
typedef struct _StringFeedbackClassRec *StringFeedbackPtr;
typedef struct _BellFeedbackClassRec *BellFeedbackPtr;
typedef struct _LedFeedbackClassRec *LedFeedbackPtr;

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
 * These are now defined in dixaccess.h - but we need them in dispatch.c
 * which includes ssx_compat.h before dixaccess.h
 * ============================================================================ */

/* These were removed to avoid redefinition warnings, but dispatch.c needs them */
#ifndef DixAddAccess
#define DixAddAccess (1<<12)
#endif

#ifndef DixReceiveAccess
#define DixReceiveAccess (1<<23)
#endif

#ifndef DixShowAccess
#define DixShowAccess (1<<15)
#endif

#ifndef DixListAccess
#define DixListAccess (1<<11)
#endif

#ifndef DixHideAccess
#define DixHideAccess (1<<14)
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
/* Note: These are now defined in include/privates.h - removed from here to avoid conflicts */

/* ===== XACE (X Access Control Extension) COMPATIBILITY ===== */
/* XACE constants are now defined in Xext/xace.h - don't redefine to avoid warnings */

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
 * LAST DEVICE INFO SCROLL ACCESS
 * Modern API uses fixed array for last.scroll, legacy code treats as ValuatorMask*
 * ============================================================================ */

/* Cast the int[36] array to ValuatorMask* for use with valuator_mask functions */
#ifndef SSX_LAST_SCROLL
#define SSX_LAST_SCROLL(dev) ((ValuatorMask*)(dev)->last.scroll)
#endif

/* Cast the int[36] array to double* for use with motion history functions */
#ifndef SSX_LAST_VALUATORS_PTR
#define SSX_LAST_VALUATORS_PTR(dev) ((double*)(dev)->last.valuators)
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
 * POINTER FLAGS (XFree86/XAA era flags)
 * ============================================================================ */

#ifndef POINTER_NORAW
#define POINTER_NORAW        (1 << 4)
#endif

#ifndef POINTER_EMULATED
#define POINTER_EMULATED     (1 << 5)
#endif

/* ============================================================================
 * DEVICE EVENT SOURCE ENUM
 * Used in getevents.c for event source classification
 * ============================================================================ */

#ifndef _DEVICE_EVENT_SOURCE_ENUM
#define _DEVICE_EVENT_SOURCE_ENUM
typedef enum {
    DEVICE_SOURCE_NONE = -1,
    EVENT_SOURCE_NORMAL = 0,
    EVENT_SOURCE_POINTER = 1,
    EVENT_SOURCE_KEYBOARD = 2,
    EVENT_SOURCE_TOUCH = 3,
    EVENT_SOURCE_BARRIER = 4
} DeviceEventSource;
#endif

/* ============================================================================
 * SCREEN X/Y ACCESSOR STUBS (Legacy XFree86 used scr->x and scr->y)
 * Modern ScreenRec doesn't have x/y in the struct, but screenInfo does
 * 
 * The screenInfo struct has x and y for multi-monitor offset.
 * For legacy code that accesses scr->x/scr->y, we provide accessor macros.
 * ============================================================================ */

/* Legacy screen x/y access - for code that accesses scr->x/scr->y 
 * Note: Modern ScreenRec doesn't have x/y, use screenInfo.x/y if needed
 * But for code using the screen pointer, we need to return 0 as default
 * since modern X doesn't use screen-specific offsets in ScreenRec */

#ifndef GET_SCREEN_X
#define GET_SCREEN_X(s) (0)
#endif

#ifndef GET_SCREEN_Y
#define GET_SCREEN_Y(s) (0)
#endif

/* Helper macro to get screen number from ScreenPtr */
#ifndef GET_SCREEN_NUM
#define GET_SCREEN_NUM(s) ((s)->myNum)
#endif

/* ============================================================================
 * VALUATOR MASK DOUBLE FUNCTIONS - FIX FOR MODERN API
 * Modern X uses double-valued valuator masks for scroll handling
 * These need to accept double* and const pointers properly
 * 
 * IMPORTANT: These macros must NOT use return statements inside do-while
 * because they may be used in conditionals. Use helper functions instead.
 * ============================================================================ */

/* Helper function for valuator_mask_set_double */
static inline void _ssx_valuator_mask_set_double(ValuatorMask *mask, int idx, double val) {
    if (mask && idx >= 0 && idx < MAX_VALUATORS) {
        mask->valuators[idx] = (int)(val);
    }
}

#ifdef valuator_mask_set_double
#undef valuator_mask_set_double
#endif

#ifndef valuator_mask_set_double
#define valuator_mask_set_double(mask, idx, val) _ssx_valuator_mask_set_double(mask, idx, val)
#endif

/* Helper function for valuator_mask_fetch_double */
static inline int _ssx_valuator_mask_fetch_double(ValuatorMask *mask, int idx, double *val) {
    if (mask && val && idx >= 0 && idx < MAX_VALUATORS) {
        *val = (double)(mask->valuators[idx]);
        return 1;
    }
    return 0;
}

#ifdef valuator_mask_fetch_double
#undef valuator_mask_fetch_double
#endif

#ifndef valuator_mask_fetch_double
#define valuator_mask_fetch_double(mask, idx, val) _ssx_valuator_mask_fetch_double(mask, idx, val)
#endif

#ifdef valuator_mask_num_valuators
#undef valuator_mask_num_valuators
#endif

#ifndef valuator_mask_num_valuators
#define valuator_mask_num_valuators(mask) ((mask) ? (mask)->num_valuators : 0)
#endif

/* Helper function for valuator_mask_copy */
static inline void _ssx_valuator_mask_copy(ValuatorMask *dst, ValuatorMask *src) {
    if (dst && src) {
        int _i;
        dst->num_valuators = src->num_valuators;
        for (_i = 0; _i < MAX_VALUATORS; _i++) {
            dst->valuators[_i] = src->valuators[_i];
        }
    }
}

#ifdef valuator_mask_copy
#undef valuator_mask_copy
#endif

#ifndef valuator_mask_copy
#define valuator_mask_copy(dst, src) _ssx_valuator_mask_copy(dst, src)
#endif

/* Helper function for valuator_mask_unset */
static inline void _ssx_valuator_mask_unset(ValuatorMask *mask, int idx) {
    if (mask && idx >= 0 && idx < MAX_VALUATORS) {
        mask->valuators[idx] = 0;
    }
}

#ifdef valuator_mask_unset
#undef valuator_mask_unset
#endif

#ifndef valuator_mask_unset
#define valuator_mask_unset(mask, idx) _ssx_valuator_mask_unset(mask, idx)
#endif

/* ============================================================================
 * MIEQ ENQUEUE COMPATIBILITY
 * Legacy code passes xEvent*, modern uses InternalEvent*
 * ============================================================================ */

/* The mieqEnqueue declaration is in mi/mi.h. We define a wrapper in dix/ssx_shims.c
 * The actual call should use the real mieqEnqueue from mi/mieq.c
 * We don't redefine - let the source files use mieqEnqueue directly from mi/mi.h */

/* ============================================================================
 * GETPOINTEREVENTS COMPATIBILITY
 * Modern GetPointerEvents has 8 parameters, legacy had fewer
 * ============================================================================ */

#ifndef GetPointerEvents
/* Provide a wrapper that expands to the modern 8-param version 
 * Legacy code calls with fewer args - this requires source changes
 * For now, just define the modern signature */
#define GetPointerEvents(events, pDev, type, buttons, flags, first_val, num_val, vals) \
    GetPointerEvents(events, pDev, type, buttons, flags, first_val, num_val, vals)
#endif

/* ============================================================================
 * GETKEYBOARDEVENTS COMPATIBILITY
 * Modern GetKeyboardEvents has 4 parameters
 * ============================================================================ */

#ifndef GetKeyboardEvents
#define GetKeyboardEvents(events, pDev, type, keycode) \
    GetKeyboardEvents(events, pDev, type, keycode)
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
 * ============================================================================ */

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
 * Note: This section is disabled for now to avoid type dependencies
 * ============================================================================ */

#ifndef HAVE_SYNC_EVENTS
/* XFree86 provides syncEvents - define if not provided by Xlibre */
/* Commented out to avoid type dependency issues - syncEvents defined in events.c */
#endif

/* ============================================================================
 * GET MOTION HISTORY COMPATIBILITY
 * ssX Legacy Note: The getevents.c file contains a legacy GetMotionHistory 
 * definition with 6 parameters (including BOOL core). The modern input.h 
 * declares a 5-parameter version wrapped with a macro.
 * 
 * We rename our legacy function to ssx_GetMotionHistory and provide a macro
 * to map the old calls to the new name.
 * ============================================================================ */

/* Legacy code calls GetMotionHistory with 6 params - map to our implementation */
#ifndef GetMotionHistory
#define GetMotionHistory(pDev, buff, start, stop, pScreen, core) \
    ssx_GetMotionHistory(pDev, buff, start, stop, pScreen, core)
#endif

/* ============================================================================
 * VALUATOR MASK DOUBLE FUNCTIONS
 * Modern X uses double-valued valuator masks for scroll handling
 * ============================================================================ */

#ifndef valuator_mask_set_double
/* Stub implementations - actual implementations in inpututils.c */
static inline void valuator_mask_set_double(void *mask, int idx, double val) { (void)mask; (void)idx; (void)val; }
#endif

#ifndef valuator_mask_fetch_double
static inline int valuator_mask_fetch_double(void *mask, int idx, double *val) { (void)mask; (void)idx; (void)val; return 0; }
#endif

#ifndef valuator_mask_num_valuators
static inline int valuator_mask_num_valuators(void *mask) { (void)mask; return 0; }
#endif

#ifndef valuator_mask_copy
static inline void valuator_mask_copy(void *dst, void *src) { (void)dst; (void)src; }
#endif

#ifndef valuator_mask_unset
static inline void valuator_mask_unset(void *mask, int idx) { (void)mask; (void)idx; }
#endif

/* ============================================================================
 * EVENT SOURCE DEFINITIONS
 * Used in getevents.c for event source classification
 * ============================================================================ */

#ifndef EVENT_SOURCE_FOCUS
#define EVENT_SOURCE_FOCUS  1
#endif

#ifndef EVENT_SOURCE_POINTER
#define EVENT_SOURCE_POINTER 0
#endif

/* ============================================================================
 * DEVICE EVENT SOURCE ENUM
 * Forward declare the enum for getevents.c
 * Modern Xlibre already defines DeviceEventSource
 * ============================================================================ */

/* ============================================================================
 * BUG_WARN STUB
 * Debugging macro used in getevents.c
 * ============================================================================ */

#ifndef BUG_WARN
#define BUG_WARN(cond) do { } while(0)
#endif

/* ============================================================================
 * INIT_DEVICE_EVENT STUB
 * Used in getevents.c for device event initialization
 * ============================================================================ */

#ifndef init_device_event
static inline void init_device_event(void *event, int type, Time time) { (void)event; (void)type; (void)time; }
#endif

/* ============================================================================
 * SSX SHIM FUNCTION DECLARATIONS
 * Note: These are now commented out to avoid type dependency issues.
 * Source files that need these functions can declare them directly
 * after including the proper headers (inputstr.h, eventstr.h, etc.)
 * ============================================================================ */

/* The following function declarations were here but require types from inputstr.h:
 * - ssx_get_last_valuators
 * - ssx_set_last_valuators
 * - ssx_screen_get_x
 * - ssx_screen_get_y
 * - ssx_mieqEnqueue
 * - ssx_init_device_event
 *
 * These are now defined in dix/ssx_shims.c and can be used directly.
 */

#endif /* SSX_COMPAT_H */
