/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/glint_regs.h,v 1.6 1997/09/29 08:40:29 hohndel Exp $ */

/*
 * glint register file 
 *
 * Copyright by Stefan Dirsch, Dirk Hohndel, Alan Hourihane
 * Authors: Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *          Dirk Hohndel, <hohndel@suse.de>
 *          Stefan Dirsch, <sndirsch@suse.de>
 *          Simon P., <sim@suse.de>
 *
 * this work is sponsored by S.u.S.E. GmbH, Fuerth, Elsa GmbH, Aachen and
 * Siemens Nixdorf Informationssysteme
 *
 */ 

#ifndef _GLINTREG_H_
#define _GLINTREG_H_



/**********************************************
*  GLINT 500TX Configuration Region Registers *
***********************************************/

/* Device Identification */
#define CFGVendorId								0x0000
#define PCI_VENDOR_3DLABS						0x3D3D
#define PCI_VENDOR_TI							0x104C
#define CFGDeviceId								0x0002

#define PCI_CHIP_3DLABS_300SX					0x01
#define PCI_CHIP_3DLABS_500TX					0x02
#define PCI_CHIP_3DLABS_DELTA					0x03
#define PCI_CHIP_3DLABS_PERMEDIA				0x04
#define PCI_CHIP_3DLABS_MX						0x06
#define PCI_CHIP_3DLABS_PERMEDIA2				0x07
#define PCI_CHIP_TI_3DLABS_PERMEDIA2			0x3d07

#define CFGRevisionId							0x08
#define CFGClassCode							0x09
#define CFGHeaderType							0x0E

#define IS_3DLABS_TX_MX_CLASS(c) 						\
	(((c) == PCI_CHIP_3DLABS_500TX) || ((c) == PCI_CHIP_3DLABS_MX))

#define IS_3DLABS_PERMEDIA_CLASS(c)  ( (c) == PCI_CHIP_3DLABS_PERMEDIA )
#define IS_3DLABS_PM2_CLASS(c)       ( (c) == PCI_CHIP_3DLABS_PERMEDIA2 )

#define IS_3DLABS_PM_FAMILY(c)							\
	(IS_3DLABS_PERMEDIA_CLASS(c) || IS_3DLABS_PM2_CLASS(c))

/* Device Control/Status */
#define CFGCommand								0x04
#define CFGStatus								0x06

/* Miscellaneous Functions */
#define CFGBist									0x0f
#define CFGLatTimer								0x0d
#define CFGCacheLine							0x0c
#define CFGMaxLat								0x3f
#define CFGMinGrant								0x3e
#define CFGIntPin								0x3d
#define CFGIntLine								0x3c

/* Base Adresses */
#define CFGBaseAddr0							0x10 
#define CFGBaseAddr1							0x14
#define CFGBaseAddr2							0x18
#define CFGBaseAddr3							0x1C
#define CFGBaseAddr4							0x20
#define CFGRomAddr								0x30



/**********************************
 * GLINT 500TX Region 0 Registers *
 **********************************/

/* Control Status Registers */
#define ResetStatus								0x0000
#define IntEnable								0x0008
#define IntFlags								0x0010
#define InFIFOSpace								0x0018
#define OutFIFOWords							0x0020
#define DMAAddress								0x0028
#define DMACount								0x0030
#define ErrorFlags								0x0038
#define VClkCtl									0x0040
#define TestRegister							0x0048
#define Aperture0								0x0050
#define Aperture1								0x0058
#define DMAControl								0x0060
#define FIFODis									0x0068

/* GLINT PerMedia Region 0 additional Registers */
#define ChipConfig								0x0070

/* GLINT 500TX LocalBuffer Registers */
#define LBMemoryCtl								0x1000
#define LBMemoryEDO								0x1008

/* GLINT PerMedia Memory Control Registers */
#define PMReboot								0x1000
#define PMRomControl							0x1040
#define PMBootAddress							0x1080
#define PMMemConfig								0x10C0
#define PMBypassWriteMask						0x1100
#define PMFramebufferWriteMask					0x1140
#define PMCount									0x1180

/* Framebuffer Registers */
#define FBMemoryCtl								0x1800
#define FBModeSel								0x1808
#define FBGCWrMask								0x1810
#define FBGCColorLower							0x1818
#define FBTXMemCtl								0x1820
#define FBWrMaskk								0x1830
#define FBGCColorUpper							0x1838

/* Core FIFO */
#define OutputFIFO								0x2000

