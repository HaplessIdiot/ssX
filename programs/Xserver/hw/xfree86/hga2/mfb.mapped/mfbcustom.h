/* $XConsortium: mfbcustom.h,v 1.1 94/03/28 21:26:47 dpw Exp $ */

/*
 * This file customizes mfb into the library suitable for the Herculse server.
 */

/*
 * This macro maps function names by adding the hga prefix in order to avoid 
 * collisions and allow coexistance with other mfb libraries within the server.
 */
#define CUSTOMNAME(name)   CATNAME(hga,name)

extern unsigned int **hgaMapping;

#define MAPRW(p) \
  ((pointer)(p)<(pointer)0x00008000 ? \
   hgaMapping[(int)(p)] : (unsigned int *)(p))
#define MAPR(p)  MAPRW(p)
#define MAPW(p)  MAPRW(p)

/* Some dummy defines for bank-related stuff */
#define CHECKSCREEN(x)		FALSE
#define SETRWF(f,x)		/**/
#define CHECKRWOF(f,x)		/**/
#define CHECKRWUF(f,x)		/**/
#define BANK_FLAG(a)		/**/
#define BANK_FLAG_BOTH(a,b)	/**/
#define SETR(a)			/**/
#define SETW(a)			/**/
#define SETRW(a)		/**/
#define CHECKRO(a)		/**/
#define CHECKWO(a)		/**/
#define CHECKRWO(a)		/**/
#define CHECKRU(a)		/**/
#define CHECKWU(a)		/**/
#define CHECKRWU(a)		/**/
#define NEXTR(a)		/**/
#define NEXTW(a)		/**/
#define PREVR(a)		/**/
#define PREVW(a)		/**/
#define SAVE_BANK()		/**/
#define RESTORE_BANK()		/**/
#define PUSHR()			/**/
#define POPR()			/**/
#define CHECKRWONEXT(a)		/**/
#define CHECKRWD(a,b,c)		/**/

