/* $XFree86$ */
/*
 * Copyright 1999 by Joseph V. Moss <joe@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Joseph Moss not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Joseph Moss makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * JOSEPH MOSS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL JOSEPH MOSS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */


/*

  This file contains Tcl bindings to the XF86Config file read/write routines

 */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Parser.h"
#include "xf86tokens.h"

#include "tcl.h"
#include "xfsconf.h"

#if NeedVarargsPrototypes
#include <stdarg.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

static int putsection_files   (Tcl_Interp *interp, char *varpfx);
static int putsection_module  (Tcl_Interp *interp, char *varpfx);
static int putsection_flags   (Tcl_Interp *interp, char *varpfx);
static int putsection_keyboard(Tcl_Interp *interp, char *varpfx);
static int putsection_pointer (Tcl_Interp *interp, char *varpfx);
static int putsection_vidadptr(Tcl_Interp *interp, char *varpfx);
static int putsection_modes   (Tcl_Interp *interp, char *varpfx);
static int putsection_monitor (Tcl_Interp *interp, char *varpfx);
static int putsection_device  (Tcl_Interp *interp, char *varpfx);
static int putsection_screen  (Tcl_Interp *interp, char *varpfx);
static int putsection_layout  (Tcl_Interp *interp, char *varpfx);
static int putsection_vendor  (Tcl_Interp *interp, char *varpfx);

static int write_options(
  Tcl_Interp *interp,
  char *section,
  char *idxname,
  XF86OptionPtr *ptr2optptr
);

static int write_modes(
  Tcl_Interp *interp,
  char *section,
  char *idxname,
  XF86ConfModeLinePtr *ptr2modeptr
);

static void
set_key_type(
  XF86ConfKeyboardPtr kptr,
  int keyidx,
  char *str
);

/*
   Implements the xf86config_writefile command which writes
   values to the XF86Config file
*/

int
TCL_XF86WriteXF86Config(clientData, interp, argc, argv)
    ClientData	clientData;
    Tcl_Interp	*interp;
    int		argc;
    char	*argv[];
{
	if (argc != 3) {
		Tcl_SetResult(interp,
			"Usage: xf86config_writefile filename varprefix" , TCL_STATIC);
		return TCL_ERROR;
	}

	config_list = (XF86ConfigPtr) XtMalloc(sizeof(XF86ConfigRec));
	memset(config_list, 0, sizeof(XF86ConfigRec));

	putsection_files   (interp, argv[2]);
	putsection_module  (interp, argv[2]);
	putsection_flags   (interp, argv[2]);
	putsection_keyboard(interp, argv[2]);
	putsection_pointer (interp, argv[2]);
	putsection_vidadptr(interp, argv[2]);
	putsection_modes   (interp, argv[2]);
	putsection_monitor (interp, argv[2]);
	putsection_device  (interp, argv[2]);
	putsection_screen  (interp, argv[2]);
	putsection_layout  (interp, argv[2]);
	putsection_vendor  (interp, argv[2]);

	if (xf86WriteConfigFile(argv[1], config_list) == 0) {
		Tcl_SetResult(interp,
			"Unable to write file" , TCL_STATIC);
		return TCL_ERROR;
	}
	return TCL_OK;
}

#define SETSTR(ptr, idx) \
		{ char *p=Tcl_GetVar2(interp,section,idx,0); \
		  ptr=XtRealloc(ptr,strlen(p)+1); strcpy(ptr, p); }
#define SETSTRORNULL(ptr, idx) \
		{ char *p=Tcl_GetVar2(interp,section,idx,0); \
		  if (p && strlen(p)) { \
		    ptr=XtRealloc(ptr,strlen(p)+1); strcpy(ptr, p); \
		  } else ptr=NULL;   }
#define SETINT(var, idx) \
		{ char *p=Tcl_GetVar2(interp,section,idx,0); \
		  if (p && strlen(p)) Tcl_GetInt(interp, p, &(var));  }
