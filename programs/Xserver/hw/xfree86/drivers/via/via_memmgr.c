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
#include <stdlib.h>
#include <string.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "via_memmgr.h"

/* kernel module global variables */
ViaOffScrnPtr MemLayOut;
unsigned long viabase0;	     /* physical address of frame buffer */
unsigned long viaBankSize;   /* Amount of total frame buffer */
			     /* Set 8MB default,update by Xserver */
BOOL   XserverIsUp = FALSE ; /* If Xserver had run(register action) */
unsigned long	S2D_Count  = 0;
unsigned long	S3D_Count  = 0;
unsigned long	Video_Count = 0;

void PrintFBMem()
{
 OffMemRange *used,*unused;

 unused = MemLayOut->unused;
 used	= MemLayOut->used;

 DBG_DD(ErrorF("\n"));
 DBG_DD(ErrorF("mem.c : Used   ID, StartAddr,  EndAddr,	    size,     type,	this,	  next\n"));
 while (used != NULL)
 {
    DBG_DD(ErrorF("mem.c : %08lx,  %08lx, %08lx, %08lx, %08x, %p, %p \n",
	      used->ID, used->StartAddr, used->EndAddr,used->size, used->type, used, used->next ));
    used = nextq(used);
 }

 DBG_DD(ErrorF("\n"));
 DBG_DD(ErrorF("mem.c : Unused ID, StartAddr,  EndAddr,	    size,     type,	this,	  next\n"));
 while (unused != NULL)
 {
    DBG_DD(ErrorF("mem.c :  %08lx,  %08lx, %08lx, %08lx, %08x, %p, %p \n",
	      unused->ID,unused->StartAddr, unused->EndAddr,unused->size, unused->type,
	      unused, unused->next  ));
    unused = nextq(unused);
 }
}

static void MergeL(OffMemRange * x1 , unsigned long size)
{
    x1->EndAddr	 = endq(x1)  + size;
    x1->size	 = sizeq(x1) + size;
}

static void MergeR(OffMemRange * right, unsigned long Addr,unsigned long size)
{
       right->StartAddr = Addr;
       right->size	= sizeq(right) + size;
}

static void MergeLR(OffMemRange *x1, OffMemRange *right, unsigned long S_Addr,unsigned long size)
{
       x1->size = sizeq(x1) + sizeq(right) + size;
       x1->EndAddr  = endq(right);
       x1->next = nextq(right);
}

static OffMemRange * lastq( OffMemRange *qSet )
{
       while ( nextq(qSet) != NULL )
	      qSet = nextq(qSet);

       return qSet;
}

static OffMemRange * prevq(OffMemRange *qSet, OffMemRange *curq )
{

       /* First Queue */
       if ( qSet == curq )
	  return NULL;

       while ( qSet != NULL ){
	     if ( nextq(qSet ) == curq )
		return qSet;
	      qSet = nextq(qSet);
       }

       /* Warnning! Not Found. This should not happen */
       return NULL;
}

static OffMemRange * numNinq(OffMemRange *qSet, int N )
{
    int i=1;

    for ( i=1; i< N; i++ ){
	qSet = nextq(qSet);
	if ( qSet == NULL )
	   return NULL;
    }

    return qSet;

}

static void swapq(OffMemRange *qSet ,OffMemRange *L, OffMemRange *R )
{
       OffMemRange *tmp, *prevL, *prevR, *nextL, *nextR;

       prevL = prevq(qSet,L);
       prevR = prevq(qSet,R);
       nextL = nextq(L);
       nextR = nextq(R);

       tmp = (OffMemRange *) xalloc(sizeof(OffMemRange));
       memcpy((void *)tmp,(void *) R ,sizeof(OffMemRange));
       memcpy((void *)R	 ,(void *) L ,sizeof(OffMemRange));
       memcpy((void *)L	 ,(void *)tmp,sizeof(OffMemRange));

       L->next = nextR;
       R->next = nextL;
       if ( prevL != NULL )
	  prevL->next = R;
       if ( prevR != NULL )
	  prevR->next = L;

       deleteq(tmp);
}


