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
#ifndef _VIA_BIOS_H
#define _VIA_BIOS_H

#define	    VIA_CRT_SUPPORT		  TRUE
#define	    VIA_LCD_SUPPORT		  TRUE
#define	    VIA_UNCOVERD_LCD_PANEL	  FALSE
#define	    VIA_NTSC_SUPPORT		  TRUE
#define	    VIA_PAL_SUPPORT		  TRUE
#define	    VIA_DVI_SUPPORT		  TRUE

#define	    VIA_CRT_SUPPORT_BIT		  0x01
#define	    VIA_LCD_SUPPORT_BIT		  0x02
#define	    VIA_NTSC_SUPPORT_BIT	  0x04
#define	    VIA_PAL_SUPPORT_BIT		  0x08
#define	    VIA_DVI_SUPPORT_BIT		  0x20

#define	    VIA_BIOS_REG_TABLE_MAX_NUM	    32
#define	    VIA_BIOS_REG_LCD_MAX_NUM	    48
#define	    VIA_BIOS_NUM_RES		    16
#define	    VIA_BIOS_NUM_REFRESH	    6
#define	    VIA_BIOS_NUM_LCD_SUPPORT_MASK   16
#define	    VIA_BIOS_NUM_LCD_POWER_SEQ	    4
#define	    VIA_BIOS_NUM_PANEL		    7
#define	    VIA_BIOS_MAX_NUM_MPATCH2	    18
#define	    VIA_BIOS_MAX_NUM_MPATCH1	    9
#define	    VIA_BIOS_MAX_NUM_CTREXP	    5
#define	    VIA_BIOS_MAX_NUM_TV_REG	    144
#define	    VIA_BIOS_MAX_NUM_TV_CRTC	    32
#define	    VIA_BIOS_NUM_TV_SPECIAL_REG	    8
#define	    VIA_BIOS_MAX_NUM_TV_PATCH	    8
#define	    VIA_BIOS_NUM_TV_OTHER	    16
#define	    VIA_BIOS_NUM_TV2		    2
#define	    VIA_BIOS_NUM_TV3		    6

#define	    VIA_RES_640X480		    0
#define	    VIA_RES_800X600		    1
#define	    VIA_RES_1024X768		    2
#define	    VIA_RES_1152X864		    3
#define	    VIA_RES_1280X1024		    4
#define	    VIA_RES_1600X1200		    5
#define	    VIA_RES_1440X1050		    6
#define	    VIA_RES_1280X768		    7
#define	    VIA_RES_1280X960		    8
#define	    VIA_RES_1920X1440		    9
#define	    VIA_RES_848X480		    10
#define	    VIA_RES_1400X1050		    11
#define	    VIA_RES_720X480		    12
#define	    VIA_RES_720X576		    13
#define	    VIA_RES_1024X512		    14
#define	    VIA_RES_856X480		    15
#define	    VIA_RES_INVALID		    255

#define	    VIA_TVRES_640X480		    0
#define	    VIA_TVRES_800X600		    1
#define	    VIA_TVRES_1024X768		    2
#define	    VIA_TVRES_848X480		    3
#define	    VIA_TVRES_720X480		    4
#define	    VIA_TVRES_720X576		    5


#define	    VIA_NUM_REFRESH_RATE	    7
#define	    VIA_PANEL6X4		    0
#define	    VIA_PANEL8X6		    1
#define	    VIA_PANEL10X7		    2
#define	    VIA_PANEL12X7		    3
#define	    VIA_PANEL12X10		    4
#define	    VIA_PANEL14X10		    5
#define	    VIA_PANEL16X12		    6
#define	    VIA_PANEL_INVALID		    255

#define	    VIA_NONEDVI			    0x00
#define	    VIA_SIL164			    0x11
#define	    VIA_VT3191			    0x21
#define	    VIA_VT3192			    0x22


#define	    TVTYPE_NONE			    0x00
#define	    TVTYPE_NTSC			    0x01
#define	    TVTYPE_PAL			    0x02

