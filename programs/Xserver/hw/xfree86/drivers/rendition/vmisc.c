/* Misc routines used elsewhere in driver */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/rendition/vmisc.c,v 1.2 1999/11/19 14:59:18 hohndel Exp $ */

#include "rendition.h"
#include "vtypes.h"
#include "vos.h"
#include "vmisc.h"

#undef DEBUG

/* block copy from and to the card */
void
v_bustomem_cpy(vu8 *dst, vu8 *src, vu32 num)
{
    int i;

#ifdef DEBUG
    ErrorF ("Rendition: DEBUG v_bustomem_cpy called\n");
#endif  
    for (i=0; i<num; i++)
        dst[i] = v_read_memory8(src, i);
}

void
v_memtobus_cpy(vu8 *dst, vu8 *src, vu32 num)
{
    int i;

#ifdef DEBUG
    ErrorF ("Rendition: DEBUG v_memtobus_cpy called\n");
#endif  

    for (i=0; i<num; i++)
        v_write_memory8(dst, i, src[i]);
}
