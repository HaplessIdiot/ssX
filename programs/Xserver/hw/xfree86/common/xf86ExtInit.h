/* $XFree86$ */

/*
 * Dummy functions to disable various extensions
 */

#ifdef MONO_SERVER
/* For Mono and VGA16 servers, always turn off PEX and XIE */

void PexExtensionInit() {}

void XieInit() {}

#else /* MONO_SERVER */

#if defined(LINKKIT) && !defined(PEXEXT)
void PexExtensionInit() {}
#endif

#if defined(LINKKIT) && !defined(XIE)
void XieInit() {}
#endif

#endif /* MONO_SERVER */

#if defined(LINKKIT) && !defined(SCREENSAVER)
void ScreenSaverExtensionInit() {}
#endif
