/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Configure.c,v 3.4 2000/01/26 01:05:50 alanh Exp $ */
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

pciVideoPtr ConfiguredPciCard;
int ConfiguredIsaCard;
int FoundPciCards = 0;
static int haveVGA = -1;
Bool havePrimary = FALSE;

static void
GetPciCard(int id, int *vendor1, int *vendor2, int *card)
{
    int k, j;
    k = 0;
    while (xf86PCIVendorNameInfo[k].token) {
	if (xf86PCIVendorNameInfo[k].token == ConfiguredPciCard[id].vendor)
	    *vendor1 = k;
	k++;
    }
    k = 0;
    while(xf86PCIVendorInfo[k].VendorID) {
    	if (xf86PCIVendorInfo[k].VendorID == ConfiguredPciCard[id].vendor) {
	    j = 0;
	    while (xf86PCIVendorInfo[k].Device[j].DeviceName) {
	        if (xf86PCIVendorInfo[k].Device[j].DeviceID ==
					ConfiguredPciCard[id].chipType) {
		    *vendor2 = k;
		    *card = j;
		    break;
	    	}
	    	j++;
	    }
	    break;
    	}
	k++;
    }
}

XF86ConfInputPtr
configureInputSection (void)
{
    XF86ConfInputPtr mouse = NULL;
    Bool foundMouse = FALSE;
    parsePrologue (XF86ConfInputPtr, XF86ConfInputRec)

    ptr->inp_identifier = "Keyboard1";
    ptr->inp_driver = "keyboard";
    ptr->list.next = NULL;
    ptr->inp_option_lst = 
		addNewOption(ptr->inp_option_lst, "Protocol", "Standard");

/*
ErrorF("Supported Mice are : 0x%x\n",SupportedInterfaces());
*/
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
        mouse->inp_identifier = "Mouse1";
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

    ptr->vnd_identifier = "Vendor1";

    return ptr;
}

XF86ConfScreenPtr
configureScreenSection (char *driver)
{
    pciVideoPtr xf86PciCard;
    int i = 0;
    int vendor1, vendor2, card;
    parsePrologue (XF86ConfScreenPtr, XF86ConfScreenRec)

    ptr->scrn_identifier = "Screen1";
    ptr->scrn_monitor_str = "Monitor1";

    xf86PciCard = ConfiguredPciCard;
    for (i = 0; i < FoundPciCards; i++) {
	if (xf86IsPrimaryPci(xf86PciCard)) {
	    havePrimary = TRUE;
    	    GetPciCard(i, &vendor1, &vendor2, &card);
    	    ptr->scrn_device_str = xalloc(
		strlen(xf86PCIVendorNameInfo[vendor1].name) + 
		strlen(xf86PCIVendorInfo[vendor2].Device[card].DeviceName) + 2);
    	    sprintf(ptr->scrn_device_str, "%s %s",
		xf86PCIVendorNameInfo[vendor1].name, 
		xf86PCIVendorInfo[vendor2].Device[card].DeviceName);
	}
	xf86PciCard++;
    }
    if (xf86IsPrimaryIsa()) {
	havePrimary = TRUE;
	ptr->scrn_device_str = "ISA Card";
    }

    /* Make sure we use depth 4 for vga driver and depth 8 for direct */
    /* Just to get a usable 640x480 display */
    if ((driver == NULL) && (haveVGA != -1))
	ptr->scrn_defaultdepth = 4;
    else
	ptr->scrn_defaultdepth = 8;

    {
	XF86ConfDisplayPtr display;
	display = xf86confmalloc(sizeof(XF86ConfDisplayRec));
    	memset((XF86ConfDisplayPtr)display,0,sizeof(XF86ConfDisplayRec));
	/* Make sure we use depth 4 for vga driver and depth 8 for direct */
	/* Just to get a usable 640x480 display */
    	if ((driver == NULL) && (haveVGA != -1))
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
	    display->disp_mode_lst = (XF86ModePtr)addListItem(
			  (glp)display->disp_mode_lst, (glp)mode);
	}
    }

    {
	XF86ConfDisplayPtr display;
	display = xf86confmalloc(sizeof(XF86ConfDisplayRec));
    	memset((XF86ConfDisplayPtr)display,0,sizeof(XF86ConfDisplayRec));
	display->disp_depth = 15;
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
	    display->disp_mode_lst = (XF86ModePtr)addListItem(
			  (glp)display->disp_mode_lst, (glp)mode);
	}
    }

    {
	XF86ConfDisplayPtr display;
	display = xf86confmalloc(sizeof(XF86ConfDisplayRec));
    	memset((XF86ConfDisplayPtr)display,0,sizeof(XF86ConfDisplayRec));
	display->disp_depth = 16;
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
	    display->disp_mode_lst = (XF86ModePtr)addListItem(
			  (glp)display->disp_mode_lst, (glp)mode);
	}
    }

    {
	XF86ConfDisplayPtr display;
	display = xf86confmalloc(sizeof(XF86ConfDisplayRec));
    	memset((XF86ConfDisplayPtr)display,0,sizeof(XF86ConfDisplayRec));
	display->disp_depth = 24;
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
	    display->disp_mode_lst = (XF86ModePtr)addListItem(
			  (glp)display->disp_mode_lst, (glp)mode);
	}
    }

    return ptr;
}

