
/* $XFree86$ */

extern Bool i128BlockCursor;
extern Bool i128ReloadCursor;

#define BLOCK_CURSOR	i128BlockCursor = TRUE;

#define UNBLOCK_CURSOR	{ \
			   if (i128ReloadCursor) \
			      i128RestoreCursor(i128savepScreen); \
			   i128BlockCursor = FALSE; \
			}
