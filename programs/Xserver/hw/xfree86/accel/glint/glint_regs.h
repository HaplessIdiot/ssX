/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/glint_regs.h,v 1.2 1997/06/25 08:24:55 hohndel Exp $ */
/*
 * glint register file 
 *
 * Copyright by Stefan Dirsch, Dirk Hohndel, Alan Hourihane
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *           Dirk Hohndel, <hohndel@suse.de>
 *	     Stefan Dirsch, <sndirsch@suse.de>
 *
 * this work is sponsored by S.u.S.E. GmbH, Fuerth, Elsa GmbH, Aachen and
 * Siemens Nixdorf Informationssysteme
 *
 */ 

#ifndef _GLINTREG_H_
#define _GLINTREG_H_
/* GLINT 500TX Configuration Region Registers */

/* Device Identification */
#define CFGVendorId	0x00

#    define PCI_VENDOR_3DLABS       0x3D3D

#define CFGDeviceId	0x02

#    define PCI_CHIP_3DLABS_300SX      0x0001
#    define PCI_CHIP_3DLABS_500TX      0x0002
#    define PCI_CHIP_3DLABS_DELTA      0x0003
#    define PCI_CHIP_3DLABS_PERMEDIA   0x0004

#define CFGRevisionId	0x08
#define CFGClassCode	0x09
#define CFGHeaderType	0x0E

/* Device Control/Status */
#define CFGCommand	0x04
#define CFGStatus	0x06

/* Miscellaneous Functions */
#define CFGBist		0x0f
#define CFGLatTimer     0x0d
#define CFGCacheLine    0x0c
#define CFGMaxLat       0x3f
#define CFGMinGrant     0x3e
#define CFGIntPin       0x3d
#define CFGIntLine      0x3c

/* Base Adresses */
#define CFGBaseAddr0	0x10 
#define CFGBaseAddr1	0x14
#define CFGBaseAddr2	0x18
#define CFGBaseAddr3	0x1C
#define CFGBaseAddr4	0x20
#define CFGRomAddr	0x30


/* GLINT 500TX Region 0 Registers */

/* Control Status Registers */
#define ResetStatus	0x0000
#define IntEnable	0x0008
#define IntFlags	0x0010
#define InFIFOSpace	0x0018
#define OutFIFOWords	0x0020
#define DMAAddress	0x0028
#define DMACount	0x0030
#define ErrorFlags	0x0038
#define VClkCtl		0x0040
#define TestRegister	0x0048
#define Aperture0	0x0050
#define Aperture1	0x0058
#define DMAControl	0x0060
#define FIFODis		0x0068

/* GLINT PerMedia Region 0 additional Registers */
#define ChipConfig	0x0070

/* GLINT 500TX LocalBuffer Registers */
#define LBMemoryCtl	0x1000
#define LBMemoryEDO	0x1008

/* GLINT PerMedia Memory Control Registers */
#define PMReboot	0x1000
#define PMRomControl	0x1040
#define PMBootAddress	0x1080
#define PMMemConfig	0x10C0
#define PMBypassWriteMask	0x1100
#define PMFramebufferWriteMask	0x1140
#define PMCount		0x1180

/* Framebuffer Registers */
#define FBMemoryCtl	0x1800
#define FBModeSel	0x1808
#define FBGCWrMask	0x1810
#define FBGCColorLower	0x1818
#define FBTXMemCtl	0x1820
#define FBWrMaskk	0x1830
#define FBGCColorUpper	0x1838

/* Core FIFO */
#define OutputFIFO	0x2000

/* 500TX Internal Video Registers */
#define VTGHLimit	0x3000
#define VTGHSyncStart	0x3008
#define VTGHSyncEnd	0x3010
#define VTGHBlankEnd	0x3018
#define VTGVLimit	0x3020
#define VTGVSyncStart	0x3028
#define	VTGVSyncEnd	0x3030
#define VTGVBlankEnd    0x3038
#define VTGHGateStart   0x3040
#define VTGHGateEnd	0x3048
#define VTGVGateStart	0x3050
#define VTGVGateEnd	0x3058
#define VTGPolarity	0x3060
#define VTGFrameRowAddr 0x3068
#define VTGVLineNumber	0x3070
#define VTGSerialClk	0x3078
#define VTGModeCtl	0x3080

