/* $XFree86: xc/programs/Xserver/Xext/appgroup.h,v 1.1tsi Exp $ */

#ifndef APPGROUP_H
#define APPGROUP_H 1

void XagClientStateChange(
    CallbackListPtr* pcbl,
    pointer nulldata,
    pointer calldata);
int ProcXagCreate (
    register ClientPtr client);
int ProcXagDestroy(
    register ClientPtr client);

#endif
