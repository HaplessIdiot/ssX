/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_opt.c,v 1.8 2001/08/01 00:44:54 tsi Exp $ */
/*
 *
 * SiS driver option evaluation
 *
 * Parts Copyright 2001, 2002, 2003 by Thomas Winischhofer, Vienna, Austria
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the supplier not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The supplier makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE SUPPLIER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  	?
 *		Thomas Winischhofer <thomas@winischhofer.net>
 */

#include "xf86.h"
#include "xf86PciInfo.h"
#include "xf86str.h"
#include "xf86Cursor.h"

#include "sis.h"

typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
/*  OPTION_PCI_RETRY,  */
    OPTION_NOACCEL,
    OPTION_TURBOQUEUE,
    OPTION_FAST_VRAM,
    OPTION_NOHOSTBUS,
/*  OPTION_SET_MEMCLOCK,   */
    OPTION_FORCE_CRT2TYPE,
    OPTION_SHADOW_FB,
    OPTION_ROTATE,
    OPTION_NOXVIDEO,
    OPTION_VESA,
    OPTION_MAXXFBMEM,
    OPTION_FORCECRT1,
    OPTION_DSTN,
    OPTION_XVONCRT2,
    OPTION_PDC,
    OPTION_TVSTANDARD,
    OPTION_USEROMDATA,
    OPTION_NOINTERNALMODES,
    OPTION_USEOEM,
    OPTION_SBIOSN,
    OPTION_NOYV12,
    OPTION_CHTVOVERSCAN,
    OPTION_CHTVSOVERSCAN,
    OPTION_CHTVLUMABANDWIDTHCVBS,
    OPTION_CHTVLUMABANDWIDTHSVIDEO,
    OPTION_CHTVLUMAFLICKERFILTER,
    OPTION_CHTVCHROMABANDWIDTH,
    OPTION_CHTVCHROMAFLICKERFILTER,
    OPTION_CHTVCVBSCOLOR,
    OPTION_CHTVTEXTENHANCE,
    OPTION_CHTVCONTRAST,
    OPTION_SISTVEDGEENHANCE,
    OPTION_SISTVANTIFLICKER,
    OPTION_SISTVSATURATION,
    OPTION_TVXPOSOFFSET,
    OPTION_TVYPOSOFFSET,
    OPTION_SIS6326ANTIFLICKER,
    OPTION_SIS6326ENABLEYFILTER,
    OPTION_SIS6326YFILTERSTRONG,
    OPTION_SIS6326FORCETVPPLUG,
    OPTION_SIS6326FSCADJUST,
    OPTION_CHTVTYPE,
    OPTION_USERGBCURSOR,
    OPTION_USERGBCURSORBLEND,
    OPTION_USERGBCURSORBLENDTH,
    OPTION_RESTOREBYSET,
    OPTION_NODDCFORCRT2,
    OPTION_CRT1GAMMA,
    OPTION_CRT2GAMMA,
    OPTION_XVDEFCONTRAST,
    OPTION_XVDEFBRIGHTNESS,
    OPTION_XVDEFHUE,
    OPTION_XVDEFSATURATION,
    OPTION_XVDEFDISABLEGFX,
    OPTION_XVDEFDISABLEGFXLR,
    OPTION_XVMEMCPY,
    OPTION_SCALELCD,
    OPTION_ENABLEHOTKEY,
    OPTION_MERGEDFB,
    OPTION_CRT2HSYNC,
    OPTION_CRT2VREFRESH,
    OPTION_CRT2POS,
    OPTION_METAMODES,
    OPTION_MERGEDFB2,
    OPTION_CRT2HSYNC2,
    OPTION_CRT2VREFRESH2,
    OPTION_CRT2POS2,
#ifdef SIS_CP
    SIS_CP_OPT_OPTIONS
#endif
    OPTION_PSEUDO
} SISOpts;

static const OptionInfoRec SISOptions[] = {
    { OPTION_SW_CURSOR,         	"SWcursor",               OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_HW_CURSOR,         	"HWcursor",               OPTV_BOOLEAN,   {0}, FALSE },
/*  { OPTION_PCI_RETRY,         	"PciRetry",               OPTV_BOOLEAN,   {0}, FALSE },  */
    { OPTION_NOACCEL,           	"NoAccel",                OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_TURBOQUEUE,        	"TurboQueue",             OPTV_BOOLEAN,   {0}, FALSE },
/*  { OPTION_SET_MEMCLOCK,      	"SetMClk",                OPTV_FREQ,      {0}, -1    },  */
    { OPTION_FAST_VRAM,         	"FastVram",               OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_NOHOSTBUS,         	"NoHostBus",              OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_FORCE_CRT2TYPE,    	"ForceCRT2Type",          OPTV_ANYSTR,    {0}, FALSE },
    { OPTION_SHADOW_FB,         	"ShadowFB",               OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_ROTATE,            	"Rotate",                 OPTV_ANYSTR,    {0}, FALSE },
    { OPTION_NOXVIDEO,          	"NoXvideo",               OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_VESA,			"Vesa",		          OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_MAXXFBMEM,         	"MaxXFBMem",              OPTV_INTEGER,   {0}, -1    },
    { OPTION_FORCECRT1,         	"ForceCRT1",              OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_DSTN,              	"DSTN",                   OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_XVONCRT2,          	"XvOnCRT2",               OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_PDC,               	"PanelDelayCompensation", OPTV_INTEGER,   {0}, -1    },
    { OPTION_TVSTANDARD,        	"TVStandard",             OPTV_STRING,    {0}, -1    },
    { OPTION_USEROMDATA,		"UseROMData",	          OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_NOINTERNALMODES,   	"NoInternalModes",        OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_USEOEM, 			"UseOEMData",		  OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_SBIOSN,            	"BIOSFile",               OPTV_STRING,    {0}, FALSE },
    { OPTION_NOYV12, 			"NoYV12",		  OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_CHTVTYPE,			"CHTVType",	          OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_CHTVOVERSCAN,		"CHTVOverscan",	          OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_CHTVSOVERSCAN,		"CHTVSuperOverscan",      OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_CHTVLUMABANDWIDTHCVBS,	"CHTVLumaBandwidthCVBS",  OPTV_INTEGER,   {0}, -1    },
    { OPTION_CHTVLUMABANDWIDTHSVIDEO,	"CHTVLumaBandwidthSVIDEO",OPTV_INTEGER,   {0}, -1    },
    { OPTION_CHTVLUMAFLICKERFILTER,	"CHTVLumaFlickerFilter",  OPTV_INTEGER,   {0}, -1    },
    { OPTION_CHTVCHROMABANDWIDTH,	"CHTVChromaBandwidth",    OPTV_INTEGER,   {0}, -1    },
    { OPTION_CHTVCHROMAFLICKERFILTER,	"CHTVChromaFlickerFilter",OPTV_INTEGER,   {0}, -1    },
    { OPTION_CHTVCVBSCOLOR,		"CHTVCVBSColor",          OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_CHTVTEXTENHANCE,		"CHTVTextEnhance",	  OPTV_INTEGER,   {0}, -1    },
    { OPTION_CHTVCONTRAST,		"CHTVContrast",		  OPTV_INTEGER,   {0}, -1    },
    { OPTION_SISTVEDGEENHANCE,		"SISTVEdgeEnhance",	  OPTV_INTEGER,   {0}, -1    },
    { OPTION_SISTVANTIFLICKER,		"SISTVAntiFlicker",	  OPTV_STRING,    {0}, FALSE },
    { OPTION_SISTVSATURATION,		"SISTVSaturation",	  OPTV_INTEGER,   {0}, -1    },
    { OPTION_TVXPOSOFFSET,		"TVXPosOffset", 	  OPTV_INTEGER,   {0}, -1    },
    { OPTION_TVYPOSOFFSET,		"TVYPosOffset", 	  OPTV_INTEGER,   {0}, -1    },
    { OPTION_SIS6326ANTIFLICKER,	"SIS6326TVAntiFlicker",   OPTV_STRING,    {0}, FALSE  },
    { OPTION_SIS6326ENABLEYFILTER,	"SIS6326TVEnableYFilter", OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_SIS6326YFILTERSTRONG,	"SIS6326TVYFilterStrong", OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_SIS6326FORCETVPPLUG,	"SIS6326TVForcePlug",     OPTV_STRING,    {0}, -1    },
    { OPTION_SIS6326FSCADJUST,		"SIS6326FSCAdjust", 	  OPTV_INTEGER,   {0}, -1    },
    { OPTION_USERGBCURSOR, 		"UseColorHWCursor",	  OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_USERGBCURSORBLEND,		"ColorHWCursorBlending",  OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_USERGBCURSORBLENDTH,	"ColorHWCursorBlendThreshold", 	  OPTV_INTEGER,   {0}, -1    },
    { OPTION_RESTOREBYSET,		"RestoreBySetMode", 	  OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_NODDCFORCRT2,		"NoCRT2Detection", 	  OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_CRT1GAMMA,			"CRT1Gamma", 	  	  OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_CRT2GAMMA,			"CRT2Gamma", 	  	  OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_XVDEFCONTRAST,		"XvDefaultContrast", 	  OPTV_INTEGER,   {0}, -1    },
    { OPTION_XVDEFBRIGHTNESS,		"XvDefaultBrightness", 	  OPTV_INTEGER,   {0}, -1    },
    { OPTION_XVDEFHUE,			"XvDefaultHue", 	  OPTV_INTEGER,   {0}, -1    },
    { OPTION_XVDEFSATURATION,		"XvDefaultSaturation", 	  OPTV_INTEGER,   {0}, -1    },
    { OPTION_XVDEFDISABLEGFX,		"XvDefaultDisableGfx", 	  OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_XVDEFDISABLEGFXLR,		"XvDefaultDisableGfxLR",  OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_XVMEMCPY,			"XvUseMemcpy",  	  OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_SCALELCD,			"ScaleLCD",	   	  OPTV_BOOLEAN,   {0}, -1    },
    { OPTION_ENABLEHOTKEY,		"EnableHotkey",	   	  OPTV_BOOLEAN,   {0}, -1    },
#ifdef SISMERGED
    { OPTION_MERGEDFB,			"MergedFB",		  OPTV_BOOLEAN,	  {0}, FALSE },
    { OPTION_CRT2HSYNC,			"CRT2HSync",		  OPTV_ANYSTR,	  {0}, FALSE },
    { OPTION_CRT2VREFRESH,		"CRT2VRefresh",		  OPTV_ANYSTR,    {0}, FALSE },
    { OPTION_CRT2POS,   		"CRT2Position",		  OPTV_ANYSTR,	  {0}, FALSE },
    { OPTION_METAMODES,   		"MetaModes",  		  OPTV_ANYSTR,	  {0}, FALSE },
    { OPTION_MERGEDFB2,			"TwinView",		  OPTV_BOOLEAN,	  {0}, FALSE },		/* alias */
    { OPTION_CRT2HSYNC2,		"SecondMonitorHorizSync", OPTV_ANYSTR,	  {0}, FALSE }, 	/* alias */
    { OPTION_CRT2VREFRESH2,		"SecondMonitorVertRefresh",	  OPTV_ANYSTR,    {0}, FALSE }, /* alias */
    { OPTION_CRT2POS2,   		"TwinViewOrientation",	  OPTV_ANYSTR,	  {0}, FALSE }, 	/* alias */
#endif
#ifdef SIS_CP
    SIS_CP_OPTION_DETAIL
#endif
    { -1,                       	NULL,                     OPTV_NONE,      {0}, FALSE }
};

