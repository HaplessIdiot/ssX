/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Configure.c,v 3.2 2000/01/25 01:36:54 dawes Exp $ */
/*
 * Copyright 2000 by Alan Hourihane, Sychdyn, North Wales.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Alan Hourihane, alanh@fairlite.demon.co.uk
 *
 */

#include <ctype.h>
#include <stdlib.h>
#include "X.h"
#include "Xmd.h"
#include "os.h"
#ifdef XFree86LOADER
#include "loaderProcs.h"
#endif
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86PciInfo.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

static int haveVGA = -1;

XF86ConfInputPtr
configureInputSection (void)
{
    XF86ConfInputPtr keyboard;
    XF86ConfInputPtr mouse;
    Bool foundMouse = FALSE;
    parsePrologue (XF86ConfInputPtr, XF86ConfInputRec)

    ptr->inp_identifier = "Primary Keyboard";
    ptr->inp_driver = "keyboard";
    ptr->inp_option_lst = NULL;
    ptr->list.next = NULL;

    /* Crude mechanism to auto-detect mouse (os dependent) */
    { 
	int fd;
	fd = open("/dev/mouse", 0);
	if (fd != -1) {
	    foundMouse = TRUE;
	    close(fd);
	}
    }

    if (foundMouse) {
        mouse = xf86confmalloc(sizeof(XF86ConfInputRec));
    	memset((XF86ConfInputPtr)mouse,0,sizeof(XF86ConfInputRec));
        mouse->inp_identifier = "Primary Mouse";
        mouse->inp_driver = "mouse";
        mouse->list.next = NULL;
        mouse->inp_option_lst = 
			addNewOption(mouse->inp_option_lst, "Protocol", "auto");
	mouse->inp_option_lst = 
		addNewOption(mouse->inp_option_lst, "Device", "/dev/mouse");
	ptr = (XF86ConfInputPtr)addListItem((glp)ptr, (glp)mouse);
    }

    return ptr;
}

XF86ConfDRIPtr
configureDRISection (void)
{
    parsePrologue (XF86ConfDRIPtr, XF86ConfDRIRec)

    return ptr;
}

XF86ConfVendorPtr
configureVendorSection (void)
{
    parsePrologue (XF86ConfVendorPtr, XF86ConfVendorRec)

    ptr->vnd_identifier = "Primary Vendor";

    return ptr;
}

XF86ConfScreenPtr
configureScreenSection (void)
{
    parsePrologue (XF86ConfScreenPtr, XF86ConfScreenRec)

    ptr->scrn_identifier = "Primary Screen";
    ptr->scrn_monitor_str = "Primary Monitor";
    ptr->scrn_device_str = "Primary Device";
    /* Make sure we use depth 4 for vga driver and depth 8 for direct */
    /* Just to get a usable 640x480 display */
    if (haveVGA != -1) 
	ptr->scrn_defaultdepth = 4;
    else
	ptr->scrn_defaultdepth = 8;

    {
	XF86ConfDisplayPtr display;
	display = xf86confmalloc(sizeof(XF86ConfDisplayRec));
    	memset((XF86ConfDisplayPtr)display,0,sizeof(XF86ConfDisplayRec));
	/* Make sure we use depth 4 for vga driver and depth 8 for direct */
	/* Just to get a usable 640x480 display */
	if (haveVGA != -1) 
	    display->disp_depth = 4;
	else
	    display->disp_depth = 8;
	display->disp_mode_lst = 
	display->list.next = NULL;
	ptr->scrn_display_lst = (XF86ConfDisplayPtr)addListItem(
				     (glp)ptr->scrn_display_lst, (glp)display);
	{
	    XF86ModePtr mode;
	    mode = xf86confmalloc(sizeof(XF86ModeRec));
    	    memset((XF86ModePtr)mode,0,sizeof(XF86ModeRec));
	    mode->mode_name = "640x480";
	    mode->list.next = NULL;
	    ptr->scrn_display_lst->disp_mode_lst = (XF86ModePtr)addListItem(
			  (glp)ptr->scrn_display_lst->disp_mode_lst, (glp)mode);
	}
    }

    return ptr;
}

XF86ConfDevicePtr
configureDeviceSection (char *driver)
{
    Bool foundFBDEV = FALSE;
    parsePrologue (XF86ConfDevicePtr, XF86ConfDeviceRec)

    /* Crude mechanism to auto-detect fbdev (os dependent) */
    {
	int fd;
	fd = open("/dev/fb0", 0);
	if (fd != -1) {
	    foundFBDEV = TRUE;
	    close(fd);
	}
    }

    ptr->dev_identifier = "Primary Device";
    ptr->dev_driver = driver;
    ptr->dev_chipid = -1;
    ptr->dev_chiprev = -1;
    ptr->dev_irq = -1;
    if (foundFBDEV) {
	XF86OptionPtr fbdev;
    	fbdev = xf86confmalloc(sizeof(XF86OptionRec));
    	memset((XF86OptionPtr)fbdev,0,sizeof(XF86OptionRec));
    	fbdev->opt_name = "UseFBDev";
	fbdev->opt_val = "ON";
	ptr->dev_option_lst = (XF86OptionPtr)addListItem(
					(glp)ptr->dev_option_lst, (glp)fbdev);
    }

    return ptr;
}

XF86ConfLayoutPtr
configureLayoutSection (void)
{
    parsePrologue (XF86ConfLayoutPtr, XF86ConfLayoutRec)

    return NULL;
#if 0
    return ptr;
#endif
}

XF86ConfModesPtr
configureModesSection (void)
{
    parsePrologue (XF86ConfModesPtr, XF86ConfModesRec)

    return ptr;
}

