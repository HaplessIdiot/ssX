/* xf86drm.h -- OS-independent header for DRM user-level library interface
 * Created: Tue Jan  5 08:17:23 1999 by faith@precisioninsight.com
 * Revised: Thu Jun 24 14:18:55 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * $PI: xc/programs/Xserver/hw/xfree86/os-support/xf86drm.h,v 1.41 1999/06/24 18:37:13 faith Exp $
 * $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86drm.h,v 1.1 1999/06/14 07:31:57 dawes Exp $
 * 
 */

#ifndef _XF86DRM_H_
#define _XF86DRM_H_

#define DRM_ERR_NO_DEVICE  (-1001)
#define DRM_ERR_NO_ACCESS  (-1002)
#define DRM_ERR_NOT_ROOT   (-1003)
#define DRM_ERR_INVALID    (-1004)
#define DRM_ERR_NO_FD      (-1005)

typedef unsigned long drmHandle,   *drmHandlePtr;   /* To mapped regions    */
typedef unsigned int  drmSize,     *drmSizePtr;	    /* For mapped regions   */
typedef void          *drmAddress, **drmAddressPtr; /* For mapped regions   */
typedef unsigned int  drmContext,  *drmContextPtr;  /* GLXContext handle    */
typedef unsigned int  drmDrawable, *drmDrawablePtr; /* Unused               */
typedef unsigned int  drmMagic,    *drmMagicPtr;    /* Magic for auth       */

typedef struct _drmVersion {
    int     version_major;        /* Major version                          */
    int     version_minor;        /* Minor version                          */
    int     version_patchlevel;   /* Patch level                            */
    int     name_len; 	          /* Length of name buffer                  */
    char    *name;	          /* Name of driver                         */
    int     date_len;             /* Length of date buffer                  */
    char    *date;                /* User-space buffer to hold date         */
    int     desc_len;	          /* Length of desc buffer                  */
    char    *desc;                /* User-space buffer to hold desc         */
} drmVersion, *drmVersionPtr;

typedef struct _drmCapability {
    int              dummy;	  /* Driver capabilities */
} drmCapability, *drmCapabilityPtr;

typedef struct _drmList {
    int              count;	  /* Length of version                      */
    drmVersionPtr    version;	  /* List of versions                       */
    drmCapabilityPtr capability;  /* List of (possibly null) capabilities   */
} drmList, *drmListPtr;

				/* All of these enums *MUST* match with the
                                   kernel implementation -- so do *NOT*
                                   change them!  (The drmlib implementation
                                   will just copy the flags instead of
                                   translating them.) */
typedef enum {
    DRM_FRAME_BUFFER    = 0,      /* WC, no caching, no core dump           */
    DRM_REGISTERS       = 1,      /* no caching, no core dump               */
    DRM_SHM             = 2       /* shared, cached                         */
} drmMapType;

typedef enum {
    DRM_RESTRICTED      = 0x0001, /* Cannot be mapped to client-virtual     */
    DRM_READ_ONLY       = 0x0002, /* Read-only in client-virtual            */
    DRM_LOCKED          = 0x0004, /* Physical pages locked                  */
    DRM_KERNEL          = 0x0008, /* Kernel requires access                 */
    DRM_WRITE_COMBINING = 0x0010, /* Use write-combining, if available      */
    DRM_CONTAINS_LOCK   = 0x0020  /* SHM page that contains lock            */
} drmMapFlags;

typedef enum {			 /* These values *MUST* match drm.h         */
				 /* Flags for DMA buffer dispatch           */
    DRM_DMA_BLOCK        = 0x01, /* Block until buffer dispatched.  Note,
				     the buffer may not yet have been
				     processed by the hardware -- getting a
				     hardware lock with the hardware
				     quiescent will ensure that the buffer
				     has been processed.                    */
    DRM_DMA_WHILE_LOCKED = 0x02, /* Dispatch while lock held                */
    DRM_DMA_PRIORITY     = 0x04, /* High priority dispatch                  */

				 /* Flags for DMA buffer request            */
    DRM_DMA_WAIT         = 0x10, /* Wait for free buffers                   */
    DRM_DMA_SMALLER_OK   = 0x20, /* Smaller-than-requested buffers ok       */
    DRM_DMA_LARGER_OK    = 0x40  /* Larger-than-requested buffers ok        */
} drmDMAFlags;