void
SiSOptions(ScrnInfoPtr pScrn)
{
    SISPtr      pSiS = SISPTR(pScrn);
    MessageType from;
/*  double      temp;  */
    char        *strptr;

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    if (!(pSiS->Options = xalloc(sizeof(SISOptions))))
	return;

    memcpy(pSiS->Options, SISOptions, sizeof(SISOptions));

    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, pSiS->Options);

    /* Set defaults */
    
    pSiS->newFastVram = -1;
    pSiS->NoHostBus = FALSE;
/*  pSiS->UsePCIRetry = TRUE; */
    pSiS->TurboQueue = TRUE;
    pSiS->HWCursor = TRUE;
    pSiS->Rotate = FALSE;
    pSiS->ShadowFB = FALSE;
    pSiS->VESA = -1;
    pSiS->NoXvideo = FALSE;
    pSiS->maxxfbmem = 0;
    pSiS->forceCRT1 = -1;
    pSiS->DSTN = FALSE;
    pSiS->XvOnCRT2 = FALSE;
    pSiS->NoYV12 = -1;
    pSiS->PDC = -1;
    pSiS->OptTVStand = -1;
    pSiS->OptROMUsage = -1;
    pSiS->noInternalModes = FALSE;
    pSiS->OptUseOEM = -1;
    pSiS->OptTVOver = -1;
    pSiS->OptTVSOver = -1;
    pSiS->chtvlumabandwidthcvbs = -1;
    pSiS->chtvlumabandwidthsvideo = -1;
    pSiS->chtvlumaflickerfilter = -1;
    pSiS->chtvchromabandwidth = -1;
    pSiS->chtvchromaflickerfilter = -1;
    pSiS->chtvcvbscolor = -1;
    pSiS->chtvtextenhance = -1;
    pSiS->chtvcontrast = -1;
    pSiS->sistvedgeenhance = -1;
    pSiS->sistvantiflicker = -1;
    pSiS->sistvsaturation = -1;
    pSiS->sis6326antiflicker = -1;
    pSiS->sis6326enableyfilter = -1;
    pSiS->sis6326yfilterstrong = -1;
    pSiS->sis6326tvplug = -1;
    pSiS->sis6326fscadjust = 0;
    pSiS->tvxpos = 0;
    pSiS->tvypos = 0;
    pSiS->NonDefaultPAL = -1;
    pSiS->chtvtype = -1;
    pSiS->restorebyset = TRUE;
    pSiS->nocrt2ddcdetection = FALSE;
    pSiS->ForceCRT2Type = CRT2_DEFAULT;
    pSiS->ForceTVType = -1;
    pSiS->CRT1gamma = TRUE;
    pSiS->CRT2gamma = TRUE;
    pSiS->XvDefBri = 0;
    pSiS->XvDefCon = 4;
    pSiS->XvDefHue = 0; 
    pSiS->XvDefSat = 0;
    pSiS->XvDefDisableGfx = FALSE;
    pSiS->XvDefDisableGfxLR = FALSE;
    pSiS->UsePanelScaler = -1;
    pSiS->XvUseMemcpy = TRUE;
    if(pSiS->sishw_ext.jChipType == SIS_730 ||
       pSiS->sishw_ext.jChipType == SIS_740) {
       pSiS->XvUseMemcpy = FALSE;
    }
#ifdef SISMERGED
    pSiS->MergedFB = FALSE;
    pSiS->CRT2Position = sisRightOf;
    pSiS->CRT2HSync = NULL;
    pSiS->CRT2VRefresh = NULL;
    pSiS->MetaModes = NULL;
#endif
#ifdef SIS_CP
    SIS_CP_OPT_DEFAULT
#endif

    /* Chipset dependent defaults */

    if(pSiS->Chipset == PCI_CHIP_SIS530) {
    	 /* TW: TQ still broken on 530/620? */
	 pSiS->TurboQueue = FALSE;
    }

    if(pSiS->Chipset == PCI_CHIP_SIS6326) {
         pSiS->newFastVram = 1;
    }

