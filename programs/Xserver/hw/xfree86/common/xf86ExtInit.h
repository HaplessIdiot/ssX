/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86ExtInit.h,v 3.2 1994/12/25 13:46:17 dawes Exp $ */

/* Hack to avoid multiple versions of dixfonts in vga{2,16}misc.o */
#ifndef LBX
void LbxFreeFontTag() {}
#endif