/* 500TX Internal Video Registers */
#define VTGHLimit								0x3000
#define VTGHSyncStart							0x3008
#define VTGHSyncEnd								0x3010
#define VTGHBlankEnd							0x3018
#define VTGVLimit								0x3020
#define VTGVSyncStart							0x3028
#define VTGVSyncEnd								0x3030
#define VTGVBlankEnd							0x3038
#define VTGHGateStart							0x3040
#define VTGHGateEnd								0x3048
#define VTGVGateStart							0x3050
#define VTGVGateEnd								0x3058
#define VTGPolarity								0x3060
#define VTGFrameRowAddr							0x3068
#define VTGVLineNumber							0x3070
#define VTGSerialClk							0x3078
#define VTGModeCtl								0x3080

/* Permedia Video Control Registers */
#define PMScreenBase							0x3000
#define PMScreenStride							0x3008
#define PMHTotal								0x3010
#define PMHgEnd									0x3018
#define PMHbEnd									0x3020
#define PMHsStart								0x3028
#define PMHsEnd									0x3030
#define PMVTotal								0x3038
#define PMVbEnd									0x3040
#define PMVsStart								0x3048
#define PMVsEnd									0x3050
#define PMVideoControl							0x3058
#define PMInterruptLine							0x3060
#define PMDDCData								0x3068
#define PMLineCount								0x3070

/* Permedia 2 RAMDAC Registers */
#define PM2DACWriteAddress						0x4000
#define PM2DACIndexReg							0x4000
#define PM2DACData								0x4008
#define PM2DACReadMask							0x4010
#define PM2DACReadAddress						0x4018
#define PM2DACCursorColorAddress				0x4020
#define PM2DACCursorColorData					0x4028
#define PM2DACIndexData							0x4050
#define PM2DACCursorData						0x4058
#define PM2DACCursorXLsb						0x4060
#define PM2DACCursorXMsb						0x4068
#define PM2DACCursorYLsb						0x4070
#define PM2DACCursorMLsb						0x4078
#define PM2DACIndexCMR							0x18
#define   PM2DAC_RGB					0x20
#define   PM2DAC_GRAPHICS				0x10
#define   PM2DAC_PACKED					0x09
#define   PM2DAC_8888					0x08
#define   PM2DAC_565					0x06
#define   PM2DAC_4444					0x05
#define   PM2DAC_5551					0x04
#define   PM2DAC_2321					0x03
#define   PM2DAC_2320					0x02
#define   PM2DAC_332					0x01
#define   PM2DAC_CI8					0x00
#define PM2DACIndexMDCR							0x19
#define PM2DACIndexPalettePage					0x1c
#define PM2DACIndexMCR							0x1e
#define PM2DACIndexClockYM						0x20
#define PM2DACIndexClockAN						0x21
#define PM2DACIndexClockAP						0x22
#define PM2DACIndexClockBM						0x23
#define PM2DACIndexClockBN						0x24
#define PM2DACIndexClockBP						0x25
#define PM2DACIndexClockCM						0x26
#define PM2DACIndexClockCN						0x27
#define PM2DACIndexClockCP						0x28
#define PM2DACIndexClockStatus					0x29
#define PM2DACIndexMemClockM					0x30
#define PM2DACIndexMemClockN					0x31
#define PM2DACIndexMemClockP					0x32
#define PM2DACIndexMemClockStatus				0x33
#define PM2DACIndexColorKeyControl				0x40
#define PM2DACIndexColorKeyOverlay				0x41
#define PM2DACIndexColorKeyRed					0x42
#define PM2DACIndexColorKeyGreen				0x43
#define PM2DACIndexColorKeyBlue					0x44

/**********************************
 * GLINT Delta Region 0 Registers *
 **********************************/

/* Control Status Registers */
#define DResetStatus							0x0800
#define DIntEnable								0x0808
#define DIntFlags								0x0810
#define DErrorFlags								0x0838
#define DTestRegister							0x0848
#define DFIFODis								0x0868



/************************
 * GLINT Core Registers *
 ************************/

#define GLINT_TAG(major,offset)			(((major) << 7) | ((offset) << 3)) 
#define GLINT_TAG_ADDR(major,offset)	(0x8000 | GLINT_TAG((major),(offset)))

#define UNIT_DISABLE							0
#define UNIT_ENABLE								1

