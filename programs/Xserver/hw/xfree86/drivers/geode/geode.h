/* $XFree86$ */
/*
 * $Workfile: geode.h $
 * $Revision: 1.1 $
 *
 * File Contents: This file contains the data structures Geode driver.
 *
 * Project:       Geode Xfree Frame buffer device driver.
 * 
 */

/* 
 * NSC_LIC_ALTERNATIVE_PREAMBLE
 *
 * Revision 1.0
 *
 * National Semiconductor Alternative GPL-BSD License
 *
 * National Semiconductor Corporation licenses this software 
 * ("Software"):
 *
 * Geode Xfree frame buffer driver
 *
 * under one of the two following licenses, depending on how the 
 * Software is received by the Licensee.
 * 
 * If this Software is received as part of the Linux Framebuffer or
 * other GPL licensed software, then the GPL license designated 
 * NSC_LIC_GPL applies to this Software; in all other circumstances 
 * then the BSD-style license designated NSC_LIC_BSD shall apply.
 *
 * END_NSC_LIC_ALTERNATIVE_PREAMBLE */

/* NSC_LIC_BSD
 *
 * National Semiconductor Corporation Open Source License for 
 *
 * Geode Xfree frame buffer driver
 *
 * (BSD License with Export Notice)
 *
 * Copyright (c) 1999-2001
 * National Semiconductor Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 *   * Redistributions of source code must retain the above copyright 
 *     notice, this list of conditions and the following disclaimer. 
 *
 *   * Redistributions in binary form must reproduce the above 
 *     copyright notice, this list of conditions and the following 
 *     disclaimer in the documentation and/or other materials provided 
 *     with the distribution. 
 *
 *   * Neither the name of the National Semiconductor Corporation nor 
 *     the names of its contributors may be used to endorse or promote 
 *     products derived from this software without specific prior 
 *     written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * NATIONAL SEMICONDUCTOR CORPORATION OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE,
 * INTELLECTUAL PROPERTY INFRINGEMENT, OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * EXPORT LAWS: THIS LICENSE ADDS NO RESTRICTIONS TO THE EXPORT LAWS OF 
 * YOUR JURISDICTION. It is licensee's responsibility to comply with 
 * any export regulations applicable in licensee's jurisdiction. Under 
 * CURRENT (2001) U.S. export regulations this software 
 * is eligible for export from the U.S. and can be downloaded by or 
 * otherwise exported or reexported worldwide EXCEPT to U.S. embargoed 
 * destinations which include Cuba, Iraq, Libya, North Korea, Iran, 
 * Syria, Sudan, Afghanistan and any other country to which the U.S. 
 * has embargoed goods and services. 
 *
 * END_NSC_LIC_BSD */

/* NSC_LIC_GPL
 *
 * National Semiconductor Corporation Gnu General Public License for 
 *
 * Geode Xfree frame buffer driver
 *
 * (GPL License with Export Notice)
 *
 * Copyright (c) 1999-2001
 * National Semiconductor Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted under the terms of the GNU General 
 * Public License as published by the Free Software Foundation; either 
 * version 2 of the License, or (at your option) any later version  
 *
 * In addition to the terms of the GNU General Public License, neither 
 * the name of the National Semiconductor Corporation nor the names of 
 * its contributors may be used to endorse or promote products derived 
 * from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * NATIONAL SEMICONDUCTOR CORPORATION OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE, 
 * INTELLECTUAL PROPERTY INFRINGEMENT, OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE. See the GNU General Public License for more details. 
 *
 * EXPORT LAWS: THIS LICENSE ADDS NO RESTRICTIONS TO THE EXPORT LAWS OF 
 * YOUR JURISDICTION. It is licensee's responsibility to comply with 
 * any export regulations applicable in licensee's jurisdiction. Under 
 * CURRENT (2001) U.S. export regulations this software 
 * is eligible for export from the U.S. and can be downloaded by or 
 * otherwise exported or reexported worldwide EXCEPT to U.S. embargoed 
 * destinations which include Cuba, Iraq, Libya, North Korea, Iran, 
 * Syria, Sudan, Afghanistan and any other country to which the U.S. 
 * has embargoed goods and services. 
 *
 * You should have received a copy of the GNU General Public License 
 * along with this file; if not, write to the Free Software Foundation, 
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 *
 * END_NSC_LIC_GPL */

#ifndef _GEODE_H_
#define _GEODE_H_

#include "xaa.h"
#include "xf86Cursor.h"
#include "vgaHW.h"
#include "xf86xv.h"

#if defined(STB_X)
#define GFX(x) Gal_##x
#else
#define GFX(x) gfx_##x
#endif

#if defined(STB_X)
#include "galproto.h"
#else
#include "gfx_rtns.h"
#include "gfx_defs.h"
#include "gfx_regs.h"
#include "panel.h"

typedef struct __TVPARAMS
{
   unsigned int dwFlags;
   unsigned short wWidth;
   unsigned short wHeight;
   unsigned short wStandard;
   unsigned short wType;
   unsigned short wOutput;
   unsigned short wResolution;
   Bool bState;
}
TVPARAMS, *PTVPARAMS;