/* Permedia Video Control Registers */
#define PMScreenBase	0x3000
#define PMScreenStride	0x3008
#define PMHTotal	0x3010
#define PMHgEnd		0x3018
#define PMHbEnd		0x3020
#define PMHsStart	0x3028
#define PMHsEnd		0x3030
#define PMVTotal	0x3038
#define PMVbEnd		0x3040
#define PMVsStart	0x3048
#define	PMVsEnd		0x3050
#define PMVideoControl	0x3058
#define PMInterruptLine	0x3060
#define PMDDCData	0x3068
#define PMLineCount	0x3070

/* GLINT Delta Region 0 Registers */

/* Control Status Registers */
#define DResetStatus	0x0800
#define DIntEnable	0x0808
#define DIntFlags	0x0810
#define DErrorFlags	0x0838
#define DTestRegister	0x0848
#define DFIFODis	0x0868

/* GLINT Core Registers */
#define GLINT_TAG(major,offset) (((major) << 7) | ((offset) << 3)) 
#define GLINT_TAG_ADDR(major,offset) (0x8000 | GLINT_TAG((major),(offset)))

#define StartXDom		GLINT_TAG_ADDR(0x00,0x00)
#define dXDom			GLINT_TAG_ADDR(0x00,0x01)
#define StartXSub		GLINT_TAG_ADDR(0x00,0x02)
#define dXSub			GLINT_TAG_ADDR(0x00,0x03)
#define StartY			GLINT_TAG_ADDR(0x00,0x04)
#define dY			GLINT_TAG_ADDR(0x00,0x05)
#define Count			GLINT_TAG_ADDR(0x00,0x06)
#define Render			GLINT_TAG_ADDR(0x00,0x07)
#define		LINE		0x00
#define		AREASTIPPLE	0x01
#define		LINESTIPPLE	0x02
#define		POINT		0x80
#define		TRAPEZOID	0x40
#define		FASTFILL	0x08
#define		SYNCONBITMASK	0x800
#define		SYNCONHOSTDATA	0x1000
#define		SPANOPERATION	0x40000
#define ContinueNewLine		GLINT_TAG_ADDR(0x00,0x08)
#define ContinueNewDom		GLINT_TAG_ADDR(0x00,0x09)
#define ContinueNewSub		GLINT_TAG_ADDR(0x00,0x0a)
#define Continue		GLINT_TAG_ADDR(0x00,0x0b)
#define FlushSpan		GLINT_TAG_ADDR(0x00,0x0c)
#define BitMaskPattern		GLINT_TAG_ADDR(0x00,0x0d)

#define PointTable0		GLINT_TAG_ADDR(0x01,0x00)
#define PointTable1		GLINT_TAG_ADDR(0x01,0x01)
#define PointTable2		GLINT_TAG_ADDR(0x01,0x02)
#define PointTable3		GLINT_TAG_ADDR(0x01,0x03)
#define RasterizerMode		GLINT_TAG_ADDR(0x01,0x04)
#define YLimits			GLINT_TAG_ADDR(0x01,0x05)
#define ScanLineOwnership	GLINT_TAG_ADDR(0x01,0x06)
#define WaitForCompletion	GLINT_TAG_ADDR(0x01,0x07)
#define PixelSize		GLINT_TAG_ADDR(0x01,0x08)
#define XLimits			GLINT_TAG_ADDR(0x01,0x09) /* PM only */

#define PackedDataLimits	GLINT_TAG_ADDR(0x02,0x0a) /* PM only */