#define StartXDom							GLINT_TAG_ADDR(0x00,0x00)
#define dXDom								GLINT_TAG_ADDR(0x00,0x01)
#define StartXSub							GLINT_TAG_ADDR(0x00,0x02)
#define dXSub								GLINT_TAG_ADDR(0x00,0x03)
#define StartY								GLINT_TAG_ADDR(0x00,0x04)
#define dY									GLINT_TAG_ADDR(0x00,0x05)
#define GLINTCount							GLINT_TAG_ADDR(0x00,0x06)
#define Render								GLINT_TAG_ADDR(0x00,0x07)
	#define AreaStippleEnable					0x00001
	#define LineStippleEnable					0x00002
	#define ResetLineStipple					0x00004
	#define FastFillEnable						0x00008
	#define PrimitiveLine						0
	#define PrimitiveTrapezoid					0x00040
	#define PrimitivePoint						0x00080
	#define AntialiasEnable         			0x00100
	#define AntialiasingQuality     			0x00200
	#define UsePointTable						0x00400
	#define SyncOnBitMask						0x00800
	#define SyncOnHostData						0x01000
	#define TextureEnable           			0x02000
	#define FogEnable               			0x04000
	#define CoverageEnable						0x08000
	#define SubPixelCorrectionEnable			0x10000
	#define SpanOperation						0x40000


#define ContinueNewLine						GLINT_TAG_ADDR(0x00,0x08)
#define ContinueNewDom						GLINT_TAG_ADDR(0x00,0x09)
#define ContinueNewSub						GLINT_TAG_ADDR(0x00,0x0a)
#define Continue							GLINT_TAG_ADDR(0x00,0x0b)
#define FlushSpan							GLINT_TAG_ADDR(0x00,0x0c)
#define BitMaskPattern						GLINT_TAG_ADDR(0x00,0x0d)

#define PointTable0							GLINT_TAG_ADDR(0x01,0x00)
#define PointTable1							GLINT_TAG_ADDR(0x01,0x01)
#define PointTable2							GLINT_TAG_ADDR(0x01,0x02)
#define PointTable3							GLINT_TAG_ADDR(0x01,0x03)
#define RasterizerMode						GLINT_TAG_ADDR(0x01,0x04)
#define YLimits								GLINT_TAG_ADDR(0x01,0x05)
#define ScanLineOwnership					GLINT_TAG_ADDR(0x01,0x06)
#define WaitForCompletion					GLINT_TAG_ADDR(0x01,0x07)
#define PixelSize							GLINT_TAG_ADDR(0x01,0x08)
#define XLimits								GLINT_TAG_ADDR(0x01,0x09) /* PM only */

#define PackedDataLimits					GLINT_TAG_ADDR(0x02,0x0a) /* PM only */

#define ScissorMode							GLINT_TAG_ADDR(0x03,0x00)
#define ScissorMinXY						GLINT_TAG_ADDR(0x03,0x01)
#define ScissorMaxXY						GLINT_TAG_ADDR(0x03,0x02)
#define ScreenSize							GLINT_TAG_ADDR(0x03,0x03)
#define AreaStippleMode						GLINT_TAG_ADDR(0x03,0x04)
	/* 0:					*/
	/* NoMirrorY			*/
	/* NoMirrorX			*/
	/* NoInvertPattern		*/
	/* YAddress_1bit		*/
	/* XAddress_1bit		*/
	/* UNIT_DISABLE			*/

	#define ASM_XAddress_2bit					1 << 1
	#define ASM_XAddress_3bit					2 << 1
	#define ASM_XAddress_4bit					3 << 1
	#define ASM_XAddress_5bit					4 << 1
	#define ASM_YAddress_2bit					1 << 4
	#define ASM_YAddress_3bit					2 << 4
	#define ASM_YAddress_4bit					3 << 4
	#define ASM_YAddress_5bit					4 << 4
	#define ASM_InvertPattern					1 << 17
	#define ASM_MirrorX							1 << 18
	#define ASM_MirrorY							1 << 19

#define LineStippleMode						GLINT_TAG_ADDR(0x03,0x05)
#define LoadLineStippleCounters				GLINT_TAG_ADDR(0x03,0x06)
#define UpdateLineStippleCounters			GLINT_TAG_ADDR(0x03,0x07)
#define SaveLineStippleState				GLINT_TAG_ADDR(0x03,0x08)
#define WindowOrigin						GLINT_TAG_ADDR(0x03,0x09)