typedef enum {
    DRM_LOCK_READY      = 0x01, /* Wait until hardware is ready for DMA */
    DRM_LOCK_QUIESCENT  = 0x02, /* Wait until hardware quiescent        */
    DRM_LOCK_FLUSH      = 0x04, /* Flush this context's DMA queue first */
    DRM_LOCK_FLUSH_ALL  = 0x08, /* Flush all DMA queues first           */
				/* These *HALT* flags aren't supported yet
                                   -- they will be used to support the
                                   full-screen DGA-like mode. */
    DRM_HALT_ALL_QUEUES = 0x10, /* Halt all current and future queues   */
    DRM_HALT_CUR_QUEUES = 0x20  /* Halt all current queues              */
} drmLockFlags;

typedef enum {
    DRM_CONTEXT_PRESERVED = 0x01, /* This context is preserved and
				     never swapped. */
    DRM_CONTEXT_2DONLY    = 0x02  /* This context is for 2D rendering only. */
} drmContextFlags, *drmContextFlagsPtr;

typedef enum {
				  /* These are present in drm.h, but there
                                     is no performance-related reason why
                                     they need to match -- it's all done in
                                     a case statement in xf86drm.c */
    DRM_IH_PRE_INST,		  /* Before IH installation                 */
    DRM_IH_POST_INST,	          /* After IH installation                  */
    DRM_IH_SERVICE,	          /* IH                                     */
    DRM_IH_PRE_UNINST,        	  /* Before IH uninstallation               */
    DRM_IH_POST_UNINST,	          /* After IH uninstallation                */
    DRM_DMA_DISPATCH,	          /* DMA dispatch (including ready)         */
    DRM_DMA_READY,		  /* Ready for DMA                          */
    DRM_DMA_IS_READY,	          /* Tests if hardware ready for another DMA*/
    DRM_DMA_QUIESCENT,	          /* HW Sync                                */
    DRM_DESC_MAX
} drmCtlDesc;

typedef struct _drmBufDesc {
    int              count;	  /* Number of buffers of this size         */
    int              size;	  /* Size in bytes                          */
    int              low_mark;	  /* Low water mark                         */
    int              high_mark;	  /* High water mark                        */
} drmBufDesc, *drmBufDescPtr;

typedef struct _drmBufInfo {
    int              count;	  /* Number of buffers described in list    */
    drmBufDescPtr    list;	  /* List of buffer descriptions            */
} drmBufInfo, *drmBufInfoPtr;

typedef struct _drmBuf {
    int              idx;	  /* Index into master buflist              */
    int              total;	  /* Buffer size                            */
    int              used;	  /* Amount of buffer in use (for DMA)      */
    drmAddress       address;	  /* Address                                */
} drmBuf, *drmBufPtr;

typedef struct _drmBufMap {
    int              count;	  /* Number of buffers mapped               */
    drmBufPtr        list;	  /* Buffers                                */
} drmBufMap, *drmBufMapPtr;

typedef struct _drmLock {
    __volatile__ unsigned int lock;
    char                      padding[60];
    /* This is big enough for most current (and future?) architectures: 
       DEC Alpha:              32 bytes
       Intel Merced:           ?
       Intel P5/PPro/PII/PIII: 32 bytes
       Intel StrongARM:        32 bytes
       Intel i386/i486:        16 bytes
       MIPS:                   32 bytes (?)
       Motorola 68k:           16 bytes
       Motorola PowerPC:       32 bytes
       Sun SPARC:              32 bytes
    */
} drmLock, *drmLockPtr;

typedef struct _drmDMAReq {
				  /* Indices here refer to the offset into
				     list in drmBufInfo                     */
    drmContext    context;  	  /* Context handle                         */
    int           send_count;     /* Number of buffers to send              */
    int           *send_list;     /* List of handles to buffers             */
    int           *send_sizes;    /* Lengths of data to send, in bytes      */
    drmDMAFlags   flags;          /* Flags                                  */
    int           request_count;  /* Number of buffers requested            */
    int           request_size;	  /* Desired size of buffers requested      */
    int           *request_list;  /* Buffer information                     */
    int           *request_sizes; /* Minimum acceptable sizes               */
    int           granted_count;  /* Number of buffers granted at this size */
} drmDMAReq, *drmDMAReqPtr;

#if 0
				/* The kernel does this, but it doesn't
                                   seem necessary with recent gcc's.  */
typedef struct { unsigned int a[100]; } __drm_dummy_lock_t;
#define __drm_dummy_lock(lock) (*(__volatile__ __drm_dummy_lock_t *)lock)
#else
#define __drm_dummy_lock(lock) (*(__volatile__ unsigned int *)lock)
#endif

