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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/via_memmgr.h,v 1.1 2003/04/15 15:35:47 alanh Exp $ */
/* kernel internal memory management */

/*#define   XV_DEBUG	  1*/	  /* write log msg to /var/log/XFree86.0.log */
#define XV_DEBUG 0

#if XV_DEBUG
  #define DBG_DD(x) (x)
#else
  #define DBG_DD(x)
#endif

#ifndef __VIA_MEMMGR_H
#define __VIA_MEMMGR_H

struct offrange
	   {
	    unsigned long      StartAddr;
	    unsigned long      EndAddr;
	    unsigned long      size;
	    unsigned char      type;
	    unsigned long      ID;
	    unsigned char      priority;
	    unsigned char      capability;
	    unsigned char      status;
	    struct offrange    * next ;
	   };

typedef struct offrange OffMemRange ;

typedef struct {
	  OffMemRange	      *unused;
	  OffMemRange	      *used;
	       } ViaOffScrnRec, *ViaOffScrnPtr;

/* user-app use */

typedef struct {
	  unsigned long	      size;
	  unsigned long	      offset;
	  unsigned long	      checkID;
	  unsigned long	      ID;
	  unsigned char	      type;
	  unsigned char	      priority;
	  unsigned char	      capability;
	  unsigned char	      alignment;
	  unsigned char	      num;

	       } ViaMMReq;

typedef struct {
	  unsigned long	      StartAddr;
	  unsigned long	      size;
	  unsigned long	      ID;
	  unsigned char	      type;
	  unsigned char	      priority;
	  unsigned char	      capability;
	       } ViaMMInfo;

/* Xserver Sync Protocol */
typedef struct {
	 unsigned long BeginAddr;
	 unsigned long EndAddr;
	       } XserverSyncK;

#define VIA_MID	 0x00004567

/* memory type */
#define VIDMGR_TYPE_2D			  0x11
#define VIDMGR_TYPE_3D			  0x12
#define VIDMGR_TYPE_VIDEO		  0x13

/* memory priority */
#define VIDMGR_PRI_LOW			  0x21
#define VIDMGR_PRI_NORM			  0x22
#define VIDMGR_PRI_HIGHT		  0x23

/* memory capability  */
#define VIDMGR_CAP_MOVABLE		  0x31
#define VIDMGR_CAP_IMMOVABLE		  0x32

/* memory return value	*/
#define MEM_OK	     0
#define MEM_ERROR   -1


/* memory macro functions */
#define nextq(x)    x->next
#define sizeq(x)    x->size
#define endq(x)	    x->EndAddr
#define startq(x)   x->StartAddr
#define deleteq(x)  free(x)
#define typeq(x)    x->type

#define VIAMGR_INFO_2D		0x00004001
#define VIAMGR_INFO_3D		0x00004002
#define VIAMGR_INFO_VIDEO	0x00004003
#define VIAMGR_ALLOC_2D		0x00004004
#define VIAMGR_ALLOC_3D		0x00004005
#define VIAMGR_ALLOC_VIDEO	0x00004006
#define VIAMGR_FREE_2D		0x00004007
#define VIAMGR_FREE_3D		0x00004008
#define VIAMGR_FREE_VIDEO	0x00004009


void PrintFBMem(void);
OffMemRange * viaAllocSurface(int *size ,int alignment);
BOOL viaFreeSurface(unsigned long S_Addr,int size,unsigned char ctype);

#define PrintMemLayOut	 PrintFBMem

#endif /* end of __VIA_MEMMGR_H */