#define SETBOOL(var, idx) \
		{ char *p=Tcl_GetVar2(interp,section,idx,0); \
		  var = 0; if (p && strlen(p)) var = 1;  }

#define SETnSTR(ptr, idx) \
		{ char *p=Tcl_GetVar2(interp,namebuf,idx,0); \
		  ptr=XtRealloc(ptr,strlen(p)+1); strcpy(ptr, p);  }
#define SETnSTRORNULL(ptr, idx)	\
		{ char *p=Tcl_GetVar2(interp,namebuf,idx,0); \
		  if (p && strlen(p)) { \
		    ptr=XtRealloc(ptr,strlen(p)+1); strcpy(ptr, p); \
		  } else ptr=NULL;   }
#define SETnINT(var, idx) \
		{ char *p=Tcl_GetVar2(interp,namebuf,idx,0); \
		  if (p && strlen(p)) Tcl_GetInt(interp, p, &(var));  }
#define SETnBOOL(var, idx) \
		{ char *p=Tcl_GetVar2(interp,namebuf,idx,0); \
		  var = 0; if (p && strlen(p)) var = 1;  }

static int
putsection_files(interp, varpfx)
  Tcl_Interp *interp;
  char *varpfx;
{
	char section[128];
	XF86ConfFilesPtr fptr;

	fprintf(stderr, "putsection_files(%x, %s)\n", interp, varpfx);
	SECTION_NAME("Files");

	fptr = (XF86ConfFilesPtr) XtMalloc(sizeof(XF86ConfFilesRec));
	config_list->conf_files = fptr;
	memset(fptr, 0, sizeof(XF86ConfFilesRec));
	SETSTR(fptr->file_logfile,	"LogFile");
	SETSTR(fptr->file_fontpath,	"FontPath");
	SETSTR(fptr->file_rgbpath,	"RGBPath");
	SETSTR(fptr->file_modulepath,	"ModulePath");
	return TCL_OK;
}

static int
putsection_module(interp, varpfx)
  Tcl_Interp *interp;
  char *varpfx;
{
	char section[128], buf[160], *tmpbuf, *tmpptr, **idxv;
	int i, idxc;
	XF86LoadPtr lptr, prev;

	fprintf(stderr, "putsection_module(%x, %s)\n", interp, varpfx);
	SECTION_NAME("Module");

	config_list->conf_modules =
		(XF86ConfModulePtr) XtMalloc(sizeof(XF86ConfModuleRec));
	memset(config_list->conf_modules, 0, sizeof(XF86ConfModuleRec));
	lptr = config_list->conf_modules->mod_load_lst = NULL;

	strcpy(buf, "array names "); strcat(buf, section);
	if (Tcl_Eval(interp, buf) != TCL_OK)
		return TCL_ERROR;

	tmpbuf = XtMalloc(strlen(interp->result)+1);
	strcpy(tmpbuf, interp->result);
	if (Tcl_SplitList(interp, tmpbuf, &idxc, &idxv) != TCL_OK)
		return TCL_ERROR;
	
	prev = (XF86LoadPtr) 0;
	for (i = 0; i < idxc; i++) {
		if (strstr(idxv[i], "_Opt:") || strstr(idxv[i], ":Options"))
			continue;
		lptr = (XF86LoadPtr) XtMalloc(sizeof(XF86LoadRec));
		memset(lptr, 0, sizeof(XF86LoadRec));
		if (prev == (XF86LoadPtr) 0) {
			config_list->conf_modules->mod_load_lst = lptr;
		} else {
			prev->list.next = lptr;
		}
		lptr->load_name = idxv[i];
		tmpptr = Tcl_GetVar2(interp, section, idxv[i], 0);
		if (!NameCompare(tmpptr, "driver")) {
			lptr->load_type = 1;
		} else {
			lptr->load_type = 0;
		}
		write_options(interp, section, idxv[i], &(lptr->load_opt));
		prev = lptr;
	}
	XtFree(tmpbuf);
	return TCL_OK;
}