#define AreaStipplePattern0					GLINT_TAG_ADDR(0x04,0x00)
#define AreaStipplePattern1					GLINT_TAG_ADDR(0x04,0x01)
#define AreaStipplePattern2					GLINT_TAG_ADDR(0x04,0x02)
#define AreaStipplePattern3					GLINT_TAG_ADDR(0x04,0x03)
#define AreaStipplePattern4					GLINT_TAG_ADDR(0x04,0x04)
#define AreaStipplePattern5					GLINT_TAG_ADDR(0x04,0x05)
#define AreaStipplePattern6					GLINT_TAG_ADDR(0x04,0x06)
#define AreaStipplePattern7					GLINT_TAG_ADDR(0x04,0x07)

#define TextureAddressMode					GLINT_TAG_ADDR(0x07,0x00)
#define SStart								GLINT_TAG_ADDR(0x07,0x01)
#define dSdx								GLINT_TAG_ADDR(0x07,0x02)
#define dSdyDom								GLINT_TAG_ADDR(0x07,0x03)
#define TStart								GLINT_TAG_ADDR(0x07,0x04)
#define dTdx								GLINT_TAG_ADDR(0x07,0x05)
#define dTdyDom								GLINT_TAG_ADDR(0x07,0x06)
#define QStart								GLINT_TAG_ADDR(0x07,0x07)
#define dQdx								GLINT_TAG_ADDR(0x07,0x08)
#define dQdyDom								GLINT_TAG_ADDR(0x07,0x09)

#define TextureReadMode						GLINT_TAG_ADDR(0x09,0x00)
#define TextureFormat						GLINT_TAG_ADDR(0x09,0x01)
#define TextureCacheControl					GLINT_TAG_ADDR(0x09,0x02)
#define GLINTBorderColor					GLINT_TAG_ADDR(0x09,0x05)

#define TxBaseAddr							GLINT_TAG_ADDR(0x0a,0x00)
#define TxBaseAddrLR						GLINT_TAG_ADDR(0x0a,0x01)
#define TexelLUT							GLINT_TAG_ADDR(0x0a,0x00)

#define Texel0								GLINT_TAG_ADDR(0x0c,0x00)
#define Texel1								GLINT_TAG_ADDR(0x0c,0x01)
#define Texel2								GLINT_TAG_ADDR(0x0c,0x02)
#define Texel3								GLINT_TAG_ADDR(0x0c,0x03)
#define Texel4								GLINT_TAG_ADDR(0x0c,0x04)
#define Texel5								GLINT_TAG_ADDR(0x0c,0x05)
#define Texel6								GLINT_TAG_ADDR(0x0c,0x06)
#define Texel7								GLINT_TAG_ADDR(0x0c,0x07)
#define Interp0								GLINT_TAG_ADDR(0x0c,0x08)
#define Interp1								GLINT_TAG_ADDR(0x0c,0x09)
#define Interp2								GLINT_TAG_ADDR(0x0c,0x0a)
#define Interp3								GLINT_TAG_ADDR(0x0c,0x0b)
#define Interp4								GLINT_TAG_ADDR(0x0c,0x0c)
#define TextureFilter						GLINT_TAG_ADDR(0x0c,0x0d)

#define TextureColorMode					GLINT_TAG_ADDR(0x0d,0x00)
#define TextureEnvColor						GLINT_TAG_ADDR(0x0d,0x01)
#define FogMode								GLINT_TAG_ADDR(0x0d,0x02)
	/* 0:					*/
	/* FOG RGBA				*/
	/* UNIT_DISABLE			*/

	#define FOG_CI								0x0002

#define FogColor							GLINT_TAG_ADDR(0x0d,0x03)
#define FStart								GLINT_TAG_ADDR(0x0d,0x04)
#define dFdx								GLINT_TAG_ADDR(0x0d,0x05)
#define dFdyDom								GLINT_TAG_ADDR(0x0d,0x06)
#define KsStart								GLINT_TAG_ADDR(0x0d,0x09)
#define dKsdx								GLINT_TAG_ADDR(0x0d,0x0a)
#define dKsdyDom							GLINT_TAG_ADDR(0x0d,0x0b)
#define KdStart								GLINT_TAG_ADDR(0x0d,0x0c)
#define dKdStart							GLINT_TAG_ADDR(0x0d,0x0d)
#define dKddyDom							GLINT_TAG_ADDR(0x0d,0x0e)

