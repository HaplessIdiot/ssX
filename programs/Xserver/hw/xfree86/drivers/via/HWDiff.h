/******************************Module*Header*******************************
* Module Name: HWDiff.h
*
* Put all video function and parameters.
*
* Copyright (c) 2003 VIA Technologies, Inc.  All rights reserved.
**************************************************************************/

#ifndef _VIDEOFUN_H
#define _VIDEOFUN_H

/**************************************************************************
//Global Variable for Video function
**************************************************************************/


/**************************************************************************
//Extern Variable for Video function
**************************************************************************/


/**************************************************************************
//Extern function for Video function
**************************************************************************/


/**************************************************************************
//Defination for Video HW Difference 
**************************************************************************/
/*For Video HW Difference */
#define VIA_REVISION_CLEC0        0x10
#define VIA_REVISION_CLEC1        0x11
#define VIA_REVISION_CLECX        0x10

#define VID_HWDIFF_TRUE           0x00000001
#define VID_HWDIFF_FALSE          0x00000000

/**************************************************************************
//Structure defination for Video function
**************************************************************************/
/*Video HW Difference Structure*/
typedef struct _VIDHWDIFFERENCE
{
    unsigned long dwThreeHQVBuffer;			/*Use Three HQV Buffers*/
    unsigned long dwV3SrcHeightSetting;			/*Set Video Source Width and Height*/
    unsigned long dwSupportExtendFIFO;			/*Support Extand FIFO*/
    unsigned long dwHQVFetchByteUnit;			/*HQV Fetch Count unit is byte*/
    unsigned long dwHQVInitPatch;			/*Initialize HQV Engine 2 times*/
    unsigned long dwSupportV3Gamma;			/*Support V3 Gamma */
    unsigned long dwUpdFlip;				/*Set HQV3D0[15] to flip video*/
    unsigned long dwHQVDisablePatch;			/*Change Video Engine Clock setting for HQV disable bug*/
    unsigned long dwSUBFlip;				/*Set HQV3D0[15] to flip video for sub-picture blending*/
    unsigned long dwNeedV3Prefetch;			/*V3 pre-fetch function for K8*/
    unsigned long dwNeedV4Prefetch;			/*V4 pre-fetch function for K8*/
    unsigned long dwUseSystemMemory;			/*Use system memory for DXVA compressed data buffers*/
    unsigned long dwExpandVerPatch;			/*Patch video HW bug in expand SIM mode or same display path*/
    unsigned long dwExpandVerHorPatch;			/*Patch video HW bug in expand SAMM mode or same display path*/
    unsigned long dwV3ExpireNumTune;			/*Change V3 expire number setting for V3 bandwidth issue*/
    unsigned long dwV3FIFOThresholdTune;		/*Change V3 FIFO, Threshold and Pre-threshold setting for V3 bandwidth issue*/
    unsigned long dwCheckHQVFIFOEmpty;                  /*HW Flip path, need to check HQV FIFO status */
    unsigned long dwUseMPEGAGP;                         /*Use MPEG AGP function*/
    unsigned long dwV3FIFOPatch;                        /*For CLE V3 FIFO Bug (srcWidth <= 8)*/
    unsigned long dwSupportTwoColorKey;                 /*Support two color key*/
    unsigned long dwCxColorSpace;                       /*CLE_Cx ColorSpace*/
}VIDHWDIFFERENCE, * LPVIDHWDIFFERENCE;


/*Wait Function Structure and Flag*/
typedef struct _WaitHWINFO
{
    unsigned char* pjVideo;				/*MMIO Address Info*/
    unsigned long dwVideoFlag;				/*Video Flag*/
}WaitHWINFO, * LPWaitHWINFO;

struct _VIA;

void VIAvfInitHWDiff( struct _VIA* Via );

#endif    

