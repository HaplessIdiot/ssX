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
#include "xf86fbman.h"
#include "via_driver.h"
#include "via_video.h"
#include "ddmpeg.h"
#include "capture.h"
#include "via_xvpriv.h"


viaPortPrivRec *viaVideoPort;
viaPortPrivRec *gviaPortPriv[6];

__inline void AllocatePortPriv()
{
   int i;
   for ( i = 0; i< XV_PORT_NUM; i ++ ){
       gviaPortPriv[i] =  (viaPortPrivPtr)xcalloc(1, sizeof(viaPortPrivRec) );
       if ( ! gviaPortPriv[i] ){
	  DBG_DD(ErrorF(" via_xvpriv.c : Fail to allocate gviaPortPriv: \n"));
       }
       else{
	  DBG_DD(ErrorF(" via_xvpriv.c : gviaPortPriv[%d] = 0x%08x \n", i,gviaPortPriv[i]));
       }
   }

}

__inline void FreePortPriv()
{

   int i;
   for ( i=0; i< XV_PORT_NUM; i++ ){
       if ( gviaPortPriv[i] ){
	  xfree(gviaPortPriv[i]);
	  gviaPortPriv[i] =  NULL;
       }
   }
}


__inline void ClearPortPriv(int nIndex)
{
   memset(gviaPortPriv[nIndex], 0, sizeof(viaPortPrivRec));
}

viaPortPrivPtr GetPortPriv(int nIndex)
{
   return gviaPortPriv[nIndex];
}


void SetPortPriv(int nIndex, unsigned long dwAction, unsigned long dwValue)
{
     switch ( dwAction )
     {
       case   SET_XVPORTID     :
	      gviaPortPriv[nIndex]->xvPortID	=  dwValue ;
	      break;
       case   SET_BRIGHTNESS   :
	      gviaPortPriv[nIndex]->brightness	=  dwValue ;
	      break;
       case   SET_CONTRAST     :
	      gviaPortPriv[nIndex]->contrast	=  dwValue ;
	      break;
       case   SET_COLORKEY     :
	      gviaPortPriv[nIndex]->colorKey	=  dwValue ;
	      break;
     }
}


unsigned long  IdentifyPort(viaPortPrivPtr pPort)
{
   int i;
   for ( i=0; i< XV_PORT_NUM; i++ ){
       if ( gviaPortPriv[i] ==	pPort ){
	  /* DBG_DD(ErrorF(" via_xvpriv.c : Command for %s ! \n",XVPORTNAME[i])); */
	  return i;
       }
   }

   if ( i ==  XV_PORT_NUM )
      DBG_DD(ErrorF(" via_xvpriv.c : No Port found ! \n"));
   return 0xff;
}