static int
putsection_flags(interp, varpfx)
  Tcl_Interp *interp;
  char *varpfx;
{
	char section[128];

	fprintf(stderr, "putsection_flags(%x, %s)\n", interp, varpfx);
	SECTION_NAME("Server");

	if (Tcl_GetVar2(interp, section, "Options", 0) != NULL) {
	    config_list->conf_flags =
		(XF86ConfFlagsPtr) XtMalloc(sizeof(XF86ConfFlagsRec));
	    memset(config_list->conf_flags, 0, sizeof(XF86ConfFlagsRec));
	    write_options(interp, section, NULL,
		&(config_list->conf_flags->flg_option_lst));
	}
	return TCL_OK;
}

static int
putsection_keyboard(interp, varpfx)
  Tcl_Interp *interp;
  char *varpfx;
{
	char section[128], *ptr, *delayptr, *rateptr;
	long val;
	XF86ConfKeyboardPtr kptr;

	fprintf(stderr, "putsection_keyboard(%x, %s)\n", interp, varpfx);
	SECTION_NAME("Keyboard");
	kptr = (XF86ConfKeyboardPtr)  XtMalloc(sizeof(XF86ConfKeyboardRec));
	config_list->conf_keyboard = kptr;
	memset(kptr, 0, sizeof(XF86ConfKeyboardRec));
	SETSTR (kptr->keyb_protocol,		"Protocol");
	SETBOOL(kptr->keyb_serverNumLock,	"ServerNumLock");
	delayptr = Tcl_GetVar2(interp, section, "AutoRepeat_delay", 0);
	rateptr  = Tcl_GetVar2(interp, section, "AutoRepeat_rate", 0);
	if (delayptr || rateptr) {
		Tcl_GetInt(interp, delayptr, &kptr->keyb_kbdDelay);
		Tcl_GetInt(interp, rateptr,  &kptr->keyb_kbdRate);
	}
	ptr = Tcl_GetVar2(interp, section, "XLeds", 0);
	while (ptr && *ptr != '\0') {
		val = strtoul(ptr, &ptr, 0);
		if (val > sizeof(long)*8)
			continue;
		kptr->keyb_xleds |= 1L << (val - 1);
	}
	SETSTRORNULL(kptr->keyb_vtinit,		"VTInit");
#ifdef USE_VT_SYSREQ
	SETBOOL(kptr->keyb_vtSysreq,		"VTSysReq");
#endif
	ptr = Tcl_GetVar2(interp, section, "LeftAlt", 0);
	if (ptr) set_key_type(kptr, CONF_K_INDEX_LEFTALT, ptr);
	ptr = Tcl_GetVar2(interp, section, "RightAlt", 0);
	if (ptr) set_key_type(kptr, CONF_K_INDEX_RIGHTALT, ptr);
	ptr = Tcl_GetVar2(interp, section, "ScrollLock", 0);
	if (ptr) set_key_type(kptr, CONF_K_INDEX_SCROLLLOCK, ptr);
	ptr = Tcl_GetVar2(interp, section, "RightCtl", 0);
	if (ptr) set_key_type(kptr, CONF_K_INDEX_RIGHTCTL, ptr);
#ifdef XKB
	SETBOOL(kptr->keyb_xkbDisable,		"XkbDisable");
	SETSTRORNULL(kptr->keyb_xkbkeycodes,	"XkbKeycodes");
	SETSTRORNULL(kptr->keyb_xkbtypes,	"XkbTypes");
	SETSTRORNULL(kptr->keyb_xkbcompat,	"XkbCompat");
	SETSTRORNULL(kptr->keyb_xkbsymbols,	"XkbSymbols");
	SETSTRORNULL(kptr->keyb_xkbgeometry,	"XkbGeometry");
	SETSTRORNULL(kptr->keyb_xkbkeymap,	"XkbKeymap");
	SETSTRORNULL(kptr->keyb_xkbrules,	"XkbRules");
	SETSTRORNULL(kptr->keyb_xkbmodel,	"XkbModel");
	SETSTRORNULL(kptr->keyb_xkblayout,	"XkbLayout");
	SETSTRORNULL(kptr->keyb_xkbvariant,	"XkbVariant");
	SETSTRORNULL(kptr->keyb_xkboptions,	"XkbOptions");
#endif
#if defined(SVR4) && defined(i386) && defined(PC98)
	SETBOOL(kptr->keyb_panix106,		"Panix106");
#endif
	return TCL_OK;
}

