/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tseng/tseng_cursor.h,v 1.2 1997/05/04 05:26:59 dawes Exp $ */


/* Variables defined in tseng_cursor.c. */

extern int tsengCursorHotX;
extern int tsengCursorHotY;
extern int tsengCursorWidth;
extern int tsengCursorHeight;
extern int tsengCursorAddress;

/* Functions defined in tseng_cursor.c. */

extern Bool TsengCursorInit();
extern void TsengRestoreCursor();
extern void TsengWarpCursor();
extern void TsengQueryBestSize();
