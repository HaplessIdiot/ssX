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
/* $XFree86$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86fbman.h"

#include "capture.h"
#include "via.h"
#include "ddmpeg.h"
#include "via_privIoctl.h"	     /* for VIAGRAPHICINFO & custom ioctl command */

#include "regrec.h"
#include "ddover.h"
#include "via_driver.h"
#include "via_swov.h"
#include "via_memmgr.h"


/* E X T E R N	 G L O B A L S ----------------------------------------------*/
extern VIAGRAPHICINFO gVIAGraphicInfo;	/* 2D information */

extern volatile CARD8  * lpVidMEMIO;

extern BOOL   XserverIsUp;		/* If Xserver had run(register action) */

/* kernel module global variables */
extern ViaOffScrnPtr MemLayOut;
extern unsigned long viaBankSize;   /* Amount of total frame buffer */

extern LPVIAVIDCTRL  lpVideoControl ;

extern CARD32 gdwOverlaySupport;

/* G L O B A L	 V A R I A B L E S ------------------------------------------*/

CARD32	 gdwVideoFlagSW=0;
CARD32	 gdwColorkeyFlag = 1;		/* For Alpha blending use*/

/* memory management */
ViaMMReq  SWMemRequest;
ViaMMReq  HQVMemRequest;


VIDEOFORMAT VFsrc;
LPVIDEOFORMAT lpVFsrc = &VFsrc;

/* device struct */
CAPDEVICE   SWDevice;
OVERLAYRECORD	overlayRecordV1;

VIAPtr	pViaSWOV;
ScrnInfoPtr pScrnSWOV;
viaPortPrivPtr pPrivSWOV = NULL;
viaPortPrivPtr pPrivHQV = NULL;
BoxRec	AvailFBArea;
FBLinearPtr   SWOVlinear;

BOOL SWVideo_ON = FALSE;

/*To solve the bandwidth issue */
CARD32	 gdwUseExtendedFIFO = 0;

/* For panning mode use */
static int panning_old_x=0;
static int panning_old_y=0;
static int panning_x=0;
static int panning_y=0;

/*To solve the bandwidth issue */
CARD8 Save_3C4_16 =0;
CARD8 Save_3C4_17 =0;
CARD8 Save_3C4_18 =0;

/*************************************************************************
   Function : PassViaInfo
*************************************************************************/
void PassViaInfo(ScrnInfoPtr pScrnOV, viaPortPrivPtr pPrivOV)
{
    pViaSWOV  = VIAPTR(pScrnOV);
    pScrnSWOV = pScrnOV;
    pPrivSWOV = pPrivOV;
}

