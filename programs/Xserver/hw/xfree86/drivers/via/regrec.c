/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/* $XFree86$ */
#include "xf86.h"

#include "via.h"
#include "regrec.h"

extern volatile CARD8  * lpVidMEMIO;

static VIDEOREGISTER VidReg[VIDEO_REG_NUM];
static CARD32 gdwVidRegCounter;


/* I N L I N E --------------------------------------------------------------*/

__inline void WaitVideoCommandFire(void)
{
    CARD32 volatile *pdwState = (CARD32 volatile *) (lpVidMEMIO+V_COMPOSE_MODE);
    while (*pdwState & V1_COMMAND_FIRE);
}

__inline void WaitHQVFlip(void)
{
    CARD32 volatile *pdwState = (CARD32 volatile *) lpVidMEMIO;
    pdwState = (CARD32 volatile *) (lpVidMEMIO+HQV_CONTROL);
}

__inline void WaitHQVFlipClear(CARD32 dwData)
{
    CARD32 volatile *pdwState = (CARD32 volatile *) (lpVidMEMIO+HQV_CONTROL);
    *pdwState =dwData;

    while ((*pdwState & HQV_FLIP_STATUS) )
    {
	VIDOutD(HQV_CONTROL, *pdwState|HQV_FLIP_STATUS);
    }
}

__inline void WaitVBI(void)
{
    while (IN_VIDEO_DISPLAY);
}

__inline void WaitHQVDone(void)
{
    CARD32 volatile *pdwState = (CARD32 volatile *) (lpVidMEMIO+HQV_CONTROL);
}

__inline void Macro_VidREGFlush(void)
{
    CARD32 i;

    WaitVideoCommandFire();

    for (i=0; i< gdwVidRegCounter; i++ )
    {
	VIDOutD(VidReg[i].dwIndex, VidReg[i].dwData);
	DBG_DD(ErrorF("Macro_VidREGFlush:%08lx %08lx\n",VidReg[i].dwIndex+0x200,VidReg[i].dwData));
    }
}

__inline void Macro_VidREGRec(CARD32 dwAction, CARD32 dwIndex, CARD32 dwData)
{

  switch (dwAction)
  {
     case VIDREGREC_RESET_COUNTER :
	  gdwVidRegCounter = 0;
	  break;

     case VIDREGREC_SAVE_REGISTER:
	  VidReg[gdwVidRegCounter].dwIndex = dwIndex;
	  VidReg[gdwVidRegCounter].dwData  = dwData;
	  gdwVidRegCounter++;
	  if ( gdwVidRegCounter > VIDEO_REG_NUM){
	     DBG_DD(ErrorF("Macro_VidREGRec:Out of Video register space"));
	  }

	  break;

     default :
	  DBG_DD(ErrorF("Macro_VidREGRec:Unknow action"));
	  break;
  }
}

