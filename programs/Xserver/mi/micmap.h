/* $XFree86: xc/programs/Xserver/mi/micmap.h,v 1.1.2.1 1998/07/19 13:22:16 dawes Exp $ */

#ifndef _MICMAP_H_
#define _MICMAP_H_

void miResolveColor(unsigned short *, unsigned short *, unsigned short *,
			VisualPtr);
Bool miInitializeColormap(ColormapPtr);
int miExpandDirectColors(ColormapPtr, int, xColorItem *, xColorItem *);
Bool miCreateDefColormap(ScreenPtr);
void miClearVisualTypes(void);
Bool miSetVisualTypes(int, int, int, int);
int miGetDefaultVisualMask(int);
Bool miInitVisuals(VisualPtr *, DepthPtr *, int *, int *, int *, VisualID *,
			unsigned long, int, int);

#define MAX_PSEUDO_DEPTH	10
#define MIN_TRUE_DEPTH		6

#define StaticGrayMask	(1 << StaticGray)
#define GrayScaleMask	(1 << GrayScale)
#define StaticColorMask	(1 << StaticColor)
#define PseudoColorMask	(1 << PseudoColor)
#define TrueColorMask	(1 << TrueColor)
#define DirectColorMask	(1 << DirectColor)
                
#define ALL_VISUALS	(StaticGrayMask|\
			 GrayScaleMask|\
			 StaticColorMask|\
			 PseudoColorMask|\
			 TrueColorMask|\
			 DirectColorMask)

#define LARGE_VISUALS	(TrueColorMask|\
			 DirectColorMask)

#define SMALL_VISUALS	(StaticGrayMask|\
			 GrayScaleMask|\
			 StaticColorMask|\
			 PseudoColorMask)

#endif /* _MICMAP_H_ */