#define RStart								GLINT_TAG_ADDR(0x0f,0x00)
#define dRdx								GLINT_TAG_ADDR(0x0f,0x01)
#define dRdyDom								GLINT_TAG_ADDR(0x0f,0x02)
#define GStart								GLINT_TAG_ADDR(0x0f,0x03)
#define dGdx								GLINT_TAG_ADDR(0x0f,0x04)
#define dGdyDom								GLINT_TAG_ADDR(0x0f,0x05)
#define BStart								GLINT_TAG_ADDR(0x0f,0x06)
#define dBdx								GLINT_TAG_ADDR(0x0f,0x07)
#define dBdyDom								GLINT_TAG_ADDR(0x0f,0x08)
#define AStart								GLINT_TAG_ADDR(0x0f,0x09)
#define dAdx								GLINT_TAG_ADDR(0x0f,0x0a)
#define dAdyDom								GLINT_TAG_ADDR(0x0f,0x0b)
#define ColorDDAMode						GLINT_TAG_ADDR(0x0f,0x0c)
	/* 0:					*/
	#define FlatShading			                0
	/* UNIT_DISABLE			*/
	#define CDDA_GouraudShading					0x0002
        

#define ConstantColor						GLINT_TAG_ADDR(0x0f,0x0d)
#define GLINTColor						GLINT_TAG_ADDR(0x0f,0x0e)
#define AlphaTestMode						GLINT_TAG_ADDR(0x10,0x00)
#define AntialiasMode						GLINT_TAG_ADDR(0x10,0x01)
#define AlphaBlendMode						GLINT_TAG_ADDR(0x10,0x02)
	/* 0:						*/
	/* SrcZERO					*/
	/* DstZERO					*/
	/* ColorFormat8888			*/
	/* AlphaBuffer present		*/
	/* ColorOrderBGR			*/
	/* TypeOpenGL				*/
	/* DstFBData				*/
	/* UNIT_DISABLE				*/

	#define ABM_SrcONE							1 << 1
	#define ABM_SrcDST_COLOR					2 << 1
	#define ABM_SrcONE_MINUS_DST_COLOR			3 << 1
	#define ABM_SrcSRC_ALPHA					4 << 1
	#define ABM_SrcONE_MINUS_SRC_ALPHA			5 << 1
	#define ABM_SrcDST_ALPHA					6 << 1
	#define ABM_SrcONE_MINUS_DST_ALPHA			7 << 1
	#define ABM_SrcSRC_ALPHA_SATURATE			8 << 1
	#define ABM_DstONE							1 << 5
	#define ABM_DstSRC_COLOR					2 << 5
	#define ABM_DstONE_MINUS_SRC_COLOR			3 << 5
	#define ABM_DstSRC_ALPHA					4 << 5
	#define ABM_DstONE_MINUS_SRC_ALPHA			5 << 5
	#define ABM_DstDST_ALPHA					6 << 5
	#define ABM_DstONE_MINUS_DST_ALPHA			7 << 5
	#define ABM_ColorFormat5555					1 << 8
	#define ABM_ColorFormat4444					2 << 8
	#define ABM_ColorFormat4444_Front			3 << 8
	#define ABM_ColorFormat4444_Back			4 << 8
	#define ABM_ColorFormat332_Front			5 << 8
	#define ABM_ColorFormat332_Back				6 << 8
	#define ABM_ColorFormat121_Front			7 << 8
	#define ABM_ColorFormat121_Back				8 << 8
	#define ABM_ColorFormat555_Back				13 << 8
	#define ABM_ColorFormat_CI8					14 << 8
	#define ABM_ColorFormat_CI4					15 << 8
	#define ABM_NoAlphaBuffer					0x1000
	#define ABM_ColorOrderRGB					0x2000
	#define ABM_TypeQuickDraw3D					0x4000
	#define ABM_DstFBSourceData					0x8000

#define DitherMode							GLINT_TAG_ADDR(0x10,0x03)
	/* 0:					*/
	/* ColorOrder BGR		*/
	/* AlphaDitherDefault	*/
	/* ColorFormat8888		*/
	/* TruncateMode 		*/
	/* DitherDisable		*/
	/* UNIT_DISABLE			*/

	#define DTM_DitherEnable					1 << 1
	#define DTM_ColorFormat5555					1 << 2
	#define DTM_ColorFormat4444					2 << 2
	#define DTM_ColorFormat4444_Front			3 << 2
	#define DTM_ColorFormat4444_Back			4 << 2
	#define DTM_ColorFormat332_Front			5 << 2
	#define DTM_ColorFormat332_Back				6 << 2
	#define DTM_ColorFormat121_Front			7 << 2
	#define DTM_ColorFormat121_Back				8 << 2
	#define DTM_ColorFormat555_Back				13 << 2
	#define DTM_ColorFormat_CI8					14 << 2
	#define DTM_ColorFormat_CI4					15 << 2
	#define DTM_ColorOrderRGB					1 << 10
	#define DTM_NoAlphaDither					1 << 14
	#define DTM_RoundMode						1 << 15

