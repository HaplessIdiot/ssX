/* $XConsortium: nglehdw.h,v 1.1 93/08/08 12:57:34 rws Exp $ */

/*************************************************************************
 * 
 * (c)Copyright 1992 Hewlett-Packard Co.,  All Rights Reserved.
 * 
Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Hewlett Packard not be used in
advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

HEWLETT-PACKARD MAKES NO WARRANTY OF ANY KIND WITH REGARD
TO THIS SOFWARE, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  Hewlett-Packard shall not be liable for errors
contained herein or direct, indirect, special, incidental or
consequential damages in connection with the furnishing,
performance, or use of this material.
 *
 *************************************************************************/


#ifndef NGLEHDW_H
#define NGLEHDW_H


typedef ngle_dregs_t	NgleHdwRec;
typedef ngle_dregs_t	*NgleHdwPtr;

#define SETUP_HW(pDregs) {						\
	    char stat;							\
	    volatile char * pstat = &((pDregs)->reg15.b.b0);		\
	    do {							\
		stat = *pstat;						\
		if (!stat)						\
		    stat = *pstat;					\
	    } while(stat);						\
	}

#define SETUP_FB(pDregs,ID, depth) {					\
	    SETUP_HW(pDregs);						\
	    switch (ID) {						\
		case S9000_ID_A1659A:					\
    		    (pDregs)->reg10 = 0x13601000;			\
		case S9000_ID_A1439A:					\
    		    if (depth == 24)					\
		        (pDregs)->reg10 = 0xBBA0A000;			\
		    else  /* depth = 8 */				\
    			(pDregs)->reg10 = 0x13601000;			\
		    break; 						\
		case S9000_ID_TIMBER:					\
		case CRX24_OVERLAY_PLANES:				\
		    (pDregs)->reg10 = 0x13602000;			\
		    break;						\
	    }								\
	    (pDregs)->reg14.all = 0x83000300;				\
	    SETUP_HW(pDregs);						\
	    (pDregs)->reg16.b.b1 = 1;					\
	}

#define START_CURSOR_COLORMAP_ACCESS(pDregs) {				\
	    SETUP_HW(pDregs);						\
	    (pDregs)->reg10 = 0xBBE0F000;				\
	    (pDregs)->reg14.all = 0x03000300;				\
	    (pDregs)->reg13 = ~0;					\
	    (pDregs)->reg3 = 0;						\
	    (pDregs)->reg4 = 0;						\
	    (pDregs)->reg4 = 0;						\
	}

#define WRITE_CURSOR_COLOR(pDregs,color) {				\
	    (pDregs)->reg4 = (color);					\
	}

#define FINISH_CURSOR_COLORMAP_ACCESS(pDregs,ID,depth) {		\
	    (pDregs)->reg2 = 0;						\
	    (pDregs)->reg1 = 0x80008004;				\
	    SETUP_FB(pDregs,ID,depth);					\
	}

#define START_IMAGE_COLORMAP_ACCESS(pDregs) {				\
	    SETUP_HW(pDregs);						\
	    (pDregs)->reg10 = 0xBBE0F000;				\
	    (pDregs)->reg14.all = 0x03000300;				\
	    (pDregs)->reg13 = ~0;					\
	}

#define WRITE_IMAGE_COLOR(pDregs,index,color) {				\
	    SETUP_HW(pDregs);						\
	    (pDregs)->reg3 = ((0x100+index) << 2);			\
	    (pDregs)->reg4 = (color);					\
	}

#define FINISH_IMAGE_COLORMAP_ACCESS(pDregs,ID, depth) {		\
	    (pDregs)->reg2 = 0x400;					\
	    if (depth == 24)						\
    		(pDregs)->reg1 = 0x83000100;				\
    	    else  /* depth = 8 */					\
	        (pDregs)->reg1 = 0x80000100;				\
	    SETUP_FB(pDregs,ID,depth);					\
	}

#define GET_CURSOR_SPECS(pDregs,pScreenPriv) {				      \
	    Card32 activeLinesHi, activeLinesLo;			      \
									      \
	    (pScreenPriv)->sprite.horizBackPorch = (pDregs)->reg19.b.b1;      \
	    activeLinesLo   = (pDregs)->reg20.b.b0;		      	      \
	    activeLinesHi   = ((pDregs)->reg21.b.b2) & 0xf;		      \
	    (pScreenPriv)->sprite.maxYLine = ((activeLinesHi << 8)	      \
		|(activeLinesLo & 0xff));				      \
	}

#define START_CURSOR_MASK_ACCESS(pDregs) {				\
	    SETUP_HW(pDregs);						\
	    (pDregs)->reg14.all = 0x00000300;				\
	    (pDregs)->reg13 = ~0;					\
	    (pDregs)->reg11 = 0x28A07000;				\
	    (pDregs)->reg3 = 0;						\
	}

#define START_CURSOR_DATA_ACCESS(pDregs) {				\
	    SETUP_HW(pDregs);						\
	    (pDregs)->reg14.all = 0x00000300;				\
	    (pDregs)->reg11 = 0x28A06000;				\
	    (pDregs)->reg3 = 0;						\
	}

#define WRITE_CURSOR_BITS(pDregs,word1,word2) {				\
	    SETUP_HW(pDregs);						\
	    (pDregs)->reg4 = (word1);					\
	    (pDregs)->reg5 = (word2);					\
	}

