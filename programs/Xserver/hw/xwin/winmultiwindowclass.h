/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:     Earle F. Philhower, III
 */
/* $XFree86$ */


/*
 * Structures
 */

typedef struct {
  long		flags;	/* marks which fields in this structure are defined */
  Bool		input;	/* does this application rely on the window manager to
		   get keyboard input? */
  int		initial_state;	/* see below */
  Pixmap	icon_pixmap;	/* pixmap to be used as icon */
  Window	icon_window; 	/* window to be used as icon */
  int		icon_x, icon_y; 	/* initial position of icon */
  Pixmap	icon_mask;	/* icon mask bitmap */
  XID		window_group;	/* id of related window group */
  /* this structure may be extended in the future */
} WinXWMHints;


/*
 * Function prototypes
 */

int
winMultiWindowGetWMHints (WindowPtr pWin, WinXWMHints *hints);

int
winMultiWindowGetClassHint (WindowPtr pWin, char **res_name, char **res_class);

int
winMultiWindowGetWindowRole (WindowPtr pWin, char **res_role);