#define FBSoftwareWriteMask					GLINT_TAG_ADDR(0x10,0x04)
#define LogicalOpMode						GLINT_TAG_ADDR(0x10,0x05)
        #define Use_ConstantFBWriteData 0x40


#define FBWriteData							GLINT_TAG_ADDR(0x10,0x06)
#define RouterMode							GLINT_TAG_ADDR(0x10,0x08)

#define LBReadMode							GLINT_TAG_ADDR(0x11,0x00)
	/* 0:					*/
	/* SrcNoRead			*/
	/* DstNoRead			*/
	/* DataLBDefault		*/
	/* WinTopLeft			*/
	/* NoPatch				*/
	/* ScanlineInterval1 */

	#define LBRM_SrcEnable						1 << 9
	#define LBRM_DstEnable						1 << 10
	#define LBRM_DataLBStencil					1 << 16
	#define LBRM_DataLBDepth					2 << 16
	#define LBRM_WinBottomLeft					1 << 18
	#define LBRM_DoPatch						1 << 19
	#define LBRM_ScanlineInt2					1 << 20
	#define LBRM_ScanlineInt4					2 << 20
	#define LBRM_ScanlineInt8					3 << 20


#define LBReadFormat						GLINT_TAG_ADDR(0x11,0x01)
#define LBSourceOffset						GLINT_TAG_ADDR(0x11,0x02)
#define LBStencil							GLINT_TAG_ADDR(0x11,0x05)
#define LBDepth								GLINT_TAG_ADDR(0x11,0x06)
#define LBWindowBase						GLINT_TAG_ADDR(0x11,0x07)
#define LBWriteMode							GLINT_TAG_ADDR(0x11,0x08)
	/* 0:					*/
	/* WriteDisable			*/
	/* UpLoad_None			*/

	#define LBWM_WriteEnable					0x1
	#define LBWM_UpLoad_LBDepth					0x2
	#define LBWM_UpLoad_LBStencil				0x4

#define LBWriteFormat						GLINT_TAG_ADDR(0x11,0x09)
#define TextureData							GLINT_TAG_ADDR(0x11,0x0d)
#define TextureDownloadOffset				GLINT_TAG_ADDR(0x11,0x0e)

#define GLINTWindow							GLINT_TAG_ADDR(0x13,0x00)
#define StencilMode							GLINT_TAG_ADDR(0x13,0x01)
#define StencilData							GLINT_TAG_ADDR(0x13,0x02)
#define GLINTStencil						GLINT_TAG_ADDR(0x13,0x03)
#define DepthMode							GLINT_TAG_ADDR(0x13,0x04)
	/* 0:					*/
	/* WriteDisable			*/
	/* SrcCompFragment		*/
	/* CompFuncNEVER		*/
	/* UNIT_DISABLE			*/

	#define DPM_WriteEnable						1 << 1
	#define DPM_SrcCompLBData					1 << 2
	#define DPM_SrcCompDregister				2 << 2
	#define DPM_SrcCompLBSourceData				3 << 2
	#define DPM_CompFuncLESS					1 << 4
	#define DPM_CompFuncEQUAL					2 << 4
	#define DPM_CompFuncLESS_OR_EQ				3 << 4
	#define DPM_CompFuncGREATER					4 << 4
	#define DPM_CompFuncNOT_EQ					5 << 4
	#define DPM_CompFuncGREATER_OR_EQ			6 << 4
	#define DPM_CompFuncALWAYS					7 << 4

#define GLINTDepth							GLINT_TAG_ADDR(0x13,0x05)
#define ZStartU								GLINT_TAG_ADDR(0x13,0x06)
#define ZStartL								GLINT_TAG_ADDR(0x13,0x07)
#define dZdxU								GLINT_TAG_ADDR(0x13,0x08)
#define dZdxL								GLINT_TAG_ADDR(0x13,0x09)
#define dZdyDomU							GLINT_TAG_ADDR(0x13,0x0a)
#define dZdyDomL							GLINT_TAG_ADDR(0x13,0x0b)
#define FastClearDepth						GLINT_TAG_ADDR(0x13,0x0c)