#define ENABLE_CURSOR(pScreenPriv, cursorXYValue, nOffscreenScanlines) { \
	    (pScreenPriv)->sprite.enabledCursorXYValue = (cursorXYValue);\
	    if ((pScreenPriv)->sprite.moveOnVBlank) {			 \
		while (((pScreenPriv)->pDregs->reg21.all) & 0x00040000); \
		while (!(((pScreenPriv)->pDregs->reg21.all)		 \
			& 0x00040000));					 \
	    }								 \
	    SETUP_HW(pScreenPriv->pDregs);				 \
	    (pScreenPriv)->pDregs->reg17.all = (cursorXYValue);		 \
	    (pScreenPriv)->pDregs->reg18.all =				 \
		    (0x80 | nOffscreenScanlines);			 \
	}

#define DISABLE_CURSOR(pScreenPriv) {					\
	    volatile unsigned long *pDregsCursorXY = 			\
		    &((pScreenPriv)->pDregs->reg17.all);		\
	    long enabledCursorValue = 					\
		    (pScreenPriv)->sprite.enabledCursorXYValue;		\
									\
	    SETUP_HW((pScreenPriv)->pDregs);				\
	    *pDregsCursorXY = (enabledCursorValue & 0xe007ffff);	\
	}

#define GET_ROMTABLE_INDEX(romTableIdx) {				\
	    (romTableIdx) = pDregs->reg16.b.b3 - 1;			\
	}

#define SETUP_RAMDAC(pDregs) {						\
	    volatile Card32         *pAuxControlSpace;			\
									\
	    pAuxControlSpace = (Card32 *) (((Card32) pDregs) + 0x1000); \
	    SETUP_HW(pDregs);						\
	    *(pAuxControlSpace + (0x20>>2)) = 0x04000000;		\
	    *(pAuxControlSpace + (0x28>>2)) = 0xff000000;		\
	}

#define CRX24_SETUP_RAMDAC(pDregs) {					\
	    volatile Card32         *pAuxControlSpace;			\
									\
	    pAuxControlSpace = (Card32 *) (((Card32) pDregs) + 0x1000); \
	    SETUP_HW(pDregs);						\
	    *(pAuxControlSpace)   = 0x04000000;				\
	    *(pAuxControlSpace+1) = 0x02000000;				\
	    *(pAuxControlSpace+2) = 0xff000000;				\
	    *(pAuxControlSpace)   = 0x05000000;				\
	    *(pAuxControlSpace+1) = 0x02000000;				\
	    *(pAuxControlSpace+2) = 0x03000000;				\
	}

#define CRX24_SET_OVLY_MASK(pDregs) {					\
	    SETUP_HW(pDregs);						\
	    (pDregs)->reg11 = 0x13a02000;				\
	    (pDregs)->reg14.all = 0x03000300;				\
    	    (pDregs)->reg3 = 0x000017f0;				\
            (pDregs)->reg13 = 0xffffffff;				\
            (pDregs)->reg22 = (long) (~0UL);				\
            (pDregs)->reg23 = 0x0;					\
    }


#define ENABLE_DISPLAY(pDregs) {					\
	    volatile Card32         *pAuxControlSpace;			\
									\
	    pAuxControlSpace = (Card32 *)(((Card32)(pDregs)) + 0x1000); \
	    SETUP_HW(pDregs);						\
	    *(pAuxControlSpace + (0x30>>2)) = 0x06000000;		\
	    *(pAuxControlSpace + (0x38>>2)) = 0x43000000;		\
	}

#define DISABLE_DISPLAY(pDregs) {					\
	    volatile Card32         *pAuxControlSpace;			\
									\
	    pAuxControlSpace = (Card32 *)(((Card32)(pDregs)) + 0x1000); \
	    SETUP_HW(pDregs);						\
	    *(pAuxControlSpace + (0x30>>2)) = 0x06000000;		\
	    *(pAuxControlSpace + (0x38>>2)) = 0x03000000;		\
	}

#define CRX24_ENABLE_DISPLAY(pDregs) {					\
	    volatile Card32         *pAuxControlSpace;			\
									\
	    pAuxControlSpace = (Card32 *)(((Card32)(pDregs)) + 0x1000); \
	    SETUP_HW(pDregs);						\
	    *pAuxControlSpace     = 0x01000000;				\
	    *(pAuxControlSpace+1) = 0x02000000;				\
	    *(pAuxControlSpace+2) = 0x10000000;				\
	}

#define CRX24_DISABLE_DISPLAY(pDregs) {					\
	    volatile Card32         *pAuxControlSpace;			\
									\
	    pAuxControlSpace = (Card32 *)(((Card32)(pDregs)) + 0x1000); \
	    SETUP_HW(pDregs);						\
	    *pAuxControlSpace     = 0x01000000;				\
	    *(pAuxControlSpace+1) = 0x02000000;				\
	    *(pAuxControlSpace+2) = 0x30000000;				\
	}


#define BUFF0_CMAP0 0x00001e02
#define BUFF1_CMAP0 0x02001e02
#define BUFF1_CMAP3 0x0c001e02

#define SETUP_ATTR_ACCESS(pDregs,BufferNumber) {			\
	    SETUP_HW(pDregs);						\
	    (pDregs)->reg11 = 0x2EA0D000;				\
	    (pDregs)->reg14.all = 0x23000302;				\
	    (pDregs)->reg12 = (BufferNumber);				\
	    (pDregs)->reg8 = 0xffffffff;				\
	}

#define SET_ATTR_SIZE(pDregs,Width,Height) {				\
	    (pDregs)->reg6.all = 0;					\
	    (pDregs)->reg9.all = (Card32)(((Width)<<16)|(Height));	\
	    (pDregs)->reg6.all = 0x05000000;				\
	    (pDregs)->reg9.all = 0x00040001;				\
	}

#define FINISH_ATTR_ACCESS(pDregs) {					\
	    SETUP_HW(pDregs);						\
	    (pDregs)->reg12 = 0;					\
	}

#define SETUP_COPYAREA(pDregs)						\
	    (pDregs)->reg16.b.b1 = 0;



#endif /* NGLEHDW_H */
