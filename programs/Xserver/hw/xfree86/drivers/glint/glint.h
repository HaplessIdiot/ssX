/* $XFree86: $ */
/*
 * glint register file 
 *
 */ 

#ifndef _GLINTREG_H_
#define _GLINTREG_H_

/* GLINT 500TX Configuration Region Registers */

/* Device Identification */
#define CFGVendorId	0x00
#define CFGDeviceId	0x02
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

/* LocalBuffer Registers */
#define LBMemoryCtl	0x1000
#define LBMemoryEDO	0x1008

/* Framebuffer Registers */
#define FBMemoryCtl	0x1800
#define FBModeSel	0x1808
#define FBGCWrMask	0x1810
#define FBGCColorLower	0x1818
#define FBTXMemCtl	0x1820
#define FBWrMaskk	0x1830
#define FBGCColorUpper	0x1838

/* Internal Video Registers */
#define VTGHLimit	0x3000
#define VTGHSyncStart	0x3008
#define VTGHSyncEnd	0x3010
#define VTGHBlankEnd	0x3018
#define VTGVLimit	0x3020
#define VTGVSyncStart	0x3028
#define	VTGVSyncEnd	0x3030
#define VTGVBlankEnd    0x3038
#define VTGVGateStart   0x3040
#define VTGHGateEnd	0x3048
#define VTGVGateStart	0x3050
#define VTGVGateEnd	0x3058
#define VTGPolarity	0x3060
#define VTGFrameRowAddr 0x3068
#define VTGVLineNumber	0x3070
#define VTGSerialClk	0x3078
#define VTGModeCtl	0x3080

/* GLINT Delta Region 0 Registers */

/* Control Status Registers */
#define DResetStatus	0x0800
#define DIntEnable	0x0808
#define DIntFlags	0x0810
#define DErrorFlags	0x0838
#define DTestRegister	0x0848
#define DFIFODis	0x0868

#endif