/*************************************************************************
   Function : VIAVidCreateSurface
   Create overlay surface depend on FOURCC
*************************************************************************/
CARD32 VIAVidCreateSurface( LPSURFACEPARAM lpSurfaceParam)
{
    CARD32   dwWidth, dwHeight, dwPitch=0;
    CARD32   dwBackBuffers;
    CARD32   dwRet=DP_OK;
    CARD32   dwAddr;
    CARD32   HQVFBSIZE = 0, SWFBSIZE = 0;
    int	    iCount;	   /* iCount for clean HQV FB use */
    CARD8    *lpTmpAddr;    /* for clean HQV FB use */
    int	    dwNewHight = 0;
    int	    depth = 0, DisplayWidth32 = 0;


    DBG_DD(ErrorF("//VIAVidCreateSurface: \n"));

    if ( lpSurfaceParam == NULL )
	return DP_FAIL;

    switch (lpSurfaceParam->dwFourCC)
    {
       case FOURCC_YUY2 :
	    lpVFsrc->dwFlags = VF_FOURCC;
	    lpVFsrc->dwFourCC = FOURCC_YUY2;

	    /* init Video status flag*/
	    gdwVideoFlagSW = VIDEO_HQV_INUSE | SW_USE_HQV | VIDEO_1_INUSE;
	    /*write Color Space Conversion param.*/
	    VIDOutD(V1_ColorSpaceReg_1, ColorSpaceValue_1);
	    VIDOutD(V1_ColorSpaceReg_2, ColorSpaceValue_2);

	    DBG_DD(ErrorF("00000284 %08x\n",ColorSpaceValue_1));
	    DBG_DD(ErrorF("00000288 %08x\n",ColorSpaceValue_2));

	    dwBackBuffers = lpSurfaceParam->dwBackBuffers;
	    dwWidth  = lpSurfaceParam->dwWidth;
	    dwHeight = lpSurfaceParam->dwHeight;
	    dwPitch  = ALIGN_TO_32_BYTES(dwWidth)*2;
	    DBG_DD(ErrorF("    srcWidth= %ld \n", dwWidth));
	    DBG_DD(ErrorF("    srcHeight= %ld \n", dwHeight));

	    SWFBSIZE = dwPitch*dwHeight;    /*YUYV*/

	    if(pPrivSWOV->area)
	    {
		pPrivSWOV->area = NULL;
		xf86FreeOffscreenArea(pPrivSWOV->area);
		DBG_DD(ErrorF("xfree86 Manager Free Init_SWOV Offscreen Memory Success!!!! \n"));
	    }

	    dwNewHight = dwHeight<<1;
	    depth = (pScrnSWOV->bitsPerPixel + 7 ) >> 3;
	    DisplayWidth32 = ALIGN_TO_32_BYTES(pScrnSWOV->displayWidth);

	    if(!(pPrivSWOV->area = xf86AllocateOffscreenArea(pScrnSWOV->pScreen,
				   pScrnSWOV->displayWidth, dwNewHight, 32, NULL, NULL, NULL)))
	    {
		 DBG_DD(ErrorF("xfree86 Manager Allocate FAIL!!!! \n"));
		 return BadAlloc;
	    }
	    DBG_DD(ErrorF("xfree86 Manager Allocate Success!!!! \n"));
	    DBG_DD(ErrorF("Reserved area from (%d,%d) to (%d,%d)\n",pPrivSWOV->area->box.x1, pPrivSWOV->area->box.y1,pPrivSWOV->area->box.x2, pPrivSWOV->area->box.y2));

	    dwAddr = (pPrivSWOV->area->box.y1) * (DisplayWidth32 * depth) + (pPrivSWOV->area->box.x1 * depth);

	    /* fill in the SW buffer with 0x8000 (YUY2-black color) to clear FB buffer*/
	    lpTmpAddr = pViaSWOV->FBBase + dwAddr;

	    for(iCount=0;iCount<(SWFBSIZE*2);iCount++)
	    {
		if((iCount%2) == 0)
		    *lpTmpAddr++=0x00;
		else
		    *lpTmpAddr++=0x80;
	    }

	    SWDevice.dwCAPPhysicalAddr[0]   = dwAddr;
	    SWDevice.lpCAPOverlaySurface[0] = pViaSWOV->FBBase+dwAddr;

	    SWDevice.dwCAPPhysicalAddr[1] = SWDevice.dwCAPPhysicalAddr[0] + SWFBSIZE;
	    SWDevice.lpCAPOverlaySurface[1] = SWDevice.lpCAPOverlaySurface[0] + SWFBSIZE;

	    DBG_DD(ErrorF("SWDevice.dwCAPPhysicalAddr[0] %08lx\n", dwAddr));
	    DBG_DD(ErrorF("SWDevice.dwCAPPhysicalAddr[1] %08lx\n", dwAddr + SWFBSIZE));

	    SWDevice.gdwCAPSrcWidth = dwWidth;
	    SWDevice.gdwCAPSrcHeight= dwHeight;
	    SWDevice.dwPitch = dwPitch;

	    /* Fill image data in overlay record*/
	    overlayRecordV1.dwV1OriWidth  = dwWidth;
	    overlayRecordV1.dwV1OriHeight = dwHeight;
	    overlayRecordV1.dwV1OriPitch  = dwPitch;
	    if (!(gdwVideoFlagSW & SW_USE_HQV))
	       break;

	case FOURCC_HQVSW :
	    DBG_DD(ErrorF("//Create HQV_SW Surface\n"));
	    dwWidth  = SWDevice.gdwCAPSrcWidth;
	    dwHeight = SWDevice.gdwCAPSrcHeight;
	    dwPitch  = SWDevice.dwPitch;

	    HQVFBSIZE = dwPitch * dwHeight;

	    if(pPrivHQV->area)
	    {
		pPrivHQV->area = NULL;
		xf86FreeOffscreenArea(pPrivHQV->area);
		DBG_DD(ErrorF("xfree86 Manager Free Init_HQV Offscreen Memory Success!!!! \n"));
	    }

	    if(!(pPrivHQV->area = xf86AllocateOffscreenArea(pScrnSWOV->pScreen,
				  pScrnSWOV->displayWidth, dwNewHight, 32, NULL, NULL, NULL)))
	    {
		 DBG_DD(ErrorF("xfree86 Manager Allocate HQV FAIL!!!! \n"));
		 return BadAlloc;
	    }
	    DBG_DD(ErrorF("xfree86 Manager Allocate HQV Success!!!! \n"));
	    DBG_DD(ErrorF("Reserved area from (%d,%d) to (%d,%d)\n",pPrivHQV->area->box.x1, pPrivHQV->area->box.y1,pPrivHQV->area->box.x2, pPrivHQV->area->box.y2));

	    dwAddr = (pPrivHQV->area->box.y1) * (DisplayWidth32 * depth) + (pPrivHQV->area->box.x1 * depth);

	    overlayRecordV1.dwHQVAddr[0] = dwAddr;
	    overlayRecordV1.dwHQVAddr[1] = dwAddr + HQVFBSIZE;

	    if (overlayRecordV1.dwHQVAddr[1] + HQVFBSIZE >=
				    (CARD32)gVIAGraphicInfo.VideoHeapEnd)
	    {
		DBG_DD(ErrorF("//    :Memory not enough for MPEG with HQV\n"));
		return DP_FAIL_CANNOT_CREATE_SURFACE;
	    }

	    /* fill in the HQV buffer with 0x8000 (YUY2-black color) to clear HQV buffers*/
	    for(iCount=0;iCount<HQVFBSIZE*2;iCount++)
	    {
		lpTmpAddr = pViaSWOV->FBBase + dwAddr + iCount;
		if((iCount%2) == 0)
		    *(lpTmpAddr)=0x00;
		else
		    *(lpTmpAddr)=0x80;
	    }

	    VIDOutD(HQV_DST_STARTADDR1,overlayRecordV1.dwHQVAddr[1]);
	    VIDOutD(HQV_DST_STARTADDR0,overlayRecordV1.dwHQVAddr[0]);

	    DBG_DD(ErrorF("000003F0 %08lx\n",VIDInD(HQV_DST_STARTADDR1) ));
	    DBG_DD(ErrorF("000003EC %08lx\n",VIDInD(HQV_DST_STARTADDR0) ));

	    break;

	default :
	    break;
    }

    return dwRet;

} /*VIAVidCreateSurface*/

/*************************************************************************
   Function : VIAVidLockSurface
   Lock Surface
*************************************************************************/
CARD32 VIAVidLockSurface(LPLOCKPARAM lpLock)
{
    switch (lpLock->dwFourCC)
    {
       case FOURCC_YUY2 :
	    lpLock->SWDevice = SWDevice ;
	    lpLock->dwPhysicalBase = pViaSWOV->FrameBufferBase;

    }

    return DP_OK;

} /*VIAVidLockSurface*/



