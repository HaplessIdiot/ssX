/*
 * Copyright (c) 2001 by The XFree86 Project, Inc.
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
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 * Author: Paulo César Pereira de Andrade
 */

/* $XFree86: xc/programs/xedit/lisp/modules/xaw.c,v 1.1 2001/08/31 15:00:14 paulo Exp $ */

#include <X11/Intrinsic.h>
#include <X11/Xaw/AsciiSink.h>
#include <X11/Xaw/AsciiSrc.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Grip.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/MultiSink.h>
#include <X11/Xaw/MultiSrc.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Panner.h>
#include <X11/Xaw/Porthole.h>
#include <X11/Xaw/Repeater.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Simple.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Sme.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/StripChart.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/TextSink.h>
#include <X11/Xaw/TextSrc.h>
#include <X11/Xaw/Tip.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Tree.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Vendor.h>
#include "internal.h"

/*
 * Prototypes
 */
int xawLoadModule(LispMac*);

/*
 * Initialization
 */
LispModuleData xawLispModuleData = {
    NULL,
    xawLoadModule
};

static int xawWidgetClass_t;

/*
 * Implementation
 */
int
xawLoadModule(LispMac *mac)
{
    char *fname = "INTERNAL:XAW-LOAD-MODULE";

    xawWidgetClass_t = LispRegisterOpaqueType(mac, "WidgetClass");

    GCPRO();
    (void)LispSetVariable(mac, ATOM2("asciiSinkObjectClass"),
			  OPAQUE(asciiSinkObjectClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("asciiSrcObjectClass"),
			  OPAQUE(asciiSinkObjectClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("asciiTextWidgetClass"),
			  OPAQUE(asciiTextWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("boxWidgetClass"),
			  OPAQUE(boxWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("commandWidgetClass"),
			  OPAQUE(commandWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("dialogWidgetClass"),
			  OPAQUE(dialogWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("formWidgetClass"),
			  OPAQUE(formWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("gripWidgetClass"),
			  OPAQUE(gripWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("labelWidgetClass"),
			  OPAQUE(labelWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("listWidgetClass"),
			  OPAQUE(listWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("menuButtonWidgetClass"),
			  OPAQUE(menuButtonWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("multiSinkObjectClass"),
			  OPAQUE(multiSinkObjectClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("multiSrcObjectClass"),
			  OPAQUE(multiSrcObjectClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("panedWidgetClass"),
			  OPAQUE(panedWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("pannerWidgetClass"),
			  OPAQUE(pannerWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("portholeWidgetClass"),
			  OPAQUE(portholeWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("repeaterWidgetClass"),
			  OPAQUE(repeaterWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("scrollbarWidgetClass"),
			  OPAQUE(scrollbarWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("simpleMenuWidgetClass"),
			  OPAQUE(simpleMenuWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("simpleWidgetClass"),
			  OPAQUE(simpleWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("smeBSBObjectClass"),
			  OPAQUE(smeBSBObjectClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("smeLineObjectClass"),
			  OPAQUE(smeLineObjectClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("smeObjectClass"),
			  OPAQUE(smeObjectClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("stripChartWidgetClass"),
			  OPAQUE(stripChartWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("textWidgetClass"),
			  OPAQUE(textWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("textSinkObjectClass"),
			  OPAQUE(textSinkObjectClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("textSrcObjectClass"),
			  OPAQUE(textSrcObjectClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("tipWidgetClass"),
			  OPAQUE(tipWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("toggleWidgetClass"),
			  OPAQUE(toggleWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("treeWidgetClass"),
			  OPAQUE(treeWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("viewportWidgetClass"),
			  OPAQUE(viewportWidgetClass, xawWidgetClass_t),
			  fname, 0);
    (void)LispSetVariable(mac, ATOM2("vendorShellWidgetClass"),
			  OPAQUE(vendorShellWidgetClass, xawWidgetClass_t),
			  fname, 0);
    GCUPRO();

    return (1);
}