#if XF86_VERSION_CURRENT < XF86_VERSION_NUMERIC(4,2,99,0,0)
    pSiS->OptUseColorCursor = 0;
#else
    if(pSiS->VGAEngine == SIS_300_VGA) {
    	pSiS->OptUseColorCursor = 0;
	pSiS->OptUseColorCursorBlend = 1;
	pSiS->OptColorCursorBlendThreshold = 0x37000000;
    } else if(pSiS->VGAEngine == SIS_315_VGA) {
    	pSiS->OptUseColorCursor = 1;
    }
#endif
    if(pSiS->VGAEngine == SIS_300_VGA) {
       pSiS->AllowHotkey = 0;
    } else if(pSiS->VGAEngine == SIS_315_VGA) {
       pSiS->AllowHotkey = 1;
    }

    /* Collect the options */

#if 0
    /* PCI retry - TW: What the heck is/was this for? */
    from = X_DEFAULT;
    if(xf86GetOptValBool(pSiS->Options, OPTION_PCI_RETRY, &pSiS->UsePCIRetry)) {
        from = X_CONFIG;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "PCI retry %s\n",
                         pSiS->UsePCIRetry ? "enabled" : "disabled");
#endif

   /* Mem clock */
#if 0  /* TW: This is not used */
    if(xf86GetOptValFreq(pSiS->Options, OPTION_SET_MEMCLOCK, OPTUNITS_MHZ,
                                                            &temp)) {
        pSiS->MemClock = (int)(temp * 1000.0);
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                    "Memory clock set to %.3f MHz\n", pSiS->MemClock/1000.0);
    }
#endif

   /* FastVRAM (5597/5598, 6326 and 530/620 only)
    */
    if((pSiS->VGAEngine == SIS_OLD_VGA) || (pSiS->VGAEngine == SIS_530_VGA)) {
       from = X_DEFAULT;
       if(xf86GetOptValBool(pSiS->Options, OPTION_FAST_VRAM, &pSiS->newFastVram)) {
          from = X_CONFIG;
       }
       xf86DrvMsg(pScrn->scrnIndex, from, "Fast VRAM %s\n",
                   (pSiS->newFastVram == -1) ? "enabled (for write only)" :
		   	(pSiS->newFastVram ? "enabled (for read and write)" : "disabled"));
    }

   /* NoHostBus (5597/5598 only)
    */
    if((pSiS->Chipset == PCI_CHIP_SIS5597)) {
       from = X_DEFAULT;
       if(xf86GetOptValBool(pSiS->Options, OPTION_NOHOSTBUS, &pSiS->NoHostBus)) {
          from = X_CONFIG;
       }
       xf86DrvMsg(pScrn->scrnIndex, from, "SiS5597/5598 VGA-to-CPU host bus %s\n",
                   pSiS->NoHostBus ? "disabled" : "enabled");
    }

   /* MaxXFBMem
    * TW: This options limits the amount of video memory X uses for screen
    *     and off-screen buffers. This option should be used if using DRI
    *     is intended. The kernel framebuffer driver required for DRM will
    *     start its memory heap at 12MB if it detects more than 16MB, at 8MB if
    *     between 8 and 16MB are available, otherwise at 4MB. So, if the amount
    *     of memory X uses, a clash between the framebuffer's memory heap
    *     and X is avoided. The amount is to be specified in KB.
    */
    if(xf86GetOptValULong(pSiS->Options, OPTION_MAXXFBMEM,
                                &pSiS->maxxfbmem)) {
            xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                    "MaxXFBMem: Framebuffer memory shall be limited to %d KB\n",
		    pSiS->maxxfbmem);
	    pSiS->maxxfbmem *= 1024;
    }

    /* NoAccel
     * Turns off 2D acceleration (and XVideo since it relies on acceleration)
     */
    if(xf86ReturnOptValBool(pSiS->Options, OPTION_NOACCEL, FALSE)) {
        pSiS->NoAccel = TRUE;
	pSiS->NoXvideo = TRUE;
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration and Xv disabled\n");
    }

    /* SWCursor
     * HWCursor
     * Chooses whether to use the hardware or software cursor
     */
    from = X_DEFAULT;
    if(xf86GetOptValBool(pSiS->Options, OPTION_HW_CURSOR, &pSiS->HWCursor)) {
        from = X_CONFIG;
    }
    if(xf86ReturnOptValBool(pSiS->Options, OPTION_SW_CURSOR, FALSE)) {
        from = X_CONFIG;
        pSiS->HWCursor = FALSE;
	pSiS->OptUseColorCursor = 0;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
                                pSiS->HWCursor ? "HW" : "SW");

    /*
     * UseColorHWCursor
     * ColorHWCursorBlending
     * ColorHWCursorBlendThreshold
     *
     * Enable/disable color hardware cursors;
     * enable/disable color hw cursor emulation for 300 series
     * select emultation transparency threshold for 300 series
     *
     */
#if XF86_VERSION_CURRENT >= XF86_VERSION_NUMERIC(4,2,99,0,0)
#ifdef ARGB_CURSOR
#ifdef SIS_ARGB_CURSOR
    if((pSiS->HWCursor) && ((pSiS->VGAEngine == SIS_300_VGA) || (pSiS->VGAEngine == SIS_315_VGA))) {
       from = X_DEFAULT;
       if(xf86GetOptValBool(pSiS->Options, OPTION_USERGBCURSOR, &pSiS->OptUseColorCursor)) {
    	   from = X_CONFIG;
       }
       xf86DrvMsg(pScrn->scrnIndex, from, "Color HW cursor is %s\n",
                    pSiS->OptUseColorCursor ? "enabled" : "disabled");

       if(pSiS->VGAEngine == SIS_300_VGA) {
          from = X_DEFAULT;
          if(xf86GetOptValBool(pSiS->Options, OPTION_USERGBCURSORBLEND, &pSiS->OptUseColorCursorBlend)) {
    	     from = X_CONFIG;
          }
          if(pSiS->OptUseColorCursor) {
             xf86DrvMsg(pScrn->scrnIndex, from,
	   	"HW cursor color blending emulation is %s\n",
		(pSiS->OptUseColorCursorBlend) ? "enabled" : "disabled");
	  }
	  {
	  int temp;
	  from = X_DEFAULT;
	  if(xf86GetOptValInteger(pSiS->Options, OPTION_USERGBCURSORBLENDTH, &temp)) {
	     if((temp >= 0) && (temp <= 255)) {
	        from = X_CONFIG;
		pSiS->OptColorCursorBlendThreshold = (temp << 24);
	     } else {
	        temp = pSiS->OptColorCursorBlendThreshold >> 24;
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   "Illegal color HW cursor blending threshold, valid range 0-255\n");
	     }
	  } else {
	        temp = pSiS->OptColorCursorBlendThreshold >> 24;
          }
	  if(pSiS->OptUseColorCursor) {
	     if(pSiS->OptUseColorCursorBlend) {
	        xf86DrvMsg(pScrn->scrnIndex, from,
	          "HW cursor color blending emulation threshold is %d\n", temp);
 	     }
	  }
	  }
       }
    }
#endif
#endif
#endif

    /*
     * MergedFB
     *
     * Enable/disable and configure merged framebuffer mode
     *
     */
#ifdef SISMERGED
#ifdef SISDUALHEAD
    if(pSiS->DualHeadMode) {
       Bool val;
       if(xf86GetOptValBool(pSiS->Options, OPTION_MERGEDFB, &val)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	     "Option \"MergedFB\" cannot be used in Dual Head mode\n");
       }
    } else
