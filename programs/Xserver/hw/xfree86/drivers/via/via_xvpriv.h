/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/via_xvpriv.h,v 1.1tsi Exp $ */
#ifndef __VIA_XVPRIV_H
#define __VIA_XVPRIV_H

#define	  XV_PORT_NUM	    1
#define	  XV_SWOV_PORTID    0

#define	  COMMAND_FOR_SWOV	XV_SWOV_PORTID

typedef struct {
    unsigned long  xvPortID;
    unsigned char  brightness;
    unsigned char  contrast;
    unsigned long  dwEncoding;
    FBAreaPtr	   area;
    RegionRec	   clip;
    CARD32	   colorKey;
    Time	   offTime;
    Time	   freeTime;
    VIAAUDCTRL	   AudCtrl;

    /* file handle */
    int				nr;

    int				*input;
    int				*norm;
    int				nenc,cenc;

} viaPortPrivRec, *viaPortPrivPtr;

#define	  SET_XVPORTID	     0
#define	  SET_VIDEOSTATUS    1
#define	  SET_BRIGHTNESS     2
#define	  SET_CONTRAST	     3
#define	  SET_COLORKEY	     4


__inline void AllocatePortPriv(void);
__inline void FreePortPriv(void);
__inline void ClearPortPriv(int);
viaPortPrivPtr GetPortPriv(int);
void SetPortPriv(int nIndex, unsigned long dwAction, unsigned long dwValue);
unsigned long  IdentifyPort(viaPortPrivPtr);
#endif /* end of  __VIA_XVPRIV_H */