#define	    TVOUTPUT_NONE		    0x00
#define	    TVOUTPUT_COMPOSITE		    0x01
#define	    TVOUTPUT_SVIDEO		    0x02
#define	    TVOUTPUT_RGB		    0x04
#define	    TVOUTPUT_YCBCR		    0x08
#define	    TVOUTPUT_SC			    0x16

#define	    VIA_NONETV			    0
#define	    VIA_TV2PLUS			    1
#define	    VIA_TV3			    2
#define	    VIA_CH7009			    3
#define	    VIA_CH7017			    4
#define	    VIA_TVNORMAL		    0
#define	    VIA_TVOVER			    1
#define	    VIA_NO_TVHSCALE		    0
#define	    VIA_TVHSCALE0		    0
#define	    VIA_TVHSCALE1		    1
#define	    VIA_TVHSCALE2		    2
#define	    VIA_TVHSCALE3		    3
#define	    VIA_TVHSCALE4		    4
#define	    VIA_TV_NUM_HSCALE_LEVEL	    8
#define	    VIA_TV_NUM_HSCALE_REG	    16

#define		VIA_DEVICE_CRT1					0x01
#define		VIA_DEVICE_LCD					0x02
#define		VIA_DEVICE_TV					0x04
#define		VIA_DEVICE_DFP					0x08
#define		VIA_DEVICE_CRT2					0x10

#define	    VIA_UTI_SETMODE		    0
#define	    VIA_UTI_TVLUMA		    1
#define	    VIA_UTI_TVCHROMA		    2
#define	    VIA_UTI_TVFLICKER		    3
#define	    VIA_UTI_TVBRIGHTNESS	    4
#define	    VIA_UTI_TVSATURATION_CR	    5
#define	    VIA_UTI_TVSATURATION_CB	    6
#define	    VIA_UTI_TVHUE		    7
#define	    VIA_UTI_TVHPOSITION		    8
#define	    VIA_UTI_TVVPOSITION		    9
#define	    VIA_UTI_TVHSCALE		    10
#define	    VIA_UTI_TVVSCALE		    11
#define	    VIA_UTI_TVDEFAULT		    12

#define	    VIA_UTI_CRT_ON		    0x01
#define	    VIA_UTI_TV_ON		    0x02
#define	    VIA_UTI_NTSC		    0x04
#define	    VIA_UTI_PAL			    0x08
#define	    VIA_UTI_COMPOSITE		    0x10
#define	    VIA_UTI_SVIDEO		    0x20
#define	    VIA_UTI_RGB			    0x40
#define	    VIA_UTI_YCBCR		    0x80

#define	    VIA_UTI_LUMA_MASK		    0xE0
#define	    VIA_UTI_CHROMA_MASK		    0x1C
#define	    VIA_UTI_FLICKER_MASK	    0x03
#define	    VIA_UTI_HUE_HIGHBYTE_MASK	    0x07


typedef struct _VIABIOSSTDVGATABLE {
    CARD8	    columns;
    CARD8	    rows;
    CARD8	    fontHeight;
    CARD16	    pageSize;
    CARD8	    SR[5];
    CARD8	    misc;
    CARD8	    CR[25];
    CARD8	    AR[20];
    CARD8	    GR[9];
} VIABIOSStdVGATableRec, *VIABIOSStdVGATablePtr;


typedef struct _VIABIOSREFRESHTABLE {
    CARD8	    refresh;
    CARD16	    VClk;
    CARD8	    CR[14];
} VIABIOSRefreshTableRec, *VIABIOSRefreshTablePtr;


typedef struct _VIABIOSREGTABLE {
    CARD8   port[VIA_BIOS_REG_TABLE_MAX_NUM];
    CARD8   offset[VIA_BIOS_REG_TABLE_MAX_NUM];
    CARD8   mask[VIA_BIOS_REG_TABLE_MAX_NUM];
    CARD8   data[VIA_BIOS_REG_TABLE_MAX_NUM];
    int	    numEntry;
} VIABIOSRegTableRec, *VIABIOSRegTablePtr;


