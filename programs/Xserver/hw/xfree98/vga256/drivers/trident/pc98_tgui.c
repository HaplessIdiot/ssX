/* $XFree86$ */

#include "X.h"
#include "input.h"
#include "screenint.h"
#include "dix.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "vga.h"
#include "t89_driver.h"
#include "vgaPCI.h"

#ifdef XFreeXDGA
#include "X.h"
#include "Xproto.h"
#include "extnsionst.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#ifdef XF86VGA16
#define MONOVGA
#endif

#ifndef MONOVGA
#include "tgui_drv.h"
#include "vga256.h"
extern vgaHWCursorRec vgaHWCursor;
#endif

pointer mmioBase = NULL;
static int hsync31;

Bool BoardInit(void);
void crtswitch(short);

Bool BoardInit(void)
{
	/* Save current horizontal sync, 1: 31.5KHz */
	hsync31 = _inb(0x9a8) & 0x01;

	_outb(0xfaa, 0x03);
	_outb(0xfab, 0xff);
	if(OFLG_ISSET(OPTION_PC98TGUI, &vga256InfoRec.options)){
		mmioBase = xf86MapVidMem(0, VGA_REGION, (pointer)(0x20400000),
					 0x10000);
	} else {
		mmioBase = xf86MapVidMem(0, VGA_REGION, (pointer)(0xFFE00000),
					 0x10000);
	}
	return TRUE;
}

void crtswitch(short crtmod)
{
	if( crtmod != 0 ){
		_outb(0x68, 0x0e);
		_outb(0x6a, 0x07);
		_outb(0x6a, 0x8f);
		_outb(0x6a, 0x06);
		if(hsync31 == 0){
			_outb(0x9a8, 0x01);	/* 24.8KHz -> 31.5KHz */
		}
		_outb(0xfaa, 0x03);
		_outb(0xfab, 0xff);
		_outb(0xfac, 0x02); /* Changing Accel output */
	} else {
		_outb(0xfac, 0x00); /* Changing Normal output */
		_outb(0xfaa, 0x03);
		_outb(0xfab, 0xfd);
		if(hsync31 == 0){
			_outb(0x9a8, 0x00);	/* 31.5KHz-> 24.8KHz */
		}
		_outb(0x6a, 0x07);
		_outb(0x6a, 0x8e);
		_outb(0x6a, 0x06);
		_outb(0x68, 0x0f);
	}
	return;
}