#define DRM_LOCK_HELD  0x80000000 /* Hardware lock is held                 */
#define DRM_LOCK_CONT  0x40000000 /* Hardware lock is contended            */

#if __GNUC__ >= 2 && defined(__i386)
				/* Reflect changes here to drmP.h */
#define DRM_CAS(lock,old,new,__ret)                                    \
	do {                                                           \
                int __dummy;	/* Can't mark eax as clobbered */      \
		__asm__ __volatile__(                                  \
			"lock ; cmpxchg %4,%1\n\t"                     \
                        "setnz %0"                                     \
			: "=d" (__ret),                                \
   			  "=m" (__drm_dummy_lock(lock)),               \
                          "=a" (__dummy)                               \
			: "2" (old),                                   \
			  "r" (new));                                  \
	} while (0)
#endif

#ifndef DRM_CAS
#define DRM_CAS(lock,old,new,ret) /* FAST LOCK FAILS */
#endif

#define DRM_LIGHT_LOCK(fd,lock,context)                                \
	do {                                                           \
                char __ret;                                            \
		DRM_CAS(lock,context,DRM_LOCK_HELD|context,__ret);     \
                if (__ret) drmGetLock(fd,context,0);                   \
        } while(0)

				/* This one counts fast locks -- for
                                   benchmarking only. */
#define DRM_LIGHT_LOCK_COUNT(fd,lock,context,count)                    \
	do {                                                           \
                char __ret;                                            \
		DRM_CAS(lock,context,DRM_LOCK_HELD|context,__ret);     \
                if (__ret) drmGetLock(fd,context,0);                   \
                else       ++count;                                    \
        } while(0)

#define DRM_LOCK(fd,lock,context,flags)                                \
	do {                                                           \
		if (flags) drmGetLock(fd,context,flags);               \
		else       DRM_LIGHT_LOCK(fd,lock,context);            \
	} while(0)
			      
#define DRM_UNLOCK(fd,lock,context)                                    \
	do {                                                           \
                char __ret;                                            \
		DRM_CAS(lock,DRM_LOCK_HELD|context,context,__ret);     \
                if (__ret) drmUnlock(fd,context);                      \
        } while(0)

				/* Simple spin locks */
#define DRM_SPINLOCK(spin,val)                                         \
	do {                                                           \
	    char __ret;                                                \
	    do {                                                       \
		DRM_CAS(spin,0,val,__ret);                             \
		if (__ret) while ((spin)->lock);                       \
	    } while (__ret);                                           \
	} while(0)

#define DRM_SPINLOCK_TAKE(spin,val)                                    \
	do {                                                           \
	    char __ret;                                                \
            int  cur;                                                  \
	    do {                                                       \
                cur = (*spin).lock;                                    \
		DRM_CAS(spin,cur,val,__ret);                           \
	    } while (__ret);                                           \
	} while(0)

#define DRM_SPINLOCK_COUNT(spin,val,count,__ret)                       \
	do {                                                           \
            int  __i;                                                  \
            __ret = 1;                                                 \
            for (__i = 0; __ret && __i < count; __i++) {               \
		DRM_CAS(spin,0,val,__ret);                             \
		if (__ret) for (;__i < count && (spin)->lock; __i++);  \
	    }                                                          \
	} while(0)

#define DRM_SPINUNLOCK(spin,val)                                       \
	do {                                                           \
	    char __ret;                                                \
            if ((*spin).lock == val) { /* else server stole lock */    \
	        do {                                                   \
		    DRM_CAS(spin,val,0,__ret);                         \
	        } while (__ret);                                       \
            }                                                          \
	} while(0)

				/* These constants MUST MATCH drm.h */
#define DRM_INST_LENGTH 5

#define DRM_M_WRITE     0x00
#define DRM_M_WHILE     0x01
#define DRM_M_IF        0x02
#define DRM_M_GOTO      0x03
#define DRM_M_NOOP      0x04
#define DRM_M_RETURN    0x05
#define DRM_M_DO        0x06
#define DRM_M_READ      0x07
#define DRM_M_TEST      0x08
	
#define DRM_T_IMM       0x00	/* MUST BE 0 */
#define DRM_T_LENGTH    0x01
#define DRM_T_ADDRESS   0x02
#define DRM_T_ACC       0x03
	
#define DRM_V_NONE      0x00	/* MUST BE 0 */
#define DRM_V_RSHIFT    0x01
#define DRM_V_LSHIFT    0x02
	
