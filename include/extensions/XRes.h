/*
   Copyright (c) 2002  XFree86 Inc
*/
/* $XFree86: xc/include/extensions/XRes.h,v 1.1 2002/03/04 19:31:35 mvojkovi Exp $ */

#ifndef _XRES_H
#define _XRES_H

#include <X11/Xfuncproto.h>

typedef struct {
  XID resource_base;
  XID resource_mask;
} XResClient;

typedef struct {
  Atom resource_type;
  unsigned int count;
} XResType;

_XFUNCPROTOBEGIN


Bool XResQueryExtension (
   Display *dpy,
   int *event_base,
   int *error_base
);

Status XResQueryVersion (
   Display *dpy,
   int *major,
   int *minor
);

Status XResQueryClients (
   Display *dpy,
   int *num_clients,
   XResClient **clients
);

Status XResQueryClientResources (
   Display *dpy,
   XID xid,
   int *num_types,
   XResType **types
);


_XFUNCPROTOEND

#endif /* _XRES_H */
