/* $XFree86$ */

extern Bool p9000BlockCursor;
extern Bool p9000ReloadCursor;

#define BLOCK_CURSOR    p9000BlockCursor = TRUE;

#define UNBLOCK_CURSOR  { \
			    if (p9000ReloadCursor) \
			       p9000RestoreCursor(p9000savepScreen); \
			    p9000BlockCursor = FALSE; \
                        }
