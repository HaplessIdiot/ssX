/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/s3virge/s3v_macros.h,v 1.2 1999/03/07 11:40:39 dawes Exp $ */

/*
Copyright (C) 1994-1999 The XFree86 Project, Inc.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the XFree86 Project shall not
be used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the XFree86 Project.
*/

#define SUB_SYS_CTL	0x8504

#define SRC_BASE	0xA4D4
#define DEST_BASE	0xA4D8
#define CLIP_L_R	0xA4DC
#define CLIP_T_B	0xA4E0
#define DEST_SRC_STR	0xA4E4
#define MONO_PAT_0	0xA4E8
#define MONO_PAT_1	0xA4EC
#define PAT_BG_CLR	0xA4F0
#define PAT_FG_CLR	0xA4F4 
#define SRC_BG_CLR	0xA4F8
#define SRC_FG_CLR	0xA4FC
#define CMD_SET		0xA500 
#define RWIDTH_HEIGHT	0xA504
#define RSRC_XY		0xA508 
#define RDEST_XY	0xA50C



#define BLT_BUG		0x00000001
#define MONO_TRANS_BUG	0x00000002


#define WAITFIFO(n) if(ps3v->NoPCIRetry) \
	 while(((INREG(SUB_SYS_CTL) >> 8) & 0x1f) < n){}

#define WAITIDLE()  while((INREG(SUB_SYS_CTL) & 0x3f00) < 0x3000){}

#define CHECK_DEST_BASE(y,h)\
    if((y < ps3v->DestBaseY) || ((y + h) > (ps3v->DestBaseY + 2048))) {\
	ps3v->DestBaseY = ((y + h) <= 2048) ? 0 : y;\
	WAITFIFO(1);\
	OUTREG(DEST_BASE, ps3v->DestBaseY * ps3v->Stride);\
    }\
    y -= ps3v->DestBaseY

#define CHECK_SRC_BASE(y,h)\
    if((y < ps3v->SrcBaseY) || ((y + h) > (ps3v->SrcBaseY + 2048))) {\
	ps3v->SrcBaseY = ((y + h) <= 2048) ? 0 : y;\
	WAITFIFO(1);\
	OUTREG(SRC_BASE, ps3v->SrcBaseY * ps3v->Stride);\
    }\
    y -= ps3v->SrcBaseY


#define NO_SRC_ROP(r) \
  ((r == GXnoop) || (r == GXclear) || (r == GXinvert) || (r == GXset))