typedef struct __DISPLAYTIMING
{
   unsigned int dwDotClock;
   unsigned short wPitch;
   unsigned short wBpp;
   unsigned short wHTotal;
   unsigned short wHActive;
   unsigned short wHSyncStart;
   unsigned short wHSyncEnd;
   unsigned short wHBlankStart;
   unsigned short wHBlankEnd;
   unsigned short wVTotal;
   unsigned short wVActive;
   unsigned short wVSyncStart;
   unsigned short wVSyncEnd;
   unsigned short wVBlankStart;
   unsigned short wVBlankEnd;
   unsigned short wPolarity;
}
DISPLAYTIMING, *PDISPLAYTIMING;

/* TV Timings */
typedef struct __TVTIMING
{
   unsigned long HorzTim;
   unsigned long HorzSync;
   unsigned long VertSync;
   unsigned long LineEnd;
   unsigned long VertDownscale;
   unsigned long HorzScaling;
   unsigned long TimCtrl1;
   unsigned long TimCtrl2;
   unsigned long Subfreq;
   unsigned long DispPos;
   unsigned long DispSize;
   unsigned long Debug;
   unsigned long DacCtrl;
   unsigned int DotClock;
}
TVTIMING, *PTVTIMING;

#endif /* STB_X */

/* XFree86 macros */
#define GEODEPTR(p) ((GeodePtr)((p)->driverPrivate))

#define DEBUGMSG(cond, drv_msg) if((cond)) xf86DrvMsg drv_msg

#define DEFAULT_NUM_OF_BUF 20		/* default # of buffers */

/* Overlay Transparency Key */
#define TRANSPARENCY_KEY 255

typedef struct
{
   /* Private struct for the server */
   unsigned long cpu_version;		/* [7:0] Type:1=GXLV,2=SC1400 */
   /* [15:8] Major version */
   /* [23:16] Minor version */
   unsigned long vid_version;		/* [7:0] Type:1=CS5530,2=SC1400 */

   EntityInfoPtr pEnt;
   ScreenBlockHandlerProcPtr BlockHandler;	/* needed for video */
   CloseScreenProcPtr CloseScreen;
   int Chipset;
   int FBLinearAddr;
   unsigned char *FBBase;
   unsigned long FBSize;
   int Pitch;
   Bool HWCursor;
   Bool NoAccel;
   Bool HWOverlay;
   Bool HWVideo;
   unsigned long ColorKey;
   unsigned long VideoKey;

   Bool TVSupport;
#if defined(STB_X)
   GAL_TVPARAMS TvParam;
#else
   TVPARAMS TvParam;
#endif					/* STB_X */

   int TVOx, TVOy, TVOw, TVOh;
   Bool TV_Overscan_On;

   Bool Panel;
   Pnl_PanelParams PnlParam;

   /* Flatpanel support from Bios */
   Bool FPInBios;
   int FPBX;				/* xres */
   int FPBY;				/* yres */
   int FPBB;				/* bpp */
   int FPBF;				/* freq */

   int Rotate;
   Bool ShadowFB;
   unsigned char *ShadowPtr;
   int ShadowPitch;
   void (*PointerMoved) (int index, int x, int y);

   Bool Compression;
   unsigned long CursorStartOffset;
   unsigned int CursorSize;
   xf86CursorInfoPtr CursorInfo;
   int CursorXHot;
   int CursorYHot;
   unsigned long OffscreenStartOffset;
   unsigned int OffscreenSize;

	/***Image Write structures ***/

   /* offset in video memory for ImageWrite Buffers */
   unsigned char **AccelImageWriteBufferOffsets;
   int NoOfImgBuffers;
	/*****************************************/
/* Saved Console State */
#if defined(STB_X)
   GAL_VGAMODEDATA FBgfxVgaRegs;
   GAL_DISPLAYTIMING FBgfxdisplaytiming;
   GAL_TVTIMING FBgfxtvtiming;
#else
   gfx_vga_struct FBgfxVgaRegs;
   DISPLAYTIMING FBgfxdisplaytiming;
   TVTIMING FBtvtiming;
#endif					/* STB_X */
   int FBSoftVGAActive;
   unsigned int FBTVEnabled;
   unsigned long FBDisplayOffset;

   /* compression */
   int FBCompressionEnable;
   unsigned long FBCompressionOffset;
   unsigned short FBCompressionPitch;
   unsigned short FBCompressionSize;

   /* Save the Cursor offset of the FB */
   unsigned long FBCursorOffset;
/*****************************************/

   XAAInfoRecPtr AccelInfoRec;

   DGAModePtr DGAModes;
   int numDGAModes;
   Bool DGAactive;
   int DGAViewportStatus;

   int videoKey;
   XF86VideoAdaptorPtr adaptor;
   int OverlaySkewX;
   int OverlaySkewY;
   int VideoZoomMax;
}
GeodeRec, *GeodePtr;

#endif /* _GEODE_H_ */