#endif
    if((pSiS->VGAEngine == SIS_300_VGA) || (pSiS->VGAEngine == SIS_315_VGA)) {
       Bool val;
       if(xf86GetOptValBool(pSiS->Options, OPTION_MERGEDFB, &val)) {
	  if(val) pSiS->MergedFB = TRUE;
       } else if(xf86GetOptValBool(pSiS->Options, OPTION_MERGEDFB2, &val)) {
          if(val) pSiS->MergedFB = TRUE;
       }

       if(pSiS->MergedFB) {
	  strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_CRT2POS);
	  if(!strptr) {
	     strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_CRT2POS2);
	  }
      	  if(strptr) {
       	     if((!strcmp(strptr,"LeftOf")) || (!strcmp(strptr,"leftof")))
                pSiS->CRT2Position = sisLeftOf;
 	     else if((!strcmp(strptr,"RightOf")) || (!strcmp(strptr,"rightof")))
                pSiS->CRT2Position = sisRightOf;
	     else if((!strcmp(strptr,"Above")) || (!strcmp(strptr,"above")))
                pSiS->CRT2Position = sisAbove;
	     else if((!strcmp(strptr,"Below")) || (!strcmp(strptr,"below")))
                pSiS->CRT2Position = sisBelow;
	     else if((!strcmp(strptr,"Clone")) || (!strcmp(strptr,"clone")))
                pSiS->CRT2Position = sisClone;
	     else {
	        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	            "\"%s\" is not a valid parameter for Option \"CRT2Position\"\n", strptr);
	    	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	            "Valid parameters are \"RightOf\", \"LeftOf\", \"Above\", \"Below\", or \"Clone\"\n");
	     }
	  }
	  strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_METAMODES);
	  if(strptr) {
	     pSiS->MetaModes = xalloc(strlen(strptr) + 1);
	     if(pSiS->MetaModes) memcpy(pSiS->MetaModes, strptr, strlen(strptr) + 1);
	  }
	  if(pSiS->MetaModes) {
	     strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_CRT2HSYNC);
	     if(!strptr) {
	        strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_CRT2HSYNC2);
	     }
	     if(strptr) {
	        pSiS->CRT2HSync = xalloc(strlen(strptr) + 1);
		if(pSiS->CRT2HSync) memcpy(pSiS->CRT2HSync, strptr, strlen(strptr) + 1);
	     }
	     strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_CRT2VREFRESH);
	     if(!strptr) {
	        strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_CRT2VREFRESH2);
	     }
	     if(strptr) {
	        pSiS->CRT2VRefresh = xalloc(strlen(strptr) + 1);
		if(pSiS->CRT2VRefresh) memcpy(pSiS->CRT2VRefresh, strptr, strlen(strptr) + 1);
	     }
	  } else {
	     xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "Option \"MergedFB\" (alias \"TwinView\") requires Option \"MetaModes\".\n");
	     xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "MergedFB (alias TwinView) mode disabled.\n");
	     pSiS->MergedFB = FALSE;
	  }
       }
    }
#endif

   /* TW: Some options can only be specified in the Master Head's Device
    *     section. Here we give the user a hint in the log.
    */
#ifdef SISDUALHEAD
    if((pSiS->DualHeadMode) && (pSiS->SecondHead)) {
       static const char *mystring = "Option \"%s\" is only accepted in Master Head's device section\n";
       Bool val;
       int vali;
       if(pSiS->VGAEngine != SIS_315_VGA) {
          if(xf86GetOptValBool(pSiS->Options, OPTION_TURBOQUEUE, &val)) {
             xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "TurboQueue");
          }
       }
       if(xf86GetOptValBool(pSiS->Options, OPTION_RESTOREBYSET, &val)) {
	  xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "RestoreBySetMode");
       }
       if(xf86GetOptValBool(pSiS->Options, OPTION_ENABLEHOTKEY, &val)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "EnableHotKey");
       }
       if(xf86GetOptValBool(pSiS->Options, OPTION_USEROMDATA, &val)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "UseROMData");
       }
       if(xf86GetOptValBool(pSiS->Options, OPTION_USEOEM, &val)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "UseOEMData");
       }
       if(xf86GetOptValBool(pSiS->Options, OPTION_FORCECRT1, &val)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "ForceCRT1");
       }
       if(xf86GetOptValBool(pSiS->Options, OPTION_NODDCFORCRT2, &val)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "NoCRT2Detection");
       }
       if(xf86GetOptValString(pSiS->Options, OPTION_FORCE_CRT2TYPE)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "ForceCRT2Type");
       }
       if(xf86GetOptValBool(pSiS->Options, OPTION_SCALELCD, &val)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "ScaleLCD");
       }
       if(xf86GetOptValInteger(pSiS->Options, OPTION_PDC, &vali)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "PanelDelayCompensation");
       }
       if(pSiS->Chipset == PCI_CHIP_SIS550) {
          if(xf86GetOptValBool(pSiS->Options, OPTION_DSTN, &val)) {
             xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "DSTN");
	  }
       }
       if(xf86GetOptValString(pSiS->Options, OPTION_TVSTANDARD)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "TVStandard");
       }
       if(xf86GetOptValString(pSiS->Options, OPTION_CHTVTYPE)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "CHTVType");
       }
       if(xf86GetOptValBool(pSiS->Options, OPTION_CHTVOVERSCAN, &val)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "CHTVOverscan");
       }
       if(xf86GetOptValBool(pSiS->Options, OPTION_CHTVSOVERSCAN, &val)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "CHTVSuperOverscan");
       }
       if((xf86GetOptValInteger(pSiS->Options, OPTION_CHTVLUMABANDWIDTHCVBS, &vali)) ||
	  (xf86GetOptValInteger(pSiS->Options, OPTION_CHTVLUMABANDWIDTHSVIDEO, &vali)) ||
	  (xf86GetOptValInteger(pSiS->Options, OPTION_CHTVLUMAFLICKERFILTER, &vali)) ||
	  (xf86GetOptValInteger(pSiS->Options, OPTION_CHTVCHROMABANDWIDTH, &vali)) ||
	  (xf86GetOptValInteger(pSiS->Options, OPTION_CHTVCHROMAFLICKERFILTER, &vali)) ||
          (xf86GetOptValBool(pSiS->Options, OPTION_CHTVCVBSCOLOR, &val)) ||
	  (xf86GetOptValInteger(pSiS->Options, OPTION_CHTVTEXTENHANCE, &vali)) ||
	  (xf86GetOptValInteger(pSiS->Options, OPTION_CHTVCONTRAST, &vali)) ||
	  (xf86GetOptValInteger(pSiS->Options, OPTION_SISTVEDGEENHANCE, &vali)) ||
	  (xf86GetOptValString(pSiS->Options, OPTION_SISTVANTIFLICKER)) ||
	  (xf86GetOptValInteger(pSiS->Options, OPTION_SISTVSATURATION, &vali)) ||
	  (xf86GetOptValInteger(pSiS->Options, OPTION_TVXPOSOFFSET, &vali)) ||
	  (xf86GetOptValInteger(pSiS->Options, OPTION_TVYPOSOFFSET, &vali))) {
	  xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	      "TV related options are only accepted in Master Head's device section");
       }
       if(xf86GetOptValBool(pSiS->Options, OPTION_CRT1GAMMA, &val)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "CRT1Gamma");
       }
       if(xf86GetOptValBool(pSiS->Options, OPTION_CRT2GAMMA, &val)) {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING, mystring, "CRT2Gamma");
       }
#ifdef SIS_CP
       SIS_CP_OPT_DH_WARN
#endif
    } else
