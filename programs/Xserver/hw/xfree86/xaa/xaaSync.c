/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaCpyWin.c,v 1.2 1998/07/25 16:58:43 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "xaa.h"
#include "xaalocal.h"

/*
    Original XAA architecture by Harm Hanemaayer (H.Hanemaayer@inter.nl.net).

    External Sync Extension added by Jens Owen (jens@precisioninsight.com).
*/

void 
XAASync(
    ScreenPtr pScreen,
    Bool force 		/* externally force a sync */
)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(pScreen);

    if (!infoRec->pScrn->vtSema || !infoRec->ScreenToScreenBitBlt) { 
	if(infoRec->pScrn->vtSema && (infoRec->NeedToSync || force)) {
	    (*infoRec->Sync)(infoRec->pScrn);
	    infoRec->NeedToSync = FALSE;
	}
    	return;
    }
}
