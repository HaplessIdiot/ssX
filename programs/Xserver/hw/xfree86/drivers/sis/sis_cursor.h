/* $XFree86$ */
/*
 * SiS hardware cursor handling
 * Definitions
 *
 * Copyright (C) 2001-2004 by Thomas Winischhofer, Vienna, Austria.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1) Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2) Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3) All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement: "This product includes
 *    software developed by Thomas Winischhofer, Vienna, Austria."
 * 4) The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author:   Thomas Winischhofer <thomas@winischhofer.net>
 *
 * Idea based on code by Can-Ru Yeou, SiS Inc.
 *
 */

#define CS(x)   (0x8500+(x<<2))

/* 300 series, CRT1 */

/* 80000000 = RGB(1) - MONO(0)
 * 40000000 = enable(1) - disable(0)
 * 20000000 = 32(1) / 16(1) bit RGB
 * 10000000 = "ghost"(1) - [other effect](0)
 */

#define sis300GetCursorStatus \
  MMIO_IN32(pSiS->IOBase, CS(0)) & 0x40000000;
  
#define sis300SetCursorStatus(status) \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0xbfffffff; \
  temp |= status; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }

#define sis300EnableHWCursor() \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0x0fffffff; \
  temp |= 0x40000000; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }
  
#define sis300EnableHWARGBCursor() \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp |= 0xF0000000; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }

#define sis300EnableHWARGB16Cursor() \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0x0fffffff; \
  temp |= 0xD0000000; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }

#define sis300SwitchToMONOCursor() \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0x4fffffff; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }
  
#define sis300SwitchToRGBCursor() \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp |= 0xB0000000; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }
  
#define sis300DisableHWCursor()\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0xbFFFFFFF; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }

#define sis300SetCursorBGColor(color)\
  MMIO_OUT32(pSiS->IOBase, CS(1), (color));
#define sis300SetCursorFGColor(color)\
  MMIO_OUT32(pSiS->IOBase, CS(2), (color));

#define sis300SetCursorPositionX(x,preset)\
  MMIO_OUT32(pSiS->IOBase, CS(3), ((x) | ((preset) << 16)));
#define sis300SetCursorPositionY(y,preset)\
  MMIO_OUT32(pSiS->IOBase, CS(4), ((y) | ((preset) << 16)));

#define sis300SetCursorAddress(address)\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0xFFFF0000; \
  temp |= address; \
  MMIO_OUT32(pSiS->IOBase,CS(0),temp); \
  }

#define sis300SetCursorPatternSelect(pat_id)\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0xF0FFFFFF; \
  temp |= (pat_id) << 24; \
  MMIO_OUT32(pSiS->IOBase,CS(0),temp); \
  }

/* 300 series, CRT2 */

/* 80000000 = RGB(1) - MONO(0)
 * 40000000 = enable(1) - disable(0)
 * 20000000 = 32(1) / 16(1) bit RGB
 * 10000000 = unused (always "ghosting")
 */

#define sis301GetCursorStatus \
  MMIO_IN32(pSiS->IOBase, CS(8)) & 0x40000000;
  
#define sis301SetCursorStatus(status) \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0xbfffffff; \
  temp |= status; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  }
  
#define sis301EnableHWCursor()\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0x0fffffff; \
  temp |= 0x40000000; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  }
  
#define sis301EnableHWARGBCursor()\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp |= 0xF0000000; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  }
  
#define sis301EnableHWARGB16Cursor()\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0x0FFFFFFF; \
  temp |= 0xD0000000; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  }
  
#define sis301SwitchToRGBCursor() \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp |= 0xB0000000; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  }
  
#define sis301SwitchToMONOCursor() \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0x4fffffff; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  }
 
#define sis301DisableHWCursor()\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0xbFFFFFFF; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  }
  
#define sis301SetCursorBGColor(color)\
  MMIO_OUT32(pSiS->IOBase, CS(9), (color));
#define sis301SetCursorFGColor(color)\
  MMIO_OUT32(pSiS->IOBase, CS(10), (color));

#define sis301SetCursorPositionX(x,preset)\
  MMIO_OUT32(pSiS->IOBase, CS(11), ((x) | ((preset) << 16)));
#define sis301SetCursorPositionY(y,preset)\
  MMIO_OUT32(pSiS->IOBase, CS(12), ((y) | ((preset) << 16)));