XF86ConfVideoAdaptorPtr
configureVideoAdaptorSection (void)
{
    parsePrologue (XF86ConfVideoAdaptorPtr, XF86ConfVideoAdaptorRec)

    return NULL;
#if 0
    return ptr;
#endif
}

XF86ConfFlagsPtr
configureFlagsSection (void)
{
    parsePrologue (XF86ConfFlagsPtr, XF86ConfFlagsRec)

    return ptr;
}

XF86ConfModulePtr
configureModuleSection (void)
{
    parsePrologue (XF86ConfModulePtr, XF86ConfModuleRec)

    return ptr;
}

XF86ConfFilesPtr
configureFilesSection (void)
{
    parsePrologue (XF86ConfFilesPtr, XF86ConfFilesRec)

    return ptr;
}

XF86ConfMonitorPtr
configureMonitorSection (void)
{
    parsePrologue (XF86ConfMonitorPtr, XF86ConfMonitorRec)

    ptr->mon_identifier = "Primary Monitor";
    ptr->mon_vendor = "Monitor Vendor";
    ptr->mon_modelname = "Monitor Model";

    /* Set monitor for allowable 640x480@25MHz */
    ptr->mon_n_hsync = 1;
    ptr->mon_hsync[0].lo = 28;
    ptr->mon_hsync[0].hi = 33;
    ptr->mon_n_vrefresh = 1;
    ptr->mon_vrefresh[0].lo = 43;
    ptr->mon_vrefresh[0].hi = 72;

    return ptr;
}

void
DoConfigure()
{
    int i;
    Bool probeResult = FALSE;
    char *foundDriver = NULL;
    XF86ConfigPtr xf86config = NULL;
    char **list, **l;

#ifdef XFree86LOADER
    /* Find the list of video driver modules. */
    const char *subdirs[] = {
	"drivers",
	NULL
    };
    const char *patlist[] = {
	"(.*)_drv\\.so",
	"(.*)_drv\\.o",
	NULL
    };

    list = LoaderListDirs(subdirs, NULL);
    if (list) {
	ErrorF("List of video driver modules:\n");
	for (l = list; *l; l++)
	    ErrorF("\t%s\n", *l);
    } else {
	ErrorF("No video driver modules found\n");
    }

    /* Load all the drivers that were found. */
    xf86LoadModules(list, NULL);
#endif /* XFree86LOADER */

    /* Disable PCI devices */
    xf86AccessInit();

    l = list;
    /* Call all of the probe functions, reporting the results. */
    for (i = 0; i < xf86NumDrivers; i++) {
	probeResult = FALSE;
	
	/* We don't allow vga as we want direct support */
	/* Then fallback later if no driver found */
	if (strcmp(*l,"vga")) {
	    if (xf86DriverList[i]->Probe != NULL)
	    	probeResult = xf86DriverList[i]->Probe(xf86DriverList[i],
						   PROBE_DETECT);
	} else {
	    haveVGA = i;
	}
	
	/* Bail when we find the primary card ! */
	if (probeResult) {
	    ErrorF("We have found a %s driver\n",*l);
	    if (foundDriver == NULL) {
	        foundDriver = *l;
	        if (xf86DriverList[i]->Identify != NULL)
	    	    xf86DriverList[i]->Identify(0);
	    }
	    break;
	}

	l++;
    }

    /* Try vga driver if we haven't found any direct modules */
    if ((haveVGA != -1) && (foundDriver == NULL)) {
	probeResult = FALSE;
	
	if (xf86DriverList[haveVGA]->Probe != NULL)
	    	probeResult = 
			xf86DriverList[haveVGA]->Probe(xf86DriverList[haveVGA],
						   PROBE_DETECT);
	
	/* Bail when we find the primary card ! */
	if (probeResult) {
	    ErrorF("Failed to find a direct driver but....\n");
	    ErrorF("We have found a vga driver\n");
	    foundDriver = "vga";
	    if (xf86DriverList[haveVGA]->Identify != NULL)
	        xf86DriverList[haveVGA]->Identify(0);
	}
    }

    if ((haveVGA == -1) && (foundDriver == NULL)) {
	ErrorF("Unable to configure XFree86 - no able drivers found.\n");
	goto bail;
    }

    /* Let's write the config file now ! */
    xf86config = malloc(sizeof(XF86ConfigRec));
    memset ((XF86ConfigPtr)xf86config, 0, sizeof(XF86ConfigRec));
    xf86config->conf_monitor_lst = configureMonitorSection();
    xf86config->conf_files = configureFilesSection();
    xf86config->conf_modules = configureModuleSection();
    xf86config->conf_flags = configureFlagsSection();
    xf86config->conf_videoadaptor_lst = configureVideoAdaptorSection();
    xf86config->conf_modes_lst = configureModesSection();
    xf86config->conf_layout_lst = configureLayoutSection();
    xf86config->conf_device_lst = configureDeviceSection(foundDriver);
    xf86config->conf_screen_lst = configureScreenSection();
    xf86config->conf_vendor_lst = configureVendorSection();
    xf86config->conf_dri = configureDRISection();
    xf86config->conf_input_lst = configureInputSection();
    xf86WriteConfigFile("/tmp/XF86Config", xf86config);

    ErrorF("\nXFree86 has configured your server for your primary card\n");
    ErrorF("\nTo test the server, try 'XFree86 -xf86config /tmp/XF86Config'\n");

bail:
#ifdef XFree86LOADER
    LoaderFreeDirList(list);
#endif /* XFree86LOADER */

    OsCleanup();
    AbortDDX();
    fflush(stderr);
    exit(0);
}
