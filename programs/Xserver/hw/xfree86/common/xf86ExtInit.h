/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86ExtInit.h,v 3.0 1994/06/28 12:29:14 dawes Exp $ */

/*
 * Dummy functions to disable various extensions
 */

#ifdef MONO_SERVER
/* For Mono and VGA16 servers, always turn off PEX and XIE */

void PexExtensionInit() {}

void XieInit() {}

#endif /* MONO_SERVER */

#if 0
#if defined(LINKKIT) && !defined(PEXEXT)
void PexExtensionInit() {}
#endif

#if defined(LINKKIT) && !defined(XIE)
void XieInit() {}
#endif

#endif /* MONO_SERVER */

#if 0
#if defined(LINKKIT) && !defined(SCREENSAVER)
void ScreenSaverExtensionInit() {}
#endif

#if defined(LBX)
void BigReqExtensionInit() {}
#else
void LbxExtensionInit() {}
void LbxFreeFontTag() {}
#endif

#endif
