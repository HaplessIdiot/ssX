/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaLineMisc.c,v 1.1.2.2 1998/06/27 14:48:18 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "miline.h"
#include "xf86str.h"
#include "xaa.h"
#include "xaalocal.h"
   

void 
XAASolidHorVertLineAsRects(
   ScrnInfoPtr pScrn,
   int x, int y, int len, int dir
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);

    if(dir == DEGREES_0) 
	(*infoRec->SubsequentSolidFillRect)(pScrn, x, y, len, 1);
    else
	(*infoRec->SubsequentSolidFillRect)(pScrn, x, y, 1, len);
}
   

void 
XAASolidHorVertLineAsTwoPoint(
   ScrnInfoPtr pScrn,
   int x, int y, int len, int dir
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);

    if(dir == DEGREES_0) 
	(*infoRec->SubsequentSolidTwoPointLine)(pScrn, x, y, x + len, y, 0);
    else
	(*infoRec->SubsequentSolidTwoPointLine)(pScrn, x, y, x, y + len, 0);
}
   
void 
XAASolidHorVertLineAsBresenham(
   ScrnInfoPtr pScrn,
   int x, int y, int len, int dir
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);

    if(dir == DEGREES_0) 
	(*infoRec->SubsequentSolidBresenhamLine)(
		pScrn, x, y, len << 1, 0, -len, len, 0);
    else
	(*infoRec->SubsequentSolidBresenhamLine)(
		pScrn, x, y, len << 1, 0, -len, len, YMAJOR);
}

