#ifndef VAXP
#define VAXP

#include "vtypes.h"

#if defined(__alpha__) && !defined(_XSERVER64)
vu8 *xf86MapVidMemSparse(vu8 *Base, vu64 Size);
void xf86UnMapVidMemSparse(vu8 *Base, vu64 Size);
vu8 xf86ReadSparse8(vu8 *Base, vu64 Offset);
vu16 xf86ReadSparse16(vu8 *Base, vu64 Offset);
vu32 xf86ReadSparse32(vu8 *Base, vu64 Offset);
void xf86WriteSparse8(vu8 Value, vu8 *Base, vu64 Offset);
void xf86WriteSparse16(vu16 Value, vu8 *Base, vu64 Offset);
void xf86WriteSparse32(vu32 Value, vu8 *Base, vu64 Offset);
#endif

#endif

