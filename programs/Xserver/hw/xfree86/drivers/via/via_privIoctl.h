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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/via_privIoctl.h,v 1.2 2003/04/16 22:29:31 alanh Exp $ */
/*
 *	Information the OS will need for implementing
 *	future extensions.
 */

typedef struct {
    CARD32 dwModeNumber;     /* Mode number to look up mode	 */
    CARD32 dwWidth;	     /* On screen Width			 */
    CARD32 dwHeight;	     /* On screen Height		 */
    CARD32 dwBPP;	     /* Bits Per Pixel			 */
    CARD32 dwPitch;	     /* On screen pitch= width x BPP	 */
    CARD32 dwRefreshRate;    /* Refresh rate of the mode	 */
    CARD32 TotalVRAM;	     /* Total Video RAM in unit of Mega	 */
    CARD32 Offset;	     /* Offset to off screen memory	 */
    CARD32 *ScreenAddress;  /* Linear Base address of screen	 */
    CARD32 *VideoHeapBase;  /* Linear Start of video heap	 */
    CARD32 * VideoHeapEnd;   /* Linear End of video heap	 */
    CARD32 * GEAddress;	     /* Linear address of GE Mem Map	 */
    CARD32 * VidMMAddress;   /* Linear address of Video Mem Map	 */
    CARD32 ScreenPhysAddr;   /* Physical address of Video Memory */
    CARD32 GEMode;	     /* the Data of HW GE mode 3c4 B1+4	 */
    CARD32 InterlaceMode;    /* Is it interlace mode ?		 */
    CARD32 dwDVIOn;	    /* Is it DVI simultaneous mode ?	*/
    CARD32 dwExpand;		    /* Is it expand mode ?		*/
    CARD32 dwPanelWidth;     /* Panel physical Width		 */
    CARD32 dwPanelHeight;    /* Panel physical Height		 */
    CARD32 dwModeWidth;	     /* Mode width			 */
    CARD32 dwModeHeight;     /* Mode Height			 */
    CARD32 Cap0_Deinterlace; /* Capture 0 deintrlace mode	 */
    CARD32 Cap1_Deinterlace; /* Capture 1 deintrlace mode	 */
    CARD32 Cap0_FieldSwap;   /* Capture 0 input field swap	 */
    BOOL  DRMEnabled;	    /* kernel module DRM flag		*/
}VIAGRAPHICINFO, * LPVIAGRAPHICINFO;