typedef struct _VIAVMODEENTRY {
    unsigned short	    Width;
    unsigned short	    Height;
    unsigned short	    Bpp;
    unsigned short	    Mode;
    unsigned short	    MemNeed;
    unsigned short	    MClk;
    unsigned short	    VClk;
    VIABIOSStdVGATableRec   stdVgaTable;
    VIABIOSRegTableRec	    extModeExtTable;
} VIAModeEntry, *VIAModeEntryPtr;


typedef struct _VIALCDMODEENTRY {
    CARD16  LCDClk;
    CARD16  VClk;
    CARD16  LCDClk_12Bit;
    CARD16  VClk_12Bit;
    CARD8   port[VIA_BIOS_REG_LCD_MAX_NUM];
    CARD8   offset[VIA_BIOS_REG_LCD_MAX_NUM];
    CARD8   data[VIA_BIOS_REG_LCD_MAX_NUM];
    int	    numEntry;
} VIALCDModeEntry, *VIALCDModeEntryPtr;


typedef struct _VIALCDMPATCHENTRY {
    CARD8   Mode;
    CARD16  LCDClk;
    CARD16  VClk;
    CARD16  LCDClk_12Bit;
    CARD16  VClk_12Bit;
    CARD8   port[VIA_BIOS_REG_LCD_MAX_NUM];
    CARD8   offset[VIA_BIOS_REG_LCD_MAX_NUM];
    CARD8   data[VIA_BIOS_REG_LCD_MAX_NUM];
    int	    numEntry;
} VIALCDMPatchEntry, *VIALCDMPatchEntryPtr;


typedef struct _VIALCDMODEFIX {
    CARD8   reqMode[32];
    CARD8   fixMode[32];
    int	    numEntry;
} VIALCDModeFixRec, *VIALCDModeFixRecPtr;


typedef struct _VIALCDPOWERSEQUENCE {
    CARD8   powerSeq;
    CARD8   port[4];
    CARD8   offset[4];
    CARD8   mask[4];
    CARD8   data[4];
    CARD16  delay[4];
    int	    numEntry;
} VIALCDPowerSeqRec, *VIALCDPowerSeqRecPtr;


typedef struct _VIALCDMODETABLE {
    CARD8		    fpIndex;
    CARD8		    fpSize;
    CARD8		    powerSeq;
    int			    numMPatchDP2Ctr;
    int			    numMPatchDP2Exp;
    int			    numMPatchDP1Ctr;
    int			    numMPatchDP1Exp;
    CARD16		    SuptMode[VIA_BIOS_NUM_LCD_SUPPORT_MASK];
    VIALCDModeEntry	    FPconfigTb;
    VIALCDModeEntry	    InitTb;
    VIALCDMPatchEntry	    MPatchDP2Ctr[VIA_BIOS_MAX_NUM_MPATCH2];
    VIALCDMPatchEntry	    MPatchDP2Exp[VIA_BIOS_MAX_NUM_MPATCH2];
    VIALCDMPatchEntry	    MPatchDP1Ctr[VIA_BIOS_MAX_NUM_MPATCH1];
    VIALCDMPatchEntry	    MPatchDP1Exp[VIA_BIOS_MAX_NUM_MPATCH1];
    VIALCDModeEntry	    LowResCtr;
    VIALCDModeEntry	    LowResExp;
    VIALCDModeEntry	    MCtr[VIA_BIOS_MAX_NUM_CTREXP];
    VIALCDModeEntry	    MExp[VIA_BIOS_MAX_NUM_CTREXP];
} VIALCDModeTableRec, *VIALCDModePtr;