#define DRM_C_EQ        0x01
#define DRM_C_LT        0x02
#define DRM_C_GT        0x03
#define DRM_C_LE        0x04
#define DRM_C_GE        0x05
#define DRM_C_NE        0x06
#define DRM_C_BIT       0x07
	
#define DRM_F_NOOP      0x00
#define DRM_F_DMA       0x01
#define DRM_F_SYNC      0x02
#define DRM_F_EXTEN     0x03
#define DRM_F_ERROR     0x04
#define DRM_F_VERT      0x05
#define DRM_F_CLEAR     0x06

#define DRM_I_CMD(x)    ((x)  & 0xfff)
#define DRM_I_TYPE(x)   (((x) & 0x00f) <<12)
#define DRM_I_MOD(x)    (((x) & 0x00f) <<16)
#define DRM_I_MODVAL(x) (((x) & 0x0ff) <<24)
#define DRM_I_COND(x)   (((x) & 0x00f) <<28)
	
#define DRM_E_CMD(x)    ((x) &0x00000fff)
#define DRM_E_TYPE(x)   (((x)&0x0000f000)>>12)
#define DRM_E_MOD(x)    (((x)&0x000f0000)>>16)
#define DRM_E_MODVAL(x) (((x)&0x0ff00000)>>24)
#define DRM_E_COND(x)   (((x)&0xf0000000)>>28)


#define DRM_I_WRITE(group,offset,type,value,mod,modval)              \
     DRM_I_CMD(DRM_M_WRITE) | DRM_I_TYPE(type)                       \
                            | DRM_I_MOD(mod)                         \
                            | DRM_I_MODVAL(modval)                   \
     ,(group), (offset), (value), 0

#define DRM_I_WRITE_IMM(group,offset,value)                          \
     DRM_I_CMD(DRM_M_WRITE),(group), (offset), (value), 0

#define DRM_I_WHILE(group,offset,type,value,mod,modval,cond)         \
     DRM_I_CMD(DRM_M_WHILE) | DRM_I_TYPE(type)                       \
                            | DRM_I_MOD(mod)                         \
                            | DRM_I_MODVAL(modval)                   \
                            | DRM_I_COND(cond)                       \
     ,(group), (offset), (value), 0

#define DRM_I_WHILE_IMM(group,offset,value,cond)                     \
     DRM_I_CMD(DRM_M_WHILE) | DRM_I_COND(cond)                       \
     ,(group), (offset), (value), 0

#define DRM_I_IF(group,offset,type,value,mod,modval,cond,inst)       \
     DRM_I_CMD(DRM_M_IF)    | DRM_I_TYPE(type)                       \
                            | DRM_I_MOD(mod)                         \
                            | DRM_I_MODVAL(modval)                   \
                            | DRM_I_COND(cond)                       \
     ,(group), (offset), (value), (inst)

#define DRM_I_IF_IMM(group,offset,value,cond,inst)                   \
     DRM_I_CMD(DRM_M_IF)    | DRM_I_COND(cond)                       \
     ,(group), (offset), (value), (inst)

#define DRM_I_TEST(type,value,mod,modval,cond,inst)                  \
     DRM_I_CMD(DRM_M_TEST)  | DRM_I_TYPE(type)                       \
                            | DRM_I_MOD(mod)                         \
                            | DRM_I_MODVAL(modval)                   \
                            | DRM_I_COND(cond)                       \
     ,0, 0, (value), (inst)

#define DRM_I_TEST_IMM(value,cond,inst)                              \
     DRM_I_CMD(DRM_M_TEST)  | DRM_I_COND(cond)                       \
     ,0, 0, (value), (inst)

#define DRM_I_GOTO(inst)    DRM_I_CMD(DRM_M_GOTO),0, 0, 0, (inst)
#define DRM_I_NOOP          DRM_I_CMD(DRM_M_NOOP),   0, 0, 0, 0
#define DRM_I_RETURN(value) DRM_I_CMD(DRM_M_RETURN), 0, 0, (value), 0
#define DRM_I_DO(inst)      DRM_I_CMD(DRM_M_DO),     0, 0, 0, (inst)
#define DRM_I_READ(group,offset) DRM_I_CMD(DRM_M_READ), (group), (offset), 0, 0



/* General user-level programmer's API: unprivileged */
extern int           drmAvailable(void);
extern int           drmOpenDRM(void);
extern int           drmCloseDRM(int fd);
extern drmVersionPtr drmGetVersion(int fd);
extern void          drmFreeVersion(drmVersionPtr);
extern drmListPtr    drmGetVersionList(int fd);
extern void          drmFreeVersionList(drmListPtr);
extern int           drmGetInterruptFromBusID(int fd, int busnum, int devnum,
					      int funcnum);