OffMemRange * viaAllocSurface(int *size ,int alignment)
{
  int new_size ;
  unsigned long old_addr;
  OffMemRange *fq,*sq,*bq=NULL;
  BOOL isFind = FALSE ;

  DBG_DD(ErrorF("mem.c : viaAllocSurface \n"));
  new_size = *size;

  fq = MemLayOut->unused;
  DBG_DD(ErrorF("mem.c : MemLayOut->unused: %p \n", MemLayOut->unused));
  bq = fq;
  while ( fq )
   {
	 /* find the smaller-suitable memory for allocation */
	 if ( (endq(fq) - startq(fq) + 1 ) >= new_size ){
	     isFind = TRUE;
	     break;
	 }
	 bq = fq ;
	 fq = nextq(fq) ;
   }

  if (!isFind){
       DBG_DD(ErrorF("mem.c : viaAllocSurface : No surface available\n"));
       PrintMemLayOut();
       return NULL ;
  }
  else{
	old_addr = startq(fq);
	fq->StartAddr = startq(fq) + new_size ;
	fq->size      = sizeq(fq) - new_size;

	/* No memory x1 in this queue */
	if (startq(fq) == endq(fq) + 1)
	   {
	    if ( bq == MemLayOut->unused ){
	       fq = nextq(fq);
	       deleteq(MemLayOut->unused);
	       MemLayOut->unused = fq ;
	    }
	    else{
	       bq->next = nextq(fq) ;
	       deleteq(fq);
	    }
	   }

	if (MemLayOut->used == NULL ){
	    MemLayOut->used = xalloc( sizeof( OffMemRange));
	    MemLayOut->used->StartAddr = old_addr;
	    MemLayOut->used->EndAddr   = old_addr + new_size - 1;
	    MemLayOut->used->size      = new_size;
	    MemLayOut->used->next = NULL ;
	    DBG_DD(ErrorF("mem.c : viaAllocSurface : First Used \n"));
	    PrintMemLayOut();
	    return (MemLayOut->used);
	   }

	sq = MemLayOut->used ;

	while (sq->next !=NULL	)
	     sq = nextq(sq);
	sq->next = xalloc( sizeof( OffMemRange));
	sq = nextq(sq);
	sq->next = NULL;
	sq->StartAddr = old_addr;
	sq->EndAddr   = old_addr + new_size - 1;
	sq->size      = new_size;
	DBG_DD(ErrorF("mem.c : viaAllocSurface : 2\n"));
	PrintMemLayOut();
	return (sq);
      }
}


