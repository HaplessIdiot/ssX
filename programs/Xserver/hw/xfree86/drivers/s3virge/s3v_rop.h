/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/s3virge/s3v_rop.h,v 1.1 1998/11/22 10:37:33 dawes Exp $ */
/*
 *
 * Copyright 1995-1997 The XFree86 Project, Inc.
 *
 */
/* This file contains the data structures which map the X ROPs to the
 * ViRGE ROPs. It also contains other mappings which are used when supporting
 * planemasks and transparency.
 *
 * Created by Sebastien Marineau, 29/03/97.
 * This file should be included only from s3v_accel.c to avoid 
 * duplicate symbols. 
 * 
 */

#include "regs3v.h"

static int s3vAlu[16] =
{
   ROP_0,               /* GXclear */
   ROP_DSa,             /* GXand */
   ROP_SDna,            /* GXandReverse */
   ROP_S,               /* GXcopy */
   ROP_DSna,            /* GXandInverted */
   ROP_D,               /* GXnoop */
   ROP_DSx,             /* GXxor */
   ROP_DSo,             /* GXor */
   ROP_DSon,            /* GXnor */
   ROP_DSxn,            /* GXequiv */
   ROP_Dn,              /* GXinvert*/
   ROP_SDno,            /* GXorReverse */
   ROP_Sn,              /* GXcopyInverted */
   ROP_DSno,            /* GXorInverted */
   ROP_DSan,            /* GXnand */
   ROP_1                /* GXset */
};

/* S -> P, for solid and pattern fills. */
static int s3vAlu_sp[16]=
{
   ROP_0,
   ROP_DPa,
   ROP_PDna,
   ROP_P,
   ROP_DPna,
   ROP_D,
   ROP_DPx,
   ROP_DPo,
   ROP_DPon,
   ROP_DPxn,
   ROP_Dn,
   ROP_PDno,
   ROP_Pn,
   ROP_DPno,
   ROP_DPan,
   ROP_1
};

/* ROP  ->  (ROP & P) | (D & ~P) */
/* These are used to support a planemask for S->D ops */
static int s3vAlu_pat[16] =
{
   ROP_0_PaDPnao,
   ROP_DSa_PaDPnao,
   ROP_SDna_PaDPnao,
   ROP_S_PaDPnao,
   ROP_DSna_PaDPnao,
   ROP_D_PaDPnao,
   ROP_DSx_PaDPnao,
   ROP_DSo_PaDPnao,
   ROP_DSon_PaDPnao,
   ROP_DSxn_PaDPnao,
   ROP_Dn_PaDPnao,
   ROP_SDno_PaDPnao,
   ROP_Sn_PaDPnao,
   ROP_DSno_PaDPnao,
   ROP_DSan_PaDPnao,
   ROP_1_PaDPnao
};

/* ROP_sp -> (ROP_sp & S) | (D & ~S) */
/* This is used for our transparent mono pattern fills to support trans/plane*/
static int s3vAlu_MonoTrans[16] =
{
   ROP_0_SaDSnao,
   ROP_DPa_SaDSnao,
   ROP_PDna_SaDSnao,
   ROP_P_SaDSnao,
   ROP_DPna_SaDSnao,
   ROP_D_SaDSnao,
   ROP_DPx_SaDSnao,
   ROP_DPo_SaDSnao,
   ROP_DPon_SaDSnao,
   ROP_DPxn_SaDSnao,
   ROP_Dn_SaDSnao,
   ROP_PDno_SaDSnao,
   ROP_Pn_SaDSnao,
   ROP_DPno_SaDSnao,
   ROP_DPan_SaDSnao,
   ROP_1_SaDSnao
};



/* This function was taken from accel/s3v.h. It adjusts the width
 * of transfers for mono images to works around some bugs.
 */

static __inline__ int S3VCheckLSPN(S3VPtr ps3v, int w, int dir)
{
   int lspn = (w * ps3v->Bpp) & 63;  /* scanline width in bytes modulo 64*/

   if (ps3v->Bpp == 1) {
      if (lspn <= 8*1)
	 w += 16;
      else if (lspn <= 16*1)
	 w += 8;
   } else if (ps3v->Bpp == 2) {
      if (lspn <= 4*2)
	 w += 8;
      else if (lspn <= 8*2)
	 w += 4;
   } else {  /* ps3v->Bpp == 3 */
      if (lspn <= 3*3) 
	 w += 6;
      else if (lspn <= 6*3)
	 w += 3;
   }
   if (dir && w >= ps3v->bltbug_width1 && w <= ps3v->bltbug_width2) {
      w = ps3v->bltbug_width2 + 1;
   }

   return w;
}

/* And this adjusts color bitblts widths to work around GE bugs */

static __inline__ int S3VCheckBltWidth(S3VPtr ps3v, int w)
{
   if (w >= ps3v->bltbug_width1 && w <= ps3v->bltbug_width2) {
      w = ps3v->bltbug_width2 + 1;
   }
   return w;
}

/* This next function determines if the Source operand is present in the
 * given ROP. The rule is that both the lower and upper nibble of the rop
 * have to be neither 0x00, 0x05, 0x0a or 0x0f. If a CPU-Screen blit is done
 * with a ROP which does not contain the source, the virge will hang when
 * data is written to the image transfer area. 
 */

static __inline__ Bool S3VROPHasSrc(int shifted_rop)
{
    int rop = (shifted_rop & (0xff << 17)) >> 17;

    if ((((rop & 0x0f) == 0x0a) | ((rop & 0x0f) == 0x0f) 
        | ((rop & 0x0f) == 0x05) | ((rop & 0x0f) == 0x00)) &
       (((rop & 0xf0) == 0xa0) | ((rop & 0xf0) == 0xf0) 
        | ((rop & 0xf0) == 0x50) | ((rop & 0xf0) == 0x00)))
            return FALSE;
    else 
            return TRUE;
}

/* This next function determines if the Destination operand is present in the
 * given ROP. The rule is that both the lower and upper nibble of the rop
 * have to be neither 0x00, 0x03, 0x0c or 0x0f. 
 */

static __inline__ Bool S3VROPHasDst(int shifted_rop)
{
    int rop = (shifted_rop & (0xff << 17)) >> 17;

    if ((((rop & 0x0f) == 0x0c) | ((rop & 0x0f) == 0x0f) 
        | ((rop & 0x0f) == 0x03) | ((rop & 0x0f) == 0x00)) &
       (((rop & 0xf0) == 0xc0) | ((rop & 0xf0) == 0xf0) 
        | ((rop & 0xf0) == 0x30) | ((rop & 0xf0) == 0x00)))
            return FALSE;
    else 
            return TRUE;
}