static int
putsection_pointer(interp, varpfx)
  Tcl_Interp *interp;
  char *varpfx;
{
	char section[128], *ptr;
	XF86ConfPointerPtr pptr;

	fprintf(stderr, "putsection_pointer(%x, %s)\n", interp, varpfx);
	SECTION_NAME("Pointer");
	pptr = (XF86ConfPointerPtr) XtMalloc(sizeof(XF86ConfPointerRec));
	config_list->conf_pointer = pptr;
	memset(pptr, 0, sizeof(XF86ConfPointerRec));
	SETSTR (pptr->pntr_protocol,		"Protocol");
#ifndef OSMOUSE_ONLY
	SETSTR (pptr->pntr_device,		"Device");
	SETINT (pptr->pntr_baudrate,		"BaudRate");
	SETINT (pptr->pntr_samplerate,		"SampleRate");
	SETINT (pptr->pntr_resolution,		"Resolution");
	SETBOOL(pptr->pntr_chordMiddle,		"ChordMiddle");
#ifdef XINPUT
	SETBOOL(pptr->pntr_alwaysCore,		"AlwaysCore");
#endif
#ifdef CLEARDTR_SUPPORT
	SETBOOL(pptr->pntr_clearDtr,		"ClearDTR");
	SETBOOL(pptr->pntr_clearRts,		"ClearRTS");
#endif
#endif /* OSMOUSE_ONLY */
	SETBOOL(pptr->pntr_emulate3Buttons,	"Emulate3Buttons");
	SETINT (pptr->pntr_emulate3Timeout,	"Emulate3Timeout");
	SETINT (pptr->pntr_buttons,		"Buttons");
	ptr = Tcl_GetVar2(interp, section, "ZAxisPosMapping", 0);
	if (!strlen(ptr))
		return TCL_OK;
	if (!strcmp(ptr, "X")) {
		pptr->pntr_negativeZ = CONF_ZAXIS_MAPTOX;
		pptr->pntr_positiveZ = CONF_ZAXIS_MAPTOX;
	} else if (!strcmp(ptr, "Y")) {
		pptr->pntr_negativeZ = CONF_ZAXIS_MAPTOY;
		pptr->pntr_positiveZ = CONF_ZAXIS_MAPTOY;
	} else {
		SETINT(pptr->pntr_negativeZ, "ZAxisNegMapping");
		SETINT(pptr->pntr_positiveZ, "ZAxisPosMapping");
	}
	return TCL_OK;
}

static int
putsection_monitor(interp, varpfx)
  Tcl_Interp *interp;
  char *varpfx;
{
	char section[128], namebuf[128], *ptr, **idxv;
	int i, idxc;
	XF86ConfMonitorPtr mptr, prev;

	fprintf(stderr, "putsection_monitor(%x, %s)\n", interp, varpfx);
	SECTION_NAME("Monitor");
	ptr = Tcl_GetVar2(interp, section, "Identifiers", 0);
	if (Tcl_SplitList(interp, ptr, &idxc, &idxv) != TCL_OK)
		return TCL_ERROR;
	
	prev = (XF86ConfMonitorPtr) 0;
	for (i = 0; i < idxc; i++) {
		mptr = (XF86ConfMonitorPtr) XtMalloc(sizeof(XF86ConfMonitorRec));
		memset(mptr, 0, sizeof(XF86ConfMonitorRec));
		snprintf(namebuf, 128, "%s_%s", section, idxv[i]);
		if (prev == (XF86ConfMonitorPtr) 0) {
			config_list->conf_monitor_lst = mptr;
		} else {
			prev->list.next = mptr;
		}
		mptr->mon_identifier = idxv[i];
		SETnSTR(mptr->mon_vendor,	"VendorName");
		SETnSTR(mptr->mon_modelname,	"ModelName");
		SETnINT(mptr->mon_width,	"Width");
		SETnINT(mptr->mon_height,	"Height");
		/* XXX mon_n{hsync,vrefresh} mon_{hsync,vrefresh} */
		/* XXX mon_gamma_{red,green,blue} */
		write_modes  (interp, namebuf, NULL, &(mptr->mon_modeline_lst));
		write_options(interp, namebuf, NULL, &(mptr->mon_option_lst));
		/* XXX mon_modes_sect_lst */
		prev = mptr;
	}
	return TCL_OK;
}