/****************************************************************************
 *
 * Upd_Video()
 *
 ***************************************************************************/
static CARD32 Upd_Video(VIAPtr pVia, CARD32 dwVideoFlag,CARD32 dwStartAddr,BOXPARAM SrcBox,BOXPARAM DestBox,CARD32 dwSrcPitch,
		 CARD32 dwOriSrcWidth,CARD32 dwOriSrcHeight,LPVIDEOFORMAT lpVFsrc,
		 CARD32 dwDeinterlaceMode,CARD32 dwColorKey,CARD32 dwChromaKey,
		 CARD32 dwKeyLow,CARD32 dwKeyHigh,CARD32 dwChromaLow,CARD32 dwChromaHigh, CARD32 dwFlags)
{
    CARD32 dwVidCtl=0, dwCompose=(VIDInD(V_COMPOSE_MODE)&~(SELECT_VIDEO_IF_COLOR_KEY|V1_COMMAND_FIRE))|V_COMMAND_LOAD_VBI;
    CARD32 srcWidth, srcHeight,dstWidth,dstHeight;
    CARD32 zoomCtl=0, miniCtl=0;
    CARD32 dwHQVCtl=0;
    CARD32 dwHQVfilterCtl=0,dwHQVminiCtl=0;
    CARD32 dwHQVzoomflagH=0,dwHQVzoomflagV=0;
    CARD32 dwHQVsrcWidth=0,dwHQVdstWidth=0;
    CARD32 dwHQVsrcFetch = 0,dwHQVoffset=0;
    CARD32 dwOffset=0,dwFetch=0,dwTmp=0;
    CARD32 dwDisplayCountW=0;
    /* VIAPtr  pVia = VIAPTR(pScrnSWOV); */

    DBG_DD(ErrorF("// Upd_Video:\n"));
    DBG_DD(ErrorF("Modified SrcBox  X (%ld,%ld) Y (%ld,%ld)\n",
		SrcBox.x1, SrcBox.x2,SrcBox.y1, SrcBox.y2));
    DBG_DD(ErrorF("Modified DestBox  X (%ld,%ld) Y (%ld,%ld)\n",
		DestBox.x1, DestBox.x2,DestBox.y1, DestBox.y2));

    if (dwVideoFlag & VIDEO_SHOW)
    {
	overlayRecordV1.dwWidth=dstWidth = DestBox.x2 - DestBox.x1;
	overlayRecordV1.dwHeight=dstHeight = DestBox.y2 - DestBox.y1;
	srcWidth = (CARD32)SrcBox.x2 - SrcBox.x1;
	srcHeight = (CARD32)SrcBox.y2 - SrcBox.y1;
	DBG_DD(ErrorF("===srcWidth= %ld \n", srcWidth));
	DBG_DD(ErrorF("===srcHeight= %ld \n", srcHeight));

	if (dwVideoFlag & VIDEO_1_INUSE)
	{
	    /* Overlay source format for V1*/
	    if (gdwUseExtendedFIFO)
	    {
		dwVidCtl = (V1_ENABLE|V1_EXPIRE_NUM_A|V1_FIFO_EXTENDED);
	    }
	    else
	    {
		dwVidCtl = (V1_ENABLE|V1_EXPIRE_NUM);
	    }
	    DDOver_GetV1Format(dwVideoFlag,lpVFsrc,&dwVidCtl,&dwHQVCtl);
	    DDOver_GetV1Format(dwVideoFlag,lpVFsrc,&dwVidCtl,&dwHQVCtl);
	}

	/* Starting address of source and Source offset*/
	dwOffset = DDOver_GetSrcStartAddress (dwVideoFlag,SrcBox,DestBox,dwSrcPitch,lpVFsrc,&dwHQVoffset );
	DBG_DD(ErrorF("===dwOffset= 0x%lx \n", dwOffset));

	overlayRecordV1.dwOffset = dwOffset;

	if (lpVFsrc->dwFourCC == FOURCC_YV12)
	{
	    YCBCRREC YCbCr;
	    if (dwVideoFlag & VIDEO_HQV_INUSE)
	    {
		dwHQVsrcWidth=(CARD32)SrcBox.x2 - SrcBox.x1;
		dwHQVdstWidth=(CARD32)DestBox.x2 - DestBox.x1;
		if (dwHQVsrcWidth>dwHQVdstWidth)
		{
		    dwOffset = dwOffset * dwHQVdstWidth / dwHQVsrcWidth;
		}

		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_STARTADDR_0, overlayRecordV1.dwHQVAddr[0]+dwOffset);
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_STARTADDR_1, overlayRecordV1.dwHQVAddr[1]+dwOffset);
		}
		YCbCr = DDOVer_GetYCbCrStartAddress(dwVideoFlag,dwStartAddr,overlayRecordV1.dwOffset,overlayRecordV1.dwUVoffset,dwSrcPitch,dwOriSrcHeight);
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, HQV_SRC_STARTADDR_Y, YCbCr.dwY);
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, HQV_SRC_STARTADDR_U, YCbCr.dwCR);
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, HQV_SRC_STARTADDR_V, YCbCr.dwCB);
	    }
	    else
	    {
		YCbCr = DDOVer_GetYCbCrStartAddress(dwVideoFlag,dwStartAddr,overlayRecordV1.dwOffset,overlayRecordV1.dwUVoffset,dwSrcPitch,dwOriSrcHeight);
		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_STARTADDR_0, YCbCr.dwY);
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_STARTADDR_CB0, YCbCr.dwCR);
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_STARTADDR_CR0, YCbCr.dwCB);
		}
	    }
	}
	else
	{
	    if (dwVideoFlag & VIDEO_HQV_INUSE)
	    {
		dwHQVsrcWidth=(CARD32)SrcBox.x2 - SrcBox.x1;
		dwHQVdstWidth=(CARD32)DestBox.x2 - DestBox.x1;
		if (dwHQVsrcWidth>dwHQVdstWidth)
		{
		    dwOffset = dwOffset * dwHQVdstWidth / dwHQVsrcWidth;
		}

		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_STARTADDR_0, overlayRecordV1.dwHQVAddr[0]+dwHQVoffset);
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_STARTADDR_1, overlayRecordV1.dwHQVAddr[1]+dwHQVoffset);
		}
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, HQV_SRC_STARTADDR_Y, dwStartAddr);
	    }
	    else
	    {
		dwStartAddr += dwOffset;

		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_STARTADDR_0, dwStartAddr);
		}
	    }
	}

	dwFetch = DDOver_GetFetch(dwVideoFlag,lpVFsrc,srcWidth,dstWidth,dwOriSrcWidth,&dwHQVsrcFetch);
	DBG_DD(ErrorF("===dwFetch= 0x%lx \n", dwFetch));

	if (dwVideoFlag & VIDEO_HQV_INUSE)
	{
	    if ( !(dwDeinterlaceMode & OVERLAY_INTERLEAVED) && (dwDeinterlaceMode & OVERLAY_BOB ) )
	    {
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, HQV_SRC_FETCH_LINE, (((dwHQVsrcFetch>>3)-1)<<16)|((dwOriSrcHeight<<1)-1));
	    }
	    else
	    {
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, HQV_SRC_FETCH_LINE, (((dwHQVsrcFetch>>3)-1)<<16)|(dwOriSrcHeight-1));
	    }
	    if (lpVFsrc->dwFourCC == FOURCC_YV12)
	    {
		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_STRIDE, dwSrcPitch<<1);
		}
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, HQV_SRC_STRIDE, ((dwSrcPitch>>1)<<16)|dwSrcPitch);
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, HQV_DST_STRIDE, (dwSrcPitch<<1));
	    }
	    else
	    {
		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_STRIDE, dwSrcPitch);
		}
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, HQV_SRC_STRIDE, dwSrcPitch);
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, HQV_DST_STRIDE, dwSrcPitch);
	    }

	}
	else
	{
	    if (dwVideoFlag & VIDEO_1_INUSE)
	    {
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_STRIDE, dwSrcPitch );
	    }
	}

	DBG_DD(ErrorF("SrcBox  X (%ld,%ld) Y (%ld,%ld)\n",
		SrcBox.x1, SrcBox.x2,SrcBox.y1, SrcBox.y2));
	DBG_DD(ErrorF("DestBox	X (%ld,%ld) Y (%ld,%ld)\n",
		DestBox.x1, DestBox.x2,DestBox.y1, DestBox.y2));

	/* Destination window key*/

	if (dwVideoFlag & VIDEO_1_INUSE)
	{

	    if	((gVIAGraphicInfo.dwDVIOn)&&(gVIAGraphicInfo.dwExpand))
	    {
		 Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V1_WIN_END_Y,
				((DestBox.x2-1)<<16) + (DestBox.y2*(gVIAGraphicInfo.dwPanelHeight)/gVIAGraphicInfo.dwHeight));
		 if (DestBox.y1 > 0)
		 {
		      Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_WIN_START_Y,
				     (DestBox.x1<<16) + (DestBox.y1*(gVIAGraphicInfo.dwPanelHeight)/gVIAGraphicInfo.dwHeight));
		 }
		 else
		 {
		      Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_WIN_START_Y,(DestBox.x1<<16));
		 }
	    }
	    else
	    {
		 Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V1_WIN_END_Y, ((DestBox.x2-1)<<16) + (DestBox.y2-1));
		 if (DestBox.y1 > 0)
		 {
		     Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_WIN_START_Y, (DestBox.x1<<16) + DestBox.y1 );
		 }
		 else
		 {
		     Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_WIN_START_Y, (DestBox.x1<<16));
		 }
		}
	}

	dwCompose |= ALWAYS_SELECT_VIDEO;

	/* Setup X zoom factor*/
	overlayRecordV1.dwFetchAlignment = 0;

	if ( DDOVER_HQVCalcZoomWidth(dwVideoFlag, srcWidth , dstWidth,
				  &zoomCtl, &miniCtl, &dwHQVfilterCtl, &dwHQVminiCtl,&dwHQVzoomflagH) == DP_FAIL )
	{
	    /* too small to handle*/
	    dwFetch <<= 20;
	    if (dwVideoFlag & VIDEO_1_INUSE)
	    {
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V12_QWORD_PER_LINE, dwFetch);
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V_COMPOSE_MODE , dwCompose|V1_COMMAND_FIRE );
	    }
	    Macro_VidREGFlush();
	    return DP_FAIL;
	}

	dwFetch <<= 20;
	if (dwVideoFlag & VIDEO_1_INUSE)
	{
	    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V12_QWORD_PER_LINE, dwFetch);
	}

	/*
	// Setup Y zoom factor
	*/

	if ( (dwDeinterlaceMode & OVERLAY_INTERLEAVED) && (dwDeinterlaceMode & OVERLAY_BOB))
	{
	    if (!(dwVideoFlag & VIDEO_HQV_INUSE))
	    {
		srcHeight /=2;
		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    dwVidCtl |= (V1_BOB_ENABLE | V1_FRAME_BASE);
		}
	    }
	    else
	    {
		dwHQVCtl |= HQV_FIELD_2_FRAME|HQV_FRAME_2_FIELD|HQV_DEINTERLACE;
	    }
	}
	else if (dwDeinterlaceMode & OVERLAY_BOB )
	{
	    if (dwVideoFlag & VIDEO_HQV_INUSE)
	    {
		srcHeight <<=1;
		dwHQVCtl |= HQV_FIELD_2_FRAME|HQV_DEINTERLACE;
	    }
	    else
	    {
		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    dwVidCtl |= V1_BOB_ENABLE;
		}
	    }
	}

	DDOver_GetDisplayCount(dwVideoFlag,lpVFsrc,srcWidth,&dwDisplayCountW);

	if (dwVideoFlag & VIDEO_1_INUSE)
	{
	    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V1_SOURCE_HEIGHT, (srcHeight<<16)|dwDisplayCountW);
	}

	if ( DDOVER_HQVCalcZoomHeight(srcHeight,dstHeight,&zoomCtl,&miniCtl, &dwHQVfilterCtl, &dwHQVminiCtl ,&dwHQVzoomflagV) == DP_FAIL )
	{
	    if (dwVideoFlag & VIDEO_1_INUSE)
	    {
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V_COMPOSE_MODE , dwCompose|V1_COMMAND_FIRE );
	    }

	    Macro_VidREGFlush();
	    return DP_FAIL;
	}

	if (miniCtl & V1_Y_INTERPOLY)
	{
	    if (lpVFsrc->dwFourCC == FOURCC_YV12)
	    {
		if (dwVideoFlag & VIDEO_HQV_INUSE)
		{
		    if (dwVideoFlag & VIDEO_1_INUSE)
		    {
			Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V_FIFO_CONTROL,
			    V1_FIFO_DEPTH32 | V1_FIFO_PRETHRESHOLD29 | V1_FIFO_THRESHOLD16);
		    }
		}
		else
		{

		    if (srcWidth <= 80) /*Fetch count <= 5*/
		    {
			if (dwVideoFlag & VIDEO_1_INUSE)
			{
			    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V_FIFO_CONTROL,V1_FIFO_DEPTH16 );
			}
		    }
		    else
		    {
			if (dwVideoFlag & VIDEO_1_INUSE)
			{
			    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V_FIFO_CONTROL,
				V1_FIFO_DEPTH16 | V1_FIFO_PRETHRESHOLD12 | V1_FIFO_THRESHOLD8);
			}
		    }
		}
	    }
	    else
	    {
		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    if (gdwUseExtendedFIFO)
		    {
			Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V_FIFO_CONTROL,
			    V1_FIFO_DEPTH48 | V1_FIFO_PRETHRESHOLD40 | V1_FIFO_THRESHOLD40);
		    }
		    else
		    {
			Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V_FIFO_CONTROL,
			    V1_FIFO_DEPTH32 | V1_FIFO_PRETHRESHOLD29 | V1_FIFO_THRESHOLD16);
		    }
		}
	    }
	}
	else
	{
	    if (lpVFsrc->dwFourCC == FOURCC_YV12)
	    {
		if (dwVideoFlag & VIDEO_HQV_INUSE)
		{
		    if (dwVideoFlag & VIDEO_1_INUSE)
		    {
			Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V_FIFO_CONTROL,
			    V1_FIFO_DEPTH32 | V1_FIFO_PRETHRESHOLD29 | V1_FIFO_THRESHOLD16);
		    }
		}
		else
		{

		    if (srcWidth <= 80) /*Fetch count <= 5*/
		    {
			if (dwVideoFlag & VIDEO_1_INUSE)
			{
			    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V_FIFO_CONTROL,V1_FIFO_DEPTH16 );
			}
		    }
		    else
		    {
			if (dwVideoFlag & VIDEO_1_INUSE)
			{
			    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V_FIFO_CONTROL,
				V1_FIFO_DEPTH16 | V1_FIFO_PRETHRESHOLD12 | V1_FIFO_THRESHOLD8);
			}
		    }
		}
	    }
	    else
	    {
		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    if (gdwUseExtendedFIFO)
		    {
			Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V_FIFO_CONTROL,
			    V1_FIFO_DEPTH48 | V1_FIFO_PRETHRESHOLD40 | V1_FIFO_THRESHOLD40);
		    }
		    else
		    {
			Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, V_FIFO_CONTROL,
			    V1_FIFO_DEPTH32 | V1_FIFO_PRETHRESHOLD29 | V1_FIFO_THRESHOLD16);
		    }
		}
	    }
	}

	if (dwVideoFlag & VIDEO_HQV_INUSE)
	{
	    miniCtl=0;
	    if (dwHQVzoomflagH||dwHQVzoomflagV)
	    {
		dwTmp = 0;
		if (dwHQVzoomflagH)
		{
		    miniCtl = V1_X_INTERPOLY;
		    dwTmp = (zoomCtl&0xffff0000);
		}

		if (dwHQVzoomflagV)
		{
		    miniCtl |= (V1_Y_INTERPOLY | V1_YCBCR_INTERPOLY);
		    dwTmp |= (zoomCtl&0x0000ffff);
		    dwHQVfilterCtl &= 0xfffdffff;
		}


		if ((gdwUseExtendedFIFO))
		{
		    miniCtl &= ~V1_Y_INTERPOLY;
		}

		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V1_MINI_CONTROL, miniCtl);
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V1_ZOOM_CONTROL, dwTmp);
		}
	    }
	    else
	    {
		if (srcHeight==dstHeight)
		{
		    dwHQVfilterCtl &= 0xfffdffff;
		}
		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V1_MINI_CONTROL, 0);
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V1_ZOOM_CONTROL, 0);
		}
	    }
	    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,HQV_MINIFY_CONTROL, dwHQVminiCtl);
	    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,HQV_FILTER_CONTROL, dwHQVfilterCtl);
	}
	else
	{
	    if (dwVideoFlag & VIDEO_1_INUSE)
	    {
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V1_MINI_CONTROL, miniCtl);
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V1_ZOOM_CONTROL, zoomCtl);
	    }
	}

	/* Colorkey*/
	if (dwColorKey) {
	    DBG_DD(ErrorF("Overlay colorkey= low:%08lx high:%08lx\n", dwKeyLow, dwKeyHigh));

	    dwKeyLow &= 0x00FFFFFF;
	    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V_COLOR_KEY, dwKeyLow);

	    dwCompose = (dwCompose & ~0x0f) | SELECT_VIDEO_IF_COLOR_KEY;
	}

	if (dwChromaKey) {
	    DBG_DD(ErrorF("Overlay Chromakey= low:%08lx high:%08lx\n", dwKeyLow, dwKeyHigh));

	    dwChromaLow	 &= CHROMA_KEY_LOW;
	    dwChromaHigh &= CHROMA_KEY_HIGH;

	    dwChromaLow	 |= (VIDInD(V_CHROMAKEY_LOW)&(~CHROMA_KEY_LOW));
	    dwChromaHigh |= (VIDInD(V_CHROMAKEY_HIGH)&(~CHROMA_KEY_HIGH));

	    /*for Chroma Key*/
	    if (lpVFsrc->dwFlags & VF_FOURCC)
	    {
		    switch (lpVFsrc->dwFourCC) {
		    case FOURCC_YV12:
			    /*to be continued...*/
			    break;
		    case FOURCC_YUY2:
			    /*to be continued...*/
			    break;
		    default:
			    /*TOINT3;*/
			    break;
		    }
	    }

	    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V_CHROMAKEY_HIGH,dwChromaHigh);
	    if (dwVideoFlag & VIDEO_1_INUSE)
	    {
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V_CHROMAKEY_LOW, dwChromaLow);

		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V1_MINI_CONTROL, miniCtl & 0xFFFFFFF8);
	    }

	    /*for select video if (color key & chroma key)*/
	    if (dwCompose==SELECT_VIDEO_IF_COLOR_KEY)
	       dwCompose = SELECT_VIDEO_IF_COLOR_KEY | SELECT_VIDEO_IF_CHROMA_KEY;
	    else
	       dwCompose = (dwCompose & ~0x0f) | SELECT_VIDEO_IF_CHROMA_KEY;
	}

	/* Setup video control*/
	if (dwVideoFlag & VIDEO_HQV_INUSE)
	{
	    if (!SWVideo_ON)
	    {
		DBG_DD(ErrorF("	   First HQV\n"));

		Macro_VidREGFlush();

		WaitHQVFlipClear(((dwHQVCtl&~HQV_SW_FLIP)|HQV_FLIP_STATUS)&~HQV_ENABLE);
		VIDOutD( HQV_CONTROL, dwHQVCtl);
		WaitHQVFlip();
		WaitHQVFlipClear(((dwHQVCtl&~HQV_SW_FLIP)|HQV_FLIP_STATUS)&~HQV_ENABLE);
		VIDOutD( HQV_CONTROL, dwHQVCtl);
		WaitHQVFlip();

		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    VIDOutD(V1_CONTROL, dwVidCtl);
		    VIDOutD(V_COMPOSE_MODE, (dwCompose|V1_COMMAND_FIRE ));
		    if (gdwUseExtendedFIFO)
		    {
			/*Set Display FIFO*/
			WaitVBI();
			VGAOUT8(0x3C4, 0x17); VGAOUT8(0x3C5, 0x2f);
			VGAOUT8(0x3C4, 0x16); VGAOUT8(0x3C5, (Save_3C4_16&0xf0)|0x14);
			VGAOUT8(0x3C4, 0x18); VGAOUT8(0x3C5, 0x56);
		    }
		}
	    }
	    else
	    {
		DBG_DD(ErrorF("	   Normal called\n"));
		if (dwVideoFlag & VIDEO_1_INUSE)
		{
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V1_CONTROL, dwVidCtl);
		    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V_COMPOSE_MODE, (dwCompose|V1_COMMAND_FIRE ));
		}
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER, HQV_CONTROL, dwHQVCtl|HQV_FLIP_STATUS);
		WaitHQVDone();
		Macro_VidREGFlush();
	    }
	}
	else
	{
	    if (dwVideoFlag & VIDEO_1_INUSE)
	    {
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V1_CONTROL, dwVidCtl);
		Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V_COMPOSE_MODE, (dwCompose|V1_COMMAND_FIRE ));
	    }
	    WaitHQVDone();
	    Macro_VidREGFlush();
	}
	SWVideo_ON = TRUE;
    }
    else
    {
	/*Hide overlay*/
	Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V_FIFO_CONTROL,V1_FIFO_PRETHRESHOLD12 |
	     V1_FIFO_THRESHOLD8 |V1_FIFO_DEPTH16);

	if (dwVideoFlag&VIDEO_HQV_INUSE)
	{
	    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,HQV_CONTROL, (VIDInD(HQV_CONTROL) & (~HQV_ENABLE)));
	}

	if (dwVideoFlag&VIDEO_1_INUSE)
	{
	    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V1_CONTROL, (VIDInD(V1_CONTROL) & (~V1_ENABLE)));
	    Macro_VidREGRec(VIDREGREC_SAVE_REGISTER,V_COMPOSE_MODE, (VIDInD(V_COMPOSE_MODE)|V1_COMMAND_FIRE));
	}

	Macro_VidREGFlush();
    }

    return DP_OK;

} /* Upd_Video */

