/* drmP.h -- Private header for Direct Rendering Manager -*- linux-c -*-
 * Created: Mon Jan  4 10:05:05 1999 by faith@precisioninsight.com
 * Revised: Fri Jun 18 09:47:36 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * All rights reserved.
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
 * $PI: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/generic/drmP.h,v 1.47 1999/06/21 14:31:21 faith Exp $
 * $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/generic/drmP.h,v 1.1 1999/06/14 07:32:03 dawes Exp $
 * 
 */

#ifndef _DRM_P_H_
#define _DRM_P_H_

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/file.h>
#include <linux/pci.h>
#include <linux/wrapper.h>
#include <asm/io.h>
#include <asm/mman.h>
#include <asm/uaccess.h>
#ifdef CONFIG_MTRR
#include <asm/mtrr.h>
#endif
#include "drm.h"
#include "drm_version.h"

#define DRM_DEBUG_CODE 2	  /* Include debugging code (if > 1, then
				     also include looping detection. */
#define DRM_DMA_HISTOGRAM 1	  /* Make histogram of DMA latency. */

#define DRM_FLAG_TRACE 0x0001
#define DRM_FLAG_DEBUG 0x0002
#define DRM_FLAG_VERB  0x0004
#define DRM_FLAG_MEM   0x0008
#define DRM_FLAG_LOCK  0x0010
#define DRM_FLAG_NOCTX 0x1000

#define DRM_HASH_SIZE         16 /* Size of key hash table                */
#define DRM_KERNEL_CONTEXT    0	 /* Change drm_resctx if changed          */
#define DRM_RESERVED_CONTEXTS 1  /* Change drm_resctx if changed          */
#define DRM_LOOPING_LIMIT     5000000
#define DRM_BSZ               1024 /* Buffer size for /dev/drm? output    */
#define DRM_TIME_SLICE        (HZ/20)  /* Time slice for GLXContexts      */
#define DRM_LOCK_SLICE        1	/* Time slice for lock, in jiffies        */

#define DRM_MEM_DMA       0
#define DRM_MEM_SAREA     1
#define DRM_MEM_DRIVER    2
#define DRM_MEM_MAGIC     3
#define DRM_MEM_IOCTLS    4
#define DRM_MEM_MAPS      5
#define DRM_MEM_VMAS      6
#define DRM_MEM_BUFS      7
#define DRM_MEM_SEGS      8
#define DRM_MEM_PAGES     9
#define DRM_MEM_FILES    10
#define DRM_MEM_QUEUES   11
#define DRM_MEM_CMDS     12
#define DRM_MEM_MAPPINGS 13
#define DRM_MEM_BUFLISTS 14

				/* Backward compatibility section */
#ifndef _PAGE_PWT
				/* The name of _PAGE_WT was changed to
                                   _PAGE_PWT in Linux 2.2.6 */
#define _PAGE_PWT _PAGE_WT
#endif
				/* Wait queue declarations changes in 2.3.1 */
#ifndef DECLARE_WAITQUEUE
#define DECLARE_WAITQUEUE(w,c) struct wait_queue w = { c, NULL }
typedef struct wait_queue *wait_queue_head_t;
#define init_waitqueue_head(q) *q = NULL;
#endif

#define __drm_dummy_lock(lock) (*(__volatile__ unsigned int *)lock)
#define _DRM_CAS(lock,old,new,__ret)                                   \
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



				/* Macros to make printk easier */