#define FBReadMode							GLINT_TAG_ADDR(0x15,0x00)
	/* 0:					*/
	/* SrcNoRead			*/
	/* DstNoRead			*/
	/* DataFBDefault		*/
	/* WinTopLeft			*/
	/* ScanlineInterval1 	*/

	#define FBRM_SrcEnable						1 << 9
	#define FBRM_DstEnable						1 << 10
	#define FBRM_DataFBColor					1 << 15
	#define FBRM_WinBottomLeft					1 << 16
	#define FBRM_ScanlineInt2					1 << 23
	#define FBRM_ScanlineInt4					2 << 23
	#define FBRM_ScanlineInt8					3 << 23


#define FBSourceOffset						GLINT_TAG_ADDR(0x15,0x01)
#define FBPixelOffset						GLINT_TAG_ADDR(0x15,0x02)
#define FBColor							GLINT_TAG_ADDR(0x15,0x03)
#define FBData							GLINT_TAG_ADDR(0x15,0x04)
#define FBSourceData						GLINT_TAG_ADDR(0x15,0x05)

#define FBWindowBase						GLINT_TAG_ADDR(0x15,0x06)
#define FBWriteMode							GLINT_TAG_ADDR(0x15,0x07)
	/* 0:					*/
	/* FBWM_NoColorUpload	*/
	/* FBWM_WriteDisable	*/

	#define FBWM_WriteEnable					1
	#define FBWM_UploadColor					1 << 3

#define FBHardwareWriteMask					GLINT_TAG_ADDR(0x15,0x08)
#define FBBlockColor						GLINT_TAG_ADDR(0x15,0x09)
#define FBReadPixel							GLINT_TAG_ADDR(0x15,0x0a) /* PM */
#define PatternRamMode						GLINT_TAG_ADDR(0x15,0x0f)

#define PatternRamData0						GLINT_TAG_ADDR(0x16,0x00)
#define PatternRamData1						GLINT_TAG_ADDR(0x16,0x01)
#define PatternRamData2						GLINT_TAG_ADDR(0x16,0x02)
#define PatternRamData3						GLINT_TAG_ADDR(0x16,0x03)
#define PatternRamData4						GLINT_TAG_ADDR(0x16,0x04)
#define PatternRamData5						GLINT_TAG_ADDR(0x16,0x05)
#define PatternRamData6						GLINT_TAG_ADDR(0x16,0x06)
#define PatternRamData7						GLINT_TAG_ADDR(0x16,0x07)

#define FilterMode							GLINT_TAG_ADDR(0x18,0x00)
	/* 0:					*/
	/* CullDepthTags		*/
	/* CullDepthData		*/
	/* CullStencilTags		*/
	/* CullStencilData		*/
	/* CullColorTag			*/
	/* CullColorData		*/
	/* CullSyncTag			*/
	/* CullSyncData			*/
	/* CullStatisticTag		*/
	/* CullStatisticData	*/

	#define FM_PassDepthTags					0x0010
	#define FM_PassDepthData					0x0020
	#define FM_PassStencilTags					0x0040
	#define FM_PassStencilData					0x0080
	#define FM_PassColorTag						0x0100
	#define FM_PassColorData					0x0200
	#define FM_PassSyncTag						0x0400
	#define FM_PassSyncData						0x0800
	#define FM_PassStatisticTag					0x1000
	#define FM_PassStatisticData				0x2000

#define StatisticMode						GLINT_TAG_ADDR(0x18,0x01)
#define MinRegion							GLINT_TAG_ADDR(0x18,0x02)
#define MaxRegion							GLINT_TAG_ADDR(0x18,0x03)
#define ResetPickResult						GLINT_TAG_ADDR(0x18,0x04)
#define MitHitRegion						GLINT_TAG_ADDR(0x18,0x05)
#define MaxHitRegion						GLINT_TAG_ADDR(0x18,0x06)
#define PickResult							GLINT_TAG_ADDR(0x18,0x07)
#define GlintSync							GLINT_TAG_ADDR(0x18,0x08)

#define FBBlockColorU						GLINT_TAG_ADDR(0x18,0x0d)
#define FBBlockColorL						GLINT_TAG_ADDR(0x18,0x0e)
#define SuspendUntilFrameBlank				GLINT_TAG_ADDR(0x18,0x0f)



/******************************
 * GLINT Delta Core Registers *
 ******************************/

