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
 * $XFree86$
 */

#include "config.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <X11/XKBlib.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include "keyboard-cfg.h"

/*
 * Prototypes
 */
void CreateAccessXDialog(void);

/*
 * Implementation
 */
void
startaccessx(void)
{
    InitializeKeyboard();
    XkbGetControls(DPY, XkbAllControlsMask, xkb_info->xkb);
    if (xkb_info->xkb->ctrls == NULL)
	xkb_info->xkb->ctrls = (XkbControlsPtr)
	    XtCalloc(1, sizeof(XkbControlsRec));
    xkb_info->xkb->ctrls->enabled_ctrls |= XkbMouseKeysMask |
					   XkbMouseKeysAccelMask;
    xkb_info->xkb->ctrls->mk_delay = 40;
    xkb_info->xkb->ctrls->mk_interval = 10;
    xkb_info->xkb->ctrls->mk_time_to_max = 1000;
    xkb_info->xkb->ctrls->mk_max_speed = 500;
    xkb_info->xkb->ctrls->mk_curve = 0;
    XkbSetControls(DPY, XkbAllControlsMask, xkb_info->xkb);
    UpdateKeyboard(True);
    CreateAccessXDialog();
}

void
CreateAccessXDialog()
{
    Widget shell, form;

    shell = XtVaCreatePopupShell("accessx", transientShellWidgetClass, toplevel,
				 XtNx, toplevel->core.x + toplevel->core.width,
				 XtNy, toplevel->core.y, NULL, 0);
    form = XtCreateManagedWidget("form", formWidgetClass, shell, NULL, 0);
    XtCreateManagedWidget("label", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("lock", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("div", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("mul", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("minus", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("7", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("8", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("9", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("plus", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("4", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("5", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("6", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("1", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("2", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("3", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("enter", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("0", labelWidgetClass, form, NULL, 0);
    XtCreateManagedWidget("del", labelWidgetClass, form, NULL, 0);

    XtRealizeWidget(shell);
    XtPopup(shell, XtGrabNone);
}