XF86ConfDevicePtr
configureDeviceSection (char *driver, OptionInfoPtr devoptions)
{
    OptionInfoPtr p;
    pciVideoPtr xf86PciCard;
    int i = 0;
    int vendor1, vendor2, card;
    Bool foundFBDEV = FALSE;
    parsePrologue (XF86ConfDevicePtr, XF86ConfDeviceRec)

    ptr->dev_driver = driver;
    ptr->dev_chipid = -1;
    ptr->dev_chiprev = -1;
    ptr->dev_irq = -1;

    /* Fill in the available driver options for people to use */
    ptr->dev_comment = xalloc(32 + 1);
    strcpy(ptr->dev_comment, "Available Driver options are:-\n");
    for (p = devoptions; p->name != NULL; p++) {
    	ptr->dev_comment = xrealloc(ptr->dev_comment, 
			strlen(ptr->dev_comment) + strlen(p->name) + 24 + 1);
	strcat(ptr->dev_comment, "        #Option     \"");
	strcat(ptr->dev_comment, p->name);
	strcat(ptr->dev_comment, "\"\n");
    }

    xf86PciCard = ConfiguredPciCard;
    for (i = 0; i < FoundPciCards; i++) {
	ErrorF("Card Found vendor = 0x%x, card = 0x%x, %s\n", 
			xf86PciCard->vendor, 
	  		xf86PciCard->chipType,(xf86IsPrimaryPci(xf86PciCard) ? 
			"Primary! - Configuring this one." : "Secondary.")); 
	if (xf86IsPrimaryPci(xf86PciCard)) {
	    havePrimary = TRUE;
    	    GetPciCard(i, &vendor1, &vendor2, &card);
    	    ptr->dev_identifier = xalloc(	
		strlen(xf86PCIVendorNameInfo[vendor1].name) + 
		strlen(xf86PCIVendorInfo[vendor2].Device[card].DeviceName) + 2);
    	    sprintf(ptr->dev_identifier, "%s %s",
		xf86PCIVendorNameInfo[vendor1].name, 
		xf86PCIVendorInfo[vendor2].Device[card].DeviceName);
    	    ptr->dev_busid = xalloc(9 + 1);
	    sprintf(ptr->dev_busid, "PCI:%d:%d:%d", xf86PciCard->bus,
	    				xf86PciCard->device, xf86PciCard->func);
	}
	xf86PciCard++;
    }
    if (xf86IsPrimaryIsa()) {
	    havePrimary = TRUE;
    	    ptr->dev_identifier = "ISA Card";
	    ptr->dev_busid = "ISA";
    }

    /* Crude mechanism to auto-detect fbdev (os dependent) */
    /* Skip it for now. Options list it anyway, and we can't
     * determine which screen/driver this belongs too anyway.
    {
	int fd;
	fd = open("/dev/fb0", 0);
	if (fd != -1) {
	    foundFBDEV = TRUE;
	    close(fd);
	}
    }

    if (foundFBDEV) {
	XF86OptionPtr fbdev;
    	fbdev = xf86confmalloc(sizeof(XF86OptionRec));
    	memset((XF86OptionPtr)fbdev,0,sizeof(XF86OptionRec));
    	fbdev->opt_name = "UseFBDev";
	fbdev->opt_val = "ON";
	ptr->dev_option_lst = (XF86OptionPtr)addListItem(
					(glp)ptr->dev_option_lst, (glp)fbdev);
    }
    */

    return ptr;
}

