/* $XFree86$ */
/*
 * file vmodes.h
 *
 * headerfile for vmodes.c
 */

#ifndef _VMODES_H_
#define _VMODES_H_



/*
 * includes
 */

#include "vtypes.h"



/*
 * function prototypes
 */

int v_setmodefixed(struct v_board_t *board);
int v_setmode(struct v_board_t *board, struct v_modeinfo_t *mode);
void v_setframebase(struct v_board_t *board, vu32 framebase);
int v_getstride(struct v_board_t *board, int *width, vu16 *stride0, vu16 *stride1);



#endif /* #ifndef _VMODES_H_ */

/*
 * end of file vmodes.h
 */
