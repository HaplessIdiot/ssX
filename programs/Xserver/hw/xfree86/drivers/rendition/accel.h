/*
 * file accel.h
 *
 * header file for accel.c
 */

#ifndef __ACCEL_H__
#define __ACCEL_H__
 


/*
 * includes
 */

#include "vtypes.h"



/*
 * function prototypes
 */

void RENDITIONAccelInit(struct v_board_t *board);
void RENDITIONAccelNone(void);
int RENDITIONInitUcode(struct v_board_t *board);

void RENDITIONDumpUcode(void);
void RENDITIONDrawSomething(void);



#endif /* #ifdef __ACCEL_H__ */

/*
 * end of file accel.h
 */
