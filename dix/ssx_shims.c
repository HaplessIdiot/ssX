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