#endif    
    {
       if(pSiS->VGAEngine != SIS_315_VGA) {

	 /* TurboQueue
          * TW: We always use this on 315 series.
	  */
          from = X_DEFAULT;
          if(xf86GetOptValBool(pSiS->Options, OPTION_TURBOQUEUE, &pSiS->TurboQueue)) {
    	     from = X_CONFIG;
          }
          xf86DrvMsg(pScrn->scrnIndex, from, "TurboQueue %s\n",
                    pSiS->TurboQueue ? "enabled" : "disabled");
       }

       if((pSiS->VGAEngine == SIS_300_VGA) || (pSiS->VGAEngine == SIS_315_VGA)) {

          Bool val;

	 /* RestoreBySetMode (300/315/330 series only)
          * TW: Set this to force the driver to set the old mode instead of restoring
          *     the register contents. This can be used to overcome problems with
          *     LCD panels and video bridges.
          */
          if(xf86GetOptValBool(pSiS->Options, OPTION_RESTOREBYSET, &val)) {
             if(val) pSiS->restorebyset = TRUE;
	     else    pSiS->restorebyset = FALSE;
          }

	 /* EnableHotkey (300/315/330 series only)
	  * TW: Enables or disables the BIOS hotkey switch for
	  *     switching the output device on laptops.
	  *     This key causes a total machine hang on many 300 series
	  *     machines, it is therefore by default disabled on such.
	  *     In dual head mode, using the hotkey is lethal, so we
	  *     forbid it then in any case.
	  *     However, although the driver disables the hotkey as
	  *     BIOS developers intented to do that, some buggy BIOSes
	  *     still cause the machine to freeze. Hence the warning.
	  */
          {
	  int flag = 0;
#ifdef SISDUALHEAD
          if(pSiS->DualHeadMode) {
	     pSiS->AllowHotkey = 0;
	     flag = 1;
	  } else
#endif
	  if(xf86GetOptValBool(pSiS->Options, OPTION_ENABLEHOTKEY, &val)) {
	     if(val)  pSiS->AllowHotkey = 1;
	     else     pSiS->AllowHotkey = 0;
          }
	  xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Hotkey display switching is %s%s\n",
	        pSiS->AllowHotkey ? "enabled" : "disabled",
	        flag ? " in dual head mode" : "");
	  if(pSiS->Chipset == PCI_CHIP_SIS630 ||
	     pSiS->Chipset == PCI_CHIP_SIS650 ||
	     pSiS->Chipset == PCI_CHIP_SIS660) {
	     xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	         "WARNING: Using the Hotkey might freeze your machine, regardless\n");
	     xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "         whether enabled or disabled. This is no driver bug.\n");
	  }
	  }

         /* UseROMData (300/315/330 series only)
          * TW: This option is enabling/disabling usage of some machine
          *     specific data from the BIOS ROM. This option can - and
          *     should - be used in case the driver makes problems
          *     because SiS changed the location of this data.
          */
	  if(xf86GetOptValBool(pSiS->Options, OPTION_USEROMDATA, &val)) {
	     if(val)  pSiS->OptROMUsage = 1;
	     else     pSiS->OptROMUsage = 0;

	     xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	         "Video ROM data usage shall be %s\n",
	          val ? "enabled":"disabled");
	  }

         /* UseOEMData (300/315/330 series only)
          * TW: The driver contains quite a lot data for OEM LCD panels
          *     and TV connector specifics which override the defaults.
          *     If this data is incorrect, the TV may lose color and
          *     the LCD panel might show some strange effects. Use this
          *     option to disable the usage of this data.
          */
	  if(xf86GetOptValBool(pSiS->Options, OPTION_USEOEM, &val)) {
	     if(val)  pSiS->OptUseOEM = 1;
	     else     pSiS->OptUseOEM = 0;

	     xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	         "Internal LCD/TV/VGA2 OEM data usage shall be %s\n",
	         val ? "enabled":"disabled");
	  }

	 /* TW: This remains undocumented :)
	  */
	  pSiS->sbiosn = NULL;
          strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_SBIOSN);
          if(strptr != NULL) {
	     pSiS->sbiosn = xalloc(strlen(strptr)+1);
             if(pSiS->sbiosn) strcpy(pSiS->sbiosn, strptr);
          }

         /* TW: ForceCRT1 (300/315/330 series only)
          *     This option can be used to force CRT1 to be switched on/off. Its
          *     intention is mainly for old monitors that can't be detected
          *     automatically. This is only useful on machines with a video bridge.
          *     In normal cases, this option won't be necessary.
          */
	  if(xf86GetOptValBool(pSiS->Options, OPTION_FORCECRT1, &val)) {
	     if(val)  pSiS->forceCRT1 = 1;
	     else     pSiS->forceCRT1 = 0;

	     xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	         "CRT1 shall be forced to %s\n",
	         val ? "ON" : "OFF");
	  }

	 /* NoCRT2DDCDetection (300/315/330 series only)
          * TW: If set to true, this disables CRT2 detection using DDC. This is
          *     to avoid problems with not entirely DDC compiant LCD panels or
          *     VGA monitors connected to the secondary VGA plug. Since LCD and
          *     VGA share the same DDC channel, it might in some cases be impossible
          *     to determine if the device is a CRT monitor or a flat panel.
          */
          if(xf86GetOptValBool(pSiS->Options, OPTION_NODDCFORCRT2, &val)) {
             if(val) pSiS->nocrt2ddcdetection = TRUE;
	     else    pSiS->nocrt2ddcdetection = FALSE;
          }

	 /* ForceCRT2Type (300/315/330 series only)
	  * TW: Used for forcing the driver to initialize a given
	  *     CRT2 device type.
          *     (SVIDEO, COMPOSITE and SCART for overriding detection)
          */
          strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_FORCE_CRT2TYPE);
          if(strptr != NULL) {
             if((!strcmp(strptr,"TV")) || (!strcmp(strptr,"tv")))
                pSiS->ForceCRT2Type = CRT2_TV;
 	     else if((!strcmp(strptr,"SVIDEO")) || (!strcmp(strptr,"svideo"))) {
                pSiS->ForceCRT2Type = CRT2_TV;
	        pSiS->ForceTVType = TV_SVIDEO;
             } else if((!strcmp(strptr,"COMPOSITE")) || (!strcmp(strptr,"composite"))) {
                pSiS->ForceCRT2Type = CRT2_TV;
	        pSiS->ForceTVType = TV_AVIDEO;
             } else if((!strcmp(strptr,"SCART")) || (!strcmp(strptr,"scart"))) {
                pSiS->ForceCRT2Type = CRT2_TV;
	        pSiS->ForceTVType = TV_SCART;
             } else if((!strcmp(strptr,"LCD")) || (!strcmp(strptr,"lcd")))
                pSiS->ForceCRT2Type = CRT2_LCD;
             else if((!strcmp(strptr,"DVI-D")) || (!strcmp(strptr,"dvi-d")))
                pSiS->ForceCRT2Type = CRT2_LCD;
             else if((!strcmp(strptr,"VGA")) || (!strcmp(strptr,"vga")))
                pSiS->ForceCRT2Type = CRT2_VGA;
 	     else if((!strcmp(strptr,"DVI-A")) || (!strcmp(strptr,"dvi-a")))
                pSiS->ForceCRT2Type = CRT2_VGA;
             else if((!strcmp(strptr,"NONE")) || (!strcmp(strptr,"none")))
                pSiS->ForceCRT2Type = 0;
	     else {
	        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	      	    "\"%s\" is not a valid parameter for Option \"ForceCRT2Type\"\n", strptr);
	        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	            "Valid parameters are \"LCD\" (= \"DVI-D\"), \"TV\", \"SVIDEO\", \"COMPOSITE\", "
		    "\"SCART\", \"VGA\" (= \"DVI-A\") or \"NONE\"\n");
	     }

             if(pSiS->ForceCRT2Type != CRT2_DEFAULT)
                xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                    "CRT2 type shall be %s\n", strptr);
          }

         /* ScaleLCD (300/315/330 series only)
          * TW: Can be used to force the bridge/panel link to [do|not do] the
	  *     scaling of modes lower than the panel's native resolution.
          *     Setting this to TRUE will force the bridge/panel link
	  *     to scale; FALSE will rely on the panel's capabilities.
          */
	  if(xf86GetOptValBool(pSiS->Options, OPTION_SCALELCD, &val)) {
	     if(val)  pSiS->UsePanelScaler = 0;
	     else     pSiS->UsePanelScaler = 1;
	     xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "LCD scaling is %s\n",
	         pSiS->UsePanelScaler ? "disabled" : "enabled");
	  }

         /* PanelDelayCompensation (300/315/330 series only; currently used
          *     on 300 series only)
          * TW: This might be required if the LCD panel shows "small waves".
          *     The parameter is an integer, usually either 4, 32 or 24.
          *     Why this option? Simply because SiS did poor BIOS design.
          *     The PDC value depends on the very LCD panel used in a
          *     particular machine. For most panels, the driver is able
          *     to detect the correct value. However, some panels require
          *     a different setting. The value given must be within the mask 0x3c.
          */
          if(xf86GetOptValInteger(pSiS->Options, OPTION_PDC, &pSiS->PDC)) {
	     if(pSiS->PDC & ~0x3c) {
	        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	            "Illegal PanelDelayCompensation value\n");
	        pSiS->PDC = -1;
	     } else {
                xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                    "Panel delay compensation shall be %d\n",
	             pSiS->PDC);
	     }
          }

       }

       if(pSiS->Chipset == PCI_CHIP_SIS550) {
         /* TW: SiS 550 DSTN/FSTN
          *     This is for notifying the driver to use the DSTN registers on 550.
          *     DSTN/FSTN is a special LCD port of the SiS550 (notably not the 551
          *     and 552, which I don't know how to detect) that uses an extended
          *     register range. The only effect of this option is that the driver
          *     saves and restores these registers. DSTN display modes are chosen
          *     by using resultion 320x480x8 or 320x480x16.
          */
          if(xf86ReturnOptValBool(pSiS->Options, OPTION_DSTN, FALSE)) {
             pSiS->DSTN = TRUE;
             xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "SiS 550 DSTN/FSTN enabled\n");
          } else {
	     xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "SiS 550 DSTN/FSTN disabled\n");
          }
       }

      /* TVStandard (300/315/330 series and 6326 w/ TV only)
       * TW: This option is for overriding the autodetection of
       *     the BIOS/Jumper option for PAL / NTSC
       */
       if((pSiS->VGAEngine == SIS_300_VGA) ||
          (pSiS->VGAEngine == SIS_315_VGA) ||
          ((pSiS->Chipset == PCI_CHIP_SIS6326) && (pSiS->SiS6326Flags & SIS6326_HASTV))) {
          strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_TVSTANDARD);
          if(strptr != NULL) {
             if((!strcmp(strptr,"PAL")) || (!strcmp(strptr,"pal")))
	        pSiS->OptTVStand = 1;
	     else if((!strcmp(strptr,"PALM")) ||
	             (!strcmp(strptr,"palm")) ||
	             (!strcmp(strptr,"PAL-M")) ||
		     (!strcmp(strptr,"pal-m"))) {
	        pSiS->OptTVStand = 1;
	        pSiS->NonDefaultPAL = 1;
  	     } else if((!strcmp(strptr,"PALN")) ||
	               (!strcmp(strptr,"paln")) ||
	               (!strcmp(strptr,"PAL-N")) ||
		       (!strcmp(strptr,"pal-n"))) {
	        pSiS->OptTVStand = 1;
	        pSiS->NonDefaultPAL = 0;
  	     } else if((!strcmp(strptr,"NTSC")) || (!strcmp(strptr,"ntsc")))
	        pSiS->OptTVStand = 0;
	     else {
	        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	            "\"%s\" is not a valid parameter for Option \"TVStandard\"\n", strptr);
                xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	            "Valid options are \"PAL\", \"PALM\", \"PALN\" or \"NTSC\"\n");
	     }

	     if(pSiS->OptTVStand != -1) {
	        if(pSiS->Chipset == PCI_CHIP_SIS6326) {
	           pSiS->NonDefaultPAL = -1;
	           xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "TV Standard shall be %s\n",
	               pSiS->OptTVStand ? "PAL" : "NTSC");
	        } else {
	           xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "TV Standard shall be %s\n",
		       (pSiS->OptTVStand ?
		           ( (pSiS->NonDefaultPAL == -1) ? "PAL" :
			      ((pSiS->NonDefaultPAL) ? "PALM" : "PALN") )
				           : "NTSC"));
	        }
	     }
          }
       }

      /* CHTVType  (315/330 series only)
       * TW: Used for telling the driver if the TV output shall
       *     be i480 HDTV or SCART.
       */
       if(pSiS->VGAEngine == SIS_315_VGA) {
          strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_CHTVTYPE);
          if(strptr != NULL) {
             if((!strcmp(strptr,"SCART")) || (!strcmp(strptr,"scart")))
                pSiS->chtvtype = 1;
 	     else if((!strcmp(strptr,"HDTV")) || (!strcmp(strptr,"hdtv")))
	        pSiS->chtvtype = 0;
	     else {
	        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	            "\"%s\" is not a valid parameter for Option \"CHTVType\"\n", strptr);
	        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	          "Valid parameters are \"SCART\" or \"HDTV\"\n");
	     }
             if(pSiS->chtvtype != -1)
                xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                  "Chrontel TV type shall be %s\n", strptr);
          }
       }

      /* CHTVOverscan (300/315/330 series only)
       * CHTVSuperOverscan (300/315/330 series only)
       * TW: These options are for overriding the BIOS option for
       *     TV Overscan. Some BIOS don't even have such an option.
       *     SuperOverscan is only supported with PAL.
       *     Both options are only effective on machines with a
       *     CHRONTEL TV encoder. SuperOverscan is only available
       *     on the 700x.
       */
       if((pSiS->VGAEngine == SIS_300_VGA) || (pSiS->VGAEngine == SIS_315_VGA)) {
	  Bool val;
	  if(xf86GetOptValBool(pSiS->Options, OPTION_CHTVOVERSCAN, &val)) {
	     if(val) pSiS->OptTVOver = 1;
	     else    pSiS->OptTVOver = 0;

	     xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	         "Chrontel: TV overscan shall be %s\n",
	         val ? "enabled":"disabled");
	  }
          if(xf86GetOptValBool(pSiS->Options, OPTION_CHTVSOVERSCAN, &pSiS->OptTVSOver)) {
	     xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	         "Chrontel: TV super overscan shall be %s\n",
	         pSiS->OptTVSOver ? "enabled":"disabled");
	  }
       }

      /* TW: Various parameters for TV output via SiS bridge, Chrontel or SiS6326
       */
       if((pSiS->VGAEngine == SIS_300_VGA) || (pSiS->VGAEngine == SIS_315_VGA)) {
          int tmp = 0;
          xf86GetOptValInteger(pSiS->Options, OPTION_CHTVLUMABANDWIDTHCVBS,
                                &pSiS->chtvlumabandwidthcvbs);
	  xf86GetOptValInteger(pSiS->Options, OPTION_CHTVLUMABANDWIDTHSVIDEO,
                                &pSiS->chtvlumabandwidthsvideo);
	  xf86GetOptValInteger(pSiS->Options, OPTION_CHTVLUMAFLICKERFILTER,
                                &pSiS->chtvlumaflickerfilter);
	  xf86GetOptValInteger(pSiS->Options, OPTION_CHTVCHROMABANDWIDTH,
                                &pSiS->chtvchromabandwidth);
	  xf86GetOptValInteger(pSiS->Options, OPTION_CHTVCHROMAFLICKERFILTER,
                                &pSiS->chtvchromaflickerfilter);
	  xf86GetOptValBool(pSiS->Options, OPTION_CHTVCVBSCOLOR,
				&pSiS->chtvcvbscolor);
	  xf86GetOptValInteger(pSiS->Options, OPTION_CHTVTEXTENHANCE,
                                &pSiS->chtvtextenhance);
	  xf86GetOptValInteger(pSiS->Options, OPTION_CHTVCONTRAST,
                                &pSiS->chtvcontrast);
	  xf86GetOptValInteger(pSiS->Options, OPTION_SISTVEDGEENHANCE,
                                &pSiS->sistvedgeenhance);
	  strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_SISTVANTIFLICKER);
          if(strptr) {
             if((!strcmp(strptr,"OFF")) || (!strcmp(strptr,"off")))
	        pSiS->sis6326antiflicker = 0;
  	     else if((!strcmp(strptr,"LOW")) || (!strcmp(strptr,"low")))
	        pSiS->sis6326antiflicker = 1;
	     else if((!strcmp(strptr,"MED")) || (!strcmp(strptr,"med")))
	        pSiS->sis6326antiflicker = 2;
	     else if((!strcmp(strptr,"HIGH")) || (!strcmp(strptr,"high")))
	        pSiS->sis6326antiflicker = 3;
	     else if((!strcmp(strptr,"ADAPTIVE")) || (!strcmp(strptr,"adaptive")))
	        pSiS->sis6326antiflicker = 4;
	     else {
	        pSiS->sis6326antiflicker = -1;
	        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	      	    "\"%s\" is not a valid parameter for Option \"SISTVAntiFlicker\"\n", strptr);
	        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	            "Valid parameters are \"OFF\", \"LOW\", \"MED\", \"HIGH\" or \"ADAPTIVE\"\n");
	     }
	  }
	  xf86GetOptValInteger(pSiS->Options, OPTION_SISTVSATURATION,
                                &pSiS->sistvsaturation);
	  xf86GetOptValInteger(pSiS->Options, OPTION_TVXPOSOFFSET,
                                &pSiS->tvxpos);
	  xf86GetOptValInteger(pSiS->Options, OPTION_TVYPOSOFFSET,
                                &pSiS->tvypos);
	  if(pSiS->tvxpos > 32)  { pSiS->tvxpos = 32;  tmp = 1; }
	  if(pSiS->tvxpos < -32) { pSiS->tvxpos = -32; tmp = 1; }
	  if(pSiS->tvypos > 32)  { pSiS->tvypos = 32;  tmp = 1; }
	  if(pSiS->tvypos < -32) { pSiS->tvypos = -32;  tmp = 1; }
	  if(tmp) xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		      "Illegal TV x or y offset. Range is from -32 to 32\n");
       }

       if((pSiS->Chipset == PCI_CHIP_SIS6326) && (pSiS->SiS6326Flags & SIS6326_HASTV)) {
          int tmp = 0;
          strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_SIS6326ANTIFLICKER);
          if(strptr) {
             if((!strcmp(strptr,"OFF")) || (!strcmp(strptr,"off")))
	        pSiS->sis6326antiflicker = 0;
  	     else if((!strcmp(strptr,"LOW")) || (!strcmp(strptr,"low")))
	        pSiS->sis6326antiflicker = 1;
	     else if((!strcmp(strptr,"MED")) || (!strcmp(strptr,"med")))
	        pSiS->sis6326antiflicker = 2;
	     else if((!strcmp(strptr,"HIGH")) || (!strcmp(strptr,"high")))
	        pSiS->sis6326antiflicker = 3;
	     else if((!strcmp(strptr,"ADAPTIVE")) || (!strcmp(strptr,"adaptive")))
	        pSiS->sis6326antiflicker = 4;
	     else {
	        pSiS->sis6326antiflicker = -1;
	        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	     	    "\"%s\" is not a valid parameter for Option \"SIS6326TVAntiFlicker\"\n", strptr);
	        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	           "Valid parameters are \"OFF\", \"LOW\", \"MED\", \"HIGH\" or \"ADAPTIVE\"\n");
	     }
	  }
	  strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_SIS6326FORCETVPPLUG);
          if(strptr) {
             if((!strcmp(strptr,"COMPOSITE")) || (!strcmp(strptr,"composite")))
	        pSiS->sis6326tvplug = 1;
  	     else if((!strcmp(strptr,"SVIDEO")) || (!strcmp(strptr,"svideo")))
	        pSiS->sis6326tvplug = 0;
	     else {
	        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	     	    "\"%s\" is not a valid parameter for Option \"SIS6326TVForcePlug\"\n", strptr);
	        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	           "Valid parameters are \"COMPOSITE\" or \"SVIDEO\"\n");
	     }
	  }
	  xf86GetOptValBool(pSiS->Options, OPTION_SIS6326ENABLEYFILTER,
                                &pSiS->sis6326enableyfilter);
	  xf86GetOptValBool(pSiS->Options, OPTION_SIS6326YFILTERSTRONG,
                                &pSiS->sis6326yfilterstrong);
	  xf86GetOptValInteger(pSiS->Options, OPTION_TVXPOSOFFSET,
                                &pSiS->tvxpos);
	  xf86GetOptValInteger(pSiS->Options, OPTION_TVYPOSOFFSET,
                                &pSiS->tvypos);
	  if(pSiS->tvxpos > 16)  { pSiS->tvxpos = 16;  tmp = 1; }
	  if(pSiS->tvxpos < -16) { pSiS->tvxpos = -16; tmp = 1; }
	  if(pSiS->tvypos > 16)  { pSiS->tvypos = 16;  tmp = 1; }
	  if(pSiS->tvypos < -16) { pSiS->tvypos = -16;  tmp = 1; }
	  if(tmp) xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		      "Illegal TV x or y offset. Range is from -16 to 16\n");
          xf86GetOptValInteger(pSiS->Options, OPTION_SIS6326FSCADJUST,
                                &pSiS->sis6326fscadjust);
	  if(pSiS->sis6326fscadjust) {
	     xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	     	"Adjusting the default FSC by %d\n",
		pSiS->sis6326fscadjust);
	  }
       }

      /* CRT1Gamma - disable gamma correction for CRT1
       */
       {
       Bool val;
       from = X_DEFAULT;
       if(xf86GetOptValBool(pSiS->Options, OPTION_CRT1GAMMA, &val)) {
          from = X_CONFIG;
	  pSiS->CRT1gamma = val;
       }
       xf86DrvMsg(pScrn->scrnIndex, from, "CRT1 gamma correction is %s\n",
                  pSiS->CRT1gamma ? "enabled" : "disabled");
       }

      /* CRT2Gamma - disable gamma correction for CRT2
       */
       if((pSiS->VGAEngine == SIS_300_VGA) || (pSiS->VGAEngine == SIS_315_VGA)) {
          Bool val;
          if(xf86GetOptValBool(pSiS->Options, OPTION_CRT2GAMMA, &val)) {
	     pSiS->CRT2gamma = val;
          }
       }

