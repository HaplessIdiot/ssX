/* Misc routines used elsewhere in driver */
/* $XFree86: $ */

#include "rendition.h"
#include "vtypes.h"
#include "vos.h"
#include "vmisc.h"

/* block copy from and to the card */
void v_bustomem_cpy(vu8 *dst, vu8 *src, vu32 num)
{
    int i;
    for (i=0; i<num; i++)
        dst[i] = v_read_memory8(src, i);
}

void v_memtobus_cpy(vu8 *dst, vu8 *src, vu32 num)
{
    int i;
    for (i=0; i<num; i++)
        v_write_memory8(dst, i, src[i]);
}