XF86ConfLayoutPtr
configureLayoutSection (void)
{
    pciVideoPtr xf86PciCard;
    int i = 0;
    int vendor1, vendor2, card;
    parsePrologue (XF86ConfLayoutPtr, XF86ConfLayoutRec)

    xf86PciCard = ConfiguredPciCard;
    for (i = 0; i < FoundPciCards; i++) {
	if (xf86IsPrimaryPci(xf86PciCard)) {
	    havePrimary = TRUE;
    	    GetPciCard(i, &vendor1, &vendor2, &card);
    	    ptr->lay_identifier = xalloc(
		strlen(xf86PCIVendorNameInfo[vendor1].name) + 
		strlen(xf86PCIVendorInfo[vendor2].Device[card].DeviceName) + 2);
    	    sprintf(ptr->lay_identifier, "%s %s",
		xf86PCIVendorNameInfo[vendor1].name, 
		xf86PCIVendorInfo[vendor2].Device[card].DeviceName);
	}
	xf86PciCard++;
    }
    if (xf86IsPrimaryIsa()) {
	havePrimary = TRUE;
	ptr->lay_identifier = "ISA Card";
    }

    { 
	XF86ConfAdjacencyPtr aptr;

	aptr = xf86confmalloc (sizeof (XF86ConfAdjacencyRec));
	aptr->list.next = NULL;
	aptr->adj_x = 0;
	aptr->adj_y = 0;
	aptr->adj_refscreen = NULL;
	aptr->adj_scrnum = 1;
	aptr->adj_screen_str = "Screen1";
	aptr->adj_where = CONF_ADJ_ABSOLUTE;
    	ptr->lay_adjacency_lst = (XF86ConfAdjacencyPtr)
			addListItem ((glp) ptr->lay_adjacency_lst, (glp) aptr);
    }

    {
	XF86ConfInputrefPtr iptr;

	iptr = xf86confmalloc (sizeof (XF86ConfInputrefRec));
	iptr->list.next = NULL;
	iptr->iref_option_lst = NULL;
	iptr->iref_inputdev_str = "Mouse1";
	iptr->iref_option_lst =
		addNewOption (iptr->iref_option_lst, "CorePointer", NULL);
	ptr->lay_input_lst = (XF86ConfInputrefPtr)
		addListItem ((glp) ptr->lay_input_lst, (glp) iptr);
    }

    {
	XF86ConfInputrefPtr iptr;

	iptr = xf86confmalloc (sizeof (XF86ConfInputrefRec));
	iptr->list.next = NULL;
	iptr->iref_option_lst = NULL;
	iptr->iref_inputdev_str = "Keyboard1";
	iptr->iref_option_lst =
		addNewOption (iptr->iref_option_lst, "CoreKeyboard", NULL);
	ptr->lay_input_lst = (XF86ConfInputrefPtr)
		addListItem ((glp) ptr->lay_input_lst, (glp) iptr);
    }

    return ptr;
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
    char **elist, **el;
#ifdef XFree86LOADER
    /* Find the list of extension modules. */
    const char *esubdirs[] = {
	"extensions",
	NULL
    };
#endif
    parsePrologue (XF86ConfModulePtr, XF86ConfModuleRec)

#ifdef XFree86LOADER
    elist = LoaderListDirs(esubdirs, NULL);
    if (elist) {
	for (el = elist; *el; el++) {
	    XF86LoadPtr module;
    	    module = xf86confmalloc(sizeof(XF86LoadRec));
    	    memset((XF86LoadPtr)module,0,sizeof(XF86LoadRec));
    	    module->load_name = *el;
	    ptr->mod_load_lst = (XF86LoadPtr)addListItem(
					(glp)ptr->mod_load_lst, (glp)module);
    	}
    }
#endif

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

    ptr->mon_identifier = "Monitor1";
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
    Bool probeResultPci = FALSE;
    Bool probeResultIsa = FALSE;
    char *foundDriver = NULL;
    OptionInfoPtr options = NULL;
    XF86ConfigPtr xf86config = NULL;
    char **vlist, **ilist, **vl, **il;

#ifdef XFree86LOADER
    /* Find the list of video driver modules. */
    const char *vsubdirs[] = {
	"drivers",
	NULL
    };
    const char *isubdirs[] = {
	"input",
	NULL
    };

    vlist = LoaderListDirs(vsubdirs, NULL);
    ilist = LoaderListDirs(isubdirs, NULL);
    if (vlist) {
	ErrorF("List of video driver modules:\n");
	for (vl = vlist; *vl; vl++)
	    ErrorF("\t%s\n", *vl);
    } else {
	ErrorF("No video driver modules found\n");
    }
    if (ilist) {
	ErrorF("List of input modules:\n");
	for (il = ilist; *il; il++)
	    ErrorF("\t%s\n", *il);
    } else {
	ErrorF("No input modules found\n");
    }

    /* Load all the drivers that were found. */
    xf86LoadModules(vlist, NULL);
    xf86LoadModules(ilist, NULL);
#endif

    /* Disable PCI devices */
    xf86AccessInit();
    xf86FindPrimaryDevice();
 
    vl = vlist;
    /* Call all of the probe functions, reporting the results. */
    for (i = 0; i < xf86NumDrivers; i++) {
	probeResultPci = FALSE;
	probeResultIsa = FALSE;
	
	/* We don't allow vga as we want direct support */
	/* Then fallback later if no driver found */
	if (strcmp(*vl,"vga")) {
	    if (xf86DriverList[i]->Probe != NULL)
	    	probeResultPci = xf86DriverList[i]->Probe(xf86DriverList[i],
						   PROBE_DETECTPCI);
		if (!probeResultPci)
	    	    probeResultIsa = xf86DriverList[i]->Probe(xf86DriverList[i],
						   PROBE_DETECTISA);
		
	} else {
	    haveVGA = i;
	}
	
	/* Bail when we find the primary card ! */
	if (probeResultPci) {
	    ErrorF("We have found a PCI %s driver\n",*vl);
	    if (foundDriver == NULL) {
	        foundDriver = *vl;
	        if (xf86DriverList[i]->Identify != NULL)
	    	    xf86DriverList[i]->Identify(0);
		if (xf86DriverList[i]->AvailableOptions != NULL)
		    options = xf86DriverList[i]->AvailableOptions(
				(ConfiguredPciCard->vendor << 16) | 
				 ConfiguredPciCard->chipType, BUS_PCI);
	    }
	} else
	if (probeResultIsa)  {
	    ErrorF("We have found an ISA %s driver\n",*vl);
	    if (foundDriver == NULL) {
	        foundDriver = *vl;
	        if (xf86DriverList[i]->Identify != NULL)
	    	    xf86DriverList[i]->Identify(0);
		if (xf86DriverList[i]->AvailableOptions != NULL)
		    options = xf86DriverList[i]->AvailableOptions(
				 ConfiguredIsaCard, BUS_ISA);
	    }
	}

	vl++;
    }

    /* Try vga driver if we haven't found any direct modules */
    if ((haveVGA != -1) && (foundDriver == NULL)) {
	probeResultPci = FALSE;
	probeResultIsa = FALSE;
	
	if (xf86DriverList[haveVGA]->Probe != NULL) {
	    	probeResultPci = 
			xf86DriverList[haveVGA]->Probe(xf86DriverList[haveVGA],
						   PROBE_DETECTPCI);
		if (!probeResultPci)
			xf86DriverList[haveVGA]->Probe(xf86DriverList[haveVGA],
						   PROBE_DETECTISA);
	} 
	
	/* Bail when we find the primary card ! */
	if (probeResultPci) {
	    ErrorF("Failed to find a direct driver but....\n");
	    ErrorF("We have found a PCI vga driver\n");
	    foundDriver = "vga";
	    if (xf86DriverList[haveVGA]->Identify != NULL)
	        xf86DriverList[haveVGA]->Identify(0);
	    if (xf86DriverList[haveVGA]->AvailableOptions != NULL)
		options = xf86DriverList[i]->AvailableOptions((
				ConfiguredPciCard->vendor << 16) | 
				ConfiguredPciCard->chipType, BUS_PCI);
	}
	if (probeResultIsa) {
	    ErrorF("Failed to find a direct driver but....\n");
	    ErrorF("We have found an ISA vga driver\n");
	    foundDriver = "vga";
	    if (xf86DriverList[haveVGA]->Identify != NULL)
	        xf86DriverList[haveVGA]->Identify(0);
	    if (xf86DriverList[haveVGA]->AvailableOptions != NULL)
		options = xf86DriverList[i]->AvailableOptions(
				ConfiguredIsaCard, BUS_ISA);
	}
    }

    if ((haveVGA == -1) && (foundDriver == NULL)) {
	ErrorF("Unable to configure XFree86 - no able drivers found.\n");
	goto bail;
    }

    if (!havePrimary) {
	ErrorF("Unable to configure XFree86 - Primary card driver not found.\n");
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
    xf86config->conf_device_lst = configureDeviceSection(foundDriver, options);
    xf86config->conf_screen_lst = configureScreenSection(foundDriver);
    xf86config->conf_vendor_lst = configureVendorSection();
    xf86config->conf_dri = configureDRISection();
    xf86config->conf_input_lst = configureInputSection();
    xf86WriteConfigFile("/tmp/XF86Config", xf86config);

    ErrorF("\nXFree86 has configured your server for your primary card\n");
    ErrorF("\nTo test the server, try 'XFree86 -xf86config /tmp/XF86Config'\n");

bail:
#ifdef XFree86LOADER
    LoaderFreeDirList(vlist);
    LoaderFreeDirList(ilist);
#endif /* XFree86LOADER */

    OsCleanup();
    AbortDDX();
    fflush(stderr);
    exit(0);
}