#ifdef SIS_CP
       SIS_CP_OPT_DOOPT
#endif

    }  /* DualHead */

   /* VESA - DEPRECATED
    * TW: This option is for forcing the driver to use
    *     the VESA BIOS extension for mode switching.
    */
    {
	Bool val;

	if(xf86GetOptValBool(pSiS->Options, OPTION_VESA, &val)) {
	    if(val)  pSiS->VESA = 1;
	    else     pSiS->VESA = 0;

	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	        "VESA: VESA usage shall be %s\n",
		val ? "enabled":"disabled");
 	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	    	"*** Option \"VESA\" is deprecated. *** \n");
	}
    }

    if((pSiS->VGAEngine == SIS_300_VGA) || (pSiS->VGAEngine == SIS_315_VGA)) {

       /* TW: NoInternalModes (300/315/330 series only)
        *     Since the mode switching code for these chipsets is a
        *     Asm-to-C translation of BIOS code, we only have timings
        *     for a pre-defined number of modes. The default behavior
        *     is to replace XFree's default modes with a mode list
        *     generated out of the known and supported modes. Use
        *     this option to disable this. However, even if using
        *     out built-in mode list will NOT make it possible to
        *     use modelines.
        */
        from = X_DEFAULT;
	if(xf86GetOptValBool(pSiS->Options, OPTION_NOINTERNALMODES, &pSiS->noInternalModes))
		from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, from, "Usage of built-in modes is %s\n",
		       pSiS->noInternalModes ? "disabled":"enabled");
    }

    /* ShadowFB */
    from = X_DEFAULT;
    if(xf86GetOptValBool(pSiS->Options, OPTION_SHADOW_FB, &pSiS->ShadowFB)) {
#ifdef SISMERGED
       if(pSiS->MergedFB) {
          pSiS->ShadowFB = FALSE;
	  xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	      "Shadow Frame Buffer not permitted in MergeFB mode\n");
       } else
