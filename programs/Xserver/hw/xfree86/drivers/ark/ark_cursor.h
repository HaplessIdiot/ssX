/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ark/ark_cursor.h,v 1.1 1997/03/06 23:14:40 hohndel Exp $ */





/* $XConsortium: ark_cursor.h /main/4 1996/02/21 18:02:02 kaleb $ */

/* Variables defined in ark_cursor.c. */

extern int arkCursorHotX;
extern int arkCursorHotY;
extern int arkCursorWidth;
extern int arkCursorHeight;

/* Functions defined in ark_cursor.c. */

extern Bool ArkCursorInit();
extern void ArkRestoreCursor();
extern void ArkWarpCursor();
extern void ArkQueryBestSize();