/*************************************************************************
 *  VIAVidUpdateOverlay
 *  Parameters:	  src rectangle, dst rectangle, colorkey...
 *  Return Value: CARD32 of state
 *  note: Update the overlay image param.
*************************************************************************/
CARD32 VIAVidUpdateOverlay(VIAPtr pVia, LPUPDATEOVERLAYREC lpUpdate)
{
    CARD32 dwFlags = lpUpdate->dwFlags;
    CARD32 dwKeyLow=0, dwKeyHigh=0;
    CARD32 dwChromaLow=0, dwChromaHigh=0;
    CARD32 dwVideoFlag=0;
    CARD32 dwColorKey=0, dwChromaKey=0;
    /*UPDATEOVERLAYREC UpdateOverlayTemp;*/
    int	  nDstTop, nDstBottom, nDstLeft, nDstRight, nTopBak=0;
    /* VIAPtr  pVia = VIAPTR(pScrnSWOV); */

    DBG_DD(ErrorF("// VIAVidUpdateOverlay: %08lx\n", dwFlags));

    /* Adjust to fix panning mode bug */
    lpUpdate->DestBox.x1 = lpUpdate->DestBox.x1 - (panning_x - panning_old_x);
    lpUpdate->DestBox.y1 = lpUpdate->DestBox.y1 - (panning_y - panning_old_y);
    lpUpdate->DestBox.x2 = lpUpdate->DestBox.x2 - (panning_x - panning_old_x);
    lpUpdate->DestBox.y2 = lpUpdate->DestBox.y2 - (panning_y - panning_old_y);

    DBG_DD(ErrorF("Raw SrcBox  X (%ld,%ld) Y (%ld,%ld)\n",
		lpUpdate->SrcBox.x1, lpUpdate->SrcBox.x2, lpUpdate->SrcBox.y1, lpUpdate->SrcBox.y2));
    DBG_DD(ErrorF("Raw DestBox	X (%ld,%ld) Y (%ld,%ld)\n",
		lpUpdate->DestBox.x1, lpUpdate->DestBox.x2, lpUpdate->DestBox.y1, lpUpdate->DestBox.y2));

    if ( lpVFsrc->dwFourCC == FOURCC_YUY2 )
	dwVideoFlag = gdwVideoFlagSW;

    dwFlags |= OVERLAY_INTERLEAVED;

    /* For Alpha windows setting*/
    if (gdwColorkeyFlag)
	dwFlags |= OVERLAY_KEYDEST;
    else
	dwFlags &= ~OVERLAY_KEYDEST;

    Macro_VidREGRec(VIDREGREC_RESET_COUNTER, 0,0);

    if ( dwFlags & OVERLAY_HIDE )
    {
	DBG_DD(ErrorF("//    :OVERLAY_HIDE \n"));

	dwVideoFlag &= ~VIDEO_SHOW;

	{
	    if (Upd_Video(pVia, dwVideoFlag,0,lpUpdate->SrcBox,lpUpdate->DestBox,0,0,0,lpVFsrc,0,
		0,0,0,0,0,0, dwFlags)== DP_FAIL)
	    {
		return DP_FAIL;
	    }
	    SWVideo_ON = FALSE;
	}

	if (gdwUseExtendedFIFO)
	{
	    /*Restore Display fifo*/
	    VGAOUT8(0x3C4, 0x16); VGAOUT8(0x3C5, Save_3C4_16);
	    DBG_DD(ErrorF("Restore 3c4.16 : %08x \n",VGAIN8(0x3C5)));

	    VGAOUT8(0x3C4, 0x17); VGAOUT8(0x3C5, Save_3C4_17);
	    DBG_DD(ErrorF("	   3c4.17 : %08x \n",VGAIN8(0x3C5)));

	    VGAOUT8(0x3C4, 0x18); VGAOUT8(0x3C5, Save_3C4_18);
	    DBG_DD(ErrorF("	   3c4.18 : %08x \n",VGAIN8(0x3C5)));
	    gdwUseExtendedFIFO = 0;
		}
	return DP_OK;
    }

    if ( dwFlags & OVERLAY_KEYDEST )
    {
	DBG_DD(ErrorF("//    :OVERLAY_KEYDEST \n"));

	dwColorKey = 1;
	dwKeyLow = lpUpdate->dwColorKey;
    }

    if (dwFlags & OVERLAY_SHOW)
    {
	CARD32 dwStartAddr=0, dwDeinterlaceMode=0;
	CARD32 dwScnWidth, dwScnHeight;

	DBG_DD(ErrorF("//    :OVERLAY_SHOW \n"));

	/*for SW decode HW overlay use*/
	dwStartAddr = VIDInD(HQV_SRC_STARTADDR_Y);
	DBG_DD(ErrorF("dwStartAddr= 0x%lx\n", dwStartAddr));

	if (dwFlags & OVERLAY_INTERLEAVED)
	{
	    dwDeinterlaceMode |= OVERLAY_INTERLEAVED;
	    DBG_DD(ErrorF("OVERLAY_INTERLEAVED\n"));
	}
	if (dwFlags & OVERLAY_BOB)
	{
	    dwDeinterlaceMode |= OVERLAY_BOB;
	    DBG_DD(ErrorF("OVERLAY_BOB\n"));
	}

	if ((gVIAGraphicInfo.dwWidth > 1024))
		{
	    DBG_DD(ErrorF("UseExtendedFIFO\n"));
			gdwUseExtendedFIFO = 1;
		}
	dwVideoFlag |= VIDEO_SHOW;

	nDstLeft = lpUpdate->DestBox.x1;
	nDstTop	 = lpUpdate->DestBox.y1;
	nDstRight= lpUpdate->DestBox.x2;
	nDstBottom=lpUpdate->DestBox.y2;

	dwScnWidth  = gVIAGraphicInfo.dwWidth;
	dwScnHeight = gVIAGraphicInfo.dwHeight;

	if (nDstLeft<0)
	    lpUpdate->SrcBox.x1	 = (((-nDstLeft) * overlayRecordV1.dwV1OriWidth) + ((nDstRight-nDstLeft)>>1)) / (nDstRight-nDstLeft);
	else
	    lpUpdate->SrcBox.x1 = 0;

	if (nDstRight>dwScnWidth)
	    lpUpdate->SrcBox.x2 = (((dwScnWidth-nDstLeft) * overlayRecordV1.dwV1OriWidth) + ((nDstRight-nDstLeft)>>1)) / (nDstRight-nDstLeft);
	else
	    lpUpdate->SrcBox.x2 = overlayRecordV1.dwV1OriWidth;

	if (nDstTop<0)
	{
	   lpUpdate->SrcBox.y1	 =  (((-nDstTop) * overlayRecordV1.dwV1OriHeight) + ((nDstBottom-nDstTop)>>1))/ (nDstBottom-nDstTop);
	   nTopBak = (-nDstTop);
	}
	else
	   lpUpdate->SrcBox.y1	 = 0;

	if (nDstBottom >dwScnHeight)
	    lpUpdate->SrcBox.y2 = (((dwScnHeight-nDstTop) * overlayRecordV1.dwV1OriHeight) + ((nDstBottom-nDstTop)>>1)) / (nDstBottom-nDstTop);
	else
	    lpUpdate->SrcBox.y2 = overlayRecordV1.dwV1OriHeight;

	/* save modified src & original dest rectangle param.*/
	if ( lpVFsrc->dwFourCC == FOURCC_YUY2 )
	{
	    SWDevice.gdwCAPDstLeft     = lpUpdate->DestBox.x1 + (panning_x - panning_old_x);
	    SWDevice.gdwCAPDstTop      = lpUpdate->DestBox.y1 + (panning_y - panning_old_y);
	    SWDevice.gdwCAPDstWidth    = lpUpdate->DestBox.x2 - lpUpdate->DestBox.x1;
	    SWDevice.gdwCAPDstHeight   = lpUpdate->DestBox.y2 - lpUpdate->DestBox.y1;

	    SWDevice.gdwCAPSrcWidth    = overlayRecordV1.dwV1SrcWidth = lpUpdate->SrcBox.x2 - lpUpdate->SrcBox.x1;
	    SWDevice.gdwCAPSrcHeight   = overlayRecordV1.dwV1SrcHeight = lpUpdate->SrcBox.y2 - lpUpdate->SrcBox.y1;
	}

	overlayRecordV1.dwV1SrcLeft   = lpUpdate->SrcBox.x1;
	overlayRecordV1.dwV1SrcRight  = lpUpdate->SrcBox.x2;
	overlayRecordV1.dwV1SrcTop    = lpUpdate->SrcBox.y1;
	overlayRecordV1.dwV1SrcBot    = lpUpdate->SrcBox.y2;

	/*
	// Figure out actual DestBox rectangle
	*/
	lpUpdate->DestBox.x1= nDstLeft<0 ? 0 : nDstLeft;
	lpUpdate->DestBox.y1= nDstTop<0 ? 0 : nDstTop;
	if ( lpUpdate->DestBox.y1 >= dwScnHeight)
	   lpUpdate->DestBox.y1 = dwScnHeight-1;
	lpUpdate->DestBox.x2= nDstRight>dwScnWidth ? dwScnWidth: nDstRight;
	lpUpdate->DestBox.y2= nDstBottom>dwScnHeight ? dwScnHeight: nDstBottom;

	if (Upd_Video(pVia, dwVideoFlag,dwStartAddr,lpUpdate->SrcBox,lpUpdate->DestBox,SWDevice.dwPitch,
	    overlayRecordV1.dwV1OriWidth,overlayRecordV1.dwV1OriHeight,lpVFsrc,
	    dwDeinterlaceMode,dwColorKey,dwChromaKey,
	    dwKeyLow,dwKeyHigh,dwChromaLow,dwChromaHigh, dwFlags)== DP_FAIL)
	{
	    return DP_FAIL;
	}
	SWVideo_ON = FALSE;

	return DP_OK;

    } /*end of OVERLAY_SHOW*/

    panning_old_x = panning_x;
    panning_old_y = panning_y;

    return DP_OK;

} /*VIAVidUpdateOverlay*/


