/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86ExtInit.h,v 3.4 1996/02/04 09:06:11 dawes Exp $ */





/* $XConsortium: xf86ExtInit.h /main/5 1995/11/12 19:21:26 kaleb $ */

/* Hack to avoid multiple versions of dixfonts in vga{2,16}misc.o */
#ifndef LBX
extern void LbxFreeFontTag(
#if NeedFunctionPrototypes
	void
#endif
	);
void LbxFreeFontTag() {}
#endif