#define sis301SetCursorAddress(address)\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0xFFFF0000; \
  temp |= address; \
  MMIO_OUT32(pSiS->IOBase,CS(8),temp); \
  }

#define sis301SetCursorPatternSelect(pat_id)\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0xF0FFFFFF; \
  temp |= (pat_id) << 24; \
  MMIO_OUT32(pSiS->IOBase,CS(8),temp); \
  }

/* 315/330 series CRT1 */

/* 80000000 = RGB(1) - MONO(0)
 * 40000000 = enable(1) - disable(0)
 * 20000000 = 32(1) / 16(1) bit RGB
 * 10000000 = "ghost"(1) - Alpha Blend(0)
 */
 
#define sis310GetCursorStatus \
  MMIO_IN32(pSiS->IOBase, CS(0)) & 0x40000000;
  
#define sis310SetCursorStatus(status) \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0xbfffffff; \
  temp |= status; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }

#define sis310EnableHWCursor()\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0x0fffffff; \
  temp |= 0x40000000; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }
  
#define sis310EnableHWARGBCursor()\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0x0FFFFFFF; \
  temp |= 0xE0000000; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }

#define sis310SwitchToMONOCursor() \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0x4fffffff; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }
  
#define sis310SwitchToRGBCursor() \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0xBFFFFFFF; \
  temp |= 0xA0000000; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }
  
#define sis310DisableHWCursor()\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0xBFFFFFFF; \
  MMIO_OUT32(pSiS->IOBase, CS(0), temp); \
  }
  
#define sis310SetCursorBGColor(color) \
  MMIO_OUT32(pSiS->IOBase, CS(1), (color));

#define sis310SetCursorFGColor(color)\
  MMIO_OUT32(pSiS->IOBase, CS(2), (color));

#define sis310SetCursorPositionX(x,preset)\
  MMIO_OUT32(pSiS->IOBase, CS(3), ((x) | ((preset) << 16)));

#define sis310SetCursorPositionY(y,preset)\
  MMIO_OUT32(pSiS->IOBase, CS(4), ((y) | ((preset) << 16)));

#define sis310SetCursorAddress(address)\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0xFFF00000; \
  temp |= address; \
  MMIO_OUT32(pSiS->IOBase,CS(0),temp); \
  }

#define sis310SetCursorPatternSelect(pat_id)\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(0)); \
  temp &= 0xF0FFFFFF; \
  temp |= (pat_id) << 24; \
  MMIO_OUT32(pSiS->IOBase,CS(0),temp); \
  }

/* 315 series CRT2 */

/* 80000000 = RGB(1) - MONO(0)
 * 40000000 = enable(1) - disable(0)
 * 20000000 = 32(1) / 16(1) bit RGB
 * 10000000 = "ghost"(1) - Alpha Blend(0)  ?
 */

#define sis301GetCursorStatus310 \
  MMIO_IN32(pSiS->IOBase, CS(8)) & 0x40000000;

#define sis301SetCursorStatus310(status) \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0xbfffffff; \
  temp |= status; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  }

#define sis301EnableHWCursor310()\
  { \
  unsigned long temp, temp1, temp2; \
  temp1 = MMIO_IN32(pSiS->IOBase, CS(11)); \
  temp2 = MMIO_IN32(pSiS->IOBase, CS(12)); \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0x0fffffff; \
  temp |= 0x40000000; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  MMIO_OUT32(pSiS->IOBase, CS(11), temp1); \
  MMIO_OUT32(pSiS->IOBase, CS(12), temp2); \
  }

#define sis301EnableHWARGBCursor310()\
  { \
  unsigned long temp, temp1, temp2; \
  temp1 = MMIO_IN32(pSiS->IOBase, CS(11)); \
  temp2 = MMIO_IN32(pSiS->IOBase, CS(12)); \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0x0FFFFFFF; \
  temp |= 0xE0000000; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  MMIO_OUT32(pSiS->IOBase, CS(11), temp1); \
  MMIO_OUT32(pSiS->IOBase, CS(12), temp2); \
  }

#define sis301SwitchToRGBCursor310() \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0xBFFFFFFF; \
  temp |= 0xA0000000; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  }

#define sis301SwitchToMONOCursor310() \
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0x4fffffff; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  }

#define sis301DisableHWCursor310()\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0xBFFFFFFF; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  }

#define sis301SetCursorBGColor310(color)\
  MMIO_OUT32(pSiS->IOBase, CS(9), (color));
