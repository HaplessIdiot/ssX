/* $XFree86$ */

/******************* compat. macros **********************/

/* #define s3vPriv.chip S3VPTR(pScrn)->Chipset*/
#if 0
#define vgaCRIndex (VGAHW_GET_IOBASE()+4)
#define vgaCRReg (VGAHW_GET_IOBASE()+5)
#define s3vMmioMem (&(VGAHWPTR(pScrn)->MemBase))
#endif
       
#define COMPVARS vgaHWPtr hwp = VGAHWPTR(pScrn); \
		S3VPtr ps3v = S3VPTR(pScrn); \
  		void *s3vMmioMem = ps3v->MapBase; \
  		int vgaCRIndex, vgaCRReg, vgaIOBase; \
  		vgaIOBase = hwp->IOBase; \
  		vgaCRIndex = vgaIOBase + 4; \
  		vgaCRReg = vgaIOBase + 5
		
#if 0
  vgaHWPtr hwp = VGAHWPTR(pScrn);
  vgaRegPtr vgaSavePtr = &hwp->SavedReg;
  S3VPtr ps3v = S3VPTR(pScrn);
  S3VRegPtr restore = &ps3v->SavedReg;
  void *s3vMmioMem = &hwp->MemBase;
  int vgaCRIndex, vgaCRReg, vgaIOBase;
  vgaIOBase = VGAHW_GET_IOBASE();
  vgaCRIndex = vgaIOBase + 4;
  vgaCRReg = vgaIOBase + 5;
#endif


/*EOF*/
