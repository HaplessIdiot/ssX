/* $TOG: Xcupstr.h /main/2 1997/10/21 15:48:35 kaleb $ */
/*
Copyright © 1986-1997   The Open Group    All Rights Reserved

Permission is hereby granted, free of charge, to any person obtaining a copy of this 
software and associated documentation files (the Software), to use the Software 
without restriction, including, without limitation, the rights to copy, modify, merge, 
publish, distribute and sublicense the Software, to make, have made, license and 
distribute derivative works thereof, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and the following permission notice shall be included in all 
copies of the Software:

THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES 
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-
INFRINGEMENT.  IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY 
CLAIM, DAMAGES OR OTHER SIABILITIY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN 
CONNNECTION WITH THE SOFTWARE OR THE USE OF OTHER DEALINGS IN 
THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be used in 
advertising or otherwise to promote the use or other dealings in this Software without 
prior written authorization from The Open Group.

X Window System is a trademark of The Open Group.
*/

#ifndef _XCUPSTR_H_ /* { */
#define _XCUPSTR_H_

#include "Xcup.h"

#define XCUPNAME "TOG-CUP"

#define XCUP_MAJOR_VERSION	1	/* current version numbers */
#define XCUP_MINOR_VERSION	0

typedef struct _XcupQueryVersion {
    CARD8	reqType;	/* always XcupReqCode */
    CARD8	xcupReqType;	/* always X_XcupQueryVersion */
    CARD16	length B16;
    CARD16	client_major_version B16;
    CARD16	client_minor_version B16;
} xXcupQueryVersionReq;
#define sz_xXcupQueryVersionReq		8

typedef struct {
    BYTE	type;		/* X_Reply */
    BOOL	pad1;
    CARD16	sequence_number B16;
    CARD32	length B32;
    CARD16	server_major_version B16;
    CARD16	server_minor_version B16;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xXcupQueryVersionReply;
#define sz_xXcupQueryVersionReply	32

typedef struct _XcupGetReservedColormapEntries {
    CARD8	reqType;	/* always XcupReqCode */
    CARD8	xcupReqType;	/* always X_XcupGetReservedColormapEntries */
    CARD16	length B16;
    CARD32	screen B32;
} xXcupGetReservedColormapEntriesReq;
#define sz_xXcupGetReservedColormapEntriesReq 8

typedef struct {
    BYTE	type;		/* X_Reply */
    BOOL	pad1;
    CARD16	sequence_number B16;
    CARD32	length B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
    CARD32	pad7 B32;
} xXcupGetReservedColormapEntriesReply;
#define sz_xXcupGetReservedColormapEntriesReply		32

typedef struct _XcupStoreColors {
    CARD8	reqType;	/* always XcupReqCode */
    CARD8	xcupReqType;	/* always X_XcupStoreColors */
    CARD16	length B16;
    CARD32	cmap B32;
} xXcupStoreColorsReq;
#define sz_xXcupStoreColorsReq		8

typedef struct {
    BYTE	type;		/* X_Reply */
    BOOL	pad1;
    CARD16	sequence_number B16;
    CARD32	length B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
    CARD32	pad7 B32;
} xXcupStoreColorsReply;
#define sz_xXcupStoreColorsReply	32

#endif /* } _XCUPSTR_H_ */