#endif
          from = X_CONFIG;
    }
    if(pSiS->ShadowFB) {
	pSiS->NoAccel = TRUE;
	pSiS->NoXvideo = TRUE;
    	xf86DrvMsg(pScrn->scrnIndex, from,
	   "Using \"Shadow Frame Buffer\" - acceleration and Xv disabled\n");
    }

    /* Rotate */
    if((strptr = (char *)xf86GetOptValString(pSiS->Options, OPTION_ROTATE))) {
#ifdef SISMERGED
       if(pSiS->MergedFB) {
	  xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	      "Screen rotation not permitted in MergeFB mode\n");
       } else
#endif
       if(!xf86NameCmp(strptr, "CW")) {
          pSiS->ShadowFB = TRUE;
          pSiS->NoAccel = TRUE;
	  pSiS->NoXvideo = TRUE;
          pSiS->HWCursor = FALSE;
          pSiS->Rotate = 1;
          xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
              "Rotating screen clockwise (acceleration and Xv disabled)\n");
       } else if(!xf86NameCmp(strptr, "CCW")) {
          pSiS->ShadowFB = TRUE;
          pSiS->NoAccel = TRUE;
          pSiS->NoXvideo = TRUE;
          pSiS->HWCursor = FALSE;
          pSiS->Rotate = -1;
          xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
              "Rotating screen counter clockwise (acceleration and Xv disabled)\n");
       } else {
          xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
              "\"%s\" is not a valid parameter for Option \"Rotate\"\n", strptr);
          xf86DrvMsg(pScrn->scrnIndex, X_INFO,
              "Valid parameters are \"CW\" or \"CCW\"\n");
       }
    }

   /* NoXVideo
    * Set this to TRUE to disable Xv hardware video acceleration
    */
    if((!pSiS->NoAccel) && (!pSiS->NoXvideo)) {
       if(xf86ReturnOptValBool(pSiS->Options, OPTION_NOXVIDEO, FALSE)) {
          pSiS->NoXvideo = TRUE;
          xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "XVideo extension disabled\n");
       }

       if(!pSiS->NoXvideo) {
          Bool val;
	  int tmp;

	  if((pSiS->VGAEngine == SIS_300_VGA) || (pSiS->VGAEngine == SIS_315_VGA)) {
      	     /* TW: XvOnCRT2:
	      * On chipsets with only one overlay (315, 650), the user should
	      * choose to display the overlay on CRT1 or CRT2. By setting this
	      * option to TRUE, the overlay will be displayed on CRT2. The
	      * default is: CRT1 if only CRT1 available, CRT2 if only CRT2
	      * available, and CRT1 if both is available and detected.
	      * Since implementation of the XV_SWITCHCRT Xv property this only
	      * selects the default CRT.
	      */
             if(xf86GetOptValBool(pSiS->Options, OPTION_XVONCRT2, &val)) {
#ifdef SISDUALHEAD
                if((pSiS->DualHeadMode) && (pSiS->SecondHead)) {
    		   xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   	"Option \"XvOnCRT2\" only accepted in Master Head's device section\n");
	        } else
#endif
	        if(val) pSiS->XvOnCRT2 = TRUE;
	        else    pSiS->XvOnCRT2 = FALSE;
	     }
	  }

	  if((pSiS->VGAEngine == SIS_OLD_VGA) || (pSiS->VGAEngine == SIS_530_VGA)) {
	     /* TW: NoYV12 (for 5597/5598, 6326 and 530/620 only)
	      *     YV12 has problems with videos larger than 384x288. So
	      *     allow the user to disable YV12 support to force the
	      *     application to use YUV2 instead.
	      */
             if(xf86GetOptValBool(pSiS->Options, OPTION_NOYV12, &val)) {
	        if(val)  pSiS->NoYV12 = 1;
	        else     pSiS->NoYV12 = 0;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			"Xv YV12/I420 support is %s\n",
			pSiS->NoYV12 ? "disabled" : "enabled");
	     }
	  }

	  /* Some Xv properties' defaults can be set by options */
          if(xf86GetOptValInteger(pSiS->Options, OPTION_XVDEFCONTRAST, &tmp)) {
             if((tmp >= 0) && (tmp <= 7)) pSiS->XvDefCon = tmp;
             else xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
       		      "Illegal XvDefaultContrast parameter. Valid range is 0 - 7\n");
          }
          if(xf86GetOptValInteger(pSiS->Options, OPTION_XVDEFBRIGHTNESS, &tmp)) {
             if((tmp >= -128) && (tmp <= 127)) pSiS->XvDefBri = tmp;
             else xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
       		      "Illegal XvDefaultBrightness parameter. Valid range is -128 - 127\n");
          }
          if(pSiS->VGAEngine == SIS_315_VGA) {
             if(xf86GetOptValInteger(pSiS->Options, OPTION_XVDEFHUE, &tmp)) {
                if((tmp >= -8) && (tmp <= 7)) pSiS->XvDefHue = tmp;
                else xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
       	              "Illegal XvDefaultHue parameter. Valid range is -8 - 7\n");
             }
             if(xf86GetOptValInteger(pSiS->Options, OPTION_XVDEFSATURATION, &tmp)) {
                if((tmp >= -7) && (tmp <= 7)) pSiS->XvDefSat = tmp;
                else xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
       	              "Illegal XvDefaultSaturation parameter. Valid range is -7 - 7\n");
             }
          }
	  if(xf86GetOptValBool(pSiS->Options, OPTION_XVDEFDISABLEGFX, &val)) {
	     if(val)  pSiS->XvDefDisableGfx = TRUE;
	     xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Graphics display will be %s during Xv usage\n",
	     	val ? "disabled" : "enabled");
          }
	  if((pSiS->VGAEngine == SIS_300_VGA) || (pSiS->VGAEngine == SIS_315_VGA)) {
	     if(xf86GetOptValBool(pSiS->Options, OPTION_XVDEFDISABLEGFXLR, &val)) {
	        if(val)  pSiS->XvDefDisableGfxLR = TRUE;
	        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	     	   "Graphics display left/right of overlay will be %s during Xv usage\n",
		   val ? "disabled" : "enabled");
             }
          }
	  if(xf86GetOptValBool(pSiS->Options, OPTION_XVMEMCPY, &val)) {
	     if(val)  pSiS->XvUseMemcpy = TRUE;
	     xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Xv will %suse memcpy()\n",
	     	val ? "" : "not ");
          }
       }
    }

}

const OptionInfoRec *
SISAvailableOptions(int chipid, int busid)
{
    return SISOptions;
}