#define sis301SetCursorFGColor310(color)\
  MMIO_OUT32(pSiS->IOBase, CS(10), (color));

#define sis301SetCursorPositionX310(x,preset)\
  MMIO_OUT32(pSiS->IOBase, CS(11), ((x) | ((preset) << 16)));
#define sis301SetCursorPositionY310(y,preset)\
  MMIO_OUT32(pSiS->IOBase, CS(12), ((y) | ((preset) << 16)));

#define sis301SetCursorAddress310(address)\
  { \
  unsigned long temp; \
  if(pSiS->sishw_ext.jChipType == SIS_315H) { \
     if(address & 0x10000) { \
        address &= ~0x10000; \
	orSISIDXREG(SISSR, 0x37, 0x80); \
     } else { \
        andSISIDXREG(SISSR, 0x37, 0x7f); \
     } \
  } \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0xFFF00000; \
  temp |= address; \
  MMIO_OUT32(pSiS->IOBase,CS(8),temp); \
  }

#define sis301SetCursorPatternSelect310(pat_id)\
  { \
  unsigned long temp; \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0xF0FFFFFF; \
  temp |= (pat_id) << 24; \
  MMIO_OUT32(pSiS->IOBase,CS(8),temp); \
  }

/* 330 series CRT2 */

/* Mono cursor engine for CRT2 on SiS330 (Xabre) has bugs
 * and cannot be used!
 */

/* 80000000 = RGB(1) - MONO(0)
 * 40000000 = enable(1) - disable(0)
 * 20000000 = 32(1) / 16(1) bit RGB
 * 10000000 = "ghost"(1) - Alpha Blend(0)  ?
 */

#define sis301EnableHWCursor330()\
  { \
  unsigned long temp, temp1, temp2; \
  andSISIDXREG(SISCR,0x5b,~0x10); \
  temp1 = MMIO_IN32(pSiS->IOBase, CS(11)); \
  temp2 = MMIO_IN32(pSiS->IOBase, CS(12)); \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0x0fffffff; \
  temp |= 0xE0000000; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  MMIO_OUT32(pSiS->IOBase, CS(11), temp1); \
  MMIO_OUT32(pSiS->IOBase, CS(12), temp2); \
  orSISIDXREG(SISCR,0x5b,0x10); \
  }

#define sis301DisableHWCursor330()\
  { \
  unsigned long temp; \
  andSISIDXREG(SISCR,0x5b,~0x10); \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0xBFFFFFFF; \
  MMIO_OUT32(pSiS->IOBase, CS(8), temp); \
  orSISIDXREG(SISCR,0x5b,0x10); \
  }

#define sis301SetCursorPositionX330(x,preset)\
  andSISIDXREG(SISCR,0x5b,~0x10); \
  MMIO_OUT32(pSiS->IOBase, CS(11), ((x) | ((preset) << 16))); \
  orSISIDXREG(SISCR,0x5b,0x10);

#define sis301SetCursorPositionY330(y,preset)\
  andSISIDXREG(SISCR,0x5b,~0x10); \
  MMIO_OUT32(pSiS->IOBase, CS(12), ((y) | ((preset) << 16))); \
  orSISIDXREG(SISCR,0x5b,0x10); \

#define sis301SetCursorAddress330(address)\
  { \
  unsigned long temp, temp1, temp2; \
  temp1 = MMIO_IN32(pSiS->IOBase, CS(11)); \
  temp2 = MMIO_IN32(pSiS->IOBase, CS(12)); \
  andSISIDXREG(SISCR,0x5b,~0x10); \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0xFFF00000; \
  temp |= address; \
  MMIO_OUT32(pSiS->IOBase,CS(8),temp); \
  MMIO_OUT32(pSiS->IOBase, CS(11), temp1); \
  MMIO_OUT32(pSiS->IOBase, CS(12), temp2); \
  orSISIDXREG(SISCR,0x5b,0x10); \
  }

#define sis301SetCursorPatternSelect330(pat_id)\
  { \
  unsigned long temp; \
  andSISIDXREG(SISCR,0x5b,~0x10); \
  temp = MMIO_IN32(pSiS->IOBase, CS(8)); \
  temp &= 0xF0FFFFFF; \
  temp |= (pat_id) << 24; \
  MMIO_OUT32(pSiS->IOBase,CS(8),temp); \
  orSISIDXREG(SISCR,0x5b,0x10); \
  }


