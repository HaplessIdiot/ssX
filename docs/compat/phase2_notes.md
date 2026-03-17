# Phase 2: DIX Compatibility Notes

## Summary
This document tracks the compatibility fixes applied to XFree86 DIX source code for integration with modern Xlibre.

## Compatibility Header Updates (include/ssx_compat.h)

### 1. Client Minor/Major Op Macros
**Issue**: Modern `ClientRec` dropped `minorOp` and `majorOp` members.

**Fix Added**:
```c
#define SSX_CLIENT_MINOR_OP(client) ((client)->requestBuffer[1])
#define SSX_CLIENT_MAJOR_OP(client) ((client)->requestBuffer[0])
#define SSX_SET_CLIENT_MINOR_OP(client, val) /* no-op */
#define SSX_SET_CLIENT_MAJOR_OP(client, val) /* no-op */
```

### 2. XI2LASTEVENT Definition
**Issue**: `XI2LASTEVENT` was undeclared.

**Fix Added**:
```c
#ifndef XI2LASTEVENT
#define XI2LASTEVENT  (32)   /* XI2 event type count */
#endif
```

### 3. EventSyncInfoRec Definition
**Issue**: `syncEvents` variable type undefined.

**Fix Added**:
```c
typedef struct _EventSyncInfo {
    struct xorg_list pending;
    BOOL playingEvents;
    TimeStamp time;
    DeviceIntPtr replayDev;
    WindowPtr replayWin;
} EventSyncInfoRec;

extern EventSyncInfoRec syncEvents;
```

### 4. POINTER_SCREEN Definition
**Issue**: `POINTER_SCREEN` undefined.

**Fix Added**:
```c
#ifndef POINTER_SCREEN
#define POINTER_SCREEN  (1 << 1)   /* XFree86 value */
#endif
```

### 5. EVENT_SOURCE_NORMAL Definition
**Issue**: `EVENT_SOURCE_NORMAL` undeclared.

**Fix Added**:
```c
#ifndef EVENT_SOURCE_NORMAL
#define EVENT_SOURCE_NORMAL  0
#endif
```

## Files Requiring Source-Level Changes

### dix/devices.c
Multiple issues require direct source changes:

1. **Line 613**: `DeviceCursorInitialize` - needs wrapper based on modern API
2. **Line 653**: `InitKeyboardDeviceStruct` - needs 5th NULL argument
3. **Line 705**: `InitPointerDeviceStruct` - has 8 args, needs 7 (drop one)
4. **Lines 830, 890, 891, 949, 950**: `xkb_acts` and `xkb_sli` members missing
5. **Lines 848-850**: Touch API calls need XI2 guards
6. **Lines 990-992**: `syncEvents.pending` access needs fixing
7. **Lines 1025, 1031-1032**: `xkb_interest` member missing

### dix/dispatch.c
1. **Lines 530, 535, 572**: Replace `client->minorOp` with `SSX_CLIENT_MINOR_OP(client)`
2. **Line 1570**: Function signature mismatch - too many arguments
3. **Line 2271**: Function signature mismatch - too many arguments

### dix/getevents.c
1. **GetMotionHistory redefinition**: Needs `#ifndef HAVE_GET_MOTION_HISTORY` guard
2. **Lines 852-853, 861-862, 979, 986**: Screen.x/Screen.y access - use GET_SCREEN_X/Y macros
3. **Line 831**: Function call arguments mismatch
4. **Line 961**: Too many arguments to function
5. **Lines 1008, 1012, 1022, 1051**: Type incompatibilities
6. **Line 1072**: EVENT_SOURCE_NORMAL undeclared

### dix/events.c
1. **syncEvents redefinition**: Needs `#ifndef HAVE_SYNC_EVENTS` guard
2. **Screen.x/y accesses**: Use GET_SCREEN_X/Y macros
3. **XI2Mask**: Uses XI2MASK_ISSET stubs

## Remaining Errors (Phase 3)

The following errors remain and should be addressed in Phase 3:

### devices.c (12 errors)
- DeviceCursorInitialize signature
- InitKeyboardDeviceStruct args
- InitPointerDeviceStruct args
- xkb_acts/xkb_sli member access
- Touch API incompatibilities
- syncEvents.pending access
- xkb_interest member access

### dispatch.c (5 errors)
- minorOp access (3 occurrences)
- Function signature mismatches (2 occurrences)

### getevents.c (multiple errors)
- GetMotionHistory redefinition
- POINTER_SCREEN/Screen.x/y
- Type incompatibilities
- Function argument mismatches

### events.c
- syncEvents redefinition
- Screen.x/y accesses
- XI2Mask issues

## Values Requiring Archaeology

| Constant | Value | Source |
|----------|-------|--------|
| POINTER_SCREEN | (1 << 1) | XFree86 include/XIproto.h |
| XI2LASTEVENT | 32 | Modern XI2 headers |
| EVENT_SOURCE_NORMAL | 0 | Modern eventstr.h |

## Notes

- All macro definitions added to `ssx_compat.h` use `#ifndef` guards to avoid redefinition
- XKB struct members (`xkb_acts`, `xkb_sli`, `xkb_interest`) were removed in modern X - need compatibility shims
- Touch API (XI 2.2) didn't exist in XFree86 - wrapped in conditionals where possible
- The compatibility approach preserves original XFree86 logic while adapting to modern API signatures