#define ScissorMode		GLINT_TAG_ADDR(0x03,0x00)
#define ScissorMinXY		GLINT_TAG_ADDR(0x03,0x01)
#define ScissorMaxXY		GLINT_TAG_ADDR(0x03,0x02)
#define ScreenSize		GLINT_TAG_ADDR(0x03,0x03)
#define AreaStippleMode		GLINT_TAG_ADDR(0x03,0x04)
#define LineStippleMode		GLINT_TAG_ADDR(0x03,0x05)
#define LoadLineStippleCounters	GLINT_TAG_ADDR(0x03,0x06)
#define UpdateLineStippleCounters	GLINT_TAG_ADDR(0x03,0x07)
#define SaveLineStippleState	GLINT_TAG_ADDR(0x03,0x08)
#define WindowOrigin		GLINT_TAG_ADDR(0x03,0x09)

#define AreaStipplePattern0	GLINT_TAG_ADDR(0x04,0x00)
#define AreaStipplePattern1	GLINT_TAG_ADDR(0x04,0x01)
#define AreaStipplePattern2	GLINT_TAG_ADDR(0x04,0x02)
#define AreaStipplePattern3	GLINT_TAG_ADDR(0x04,0x03)
#define AreaStipplePattern4	GLINT_TAG_ADDR(0x04,0x04)
#define AreaStipplePattern5	GLINT_TAG_ADDR(0x04,0x05)
#define AreaStipplePattern6	GLINT_TAG_ADDR(0x04,0x06)
#define AreaStipplePattern7	GLINT_TAG_ADDR(0x04,0x07)

#define TextureAddressMode	GLINT_TAG_ADDR(0x07,0x00)
#define SStart			GLINT_TAG_ADDR(0x07,0x01)
#define dSdx			GLINT_TAG_ADDR(0x07,0x02)
#define dSdyDom			GLINT_TAG_ADDR(0x07,0x03)
#define TStart			GLINT_TAG_ADDR(0x07,0x04)
#define dTdx			GLINT_TAG_ADDR(0x07,0x05)
#define dTdyDom			GLINT_TAG_ADDR(0x07,0x06)
#define QStart			GLINT_TAG_ADDR(0x07,0x07)
#define dQdx			GLINT_TAG_ADDR(0x07,0x08)
#define dQdyDom			GLINT_TAG_ADDR(0x07,0x09)

#define TextureReadMode		GLINT_TAG_ADDR(0x09,0x00)
#define TextureFormat		GLINT_TAG_ADDR(0x09,0x01)
#define TextureCacheControl	GLINT_TAG_ADDR(0x09,0x02)
#define BorderColor		GLINT_TAG_ADDR(0x09,0x05)

#define TxBaseAddr		GLINT_TAG_ADDR(0x0a,0x00)
#define TxBaseAddrLR		GLINT_TAG_ADDR(0x0a,0x01)
#define TexelLUT		GLINT_TAG_ADDR(0x0a,0x00)

#define Texel0			GLINT_TAG_ADDR(0x0c,0x00)
#define Texel1			GLINT_TAG_ADDR(0x0c,0x01)
#define Texel2			GLINT_TAG_ADDR(0x0c,0x02)
#define Texel3			GLINT_TAG_ADDR(0x0c,0x03)
#define Texel4			GLINT_TAG_ADDR(0x0c,0x04)
#define Texel5			GLINT_TAG_ADDR(0x0c,0x05)
#define Texel6			GLINT_TAG_ADDR(0x0c,0x06)
#define Texel7			GLINT_TAG_ADDR(0x0c,0x07)
#define Interp0			GLINT_TAG_ADDR(0x0c,0x08)
#define Interp1			GLINT_TAG_ADDR(0x0c,0x09)
#define Interp2			GLINT_TAG_ADDR(0x0c,0x0a)
#define Interp3			GLINT_TAG_ADDR(0x0c,0x0b)
#define Interp4			GLINT_TAG_ADDR(0x0c,0x0c)
#define TextureFilter		GLINT_TAG_ADDR(0x0c,0x0d)