#define V0FixedTag							GLINT_TAG_ADDR(0x20,0x00)
#define V1FixedTag							GLINT_TAG_ADDR(0x21,0x00)
#define V2FixedTag							GLINT_TAG_ADDR(0x22,0x00)
#define V0FloatTag							GLINT_TAG_ADDR(0x23,0x00)
#define V1FloatTag							GLINT_TAG_ADDR(0x24,0x00)
#define V2FloatTag							GLINT_TAG_ADDR(0x25,0x00)

#define VPAR_s		0x00
#define VPAR_t		0x08
#define VPAR_q		0x10
#define VPAR_Ks		0x18
#define VPAR_Kd		0x20
#define VPAR_R		0x28
#define VPAR_G		0x30
#define VPAR_B		0x38
#define VPAR_A		0x40
#define VPAR_f		0x48
#define VPAR_x		0x50
#define VPAR_y		0x58
#define VPAR_z		0x60

#define DeltaModeTag						GLINT_TAG_ADDR(0x26,0x00)
	/* 0:					*/
	/* GLINT_300SX			*/

	/* DeltaMode Register Bit Field Assignments */
	#define DM_GLINT_300SX						0x0000
	#define DM_GLINT_500TX						0x0001
	#define DM_PERMEDIA							0x0002
	#define DM_Depth_16BPP						0x0004
	#define DM_Depth_32BPP						0x000C
	#define DM_FogEnable						0x0010
	#define DM_TextureEnable					0x0020
	#define DM_SmoothShadingEnable				0x0040
	#define DM_DepthEnable						0x0080
	#define DM_SpecularTextureEnable			0x0100
	#define DM_DiffuseTextureEnable				0x0200
	#define DM_SubPixelCorrectionEnable			0x0400
	#define DM_DiamondExit						0x0800
	#define DM_NoDraw							0x1000
	#define DM_ClampEnable						0x2000
	#define DM_ClampedTexParMode				0x4000 
	#define DM_NormalizedTexParMode				0xC000 


#define DrawTriangle						GLINT_TAG_ADDR(0x26,0x01)
#define RepeatTriangle						GLINT_TAG_ADDR(0x26,0x02)
#define DrawLine01							GLINT_TAG_ADDR(0x26,0x03)
#define DrawLine10							GLINT_TAG_ADDR(0x26,0x04)
#define RepeatLine							GLINT_TAG_ADDR(0x26,0x05)
#define BroadcastMaskTag					GLINT_TAG_ADDR(0x26,0x0F)



typedef struct {
	unsigned char r, g, b;
} LUTENTRY;


typedef struct {
	unsigned int h_limit, h_sync_start, h_sync_end, h_blank_end;
	unsigned int v_limit, v_sync_start, v_sync_end, v_blank_end;
	unsigned int clock_sel;
	unsigned int fifodis;
	unsigned int vclkctl;
	unsigned int vtgpolarity;
	unsigned int vtgserialclk;
	unsigned int vtgmodectl;
	unsigned int fbmodesel;
	unsigned int pitch;
} glintCRTCRegRec, *glintCRTCRegPtr;


#if DEBUG
#define GLINT_WRITE_REG(v,r)									\
{																\
	if( xf86Verbose > 2)										\
		ErrorF("reg 0x%04x to 0x%08x\n",r,v);					\
	*(unsigned int *)((char*)GLINTMMIOBase+r) = v;				\
}


#else
#define GLINT_WRITE_REG(v,r)									\
{																\
	*(unsigned int *)((char*)GLINTMMIOBase+r) = v;				\
}
#endif


#define GLINT_READ_REG(r)										\
	*(unsigned int *)((char*)GLINTMMIOBase+r)


#define GLINT_WAIT(n)											\
{																\
	if (!OFLG_ISSET(OPTION_PCI_RETRY, &glintInfoRec.options))	\
			while(GLINT_READ_REG(InFIFOSpace)<n);				\
}


#define GLINT_SLOW_WRITE_REG(v,r)								\
{																\
	GLINT_WAIT(1);												\
	GLINT_WRITE_REG(v,r);										\
}


/* ### test vga */
/* #define VGA_REGX     0x1000 */
/* #define PMVGAControl 0x6000 */
/* #define VGA_WRITE_REG(v,r)					\ */
/* 	{							\ */
/* 	  *(unsigned int *)((char*)PMIOBase0 + (PMVGAControl + r)) = v;	\ */
/* 	} */
/* #define VGA_READ_REG(r)					\ */
/* 	  *(unsigned int *)((char*)PMIOBase0 + (PMVGAControl + r)) */


#endif

/* end of glint_regs.h *******************************************************/
