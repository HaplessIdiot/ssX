/* $XConsortium: vgaAsm.h,v 1.1 94/03/28 21:55:37 dpw Exp $ */
/* $XFree86$ */

/* Definitions for VGA bank assembler routines */

#ifdef CSRG_BASED
#define VGABASE CONST(0xFF000000)
#else
#define VGABASE CONST(0xF0000000)
#endif