#define TextureColorMode	GLINT_TAG_ADDR(0x0d,0x00)
#define TextureEnvColor		GLINT_TAG_ADDR(0x0d,0x01)
#define FogMode			GLINT_TAG_ADDR(0x0d,0x02)
#define FogColor		GLINT_TAG_ADDR(0x0d,0x03)
#define FStart			GLINT_TAG_ADDR(0x0d,0x04)
#define dFdx			GLINT_TAG_ADDR(0x0d,0x05)
#define dFdyDom			GLINT_TAG_ADDR(0x0d,0x06)
#define KsStart			GLINT_TAG_ADDR(0x0d,0x09)
#define dKsdx			GLINT_TAG_ADDR(0x0d,0x0a)
#define dKsdyDom		GLINT_TAG_ADDR(0x0d,0x0b)
#define KdStart			GLINT_TAG_ADDR(0x0d,0x0c)
#define dKdStart		GLINT_TAG_ADDR(0x0d,0x0d)
#define dKddyDom		GLINT_TAG_ADDR(0x0d,0x0e)

#define RStart			GLINT_TAG_ADDR(0x0f,0x00)
#define dRdx			GLINT_TAG_ADDR(0x0f,0x01)
#define dRdyDom			GLINT_TAG_ADDR(0x0f,0x02)
#define GStart			GLINT_TAG_ADDR(0x0f,0x03)
#define dGdx			GLINT_TAG_ADDR(0x0f,0x04)
#define dGdyDom			GLINT_TAG_ADDR(0x0f,0x05)
#define BStart			GLINT_TAG_ADDR(0x0f,0x06)
#define dBdx			GLINT_TAG_ADDR(0x0f,0x07)
#define dBdyDom			GLINT_TAG_ADDR(0x0f,0x08)
#define AStart			GLINT_TAG_ADDR(0x0f,0x09)
#define dAdx			GLINT_TAG_ADDR(0x0f,0x0a)
#define dAdyDom			GLINT_TAG_ADDR(0x0f,0x0b)
#define ColorDDAMode		GLINT_TAG_ADDR(0x0f,0x0c)
#define ConstantColor		GLINT_TAG_ADDR(0x0f,0x0d)
#define Color			GLINT_TAG_ADDR(0x0f,0x0e)

#define AlphaTestMode		GLINT_TAG_ADDR(0x10,0x00)
#define AntialiasMode		GLINT_TAG_ADDR(0x10,0x01)
#define AlphaBlendMode		GLINT_TAG_ADDR(0x10,0x02)
#define DitherMode		GLINT_TAG_ADDR(0x10,0x03)
#define FBSoftwareWriteMask	GLINT_TAG_ADDR(0x10,0x04)
#define LogicalOpMode		GLINT_TAG_ADDR(0x10,0x05)
#define FBWriteData		GLINT_TAG_ADDR(0x10,0x06)
#define RouterMode		GLINT_TAG_ADDR(0x10,0x08)

#define LBReadMode		GLINT_TAG_ADDR(0x11,0x00)
#define LBReadFormat		GLINT_TAG_ADDR(0x11,0x01)
#define LBSourceOffset		GLINT_TAG_ADDR(0x11,0x02)
#define LBStencil		GLINT_TAG_ADDR(0x11,0x05)
#define LBDepth			GLINT_TAG_ADDR(0x11,0x06)
#define LBWindowBase		GLINT_TAG_ADDR(0x11,0x07)
#define LBWriteMode		GLINT_TAG_ADDR(0x11,0x08)
#define		WriteEnable	0x01
#define LBWriteFormat		GLINT_TAG_ADDR(0x11,0x09)
#define TextureData		GLINT_TAG_ADDR(0x11,0x0d)
#define TextureDownloadOffset	GLINT_TAG_ADDR(0x11,0x0e)

#define GLINTWindow		GLINT_TAG_ADDR(0x13,0x00)
#define StencilMode		GLINT_TAG_ADDR(0x13,0x01)
#define StencilData		GLINT_TAG_ADDR(0x13,0x02)
#define Stencil			GLINT_TAG_ADDR(0x13,0x03)
#define DepthMode		GLINT_TAG_ADDR(0x13,0x04)
#define Depth			GLINT_TAG_ADDR(0x13,0x05)
#define ZStartU			GLINT_TAG_ADDR(0x13,0x06)
#define ZStartL			GLINT_TAG_ADDR(0x13,0x07)
#define dZdxU			GLINT_TAG_ADDR(0x13,0x08)
#define dZdxL			GLINT_TAG_ADDR(0x13,0x09)
#define dZdyDomU		GLINT_TAG_ADDR(0x13,0x0a)
#define dZdyDomL		GLINT_TAG_ADDR(0x13,0x0b)
#define FastClearDepth		GLINT_TAG_ADDR(0x13,0x0c)

