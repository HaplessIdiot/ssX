/* $XFree86$ */

/*
 *
 * Copyright 1998 The XFree86 Project, Inc.
 *
 */


/*
 * s3v_pio.c
 * Port to 4.0 design level
 *
 * S3 ViRGE driver
 *
 * Functions to support getting a ViRGE card into MMIO mode if it fails to
 * default to MMIO enabled.  These are in a different file so they can use
 * the PIO version of vgaHW.h.  All other stuff uses the MMIO version.
 *
 */


	/* Most xf86 commons are already in s3v.h */
#define		_S3V_PIO_VGA
#include	"s3v.h"


/* proto's */
 
void S3VEnableMmio(ScrnInfoPtr pScrn);
void S3VDisableMmio(ScrnInfoPtr pScrn);


void
S3VEnableMmio(ScrnInfoPtr pScrn)
{
  vgaHWPtr hwp;
  S3VPtr ps3v;
  int vgaCRIndex, vgaCRReg;
  
  PVERB5("	S3VEnableMmio\n");
  
  hwp = VGAHWPTR(pScrn);
  ps3v = S3VPTR(pScrn);
  			  
  vgaHWGetIOBase(hwp);             	/* Get VGA I/O base */
  vgaCRIndex = hwp->IOBase + 4;
  vgaCRReg = hwp->IOBase + 5;

  
  outb(vgaCRIndex, 0x53);
  					/* Save register for restore */
  ps3v->EnableMmioCR53 = inb(vgaCRReg);
      
  			      	/* Enable new MMIO, if TRIO mmio is already */
				/* enabled, then it stays enabled. */
  outb(vgaCRReg, ps3v->EnableMmioCR53 | 0x08 );
  
  return;
}



void
S3VDisableMmio(ScrnInfoPtr pScrn)
{
  vgaHWPtr hwp;
  S3VPtr ps3v;
  int vgaCRIndex, vgaCRReg;

  PVERB5("	S3VDisableMmio\n");
 
  hwp = VGAHWPTR(pScrn);
  ps3v = S3VPTR(pScrn);
 
  vgaCRIndex = hwp->IOBase + 4;
  vgaCRReg = hwp->IOBase + 5;
  
  outb(vgaCRIndex, 0x53);
				/* Restore register's original state */
  outb(vgaCRReg, ps3v->EnableMmioCR53 );
   
  return;
}