BOOL viaFreeSurface(unsigned long S_Addr,int size,unsigned char ctype)
{
  BOOL isFind;
  OffMemRange *this_free,*fq,*bq;
  OffMemRange *rightq=NULL,*leftq=NULL;
  BOOL fm = FALSE, bm = FALSE, bRightest=FALSE;

  DBG_DD(ErrorF("mem.c : viaFreeSurface : \n"));
  isFind = FALSE;
  this_free = MemLayOut->used;
  fq = MemLayOut->unused;
  bq = NULL;

  bq = this_free;
  /* Find the queue to be deleted */
  while (this_free != NULL) {
	if (startq(this_free) == S_Addr && size ==sizeq(this_free) && typeq(this_free) == ctype){
	/*if (this_free->StartAddr == S_Addr && size ==this_free->size && this_free->type == ctype){*/
	   isFind = TRUE;
	   break;
	}
	else
	{
	   bq = this_free;
	   this_free = nextq(this_free);
	}
  }


  if ( ! isFind ) {
     PrintMemLayOut();
     DBG_DD(ErrorF("mem.c : viaFreeSurface : Warnning! Surface to be freed not found. \n"));
     return isFind ;
  }

  /* bq is the first queue in the link list */
  if (bq == this_free && isFind){
	  MemLayOut->used = nextq(bq) ;
	  deleteq(this_free);
  /* bq -> this_free			    */
  }else if(bq != this_free && isFind) {
	  bq->next = nextq(this_free);
	  deleteq(this_free);
  }

  DBG_DD(ErrorF("mem.c : viaFreeSurface : 2\n"));
  /* add the free memory block to unused queue, first find the
	     "correct" queue to insert */

 /* ====================================================================== */
  bq = fq; /* where fq is MemLayOut->unused */


  while( fq!=NULL )
  {
       /* fq -> this_free->nextq(fq) */
       if ( (endq(fq)  == (S_Addr - 1)) && (nextq(fq)->StartAddr == (S_Addr + size)) ){
	  bm = TRUE ;
	  fm = TRUE ;
	  leftq = fq;
	  rightq = nextq(fq);
	  break;
       }

       /* fq -> this_free */
       if (endq(fq)  == (S_Addr - 1) ){
	  bm = TRUE ;
	  leftq = fq;
	  break;
       }

       /* this_free -> fq */
       if ( startq(fq) == (S_Addr + size) ) {
	  fm = TRUE ;
	  rightq = fq;
	  break;
       }

       bq=fq;
       fq = nextq(fq);
  }

  DBG_DD(ErrorF("mem.c : viaFreeSurface : 3\n"));


  if ( fm && bm ){
     PrintMemLayOut();
     DBG_DD(ErrorF("mem.c : viaFreeSurface : MergeLR\n"));
     MergeLR(leftq, rightq, S_Addr, size);
     PrintMemLayOut();
     deleteq(rightq);
  }
  else if ( fm ){
     DBG_DD(ErrorF("mem.c : viaFreeSurface : MergeR\n"));
     MergeR(rightq ,S_Addr, size);
  }
  else if ( bm ) {
     DBG_DD(ErrorF("mem.c : viaFreeSurface : MergeL\n"));
     MergeL(leftq ,size);
  }

  /* Merged and return */
  if ( fm || bm ){
     PrintMemLayOut();
     return isFind ;
  }

  /* sort it when insert unused queue */
  fq = MemLayOut->unused;
  bq = nextq(fq);

  /* fq -> NULL */
  if ( bq == NULL ){
     OffMemRange * this_add;
     this_add = (OffMemRange *)xalloc(sizeof(OffMemRange));
     this_add->StartAddr = S_Addr;
     this_add->EndAddr	 = S_Addr + size -1;
     this_add->type	 = ctype;
     this_add->size	 = size;


     if ( startq(fq) > S_Addr ) {
	  MemLayOut->unused = this_add;
	  this_add->next    = fq;
     }
     else{
	  fq->next	 = this_add;
	  this_add->next = NULL;
     }
     PrintMemLayOut();
     return isFind;
  }
  /* fq -> bq */
  else{
      while (fq != NULL && bq != NULL){
	  if ( (startq(fq) < S_Addr) && (startq(bq) > S_Addr) )
	      break;
	  else {
	      fq = nextq(fq);
	      bq = nextq(bq);
	  }
      }
  }

  if ( bq == NULL )
     bRightest = TRUE;

  if (MemLayOut->unused == NULL) {
       MemLayOut->unused = xalloc(sizeof(OffMemRange));
       bq= MemLayOut->unused;
       bq->next = NULL;
  }else { /* fq -> bq and bq = NULL*/
       if ( bRightest ) {
	 bq = xalloc(sizeof(OffMemRange));
	 bq->next = NULL;
	 fq->next = bq;
	 bq->EndAddr = S_Addr+size-1;
	 bq->StartAddr = S_Addr;
	 bq->size = size;
       }
       else { /* fq -> bq */
	  OffMemRange * cq;
	  cq = xalloc(sizeof(OffMemRange));
	  fq->next = cq;
	  cq->next = bq;
	  cq->EndAddr = S_Addr+size-1;
	  cq->StartAddr = S_Addr;
	  cq->size = size;
       }
  }


  PrintMemLayOut();
  return isFind ;
 /* ====================================================================== */
}


