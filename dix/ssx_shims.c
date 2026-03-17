/*
 * ssX Shim Layer Implementation
 * 
 * This file provides the actual storage for global variables declared in ssx_compat.h.
 * These are needed to bridge legacy XFree86 code with modern XLibre/DIX.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "ssx_compat.h"
#include "inputstr.h"
#include "misc.h"

/* Connection info pointer - legacy global, can be reassigned */
char *ConnectionInfo = NULL;

/* Client limit configuration */
int LimitClients = 256;

/* Client operation tracking for XACE (shadow storage)
 * Modern ClientRec no longer has minorOp/majorOp, but XACE may need them */
typedef struct {
    CARD8 majorOp;
    CARD8 minorOp;
} SSX_ClientOpTracker;

SSX_ClientOpTracker ssx_client_ops[256] = {{0}};

/* Accessor functions for client operations */
void ssx_set_client_major(void *client, CARD8 op) {
    unsigned long idx = ((unsigned long)client) % 256;
    ssx_client_ops[idx].majorOp = op;
}

void ssx_set_client_minor(void *client, CARD8 op) {
    unsigned long idx = ((unsigned long)client) % 256;
    ssx_client_ops[idx].minorOp = op;
}

CARD8 ssx_get_client_major(void *client) {
    unsigned long idx = ((unsigned long)client) % 256;
    return ssx_client_ops[idx].majorOp;
}

CARD8 ssx_get_client_minor(void *client) {
    unsigned long idx = ((unsigned long)client) % 256;
    return ssx_client_ops[idx].minorOp;
}

/* ============================================================================
 * Legacy XFree86 to Modern Xlibre Shim Wrappers
 * ============================================================================ */

/* Include required headers for wrapper implementations */
#include "dixstruct.h"
#include "mipointer.h"
#include "eventstr.h"
#include "input.h"
#include "windowstr.h"
#include "scrnintstr.h"

/* ============================================================================
 * SSX_LAST_VALUATORS - Convert int[36] to double* for modern APIs
 * ============================================================================ */

/* Legacy XFree86 used int[36] for last.valuators, modern uses double*
 * This wrapper provides the conversion */
#ifndef MAX_VALUATORS
#define MAX_VALUATORS 36
#endif

/* Static buffer for conversion - not thread-safe but matches X server single-threaded event model */
static double ssx_valuator_buffer[MAX_VALUATORS];

double* ssx_get_last_valuators(DeviceIntPtr dev) {
    int i;
    for (i = 0; i < MAX_VALUATORS && i < dev->valuator->numAxes; i++) {
        ssx_valuator_buffer[i] = (double)dev->last.valuators[i];
    }
    return ssx_valuator_buffer;
}

void ssx_set_last_valuators(DeviceIntPtr dev, double *vals) {
    int i;
    for (i = 0; i < MAX_VALUATORS && i < dev->valuator->numAxes; i++) {
        dev->last.valuators[i] = (int)vals[i];
    }
}

/* ============================================================================
 * SCREEN X/Y Shim Functions
 * ============================================================================ */

/* Modern ScreenRec doesn't have x/y members. Use screenInfo for desktop coordinates.
 * These functions provide the legacy screen offset behavior. */

int ssx_screen_get_x(ScreenPtr scr) {
    /* Modern X doesn't use per-screen offsets. Return 0. */
    return 0;
}

int ssx_screen_get_y(ScreenPtr scr) {
    /* Modern X doesn't use per-screen offsets. Return 0. */
    return 0;
}

/* ============================================================================
 * GetMotionHistory Shim
 * ============================================================================ */

/* Legacy GetMotionHistory has 6 parameters including BOOL core
 * Modern GetMotionHistory in input.h has 5 parameters wrapped with macro
 * This shim provides the legacy signature by calling the modern version */

#ifndef HAVE_GET_MOTION_HISTORY

int
GetMotionHistory(DeviceIntPtr pDev, xTimecoord **buff, unsigned long start,
                 unsigned long stop, ScreenPtr pScreen, BOOL core)
{
    /* Call the modern 5-parameter version
     * Note: This is a placeholder - actual implementation in getevents.c */
    xTimecoord *modern_buff = NULL;
    int ret;
    
    ret = GetMotionHistory(pDev, modern_buff, start, stop, pScreen);
    
    /* Copy results if needed - simplified for now */
    if (ret > 0 && modern_buff) {
        *buff = modern_buff;
    }
    
    return ret;
}

#endif

/* ============================================================================
 * mieqEnqueue Shim
 * ============================================================================ */

/* Legacy code may pass xEvent*, modern expects InternalEvent*
 * This wrapper handles the conversion */

void ssx_mieqEnqueue(DeviceIntPtr pDev, void *event) {
    /* Cast to InternalEvent - in most cases this should work since
     * InternalEvent is the union that contains xEvent */
    mieqEnqueue(pDev, (InternalEvent *)event);
}

/* ============================================================================
 * init_device_event Shim
 * ============================================================================ */

/* Legacy init_device_event may have different signature
 * This provides compatibility */

void ssx_init_device_event(DeviceEvent *event, DeviceIntPtr dev, Time ms, 
                          enum DeviceEventSource source) {
    memset(event, 0, sizeof(DeviceEvent));
    event->header = ET_Internal;
    event->type = ET_KeyPress;  /* Default */
    event->length = sizeof(DeviceEvent);
    event->time = ms;
    event->deviceid = dev->id;
    event->sourceid = dev->id;
}
