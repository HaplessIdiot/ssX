/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/rendition/vloaduc.h,v 1.2 1999/04/17 07:06:39 dawes Exp $ */

/*
 * file vloaduc.h
 *
 * loads microcode
 */

#ifndef __VLOADUC_H__
#define __VLOADUC_H__



/*
 * includes
 */

#include "vos.h"
#include "vtypes.h"



/*
 * defines 
 */



/*
 * function prototypes
 */

int v_load_ucfile(ScrnInfoPtr pScreenInfo, char *file_name);



#endif /* __VLOADUC_H__ */

/*
 * end of file vloaduc.h
 */