static int
putsection_device(interp, varpfx)
  Tcl_Interp *interp;
  char *varpfx;
{
	char section[128], namebuf[128], *ptr, **idxv, **av;
	int i, j, idxc, ac, n;
	XF86ConfDevicePtr dptr, prev;

	fprintf(stderr, "putsection_device(%x, %s)\n", interp, varpfx);
	SECTION_NAME("Device");
	ptr = Tcl_GetVar2(interp, section, "Identifiers", 0);
	if (Tcl_SplitList(interp, ptr, &idxc, &idxv) != TCL_OK)
		return TCL_ERROR;
	
	prev = (XF86ConfDevicePtr) 0;
	for (i = 0; i < idxc; i++) {
		dptr = (XF86ConfDevicePtr) XtMalloc(sizeof(XF86ConfDeviceRec));
		memset(dptr, 0, sizeof(XF86ConfDeviceRec));
		snprintf(namebuf, 128, "%s_%s", section, idxv[i]);
		if (prev == (XF86ConfDevicePtr) 0) {
			config_list->conf_device_lst = dptr;
		} else {
			prev->list.next = dptr;
		}
		dptr->dev_identifier = idxv[i];
		SETnSTR(dptr->dev_vendor,	"VendorName");
		SETnSTR(dptr->dev_board,	"BoardName");
		SETnSTR(dptr->dev_chipset,	"Chipset");
		SETnSTR(dptr->dev_busid,	"BusID");
		SETnSTR(dptr->dev_card,		"Card");
		SETnSTR(dptr->dev_driver,	"Driver");
		SETnSTR(dptr->dev_ramdac,	"Ramdac");
		ptr = Tcl_GetVar2(interp, namebuf, "DacSpeed", 0);
		if (strlen(ptr)) {
		    if (Tcl_SplitList(interp, ptr, &ac, &av) != TCL_OK)
			return TCL_ERROR;
		    for (j = 0; j < ac; j++) {
			Tcl_GetInt(interp, av[j], &n);
			dptr->dev_dacSpeeds[j] = n * 1000;
		    }
		}
		SETnINT(dptr->dev_videoram,	"Videoram");
		/* XXX dev_textclockfreq, dev_memclk */
		ptr = Tcl_GetVar2(interp, namebuf, "BIOSBase", 0);
		dptr->dev_bios_base = strtoul(ptr, NULL, 16);
		ptr = Tcl_GetVar2(interp, namebuf, "MemBase", 0);
		dptr->dev_mem_base = strtoul(ptr, NULL, 16);
		ptr = Tcl_GetVar2(interp, namebuf, "IOBase", 0);
		dptr->dev_io_base = strtoul(ptr, NULL, 16);
		SETnSTR(dptr->dev_clockchip,	"ClockChip");
		ptr = Tcl_GetVar2(interp, namebuf, "Clocks", 0);
		if (strlen(ptr)) {
		    if (Tcl_SplitList(interp, ptr, &ac, &av) != TCL_OK)
			return TCL_ERROR;
		    dptr->dev_clocks = ac;
		    for (j = 0; j < ac; j++) {
			Tcl_GetInt(interp, av[j], &n);
			dptr->dev_clock[j] = n * 1000;
		    }
		}
		ptr = Tcl_GetVar2(interp, namebuf, "ChipID", 0);
		dptr->dev_chipid = strtoul(ptr, NULL, 16);
		ptr = Tcl_GetVar2(interp, namebuf, "ChipRev", 0);
		dptr->dev_chiprev = strtoul(ptr, NULL, 16);
		write_options(interp, namebuf, NULL, &(dptr->dev_option_lst));
		prev = dptr;
	}
	return TCL_OK;
}