typedef struct _VIATVMASKTABLE {
    CARD8   TV[VIA_BIOS_MAX_NUM_TV_REG];
    CARD8   CRTC1[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   CRTC2[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   misc1;
    CARD8   misc2;
    int	    numTV;
    int	    numCRTC1;
    int	    numCRTC2;
} VIABIOSTVMASKTableRec, *VIABIOSTVMASKTablePtr;


typedef struct _VIATVMODETABLE {
    CARD8   TVNTSCC[VIA_BIOS_MAX_NUM_TV_REG];
    CARD8   TVNTSCS[VIA_BIOS_MAX_NUM_TV_REG];
    CARD8   CRTCNTSC1[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   MiscNTSC1[VIA_BIOS_NUM_TV_SPECIAL_REG];
    CARD8   MiscNTSC2[VIA_BIOS_NUM_TV_SPECIAL_REG];
    CARD8   CRTCNTSC2_8BPP[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   CRTCNTSC2_16BPP[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   CRTCNTSC2_32BPP[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD16  PatchNTSC2[VIA_BIOS_MAX_NUM_TV_PATCH];
    CARD16  DotCrawlNTSC[VIA_BIOS_NUM_TV_OTHER];
    CARD8   TVPALC[VIA_BIOS_MAX_NUM_TV_REG];
    CARD8   TVPALS[VIA_BIOS_MAX_NUM_TV_REG];
    CARD8   CRTCPAL1[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   MiscPAL1[VIA_BIOS_NUM_TV_SPECIAL_REG];
    CARD8   MiscPAL2[VIA_BIOS_NUM_TV_SPECIAL_REG];
    CARD8   CRTCPAL2_8BPP[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   CRTCPAL2_16BPP[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   CRTCPAL2_32BPP[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD16  PatchPAL2[VIA_BIOS_MAX_NUM_TV_PATCH];
} VIABIOSTV2TableRec, *VIABIOSTV2TablePtr;


typedef struct _VIATV3MODETABLE {
    CARD8   TVNTSC[VIA_BIOS_MAX_NUM_TV_REG];
    CARD8   CRTCNTSC1[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   MiscNTSC1[VIA_BIOS_NUM_TV_SPECIAL_REG];
    CARD8   MiscNTSC2[VIA_BIOS_NUM_TV_SPECIAL_REG];
    CARD8   CRTCNTSC2_8BPP[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   CRTCNTSC2_16BPP[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   CRTCNTSC2_32BPP[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD16  PatchNTSC2[VIA_BIOS_MAX_NUM_TV_PATCH];
    CARD16  RGBNTSC[VIA_BIOS_NUM_TV_OTHER];
    CARD16  YCbCrNTSC[VIA_BIOS_NUM_TV_OTHER];
    CARD16  SDTV_RGBNTSC[VIA_BIOS_NUM_TV_OTHER];
    CARD16  SDTV_YCbCrNTSC[VIA_BIOS_NUM_TV_OTHER];
    CARD16  DotCrawlNTSC[VIA_BIOS_NUM_TV_OTHER];
    CARD8   TVPAL[VIA_BIOS_MAX_NUM_TV_REG];
    CARD8   CRTCPAL1[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   MiscPAL1[VIA_BIOS_NUM_TV_SPECIAL_REG];
    CARD8   MiscPAL2[VIA_BIOS_NUM_TV_SPECIAL_REG];
    CARD8   CRTCPAL2_8BPP[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   CRTCPAL2_16BPP[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD8   CRTCPAL2_32BPP[VIA_BIOS_MAX_NUM_TV_CRTC];
    CARD16  PatchPAL2[VIA_BIOS_MAX_NUM_TV_PATCH];
    CARD16  RGBPAL[VIA_BIOS_NUM_TV_OTHER];
    CARD16  YCbCrPAL[VIA_BIOS_NUM_TV_OTHER];
    CARD16  SDTV_RGBPAL[VIA_BIOS_NUM_TV_OTHER];
    CARD16  SDTV_YCbCrPAL[VIA_BIOS_NUM_TV_OTHER];
} VIABIOSTV3TableRec, *VIABIOSTV3TablePtr;

typedef struct _VIAVMODETABLE {
    unsigned short	    BIOSVer;
    char *		    BIOSDate;
    unsigned short	    NumModes;
    int			    NumPowerOn;
    int			    NumPowerOff;
    VIAModeEntryPtr	    Modes;
    VIABIOSRegTableRec	    commExtTable;
    VIABIOSRegTableRec	    stdModeExtTable;
    VIABIOSRefreshTableRec  refreshTable[VIA_BIOS_NUM_RES][VIA_BIOS_NUM_REFRESH];
    VIALCDModeTableRec	    lcdTable[VIA_BIOS_NUM_PANEL];
    VIALCDPowerSeqRec	    powerOn[VIA_BIOS_NUM_LCD_POWER_SEQ];
    VIALCDPowerSeqRec	    powerOff[VIA_BIOS_NUM_LCD_POWER_SEQ];
    VIALCDModeFixRec	    modeFix;
    VIABIOSTVMASKTableRec   tv2MaskTable;
    VIABIOSTVMASKTableRec   tv3MaskTable;
    VIABIOSTV2TableRec	    tv2Table[VIA_BIOS_NUM_TV2];
    VIABIOSTV2TableRec	    tv2OverTable[VIA_BIOS_NUM_TV2];
    VIABIOSTV3TableRec	    tv3Table[VIA_BIOS_NUM_TV3];
    VIABIOSTV3TableRec	    tv3OverTable[VIA_BIOS_NUM_TV3];
} VIAModeTableRec, *VIAModeTablePtr;

typedef struct _VIABIOSINFO {

    VIAModeTablePtr	pModeTable;

    int			Chipset;
    int			DVIEncoder;
    int			TVEncoder;
    int			BIOSTVTabVer;

    unsigned char*	MapBase;
    Bool		FirstInit;
    unsigned char*	FBBase;
    unsigned long	videoRambytes;
    int			scrnIndex;

    unsigned int	mode, refresh, resMode;
    int			countWidthByQWord;
    int			offsetWidthByQWord;

    unsigned char		ConnectedDevice;
    unsigned char		ActiveDevice;

    /* Here are all the BIOS Relative Options */
#ifdef CallBIOS
    int			BIOSMajorVersion;
    int			BIOSMinorVersion;
    unsigned char		BIOSDateYear;
    unsigned char		BIOSDateMonth;
    unsigned char		BIOSDateDay;
#endif
    Bool		A2;
    Bool		UseBIOS;
	Bool				LCDDualEdge;	/* mean add-on card is 2CH or Dual or DDR */
    Bool		DVIAttach;
    Bool		Center;
    Bool		TVAttach;
    Bool		TVDotCrawl;
    int			PanelSize;
    int			TVType;
    int			TVOutput;
    int			TVVScan;
    int			TVHScale;
    int					OptRefresh;
    int			Refresh;
    int			FoundRefresh;

    /* LCD Simultaneous Expand Mode HWCursor Y Scale */
    Bool		scaleY;
    int			panelX;
    int			panelY;
    int			resY;

    /* Some Used Member of ScrnInfoRec */
    int			bitsPerPixel;	/* fb bpp */
    int			displayWidth;	/* memory pitch */
    int			frameX1;	/* viewport position */
    int			frameY1;
    int			SaveframeX1;
    int			SaveframeY1;

    /* Some Used Member of DisplayModeRec */
    int			Clock;		/* pixel clock freq */
    int			HTotal;
    int			VTotal;
    int			HDisplay;	/* horizontal timing */
    int			VDisplay;	/* vertical timing */
    int			SaveHDisplay;
    int			SaveVDisplay;
    int			CrtcHDisplay;
    int			CrtcVDisplay;
    int			SaveCrtcHDisplay;
    int			SaveCrtcVDisplay;

    /* I2C & DDC */
    I2CBusPtr		I2C_Port1;
    I2CBusPtr		I2C_Port2;
    xf86MonPtr		DDC1;
    xf86MonPtr		DDC2;
    I2CDevPtr		dev;

    unsigned char	TVRegs[9*16];

    /* MHS */
    Bool				SAMM;			/* SAMM success or not? */
    Bool		IsSecondary;
    Bool		HasSecondary;
    Bool		SetTV;
    Bool		SetDVI;

} VIABIOSInfoRec, *VIABIOSInfoPtr;

/* Functions protype */

int VIACheckTVExist(VIABIOSInfoPtr pBIOSInfo);
long VIAQueryChipInfo(VIABIOSInfoPtr pBIOSInfo);
char* VIAGetBIOSInfo(VIABIOSInfoPtr pBIOSInfo);
Bool VIASetActiveDisplay(VIABIOSInfoPtr pBIOSInfo, unsigned char display);
unsigned char VIAGetActiveDisplay(VIABIOSInfoPtr pBIOSInfo);
unsigned char VIASensorTV2(VIABIOSInfoPtr pBIOSInfo);
unsigned char VIASensorTV3(VIABIOSInfoPtr pBIOSInfo);
Bool VIASensorDVI(VIABIOSInfoPtr pBIOSInfo);
Bool VIAPostDVI(VIABIOSInfoPtr pBIOSInfo);
unsigned char VIAGetDeviceDetect(VIABIOSInfoPtr pBIOSInfo);
unsigned char VIAGetPanelInfo(VIABIOSInfoPtr pBIOSInfo);
Bool VIASetPanelState(VIABIOSInfoPtr pBIOSInfo, unsigned char state);
unsigned char VIAGetPanelState(VIABIOSInfoPtr pBIOSInfo);
int VIAQueryDVIEDID(VIABIOSInfoPtr pBIOSInfo);
unsigned char VIAGetPanelSizeFromDDCv1(VIABIOSInfoPtr pBIOSInfo);
unsigned char VIAGetPanelSizeFromDDCv2(VIABIOSInfoPtr pBIOSInfo);
int VIAGetTVTabVer(VIABIOSInfoPtr pBIOSInfo, unsigned char *pBIOS);
Bool VIAGetBIOSTable(VIABIOSInfoPtr pBIOSInfo);
Bool VIAFindModeUseBIOSTable(VIABIOSInfoPtr pBIOSInfo);
Bool VIASetModeUseBIOSTable(VIABIOSInfoPtr pBIOSInfo);
Bool VIASetModeForMHS(VIABIOSInfoPtr pBIOSInfo);
Bool VIASavePalette(ScrnInfoPtr pScrn, LOCO *colors);
Bool VIARestorePalette(ScrnInfoPtr pScrn, LOCO *colors);
void VIAPreSetTV3Mode(VIABIOSInfoPtr pBIOSInfo);
void VIAPostSetTV3Mode(VIABIOSInfoPtr pBIOSInfo);
void VIAPreSetTV2Mode(VIABIOSInfoPtr pBIOSInfo);
void VIAPostSetTV2Mode(VIABIOSInfoPtr pBIOSInfo);
int VIAFindSupportRefreshRate(VIABIOSInfoPtr pBIOSInfo,
			      int resIndex);
void VIASetLCDMode(VIABIOSInfoPtr pBIOSInfo);
void VIADisableExtendedFIFO(VIABIOSInfoPtr pBIOSInfo);
void VIAEnableExtendedFIFO(VIABIOSInfoPtr pBIOSInfo);
#ifdef CallBIOS
Bool BIOS_SetActiveDevice(ScrnInfoPtr pScrn, unsigned short cx);
unsigned int BIOS_GetBIOSVersion(ScrnInfoPtr pScrn);
unsigned char BIOS_GetActiveDevice(ScrnInfoPtr pScrn);
unsigned char BIOS_GetDisplayDeviceAttached(ScrnInfoPtr pScrn);
unsigned int BIOS_GetTVConfiguration(ScrnInfoPtr pScrn);
unsigned char BIOS_GetTVEncoderType(ScrnInfoPtr pScrn);
unsigned int BIOS_GetDisplayDeviceInfo(ScrnInfoPtr pScrn, unsigned char *numDevice);
unsigned char BIOS_GetFlatPanelInfo(ScrnInfoPtr pScrn);
Bool BIOS_GetBIOSDate(ScrnInfoPtr pScrn);
#endif /* CallBIOS */
#endif /* _VIA_BIOS_H */