#define FBReadMode		GLINT_TAG_ADDR(0x15,0x00)
#define		ReadSource	0x200
#define		ReadDestination	0x400
#define		DataType	0x8000
#define FBSourceOffset		GLINT_TAG_ADDR(0x15,0x01)
#define FBPixelOffset		GLINT_TAG_ADDR(0x15,0x02)
#define FBColor			GLINT_TAG_ADDR(0x15,0x03)
#define FBWindowBase		GLINT_TAG_ADDR(0x15,0x06)
#define FBWriteMode		GLINT_TAG_ADDR(0x15,0x07)
#define FBHardwareWriteMask	GLINT_TAG_ADDR(0x15,0x08)
#define FBBlockColor		GLINT_TAG_ADDR(0x15,0x09)
#define FBReadPixel		GLINT_TAG_ADDR(0x15,0x0a) /* PM only */
#define PatternRamMode		GLINT_TAG_ADDR(0x15,0x0f)

#define PatternRamData0		GLINT_TAG_ADDR(0x16,0x00)
#define PatternRamData1		GLINT_TAG_ADDR(0x16,0x01)
#define PatternRamData2		GLINT_TAG_ADDR(0x16,0x02)
#define PatternRamData3		GLINT_TAG_ADDR(0x16,0x03)
#define PatternRamData4		GLINT_TAG_ADDR(0x16,0x04)
#define PatternRamData5		GLINT_TAG_ADDR(0x16,0x05)
#define PatternRamData6		GLINT_TAG_ADDR(0x16,0x06)
#define PatternRamData7		GLINT_TAG_ADDR(0x16,0x07)

#define FilterMode		GLINT_TAG_ADDR(0x18,0x00)
#define StatisticMode		GLINT_TAG_ADDR(0x18,0x01)
#define MinRegion		GLINT_TAG_ADDR(0x18,0x02)
#define MaxRegion		GLINT_TAG_ADDR(0x18,0x03)
#define ResetPickResult		GLINT_TAG_ADDR(0x18,0x04)
#define MitHitRegion		GLINT_TAG_ADDR(0x18,0x05)
#define MaxHitRegion		GLINT_TAG_ADDR(0x18,0x06)
#define PickResult		GLINT_TAG_ADDR(0x18,0x07)
#define GlintSync		GLINT_TAG_ADDR(0x18,0x08)

#define FBBlockColorU		GLINT_TAG_ADDR(0x18,0x0d)
#define FBBlockColorL		GLINT_TAG_ADDR(0x18,0x0e)
#define SuspendUntilFrameBlank	GLINT_TAG_ADDR(0x18,0x0f)

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
#define GLINT_WRITE_REG(v,r)					\
	{							\
	    if( xf86Verbose > 2)				\
	  	ErrorF("reg 0x%04x to 0x%08x\n",r,v);   	\
		*(unsigned int *)((char*)GLINTMMIOBase+r) = v;	\
	}
#else
#define GLINT_WRITE_REG(v,r)					\
	{							\
		*(unsigned int *)((char*)GLINTMMIOBase+r) = v;	\
	}
#endif

#define GLINT_READ_REG(r)					\
	*(unsigned int *)((char*)GLINTMMIOBase+r)

#define GLINT_WAIT(n)						\
	{							\
		if (!OFLG_ISSET(OPTION_PCI_RETRY, &glintInfoRec.options))					\
			while(GLINT_READ_REG(InFIFOSpace)<n);	\
	}

#define GLINT_SLOW_WRITE_REG(v,r) 				\
	{							\
		GLINT_WRITE_REG(v,r);				\
		GLINT_WAIT(1);					\
	}

#endif