static int
putsection_screen(interp, varpfx)
  Tcl_Interp *interp;
  char *varpfx;
{
	char section[128], *ptr, **idxv;
	int i, idxc;
	XF86ConfScreenPtr sptr, prev;

	fprintf(stderr, "putsection_screen(%x, %s)\n", interp, varpfx);
	SECTION_NAME("Screen");
	ptr = Tcl_GetVar2(interp, section, "Identifiers", 0);
	if (Tcl_SplitList(interp, ptr, &idxc, &idxv) != TCL_OK)
		return TCL_ERROR;
	
	prev = (XF86ConfScreenPtr) 0;
	for (i = 0; i < idxc; i++) {
		sptr = (XF86ConfScreenPtr) XtMalloc(sizeof(XF86ConfScreenRec));
		memset(sptr, 0, sizeof(XF86ConfScreenRec));
		if (prev == (XF86ConfScreenPtr) 0) {
			config_list->conf_screen_lst = sptr;
		} else {
			prev->list.next = sptr;
		}
		sptr->scrn_identifier = idxv[i];
		prev = sptr;
	}
	return TCL_OK;
}

static int
putsection_layout(interp, varpfx)
  Tcl_Interp *interp;
  char *varpfx;
{
	char section[128], *ptr, **idxv;
	int i, idxc;
	XF86ConfLayoutPtr lptr, prev;

	fprintf(stderr, "putsection_layout(%x, %s)\n", interp, varpfx);
	SECTION_NAME("Layout");
	ptr = Tcl_GetVar2(interp, section, "Identifiers", 0);
	if (Tcl_SplitList(interp, ptr, &idxc, &idxv) != TCL_OK)
		return TCL_ERROR;
	
	prev = (XF86ConfLayoutPtr) 0;
	for (i = 0; i < idxc; i++) {
		lptr = (XF86ConfLayoutPtr) XtMalloc(sizeof(XF86ConfLayoutRec));
		memset(lptr, 0, sizeof(XF86ConfLayoutRec));
		if (prev == (XF86ConfLayoutPtr) 0) {
			config_list->conf_layout_lst = lptr;
		} else {
			prev->list.next = lptr;
		}
		lptr->lay_identifier = idxv[i];
		prev = lptr;
	}
	return TCL_OK;
}

static int
putsection_vidadptr(interp, varpfx)
  Tcl_Interp *interp;
  char *varpfx;
{
	char section[128], namebuf[128], *ptr, **idxv;
	int i, idxc;
	XF86ConfVideoAdaptorPtr	aptr, prev;
	XF86ConfVideoPortPtr	pptr;

	fprintf(stderr, "putsection_vidadptr(%x, %s)\n", interp, varpfx);
	SECTION_NAME("VidAdaptr");
	ptr = Tcl_GetVar2(interp, section, "Identifiers", 0);
	if (Tcl_SplitList(interp, ptr, &idxc, &idxv) != TCL_OK)
		return TCL_ERROR;
	
	prev = (XF86ConfVideoAdaptorPtr) 0;
	for (i = 0; i < idxc; i++) {
		aptr = (XF86ConfVideoAdaptorPtr) XtMalloc(sizeof(XF86ConfVideoAdaptorRec));
		memset(aptr, 0, sizeof(XF86ConfVideoAdaptorRec));
		snprintf(namebuf, 128, "%s_%s", section, idxv[i]);
		if (prev == (XF86ConfVideoAdaptorPtr) 0) {
			config_list->conf_videoadaptor_lst = aptr;
		} else {
			prev->list.next = aptr;
		}
		aptr->va_identifier = idxv[i];
		SETnSTR(aptr->va_vendor,	"VendorName");
		SETnSTR(aptr->va_board,		"BoardName");
		SETnSTR(aptr->va_busid,		"BusID");
		SETnSTR(aptr->va_driver,	"Driver");
		/* XXX aptr->vp_port_lst */
		write_options(interp, namebuf, NULL, &(aptr->va_option_lst));
		prev = aptr;
	}
	return TCL_OK;
}

