/* $XConsortium: vgaAsm.h,v 1.1 94/03/28 21:55:37 dpw Exp $ */

/* Definitions for VGA bank assembler routines */

#if defined(__386BSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__bsdi__)
#define VGABASE CONST(0xFF000000)
#else
#define VGABASE CONST(0xF0000000)
#endif