/* General user-level programmer's API: X server (root) only  */
extern int           drmCreateSub(int fd, const char *name, const char *busid);
extern int           drmDestroySub(int fd, const char *busid);
extern int           drmAuthMagic(int fd, drmMagic magic);
extern int           drmAddMap(int fd,
			       drmHandle offset,
			       drmSize size,
			       drmMapType type,
			       drmMapFlags flags,
			       drmHandlePtr handle);
extern int           drmAddBufs(int fd, int count, int size, int flags);
extern int           drmMarkBufs(int fd, double low, double high);
extern int           drmCreateContext(int fd, drmContextPtr handle);
extern int           drmSetContextFlags(int fd, drmContext context,
					drmContextFlags flags);
extern int           drmGetContextFlags(int fd, drmContext context,
					drmContextFlagsPtr flags);
extern int           drmAddContextTag(int fd, drmContext context, void *tag);
extern int           drmDelContextTag(int fd, drmContext context);
extern void          *drmGetContextTag(int fd, drmContext context);
extern drmContextPtr drmGetReservedContextList(int fd, int *count);
extern void          drmFreeReservedContextList(drmContextPtr);
extern int           drmSwitchToContext(int fd, drmContext context);
extern int           drmDestroyContext(int fd, drmContext handle);
extern int           drmCreateDrawable(int fd, drmDrawablePtr handle);
extern int           drmDestroyDrawable(int fd, drmDrawable handle);
extern int           drmCtlAddCommand(int fd, drmCtlDesc desc,
				      int count, int *inst);
extern int           drmCtlRemoveCommands(int fd);
extern int           drmCtlInstHandler(int fd, int irq);
extern int           drmCtlUninstHandler(int fd);
extern int           drmInstallSIGIOHandler(int fd,
					    void (*f)(int fd,
						      void *oldctx,
						      void *newctx));
extern int           drmRemoveSIGIOHandler(int fd);

/* General user-level programmer's API: authenticated client and/or X */
extern int           drmOpenSub(const char *busid);
extern int           drmCloseSub(int fd);
extern int           drmGetMagic(int fd, drmMagicPtr magic);
extern int           drmMap(int fd,
			    drmHandle handle,
			    drmSize size,
			    drmAddressPtr address);
extern int           drmUnmap(drmAddress address, drmSize size);
extern drmBufInfoPtr drmGetBufInfo(int fd);
extern drmBufMapPtr  drmMapBufs(int fd);
extern int           drmUnmapBufs(drmBufMapPtr bufs);
extern int           drmDMA(int fd, drmDMAReqPtr request);
extern int           drmFreeBufs(int fd, int count, int *list);
extern int           drmGetLock(int fd,
			        drmContext context,
			        drmLockFlags flags);
extern int           drmUnlock(int fd, drmContext context);
extern int           drmFinish(int fd, int context, drmLockFlags flags);

/* Support routines */
extern int           drmError(int err, const char *label);
extern void          *drmMalloc(int size);
extern void          drmFree(void *pt);

/* Hash table routines */
extern void *drmHashCreate(void);
extern int  drmHashDestroy(void *t);
extern int  drmHashLookup(void *t, unsigned long key, void **value);
extern int  drmHashInsert(void *t, unsigned long key, void *value);
extern int  drmHashDelete(void *t, unsigned long key);
extern int  drmHashFirst(void *t, unsigned long *key, void **value);
extern int  drmHashNext(void *t, unsigned long *key, void **value);

/* PRNG routines */
extern void          *drmRandomCreate(unsigned long seed);
extern int           drmRandomDestroy(void *state);
extern unsigned long drmRandom(void *state);
extern double        drmRandomDouble(void *state);

/* Skip list routines */

extern void *drmSLCreate(void);
extern int  drmSLDestroy(void *l);
extern int  drmSLLookup(void *l, unsigned long key, void **value);
extern int  drmSLInsert(void *l, unsigned long key, void *value);
extern int  drmSLDelete(void *l, unsigned long key);
extern int  drmSLNext(void *l, unsigned long *key, void **value);
extern int  drmSLFirst(void *l, unsigned long *key, void **value);
extern void drmSLDump(void *l);
extern int  drmSLLookupNeighbors(void *l, unsigned long key,
				 unsigned long *prev_key, void **prev_value,
				 unsigned long *next_key, void **next_value);

#endif