static int
putsection_modes(interp, varpfx)
  Tcl_Interp *interp;
  char *varpfx;
{
	char section[128], namebuf[128], *ptr, **idxv;
	int i, idxc;
	XF86ConfModesPtr	mptr, prev;

	fprintf(stderr, "putsection_modes(%x, %s)\n", interp, varpfx);
	SECTION_NAME("Modes");
	ptr = Tcl_GetVar2(interp, section, "Identifiers", 0);
	if (Tcl_SplitList(interp, ptr, &idxc, &idxv) != TCL_OK)
		return TCL_ERROR;
	
	prev = (XF86ConfModesPtr) 0;
	for (i = 0; i < idxc; i++) {
		mptr = (XF86ConfModesPtr) XtMalloc(sizeof(XF86ConfModesRec));
		memset(mptr, 0, sizeof(XF86ConfModesRec));
		snprintf(namebuf, 128, "%s_%s", section, idxv[i]);
		if (prev == (XF86ConfModesPtr) 0) {
			config_list->conf_modes_lst = mptr;
		} else {
			prev->list.next = mptr;
		}
		mptr->modes_identifier = idxv[i];
		write_modes(interp, namebuf, NULL, &(mptr->mon_modeline_lst));
		prev = mptr;
	}
	return TCL_OK;
}

static int
putsection_vendor(interp, varpfx)
  Tcl_Interp *interp;
  char *varpfx;
{
	char section[128], namebuf[128], *ptr, **idxv;
	int i, idxc;
	XF86ConfVendorPtr	vptr, prev;

	fprintf(stderr, "putsection_vendor(%x, %s)\n", interp, varpfx);
	SECTION_NAME("Vendor");
	ptr = Tcl_GetVar2(interp, section, "Identifiers", 0);
	if (Tcl_SplitList(interp, ptr, &idxc, &idxv) != TCL_OK)
		return TCL_ERROR;
	
	prev = (XF86ConfVendorPtr) 0;
	for (i = 0; i < idxc; i++) {
		vptr = (XF86ConfVendorPtr) XtMalloc(sizeof(XF86ConfVendorRec));
		memset(vptr, 0, sizeof(XF86ConfVendorRec));
		snprintf(namebuf, 128, "%s_%s", section, idxv[i]);
		if (prev == (XF86ConfVendorPtr) 0) {
			config_list->conf_vendor_lst = vptr;
		} else {
			prev->list.next = vptr;
		}
		vptr->vnd_identifier = idxv[i];
		write_options(interp, namebuf, NULL, &(vptr->vnd_option_lst));
		prev = vptr;
	}
	return TCL_OK;
}
	

static int
write_options(interp, section, idxname, ptr2optptr)
  Tcl_Interp *interp;
  char *section, *idxname;
  XF86OptionPtr *ptr2optptr;
{
	XF86OptionPtr optr, prev;
	char **optv, *tmpbuf, *optlist, *ptr, *ptr2;
	int  i, optc, idxlen;

	if (idxname) {
	    idxlen = strlen(idxname);
	    tmpbuf = XtMalloc(idxlen+9);
	    sprintf(tmpbuf, "%s:Options", idxname);
	} else {
	    idxlen = 0;
	    tmpbuf = XtMalloc(8);
	    strcpy(tmpbuf, "Options");
	}
	optlist = Tcl_GetVar2(interp, section, tmpbuf, 0);
	Tcl_SplitList(interp, optlist, &optc, &optv);

	prev = (XF86OptionPtr) 0;
	for (i = 0; i < optc; i++) {
	    ptr = XtMalloc(idxlen+strlen(optv[i])+6);
	    sprintf(ptr, "%s%sOpt:%s", StrOrNull(idxname),
		(idxname? "_": ""), optv[i]);
	    optr = (XF86OptionPtr) XtMalloc(sizeof(XF86OptionRec));
	    memset(optr, 0, sizeof(XF86OptionRec));
	    if (prev == (XF86OptionPtr) 0) {
		*ptr2optptr = optr;
	    } else {
		prev->list.next = optr;
	    }
	    optr->opt_name= ConfigStrdup(optv[i]);
	    ptr2 = Tcl_GetVar2(interp, section, ptr, 0);
	    if (strlen(ptr2)) {
		    optr->opt_val = ConfigStrdup(ptr2);
	    }
	    optr->opt_used = 0;
	    prev = optr;
	    XtFree(ptr);
	}
	XtFree(tmpbuf);
	return TCL_OK;
}

