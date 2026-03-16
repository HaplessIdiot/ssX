/*
 * XAA Compatibility Adapter Layer
 * 
 * This header provides backward compatibility definitions for XAA code
 * that was written for XFree86 to work with modern Xorg headers.
 */

#ifndef XAA_COMPAT_H
#define XAA_COMPAT_H

/* Only include inputstr.h if it hasn't been included yet (avoid circular deps) */
#ifndef INPUTSTR_H
#include "inputstr.h"
#endif

#ifndef CURSORSTR_H
#include "cursorstr.h"
#endif

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

/* Touch ownership event structure */
typedef struct _TouchOwnershipEvent {
    unsigned char header;
    unsigned char type;
    unsigned short length;
    Time time;
    int deviceid;
    int sourceid;
    uint32_t touchid;
    uint32_t resource;
    uint32_t flags;
    uint8_t reason;
} TouchOwnershipEvent;

/* ValuatorMask - for XI2 valuator handling */
#ifndef VALUATOR_MASK_SIZE
#define VALUATOR_MASK_SIZE 16
#endif

typedef struct _ValuatorMask {
    int last_bit;
    double valuators[VALUATOR_MASK_SIZE];
    union {
        uint8_t mask[(VALUATOR_MASK_SIZE + 7) / 8];
        unsigned long bits;
    } mask;
} ValuatorMask;

/* ValuatorMask functions - stub implementations */
static inline int valuator_mask_size(const ValuatorMask *m) { return m->last_bit + 1; }
static inline int valuator_mask_isset(const ValuatorMask *m, int i) { return (m->mask.mask[i / 8] >> (i % 8)) & 1; }
static inline void valuator_mask_set(ValuatorMask *m, int i, int val) { m->mask.mask[i / 8] |= (1 << (i % 8)); m->valuators[i] = val; if (i > m->last_bit) m->last_bit = i; }
static inline void valuator_mask_set_double(ValuatorMask *m, int i, double val) { m->mask.mask[i / 8] |= (1 << (i % 8)); m->valuators[i] = val; if (i > m->last_bit) m->last_bit = i; }
static inline double valuator_mask_get(const ValuatorMask *m, int i) { return m->valuators[i]; }
static inline double valuator_mask_get_double(const ValuatorMask *m, int i) { return m->valuators[i]; }
static inline void valuator_mask_copy(ValuatorMask *dst, const ValuatorMask *src) { *dst = *src; }
static inline void valuator_mask_zero(ValuatorMask *m) { memset(m, 0, sizeof(ValuatorMask)); }
static inline void valuator_mask_unset(ValuatorMask *m, int i) { m->mask.mask[i / 8] &= ~(1 << (i % 8)); }

/* Scroll types */
#ifndef SCROLL_TYPE_NONE
#define SCROLL_TYPE_NONE 0
#define SCROLL_TYPE_VERTICAL 1
#define SCROLL_TYPE_HORIZONTAL 2
#endif

/* valuator_get_mode stub */
static inline int valuator_get_mode(DeviceIntPtr dev, int i) { return Relative; }

/* valuator_mask_num_valuators stub */
static inline int valuator_mask_num_valuators(const ValuatorMask *m) { return m->last_bit + 1; }

/* valuator_mask_has_unaccelerated stub */
static inline int valuator_mask_has_unaccelerated(const ValuatorMask *m) { return 0; }

/* valuator_mask_get_unaccelerated stub */
static inline double valuator_mask_get_unaccelerated(const ValuatorMask *m, int i) { return m->valuators[i]; }

/* valuator_mask_drop_unaccelerated stub */
static inline void valuator_mask_drop_unaccelerated(ValuatorMask *m) { }

/* valuator_mask_fetch_double stub */
static inline int valuator_mask_fetch_double(const ValuatorMask *m, int i, double *val) { if (valuator_mask_isset(m, i)) { *val = m->valuators[i]; return 1; } return 0; }

/* DeviceChangedEvent valuators extension */
typedef struct _ValuatorClassChangedInfo {
    uint32_t min;
    uint32_t max;
    uint32_t resolution;
    uint8_t mode;
    Atom name;
} ValuatorClassChangedInfo;

/* Compatibility for callback structures */
#ifndef _CallbackFuncs
#include "callback.h"
#endif

/* Screen->root compatibility for XAA code */
#ifndef ScreenGetRoot
#define ScreenGetRoot(pScreen) WindowTable[(pScreen)->myNum]
#endif

/* Define root in ScreenRec if not present - for XAA compatibility */
#ifndef HAVE_SCREEN_ROOT
/* Note: This uses WindowTable to provide root window access */
#endif

#endif /* XAA_COMPAT_H */