/*************************************************************************
 *  Destroy Surface
*************************************************************************/
CARD32 VIAVidDestroySurface( LPSURFACEPARAM lpSurfaceParam)
{

    DBG_DD(ErrorF("//VIAVidDestroySurface: \n"));

    switch (lpSurfaceParam->dwFourCC)
    {
       case FOURCC_YUY2 :
	    lpVFsrc->dwFlags = 0;
	    lpVFsrc->dwFourCC = 0;
	    SWMemRequest.checkID = VIA_MID;
	    SWMemRequest.type	 = VIDMGR_TYPE_VIDEO;

	    if(pPrivSWOV->area)
	    {
		xf86FreeOffscreenArea(pPrivSWOV->area);
		pPrivSWOV->area = NULL;
		DBG_DD(ErrorF("xfree86 Manager Free SWOV Offscreen Memory Success!!!! \n"));
	    }

	    if (!(gdwVideoFlagSW & SW_USE_HQV))
	    {
		gdwVideoFlagSW = 0;
		break;
	    }

       case FOURCC_HQVSW :
	    HQVMemRequest.checkID = VIA_MID;
	    HQVMemRequest.type	  = VIDMGR_TYPE_VIDEO;

	    if(pPrivHQV->area)
	    {
		xf86FreeOffscreenArea(pPrivHQV->area);
		pPrivHQV->area = NULL;
		AvailFBArea.x1 = 0;
		AvailFBArea.y1 = 0;
		AvailFBArea.x2 = 0;
		AvailFBArea.y2 = 0;
		DBG_DD(ErrorF("xfree86 Manager Free HQV Offscreen Memory Success!!!! \n"));
	    }

	    gdwVideoFlagSW = 0;
	    break;

    }
    DBG_DD(ErrorF("\n//VIAVidDestroySurface : OK!!\n"));
    return DP_OK;

} /*VIAVidDestroySurface*/








