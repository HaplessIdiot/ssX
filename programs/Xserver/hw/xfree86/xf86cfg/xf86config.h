/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Author: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/xf86cfg/xf86config.h,v 1.1 2000/04/04 22:37:03 dawes Exp $
 */

#include "config.h"

#define xf86OptionListFree OptionListFree
#define xf86AddInput(head, ptr)		\
	(XF86ConfInputPtr)addListItem((GenericListPtr)(head), (GenericListPtr)(ptr))
#define xf86AddInputref(head, ptr)	\
	(XF86ConfInputrefPtr)addListItem((GenericListPtr)(head), (GenericListPtr)(ptr))
#define xf86AddDevice(head, ptr)	\
	(XF86ConfDevicePtr)addListItem((GenericListPtr)(head), (GenericListPtr)(ptr))
#define xf86AddMonitor(head, ptr)	\
	(XF86ConfMonitorPtr)addListItem((GenericListPtr)(head), (GenericListPtr)(ptr))
#define xf86AddScreen(head, ptr)	\
	(XF86ConfScreenPtr)addListItem((GenericListPtr)(head), (GenericListPtr)(ptr))
#define xf86AddLayout(head, ptr)	\
	(XF86ConfLayoutPtr)addListItem((GenericListPtr)(head), (GenericListPtr)(ptr))
#define xf86AddModeLine(head, ptr)	\
	(XF86ConfModeLinePtr)addListItem((GenericListPtr)(head), (GenericListPtr)(ptr))
#define xf86NewOption NewOption
#define xf86addNewOption addNewOption
#define xf86FindOption FindOption

int ErrorF(const char*, ...);
int VErrorF(const char*, va_list);
int xf86RemoveOption(XF86OptionPtr*, char*);
int xf86RemoveInput(XF86ConfigPtr, XF86ConfInputPtr);
int xf86RemoveInputRef(XF86ConfLayoutPtr, XF86ConfInputPtr);
int xf86RemoveDevice(XF86ConfigPtr, XF86ConfDevicePtr);
int xf86RemoveMonitor(XF86ConfigPtr, XF86ConfMonitorPtr);
int xf86RemoveScreen(XF86ConfigPtr, XF86ConfScreenPtr);
int xf86RemoveAdjacency(XF86ConfLayoutPtr, XF86ConfAdjacencyPtr);
int xf86RemoveInactive(XF86ConfLayoutPtr, XF86ConfInactivePtr);
int xf86RemoveLayout(XF86ConfigPtr, XF86ConfLayoutPtr);

int xf86RenameInput(XF86ConfigPtr, XF86ConfInputPtr, char*);
int xf86RenameDevice(XF86ConfigPtr, XF86ConfDevicePtr, char*);
int xf86RenameMonitor(XF86ConfigPtr, XF86ConfMonitorPtr, char*);
int xf86RenameLayout(XF86ConfigPtr, XF86ConfLayoutPtr, char*);
int xf86RenameScreen(XF86ConfigPtr, XF86ConfScreenPtr, char*);