#define DRM_ERROR(fmt, arg...) \
        printk(KERN_ERR "[" DRM_NAME ":" __FUNCTION__ "] *ERROR* " fmt , ##arg)
#define DRM_MEM_ERROR(area, fmt, arg...) \
        printk(KERN_ERR "[" DRM_NAME ":" __FUNCTION__ ":%s] *ERROR* " fmt , \
               drm_mem_stats[area].name , ##arg)
#define DRM_INFO(fmt, arg...)  printk(KERN_INFO "[" DRM_NAME "] " fmt , ##arg)
#define DRM_MAX(a,b) ((a)>(b)?(a):(b))
#define DRM_MIN(a,b) ((a)<(b)?(a):(b))

#if DRM_DEBUG_CODE
#define DRM_TRACE(fmt, arg...)                                            \
	do {                                                              \
		if (drm_flags&DRM_FLAG_TRACE)                             \
			printk(KERN_DEBUG                                 \
			       "[" DRM_NAME ":" __FUNCTION__ "] " fmt ,   \
			       ##arg);                                    \
	} while (0)
#define DRM_DEBUG(fmt, arg...)                                            \
	do {                                                              \
		if (drm_flags&DRM_FLAG_DEBUG)                             \
			printk(KERN_DEBUG                                 \
			       "[" DRM_NAME ":" __FUNCTION__ "] " fmt ,   \
			       ##arg);                                    \
	} while (0)
#define DRM_VERB(fmt, arg...)                                             \
	do {                                                              \
		if (drm_flags&DRM_FLAG_VERB)                              \
			printk(KERN_DEBUG                                 \
			       "[" DRM_NAME ":" __FUNCTION__ "] " fmt ,   \
			       ##arg);                                    \
	} while (0)
#define DRM_LOCK_VERB(fmt, arg...)                                        \
	do {                                                              \
		if (drm_flags&DRM_FLAG_VERB)                              \
			printk(KERN_DEBUG                                 \
			       "[" DRM_NAME ":" __FUNCTION__ "] " fmt ,   \
			       ##arg);                                    \
	} while (0)
#define DRM_MEM_VERB(area, fmt, arg...)                                   \
	do {                                                              \
		if (drm_flags&DRM_FLAG_MEM)                               \
			printk(KERN_DEBUG                                 \
			       "[" DRM_NAME ":" __FUNCTION__ ":%s] " fmt ,\
			       drm_mem_stats[area].name, ##arg);          \
	} while (0)
#else
#define DRM_TRACE(fmt, arg...)           do { } while (0)
#define DRM_VERB(fmt, arg...)            do { } while (0)
#define DRM_DEBUG(fmt, arg...)           do { } while (0)
#define DRM_LOCK_VERB(fmt, arg...)       do { } while (0)
#define DRM_MEM_VERB(area, fmt, arg...)  do { } while (0)
#define DRM_MEM_ERROR(area, fmt, arg...) do { } while (0)
#endif

				/* Internal types and structures */
#define DRM_ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define DRM_MIN(a,b) ((a)<(b)?(a):(b))

#define DRM_LEFTCOUNT(x) (((x)->rp + (x)->count - (x)->wp) % ((x)->count + 1))
#define DRM_BUFCOUNT(x) ((x)->count - DRM_LEFTCOUNT(x))
#define DRM_WAITCOUNT(dev,idx) DRM_BUFCOUNT(&dev->queuelist[idx]->waitlist)

#define DRM_IOCTL_ARGS \
(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)

typedef int drm_ioctl_t DRM_IOCTL_ARGS;

typedef struct drm_ioctl_desc {
	drm_ioctl_t          *func;
	int                  root_only;
	enum {
		DRM_BASE   = 0x01, /* Always route via base driver      */
		DRM_NOLOCK = 0x02, /* Don't hold read lock for ioctl    */
	}                    dispatch;
} drm_ioctl_desc_t;

typedef struct drm_func_desc {
	int (*reg)(void);	/* Executed at driver registration   */
	int (*unreg)(void);	/* Executed at driver unregistration */
	int (*load)(void);	/* Executed when device loaded       */
	int (*unload)(void);	/* Executed when device unloaded     */
	
				/* These are similar to struct
				   file_operations, but are called after
				   our routines. */
	int (*open)(struct inode *inode, struct file *filp);
	int (*release)(struct inode *inode, struct file *filp);
} drm_func_desc_t;

typedef struct drm_driver {
				/* Defined by driver */
	const char        *name;
	drm_version_t     *version;
	drm_ioctl_desc_t  *ioctls;
	int               ioctl_count;
	drm_func_desc_t   *funcs;
				/* Bookkeeping */
	struct drm_driver *next;
	int               refcount;
	int               base;	/* Base driver, automatically instantiated */
} drm_driver_t;

typedef struct drm_devstate {
	pid_t             owner;        /* X server pid holding x_lock */
	
} drm_devstate_t;

typedef struct drm_magic_entry {
	drm_magic_t            magic;
	struct drm_file        *priv;
	struct drm_magic_entry *next;
} drm_magic_entry_t;

typedef struct drm_magic_head {
       struct drm_magic_entry *head;
       struct drm_magic_entry *tail;
} drm_magic_head_t;

typedef struct drm_vma_entry {
	struct vm_area_struct *vma;
	struct drm_vma_entry  *next;
	pid_t                 pid;
} drm_vma_entry_t;

typedef struct drm_buf {
	int               idx;	       /* Index into master buflist          */
	int               total;       /* Buffer size                        */
	int               order;       /* log-base-2(total)                  */
	int               used;	       /* Amount of buffer in use (for DMA)  */
	unsigned long     offset;      /* Byte offset (used internally)      */
	void              *address;    /* Address of buffer                  */
	struct drm_buf    *next;       /* Kernel-only: used for free list    */
	__volatile__ int  waiting;     /* On kernel DMA queue                */
	__volatile__ int  pending;     /* On hardware DMA queue              */
	wait_queue_head_t dma_wait;    /* Processes waiting                  */
	pid_t             pid;	       /* PID of holding process             */
	int               context;     /* Kernel queue for this buffer       */
	int               while_locked;/* Dispatch this buffer while locked  */
//	int               stamp;       /* Pending stamp                      */
	enum {
		DRM_LIST_NONE    = 0,
		DRM_LIST_FREE    = 1,
		DRM_LIST_WAIT    = 2,
		DRM_LIST_PEND    = 3,
		DRM_LIST_PRIO    = 4,
		DRM_LIST_RECLAIM = 5
	}                 list;	       /* Which list we're on                */
#if DRM_DMA_HISTOGRAM
	cycles_t          time_queued;     /* Queued to kernel DMA queue     */
	cycles_t          time_dispatched; /* Dispatched to hardware         */
	cycles_t          time_completed;  /* Completed by hardware          */
	cycles_t          time_freed;      /* Back on freelist               */
#endif
} drm_buf_t;

#if DRM_DMA_HISTOGRAM
#define DRM_DMA_HISTOGRAM_SLOTS           9
#define DRM_DMA_HISTOGRAM_INITIAL        10
#define DRM_DMA_HISTOGRAM_NEXT(current)  ((current)*10)
typedef struct drm_histogram {
	atomic_t          total;
	
	atomic_t          queued_to_dispatched[DRM_DMA_HISTOGRAM_SLOTS];
	atomic_t          dispatched_to_completed[DRM_DMA_HISTOGRAM_SLOTS];
	atomic_t          completed_to_freed[DRM_DMA_HISTOGRAM_SLOTS];
	
	atomic_t          queued_to_completed[DRM_DMA_HISTOGRAM_SLOTS];
	atomic_t          queued_to_freed[DRM_DMA_HISTOGRAM_SLOTS];
	
	atomic_t          dma[DRM_DMA_HISTOGRAM_SLOTS];
	atomic_t          schedule[DRM_DMA_HISTOGRAM_SLOTS];
	atomic_t          ctx[DRM_DMA_HISTOGRAM_SLOTS];
	atomic_t          lacq[DRM_DMA_HISTOGRAM_SLOTS];
	atomic_t          lhld[DRM_DMA_HISTOGRAM_SLOTS];
} drm_histogram_t;
#endif

				/* bufs is one longer than it has to be */
typedef struct drm_waitlist {
	int               count;        /* Number of possible buffers      */
	drm_buf_t         **bufs;       /* List of pointers to buffers     */
	drm_buf_t         **rp;	        /* Read pointer                    */
	drm_buf_t         **wp;	        /* Write pointer                   */
	drm_buf_t         **end;        /* End pointer                     */
	spinlock_t        read_lock;
	spinlock_t        write_lock;
} drm_waitlist_t;

typedef struct drm_freelist {
	int               initialized; /* Freelist in use                  */
	atomic_t          count;       /* Number of free buffers           */
	drm_buf_t         *next;       /* End pointer                      */
	
	wait_queue_head_t waiting;     /* Processes waiting on free bufs   */
	int               low_mark;    /* Low water mark                   */
	int               high_mark;   /* High water mark                  */
	atomic_t          wfh;	       /* If waiting for high mark         */
} drm_freelist_t;

typedef struct drm_buf_entry {
	int               buf_size;
	int               buf_count;
	drm_buf_t         *buflist;
	int               seg_count;
	int               page_order;
	unsigned long     *seglist;

	drm_freelist_t    freelist;
} drm_buf_entry_t;

typedef struct drm_hw_lock {
	__volatile__ unsigned int lock;
	char                      padding[60];
				 /* We'll make this big enough for most
				    current (and future?) architectures:
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
} drm_hw_lock_t;

typedef struct drm_file {
	enum {
		DRM_AUTH_OK = 0,
		DRM_AUTH_PENDING,
		DRM_AUTH_FAILED
	}                 auth;
	int               minor;
	pid_t             pid;
	uid_t             uid;
	drm_magic_t       magic;
	unsigned long     ioctl_count;
	struct drm_file   *next;
	struct drm_file   *prev;
	struct drm_device *dev;
} drm_file_t;


typedef struct drm_queue {
	atomic_t          use_count;    /* Outstanding uses (+1)            */
	atomic_t          finalization;	/* Finalization in progress         */
	atomic_t          block_count;  /* Count of processes waiting       */
	atomic_t          block_read;   /* Queue blocked for reads          */
	wait_queue_head_t read_queue;  /* Processes waiting on block_read  */
	atomic_t          block_write;  /* Queue blocked for writes         */
	wait_queue_head_t write_queue;	/* Processes waiting on block_write */
	atomic_t          total_queued;	/* Total queued statistic           */
	atomic_t          total_flushed;/* Total flushes statistic          */
	atomic_t          total_locks;	/* Total locks statistics           */
	drm_ctx_flags_t   flags;        /* Context preserving and 2D-only   */
	drm_waitlist_t    waitlist;     /* Pending buffers                  */
	wait_queue_head_t flush_queue; /* Processes waiting until flush    */
} drm_queue_t;

typedef struct drm_device *drm_device_t_dummy_reference;

typedef struct drm_command {
	int               count;        /* Number of instructions          */
	int               *inst;        /* Byte-code instructions          */
	int               (*f)(struct drm_device *dev,
			       drm_desc_t command,
			       unsigned long address,
			       unsigned long length); /* Loadable callback */
} drm_command_t;

typedef struct drm_lock_data {
	drm_hw_lock_t     *hw_lock;     /* Hardware lock                   */
	pid_t             pid;	        /* PID of lock holder (0=kernel)   */
	wait_queue_head_t lock_queue;   /* Queue of blocked processes      */
	unsigned long     lock_time;    /* Time of last lock in jiffies    */
} drm_lock_data_t;

typedef struct drm_device {
	dev_t             device_minor;	/* Minor for this device           */
	drm_driver_t      *driver;
	char              *busid;
	char              *devname;     /* For /proc/interrupts            */
	int               blocked;      /* Blocked due to VC switch?       */
	struct proc_dir_entry *root;    /* Root for this device's entries  */

				/* Locks */
	spinlock_t        count_lock;   /* For inuse, open_count, buf_use  */
	rwlock_t          queue_lock;   /* For queue_count, queuelist      */
	spinlock_t        struct_lock;  /* For others                      */

				/* Usage Counters */
	int               inuse;        /* Slot is in use                  */
	int               open_count;   /* Outstanding files open          */
	atomic_t          ioctl_count;  /* Outstanding IOCTLs pending      */
	atomic_t          vma_count;    /* Outstanding vma areas open      */
	int               buf_use;      /* Buffers in use -- cannot alloc  */
	atomic_t          buf_alloc;    /* Buffer allocation in progress   */

				/* Performance Counters */
	atomic_t          total_open;
	atomic_t          total_close;
	atomic_t          total_ioctl;
	atomic_t          total_irq;    /* Total interruptions             */
	atomic_t          total_prio;   /* Total DRM_DMA_PRIORITY          */
	atomic_t          total_ctx;    /* Total context switches          */
	atomic_t          total_bytes;  /* Total bytes DMA'd               */
	atomic_t          total_dmas;   /* Total DMA buffers dispatched    */

	
	atomic_t          total_missed_dma;  /* Missed drm_do_dma           */
	atomic_t          total_missed_lock; /* Missed lock in drm_do_dma   */
	atomic_t          total_missed_free; /* Missed drm_free_this_buffer */
	atomic_t          total_missed_sched;/* Missed drm_dma_schedule     */

	atomic_t          total_tried;  /* Tried next_buffer                */
	atomic_t          total_hit;    /* Sent next_buffer                 */
	atomic_t          total_lost;   /* Lost interrupt                   */

	atomic_t          total_locks;
	atomic_t          total_unlocks;
	atomic_t          total_contends;
	atomic_t          total_sleeps;

				/* Magic numbers for authentication */
	drm_magic_head_t  magiclist[DRM_HASH_SIZE];

				/* Memory management */
	drm_map_t         **maplist;    /* Vector of pointers to regions   */
	int               map_count;    /* Number of mappable regions      */

	drm_vma_entry_t   *vmalist;     /* List of vmas (for debugging)    */
	drm_lock_data_t   lock;         /* Information on hardware lock    */

	drm_buf_entry_t   bufs[DRM_MAX_ORDER+1];
	int               buf_count;
	drm_buf_t         **buflist;    /* Vector of pointers info bufs    */
	int               seg_count; 
	int               page_count;
	unsigned long     *pagelist;
	unsigned long     byte_count;

				/* DMA queues */
	int               queue_count;  /* Number of active DMA queues     */
	int               queue_reserved; /* Number of reserved DMA queues */
	int               queue_slots;  /* Actual length of queuelist      */
	drm_queue_t       **queuelist;  /* Vector of pointers to DMA queues */

				/* DMA support */
	drm_command_t     cmd[_DRM_DESC_MAX];
	int               irq;	        /* Interrupt used by board         */
	__volatile__ int  context_flag;  /* Context swapping flag          */
	__volatile__ int  interrupt_flag;/* Interruption handler flag      */
	__volatile__ int  dma_flag;      /* DMA dispatch flag              */
	int               last_context; /* Last context to do DMA          */
	unsigned long     last_switch;  /* Number of jiffies at last switch*/
	int               last_checked;	/* Last context checked for DMA    */
	struct timer_list timer;        /* Timer for delaying ctx switch   */
	drm_buf_t         *this_buffer;	/* Buffer being sent               */
	drm_buf_t         *next_buffer; /* Selected buffer to send         */
	drm_queue_t       *next_queue;  /* Queue from which buffer selected*/
	wait_queue_head_t waiting;     /* Processes waiting on free bufs  */
	wait_queue_head_t context_wait;/* Processes waiting on ctx switch */
#if DRM_DMA_HISTOGRAM
	drm_histogram_t   histo;
#endif
	struct tq_struct  tq;
	cycles_t          ctx_start;
	cycles_t          lck_start;
	
				/* Callback to X server for context switch */
	char              buf[DRM_BSZ]; /* Output buffer                 */
	char              *buf_rp;      /* Read pointer                    */
	char              *buf_wp;      /* Write pointer                   */
	char              *buf_end;     /* End pointer                     */
	struct fasync_struct *buf_async;/* Processes waiting for SIGIO     */
	wait_queue_head_t buf_readers; /* Processes waiting to read       */
	wait_queue_head_t buf_writers; /* Processes waiting to ctx switch */
} drm_device_t;

typedef struct drm_mem_stats {
	const char        *name;
	int               succeed_count;
	int               free_count;
	int               fail_count;
	unsigned long     bytes_allocated;
	unsigned long     bytes_freed;
} drm_mem_stats_t;


				/* Internal global variables */

extern int             drm_flags;	  /* Debugging flags                */
extern int             drm_major;	  /* Major device used              */
extern spinlock_t      drm_mem_lock;      /* For drm_mem_stats & drm_ram_*  */
extern drm_mem_stats_t drm_mem_stats[];	  /* Memory usage statistics        */
extern drm_file_t      *drm_file_first;   /* List of per-connection info    */
extern drm_file_t      *drm_file_last;
extern unsigned long   drm_ram_available; /* Total System RAM               */
extern unsigned long   drm_ram_used;      /* RAM locked in kernel space     */

				/* GLOBAL LOCKS */

				/* drm_meta_lock is used to serialize
                                   creation and deletion of drm_drivers,
                                   drm_devices, and drm_file_*. */
extern spinlock_t      drm_meta_lock;


				/* Drivers can be a linked list, because
                                   this never has to be indexed.  This list
                                   is only searched inefficiently once,
                                   when a device is instantiated.*/
extern drm_driver_t *drm_drivers;
extern drm_driver_t *drm_drivers_tail;

				/* Devices needs to be indexed for
                                   efficient ioctls, so use a vector
                                   instead of a linked list.  This
                                   introduces a hard limit on the number of
                                   devices, but the limit is reasonable,
                                   since most people won't have more than
                                   2-3 graphics adapters in their machine.
                                   Further, the limit decreases ioctl
                                   processing time, so the tradeoff is
                                   reasonable. */
extern drm_device_t drm_devices[DRM_MAX_DEVICES];
extern int          drm_device_count;


				/* Internal function definitions */


				/* Setup/cleanup support */
extern void __init drm_setup(char *str, int *ints);
extern int         drm_init(void);
extern void        drm_cleanup(void);
extern void        drm_register_driver(const char *name,
				       drm_version_t *version,
				       int ioctl_count,
				       drm_ioctl_desc_t *ioctls,
				       drm_func_desc_t *funcs);
extern void        drm_register_driver_internal(const char *name,
						drm_version_t *version,
						int ioctl_count,
						drm_ioctl_desc_t *ioctls,
						drm_func_desc_t *funcs,
						int base);
extern void        drm_unregister_driver(const char *name);
extern int         drm_register_device(const char *name,
				       const char *busid,
				       int allow_base);
extern int         drm_unregister_device(int minor);

				/* Device support */
extern int         drm_open(struct inode *inode, struct file *filp);
extern int         drm_flush(struct file *filp);
extern int         drm_release(struct inode *inode, struct file *filp);
extern int         drm_ioctl(struct inode *inode, struct file *filp,
			     unsigned int cmd, unsigned long arg);
extern ssize_t     drm_read(struct file *filp, char *buf, size_t count,
			    loff_t *off);
extern int         drm_fasync(int fd, struct file *filp, int on);
extern int         drm_write_string(drm_device_t *dev, const char *s);

extern int         drm_mmap(struct file *filp, struct vm_area_struct *vma);
extern int         drm_mmap_dma(struct file *filp, struct vm_area_struct *vma);
extern void        drm_vm_open(struct vm_area_struct *vma);
extern void        drm_vm_close(struct vm_area_struct *vma);
extern unsigned long drm_vm_nopage(struct vm_area_struct *vma,
				   unsigned long address,
				   int write_access);
extern unsigned long drm_vm_shm_nopage(struct vm_area_struct *vma,
				       unsigned long address,
				       int write_access);
extern unsigned long drm_vm_dma_nopage(struct vm_area_struct *vma,
				       unsigned long address,
				       int write_access);


				/* Proc support (drm_proc.c) */
extern int         drm_proc_init(void);
extern int         drm_proc_cleanup(void);
extern int         drm_proc_add_device(drm_device_t *dev);
extern int         drm_proc_del_device(drm_device_t *dev);
extern int         drm_driver_info(char *buf, char **start, off_t offset,
				   int len, int *eof, void *data);
extern int         drm_device_info(char *buf, char **start, off_t offset,
				   int len, int *eof, void *data);
extern int         drm_mem_info(char *buf, char **start, off_t offset,
				int len, int *eof, void *data);
extern int         drm_meminfo_info(char *buf, char **start, off_t offset,
				    int len, int *eof, void *data);
extern int         drm_clients_info(char *buf, char **start, off_t offset,
				    int len, int *eof, void *data);
#if DRM_DEBUG_CODE
extern int         drm_vmainfo_info(char *buf, char **start, off_t offset,
				    int len, int *eof, void *data);
#endif
#if DRM_DMA_HISTOGRAM
extern int         drm_histo_info(char *buf, char **start, off_t offset,
				  int len, int *eof, void *data);
#endif
extern int         drm_queues_info(char *buf, char **start, off_t offset,
				   int len, int *eof, void *data);
extern int         drm_bufs_info(char *buf, char **start, off_t offset,
				 int len, int *eof, void *data);

				/* Memory management support (drm_mem.c) */

extern void          *drm_alloc(size_t size, int area);
extern void          drm_free(void *pt, size_t size, int area);
extern char          *drm_strdup(const char *s, int area);
extern void          drm_strfree(const char *s, int area);
extern void          *drm_realloc(void *oldpt, size_t oldsize, size_t size,
				  int area);
extern unsigned long drm_alloc_dma(int order);
extern void          drm_free_dma(unsigned long address, int order);
extern void          *drm_vmalloc(unsigned int size);
extern void          drm_vfree(void *pt, unsigned int size);
extern void          *drm_ioremap(unsigned long offset, unsigned long size);
extern void          drm_ioremapfree(void *pt, unsigned long size);

				/* Buffer list management support
                                   (gen_bufs.c) */
extern int           drm_waitlist_create(drm_waitlist_t *bl, int count);
extern int           drm_waitlist_destroy(drm_waitlist_t *bl);
extern int           drm_waitlist_put(drm_waitlist_t *bl, drm_buf_t *buf);
extern drm_buf_t     *drm_waitlist_get(drm_waitlist_t *bl);

extern int           drm_freelist_create(drm_freelist_t *bl, int count);
extern int           drm_freelist_destroy(drm_freelist_t *bl);
extern int           drm_freelist_put(drm_device_t *dev, drm_freelist_t *bl,
				      drm_buf_t *buf);
extern drm_buf_t     *drm_freelist_get(drm_freelist_t *bl, int block);

extern void          drm_free_buffer(drm_device_t *dev, drm_buf_t *buf);

				/* DMA support (gen_dma.c) */
extern int           drm_irq_install(drm_device_t *dev, int irq);
extern int           drm_irq_uninstall(drm_device_t *dev);
extern int           drm_engine(drm_device_t *dev,
				drm_desc_t command,
				unsigned long address,
				unsigned long length);
extern int           drm_dma_schedule(drm_device_t *dev, int locked);
extern void          drm_wakeup(drm_device_t *dev, drm_buf_t *buf);
extern int           drm_do_dma(drm_device_t *dev, int locked);
extern int           drm_context_switch(drm_device_t *dev, int old, int new);
extern int           drm_context_switch_complete(drm_device_t *dev, int new);
extern void          drm_reclaim_buffers(drm_device_t *dev, pid_t pid);
#if DRM_DMA_HISTOGRAM
extern void          drm_histogram_compute(drm_device_t *dev, drm_buf_t *buf);
extern int           drm_histogram_slot(unsigned long count);
#endif


extern __inline__ int drm_do_command(drm_device_t *dev,
				     drm_desc_t   command)
{
	if (dev->cmd[command].f)
		return dev->cmd[command].f(dev, command, 0, 0);
	return drm_engine(dev, command, 0, 0);
}


				/* BASE IOCTLs */
extern int         drm_version DRM_IOCTL_ARGS;
extern int         drm_list DRM_IOCTL_ARGS;
extern int         drm_create DRM_IOCTL_ARGS;
extern int         drm_destroy DRM_IOCTL_ARGS;
extern int         drm_irq_busid DRM_IOCTL_ARGS;

				/* GENERIC IOCTLs and support functions*/
extern int         drm_order(unsigned long size);
extern int         drm_block DRM_IOCTL_ARGS;
extern int         drm_unblock DRM_IOCTL_ARGS;
extern int         drm_control DRM_IOCTL_ARGS;
extern int         drm_getmagic DRM_IOCTL_ARGS;
extern int         drm_authmagic DRM_IOCTL_ARGS;
extern int         drm_add_magic(drm_device_t *dev, drm_file_t *priv,
				 drm_magic_t magic);
extern int         drm_remove_magic(drm_device_t *dev, drm_magic_t magic);
extern int         drm_addmap DRM_IOCTL_ARGS;
extern int         drm_addbufs DRM_IOCTL_ARGS;
extern int         drm_markbufs DRM_IOCTL_ARGS;
extern int         drm_infobufs DRM_IOCTL_ARGS;
extern int         drm_freebufs DRM_IOCTL_ARGS;
extern int         drm_mapbufs DRM_IOCTL_ARGS;
extern int         drm_dma DRM_IOCTL_ARGS;
extern int         drm_lock DRM_IOCTL_ARGS;
extern int         drm_unlock DRM_IOCTL_ARGS;
extern int         drm_reg( void );
extern int         drm_unreg( void );
extern int         drm_addctx DRM_IOCTL_ARGS;
extern int         drm_resctx DRM_IOCTL_ARGS;
extern int         drm_rmctx DRM_IOCTL_ARGS;
extern int         drm_modctx DRM_IOCTL_ARGS;
extern int         drm_getctx DRM_IOCTL_ARGS;
extern int         drm_newctx DRM_IOCTL_ARGS;
extern int         drm_switchctx DRM_IOCTL_ARGS;
extern int         drm_adddraw DRM_IOCTL_ARGS;
extern int         drm_rmdraw DRM_IOCTL_ARGS;
extern int         drm_auth DRM_IOCTL_ARGS;
extern int         drm_finish DRM_IOCTL_ARGS;

extern int         drm_lock_take(__volatile__ unsigned int *lock,
				 unsigned int context);
extern int         drm_lock_free(drm_device_t *dev,
				 __volatile__ unsigned int *lock,
				 unsigned int context);
#endif
#endif
