/* $XFree86$ */

#ifndef _FONTUTIL_H_
#define _FONTUTIL_H_

extern int FontCouldBeTerminal(FontInfoPtr);
extern int CheckFSFormat(fsBitmapFormat, fsBitmapFormatMask, int *, int *,
			 int *, int *, int *);
extern void FontComputeInfoAccelerators(FontInfoPtr);

#endif /* _FONTUTIL_H_ */