static int
write_modes(interp, section, idxname, ptr2modeptr)
  Tcl_Interp *interp;
  char *section, *idxname;
  XF86ConfModeLinePtr *ptr2modeptr;
{
	XF86ConfModeLinePtr mptr, prev;
	char **modev, *tmpbuf, *modelist, *ptr, *ptr2;
	int  i, modec, idxlen;
	float tmpclk;

	if (idxname) {
	    idxlen = strlen(idxname);
	    tmpbuf = XtMalloc(idxlen+9);
	    sprintf(tmpbuf, "%s:Modes", idxname);
	} else {
	    idxlen = 0;
	    tmpbuf = XtMalloc(8);
	    strcpy(tmpbuf, "Modes");
	}
	modelist = Tcl_GetVar2(interp, section, tmpbuf, 0);
	Tcl_SplitList(interp, modelist, &modec, &modev);

	prev = (XF86ConfModeLinePtr) 0;
	for (i = 0; i < modec; i++) {
	    ptr = XtMalloc(idxlen+strlen(modev[i])+6);
	    sprintf(ptr, "%s%sMode:%s", StrOrNull(idxname),
		(idxname? "_": ""), modev[i]);
	    mptr = (XF86ConfModeLinePtr) XtMalloc(sizeof(XF86ConfModeLineRec));
	    memset(mptr, 0, sizeof(XF86ConfModeLineRec));
	    if (prev == (XF86ConfModeLinePtr) 0) {
		*ptr2modeptr = mptr;
	    } else {
		prev->list.next = mptr;
	    }
	    mptr->ml_identifier = ConfigStrdup(modev[i]);
	    ptr2 = Tcl_GetVar2(interp, section, ptr, 0);
	    if (ptr2 && strlen(ptr2)) {
		    sscanf(ptr2, "%g %d %d %d %d %d %d %d %d",
			&tmpclk,
			&(mptr->ml_hdisplay), &(mptr->ml_hsyncstart),
			&(mptr->ml_hsyncend), &(mptr->ml_htotal),
			&(mptr->ml_vdisplay), &(mptr->ml_vsyncstart),
			&(mptr->ml_vsyncend), &(mptr->ml_vtotal));
		    mptr->ml_clock = (int) (tmpclk * 1000.0 + 0.5);
	    }
	    prev = mptr;
	    XtFree(ptr);
	}
	XtFree(tmpbuf);
	return TCL_OK;
}

static void
set_key_type(kptr, keyidx, str)
  XF86ConfKeyboardPtr kptr;
  int keyidx;
  char *str;
{
	static xf86ConfigSymTabRec keynametab[] = {
		{CONF_KM_META, "meta"},
		{CONF_KM_COMPOSE, "compose"},
		{CONF_KM_MODESHIFT, "modeshift"},
		{CONF_KM_MODELOCK, "modelock"},
		{CONF_KM_SCROLLLOCK, "scrolllock"},
		{CONF_KM_CONTROL, "control"},
		{-1, ""},
	};
	int i;

	for (i = 0; keynametab[i].token >= 0; i++) {
	    if (!NameCompare(keynametab[i].name, str))
		kptr->keyb_specialKeyMap[keyidx] = keynametab[i].token;
	}
}

