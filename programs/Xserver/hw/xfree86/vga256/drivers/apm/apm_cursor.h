/* $XFree86$ */

/* Variables defined in apm_cursor.c. */

extern int apmCursorHotX;
extern int apmCursorHotY;
extern int apmCursorWidth;
extern int apmCursorHeight;

/* Functions defined in apm_cursor.c. */

extern void ApmCursorInit();
extern void ApmRestoreCursor();
extern void ApmWarpCursor();
extern void ApmQueryBestSize();
