/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tseng/tseng_cursor.h,v 1.1 1997/03/06 23:17:14 hohndel Exp $ */


/* Variables defined in tseng_cursor.c. */

extern int tsengCursorHotX;
extern int tsengCursorHotY;
extern int tsengCursorWidth;
extern int tsengCursorHeight;

/* Functions defined in tseng_cursor.c. */

extern Bool TsengCursorInit();
extern void TsengRestoreCursor();
extern void TsengWarpCursor();
extern void TsengQueryBestSize();
