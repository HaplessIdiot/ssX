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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/regrec.c,v 1.3 2003/08/04 10:32:26 eich Exp $ */
/*#include <stdlib.h>
#include <string.h>*/
#include "xf86.h"

#include "via.h"
#include "via_driver.h"
#include "via_regrec.h"


static VIDEOREGISTER VidReg[VIDEO_REG_NUM];
static unsigned long gdwVidRegCounter;

void viaWaitHQVIdle(VIAPtr pVia)
{
    while (!IN_HQV_FIRE);
}

void viaWaitVideoCommandFire(VIAPtr pVia)
{
    /*while (IN_VIDEO_FIRE);*/
    CARD32 volatile *pdwState = (CARD32 volatile *) (pVia->VidMapBase+V_COMPOSE_MODE);
    /*pdwState = (CARD32 volatile *) (pVia->VidMapBase+V_COMPOSE_MODE);*/
    while ((*pdwState & V1_COMMAND_FIRE)||(*pdwState & V3_COMMAND_FIRE));
}

void viaWaitHQVFlip(VIAPtr pVia)
{
    CARD32 volatile *pdwState = (CARD32 volatile *) pVia->VidMapBase;
    pdwState = (CARD32 volatile *) (pVia->VidMapBase+HQV_CONTROL);
    while (!(*pdwState & HQV_FLIP_STATUS) );
/*
    while (!((*pdwState & 0xc0)== 0xc0) );
    while (!((*pdwState & 0xc0)!= 0xc0) );
*/    
}

void viaWaitHQVFlipClear(VIAPtr pVia, unsigned long dwData)
{
    CARD32 volatile *pdwState = (CARD32 volatile *) (pVia->VidMapBase+HQV_CONTROL);
    *pdwState =dwData;

    while ((*pdwState & HQV_FLIP_STATUS) )
    {
        VIDOutD(HQV_CONTROL, *pdwState|HQV_FLIP_STATUS);
    }
}

void viaWaitVBI(VIAPtr pVia)
{
    while (IN_VIDEO_DISPLAY);
}

void viaWaitHQVDone(VIAPtr pVia)
{
    CARD32 volatile *pdwState = (CARD32 volatile *) (pVia->VidMapBase+HQV_CONTROL);
    /*pdwState = (CARD32 volatile *) (GEAddr+HQV_CONTROL);*/

    /*if (*pdwState & HQV_ENABLE)*/
    if (pVia->swov.MPEG_ON)
    {
        while ((*pdwState & HQV_SW_FLIP) );
    }
}

void viaMacro_VidREGFlush(VIAPtr pVia)
{
    unsigned long i;
    
    viaWaitVideoCommandFire(pVia);

    for (i=0; i< gdwVidRegCounter; i++ )
    {
        VIDOutD(VidReg[i].dwIndex, VidReg[i].dwData);
        /*DBG_DD("viaMacro_VidREGFlush:%08lx %08lx\n",VidReg[i].dwIndex+0x200,VidReg[i].dwData);*/
        DBG_DD(ErrorF("viaMacro_VidREGFlush:%08lx %08lx\n",VidReg[i].dwIndex+0x200,VidReg[i].dwData));
    }
}

void viaMacro_VidREGRec(VIAPtr pVia, unsigned long dwAction, unsigned long dwIndex, unsigned long dwData)
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
             /*DBG_DD("viaMacro_VidREGRec:Out of Video register space");*/
             DBG_DD(ErrorF("viaMacro_VidREGRec:Out of Video register space"));
          }

          break;

     default :
          /*DBG_DD("viaMacro_VidREGRec:Unknow action");*/
          DBG_DD(ErrorF("viaMacro_VidREGRec:Unknow action"));
          break;
  }
}

void viaMacro_VidREGFlushVPE(VIAPtr pVia)
{
    unsigned long i;
    CARD32 volatile *pdwState = (CARD32 volatile *) pVia->VidMapBase;
    pdwState = (CARD32 volatile *) (pVia->VidMapBase+V_COMPOSE_MODE);

    while ((*pdwState & V1_COMMAND_FIRE)||(*pdwState & V3_COMMAND_FIRE));

    for (i=0; i< gdwVidRegCounter; i++ )
    {
     	VIDOutD(VidReg[i].dwIndex, VidReg[i].dwData);
        /*DBG_DD("viaMacro_VidREGFlush V3:%08lx %08lx\n",VidReg[i].dwIndex+0x200,VidReg[i].dwData);*/
        DBG_DD(ErrorF("viaMacro_VidREGFlush V3:%08lx %08lx\n",VidReg[i].dwIndex+0x200,VidReg[i].dwData));
    }
}

void viaMacro_VidREGRecVPE(VIAPtr pVia, unsigned long dwAction, unsigned long dwIndex, unsigned long dwData)
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
             /*TOINT3;*/
          }

          break;

     default :
          /*TOINT3;*/
          break;
  }
}
