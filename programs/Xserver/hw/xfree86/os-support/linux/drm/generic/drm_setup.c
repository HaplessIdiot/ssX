/* drm_setup.c -- Setup/Cleanup for DRM -*- linux-c -*-
 * Created: Mon Jan  4 08:58:31 1999 by faith@precisioninsight.com
 * Revised: Thu Jun 24 15:49:09 1999 by faith@precisioninsight.com
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
 * $PI: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/generic/drm_setup.c,v 1.52 1999/06/24 20:29:42 faith Exp $
 * $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/generic/drm_setup.c,v 1.1 1999/06/14 07:32:04 dawes Exp $
 *
 */

#define EXPORT_SYMTAB
#include "drmP.h"
EXPORT_SYMBOL(drm_init);
EXPORT_SYMBOL(drm_cleanup);
EXPORT_SYMBOL(drm_register_driver);
EXPORT_SYMBOL(drm_unregister_driver);

#define DRM_DESC     "DRM Driver Base"
#define DRM_GEN_DESC "Generic DRM Driver"

int                           drm_flags         = 0;
int                           drm_major         = DRM_DEV_MAJOR;
int                           drm_base_minor    = 0;
drm_driver_t                  *drm_drivers      = NULL;
drm_driver_t                  *drm_drivers_tail = NULL;
drm_device_t                  drm_devices[DRM_MAX_DEVICES];
spinlock_t                    drm_meta_lock     = SPIN_LOCK_UNLOCKED;
spinlock_t                    drm_mem_lock      = SPIN_LOCK_UNLOCKED;
drm_file_t                    *drm_file_first   = NULL;
drm_file_t                    *drm_file_last    = NULL;
unsigned long                 drm_ram_available = 0;
unsigned long                 drm_ram_used      = 0;

drm_mem_stats_t               drm_mem_stats[] = {
	[DRM_MEM_DMA]      = { "dmabufs"  },
	[DRM_MEM_SAREA]    = { "sareas"   },
	[DRM_MEM_DRIVER]   = { "driver"   },
	[DRM_MEM_MAGIC]    = { "magic"    },
	[DRM_MEM_IOCTLS]   = { "ioctltab" },
	[DRM_MEM_MAPS]     = { "maplist"  },
	[DRM_MEM_VMAS]     = { "vmalist"  },
	[DRM_MEM_BUFS]     = { "buflist"  },
	[DRM_MEM_SEGS]     = { "seglist"  },
	[DRM_MEM_PAGES]    = { "pagelist" },
	[DRM_MEM_FILES]    = { "files"    },
	[DRM_MEM_QUEUES]   = { "queues"   },
	[DRM_MEM_CMDS]     = { "commands" },
	[DRM_MEM_MAPPINGS] = { "mappings" },
	[DRM_MEM_BUFLISTS] = { "buflists" },
	{ NULL, 0, }		/* Last entry must be null */
};

struct vm_operations_struct   drm_vm_ops = {
	nopage:  drm_vm_nopage,
	open:    drm_vm_open,
	close:   drm_vm_close,
};

struct vm_operations_struct   drm_vm_shm_ops = {
	nopage:  drm_vm_shm_nopage,
	open:    drm_vm_open,
	close:   drm_vm_close,
};

struct vm_operations_struct   drm_vm_dma_ops = {
	nopage:  drm_vm_dma_nopage,
	open:    drm_vm_open,
	close:   drm_vm_close,
};

static struct file_operations drm_fops = {
	open:    drm_open,
	flush:   drm_flush,
	release: drm_release,
	ioctl:   drm_ioctl,
	mmap:    drm_mmap,
	read:    drm_read,
	fasync:  drm_fasync,
};

static drm_version_t          drm_base_version = {
	DRM_MAJOR,
	DRM_MINOR,
	DRM_PATCHLEVEL,
	sizeof(DRM_NAME),
	(char *)DRM_NAME,
	sizeof(DRM_DATE),
	(char *)DRM_DATE,
	sizeof(DRM_DESC),
	(char *)DRM_DESC
};

				/* All of these values must be consecutive
                                   small integers < DRM_BASE_IOCTL_COUNT. */
static drm_ioctl_desc_t       drm_base_ioctls[] = {
	[DRM_IOCTL_NR(DRM_IOCTL_VERSION)]   = {drm_version,   0, DRM_BASE},
	[DRM_IOCTL_NR(DRM_IOCTL_LIST)]      = {drm_list,      0, DRM_BASE},
	[DRM_IOCTL_NR(DRM_IOCTL_IRQ_BUSID)] = {drm_irq_busid, 0, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_CREATE)] = {drm_create, 1,DRM_BASE|DRM_NOLOCK},
	[DRM_IOCTL_NR(DRM_IOCTL_DESTROY)]= {drm_destroy,1,DRM_BASE|DRM_NOLOCK},
};
#define DRM_BASE_IOCTL_COUNT DRM_ARRAY_SIZE(drm_base_ioctls)

static drm_version_t          drm_gen_version = {
	DRM_MAJOR,
	DRM_MINOR,
	DRM_PATCHLEVEL,
	sizeof(DRM_GEN_NAME),
	(char *)DRM_GEN_NAME,
	sizeof(DRM_DATE),
	(char *)DRM_DATE,
	sizeof(DRM_GEN_DESC),
	(char *)DRM_GEN_DESC
};

static drm_ioctl_desc_t       drm_gen_ioctls[] = {
	[DRM_IOCTL_NR(DRM_IOCTL_BLOCK)]      = { drm_block,    1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_UNBLOCK)]    = { drm_unblock,  1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_CONTROL)]    = { drm_control,  1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_GET_MAGIC)]  = { drm_getmagic, 0, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_AUTH_MAGIC)] = { drm_authmagic,1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_ADD_MAP)]    = { drm_addmap,   1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_ADD_BUFS)]   = { drm_addbufs,  1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_MARK_BUFS)]  = { drm_markbufs, 1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_INFO_BUFS)]  = { drm_infobufs, 0, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_MAP_BUFS)]   = { drm_mapbufs,  0, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_FREE_BUFS)]  = { drm_freebufs, 0, 0 },
	
	[DRM_IOCTL_NR(DRM_IOCTL_ADD_CTX)]    = { drm_addctx,   1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_RM_CTX)]     = { drm_rmctx,    1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_MOD_CTX)]    = { drm_modctx,   1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_GET_CTX)]    = { drm_getctx,   0, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_SWITCH_CTX)] = { drm_switchctx,1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_NEW_CTX)]    = { drm_newctx,   1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_RES_CTX)]    = { drm_resctx,   0, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_ADD_DRAW)]   = { drm_adddraw,  1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_RM_DRAW)]    = { drm_rmdraw,   1, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_DMA)]        = { drm_dma,      0, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_LOCK)]       = { drm_lock,     0, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_UNLOCK)]     = { drm_unlock,   0, 0 },
	[DRM_IOCTL_NR(DRM_IOCTL_FINISH)]     = { drm_finish,   0, 0 },
};
#define DRM_GEN_IOCTL_COUNT DRM_ARRAY_SIZE(drm_gen_ioctls)

static drm_func_desc_t        drm_gen_funcs = {
	reg:   drm_reg,
	unreg: drm_unreg,
};


#ifdef MODULE
int                           init_module(void);
void                          cleanup_module(void);
static char                   *drm = NULL;

MODULE_AUTHOR("Precision Insight, Inc., Cedar Park, Texas.");
MODULE_DESCRIPTION("Direct Rendering Manager");
MODULE_PARM(drm, "s");

/* init_module is called when insmod is used to load the module */

int init_module(void)
{
	return drm_init();
}

/* cleanup_module is called when rmmod is used to unload the module */

void cleanup_module(void)
{
	drm_cleanup();
}
#endif

/* drm_parse_option parses a single option.  See description for
   drm_parse_drm for details. */

static void drm_parse_option(char *s)
{
	char *c, *r;
	
	DRM_TRACE("\"%s\"\n", s);
	if (!s || !*s) return;
	for (c = s; *c && *c != ':'; c++); /* find : or \0 */
	if (*c) r = c + 1; else r = NULL;  /* remember remainder */
	*c = '\0';		           /* terminate */
	if (!strcmp(s, "device")) {
		for (c = r; c && *c && *c != ':'; c++); /* find : or \0 */
		if (!c || !*c) {
			DRM_ERROR("Usage: device:<major>\n");
			return;
		}
		*c++ = '\0';
		drm_major = simple_strtoul(r, NULL, 0);
		DRM_DEBUG("Using device major %d\n", drm_major);
		return;
	}
	if (!strcmp(s, "noctx")) {
		drm_flags |= DRM_FLAG_NOCTX;
		return;
	}
	if (!strcmp(s, "debug")) {
		char *h, *t, *n;

		if (!r) {
			DRM_ERROR("Usage: debug:<flag>[:<flag>]*\n");
			return;
		}

		for (h = t = n = r; h && *h; h = t = n) {
			for (; *t && *t != ':'; t++);     /* find : or \0 */
			if (*t) n = t + 1; else n = NULL; /* remember next */
			*t = '\0';                        /* terminate */
			if (!strcmp(h, "on")) {
				drm_flags |= DRM_FLAG_DEBUG;
				continue;
			}
			if (!strcmp(h, "off")) {
				drm_flags = 0;
				continue;
			}
			if (!strcmp(h, "trace")) {
				drm_flags |= DRM_FLAG_TRACE;
				continue;
			}
			if (!strcmp(h, "verb")) {
				drm_flags |= DRM_FLAG_VERB;
				continue;
			}
			if (!strcmp(h, "mem")) {
				drm_flags |= DRM_FLAG_MEM;
				continue;
			}
			if (!strcmp(h, "lock")) {
				drm_flags |= DRM_FLAG_LOCK;
				continue;
			}
			if (!strcmp(h, "all")) {
				drm_flags |= ~0;
				continue;
			}
			DRM_ERROR("\"%s\" is not a valid debug flag\n", h);
		}
		DRM_DEBUG("Debug mask = 0x%08x\n", drm_flags);
		return;
	}
	DRM_ERROR("\"%s\" is not a valid option\n", s);
	return;
}

/* drm_parse_drm parse the insmod "drm=" options, or the command-line
 * options passed to the kernel via LILO.  The grammar of the format is as
 * follows:
 *
 * drm          ::= 'drm=' option_list
 * option_list  ::= option [ ';' option_list ]
 * option       ::= 'device:' major
 *              |   'debug:' debug_list
 *              |   'noctx'
 * major        ::= INTEGER
 * debug_list   ::= debug_option [ ':' debug_list ]
 * debug_option ::= 'on' | 'off' | 'trace' | 'verb' | 'mem' | 'lock' | 'all'
 *
 * Note that 's' contains option_list without the 'drm=' part.
 *
 * device=major,minor specifies the device number used for /dev/drm
 *        if major == 0 then the misc device is used
 *        if major == 0 and minor == 0 then dynamic misc allocation is used
 * debug=on specifies that debugging messages will be printk'd
 * debug=trace specifies that each function call will be logged via printk
 * debug=off turns off all debugging options
 *
 */

static void drm_parse_drm(char *s)
{
	char *h, *t, *n;
	
	DRM_TRACE("\"%s\"\n", s ?: "");
	if (!s || !*s) return;

	for (h = t = n = s; h && *h; h = n) {
		for (; *t && *t != ';'; t++);          /* find ; or \0 */
		if (*t) n = t + 1; else n = NULL;      /* remember next */
		*t = '\0';	                       /* terminate */
		drm_parse_option(h);                   /* parse */
	}
}

#ifndef MODULE
/* drm_setup is called by the kernel to parse command-line options passed
 * via the boot-loader (e.g., LILO).  It calls the insmod option routine,
 * drm_parse_drm.
 *
 * This is not currently supported, since it requires changes to
 * linux/init/main.c. */
 

void __init drm_setup(char *str, int *ints)
{
	DRM_TRACE("\n");
	if (ints[0] != 0) {
		DRM_ERROR("Illegal command line format, ignored\n");
		return;
	}
	drm_parse_drm(str);
}
#endif

/* drm_init is called via init_module at module load time, or via
 * linux/init/main.c (this is not currently supported). */

int drm_init(void)
{
	int                   retcode;
	struct sysinfo        si;
	drm_mem_stats_t       *mem;

	DRM_TRACE("\n");

#ifdef MODULE
	drm_parse_drm(drm);
#endif

	retcode = register_chrdev(drm_major, DRM_NAME, &drm_fops);
	if (retcode < 0) {
		DRM_ERROR("Cannot register /dev/%s on major %d\n",
			  DRM_NAME, drm_major);
		return retcode;
	}
	if (!drm_major) drm_major = retcode;

	for (mem = drm_mem_stats; mem->name; ++mem) {
		mem->succeed_count   = 0;
		mem->free_count      = 0;
		mem->fail_count      = 0;
		mem->bytes_allocated = 0;
		mem->bytes_freed     = 0;
	}
	
	drm_proc_init();
	drm_register_driver_internal(DRM_NAME,
				     &drm_base_version,
				     DRM_BASE_IOCTL_COUNT,
				     drm_base_ioctls,
				     NULL,
				     1);
	drm_register_driver(DRM_GEN_NAME,
			    &drm_gen_version,
			    DRM_GEN_IOCTL_COUNT,
			    drm_gen_ioctls,
			    &drm_gen_funcs);

	drm_base_minor = drm_register_device(DRM_NAME, DRM_NAME, 1);
	if (drm_base_minor < 0) {
		DRM_ERROR("Cannot install %s: %d\n", DRM_NAME, drm_base_minor);
		drm_cleanup();
		return drm_base_minor;
	}

	si_meminfo(&si);
	drm_ram_available = si.totalram;
	drm_ram_used      = 0;

	DRM_INFO("Initialized\n");
	
	return 0;
}

/* drm_cleanup is called via cleanup_module at module unload time. */

void drm_cleanup(void)
{
	int          i;
	drm_driver_t *pt;
	
	DRM_TRACE("\n");

	for (i = DRM_MAX_DEVICES-1; i >= 0; --i) drm_unregister_device(i);
	
	drm_proc_cleanup();

	for (pt = drm_drivers; pt; pt = pt->next) {
		drm_unregister_driver(pt->name);
	}

	if (unregister_chrdev(drm_major, DRM_NAME)) {
		DRM_ERROR("Cannot unload device driver\n");
	} else {
		DRM_INFO("Module unloaded\n");
	}
}

/* drm_open is called whenever a process opens /dev/drm. */

int drm_open(struct inode *inode, struct file *filp)
{
	kdev_t       minor = MINOR(inode->i_rdev);
	drm_device_t *dev;
	drm_driver_t *drv;
	drm_file_t   *priv;

	if (minor >= DRM_MAX_DEVICES) return -EINVAL;
	if (filp->f_flags & O_EXCL)   return -EBUSY; /* No exclusive opens */

	dev = &drm_devices[minor];
	drv = dev->driver;
	
	spin_lock(&dev->count_lock);
	if (!dev->inuse) {
		spin_unlock(&dev->count_lock);
		return -EINVAL;
	}
	++dev->open_count;
	spin_unlock(&dev->count_lock);
	atomic_inc(&dev->total_open);
	MOD_INC_USE_COUNT;
	
	DRM_TRACE("pid = %d, minor = %d, open_count = %d, ioctl_count = %d\n",
		  current->pid, minor, dev->open_count,
		  atomic_read(&dev->ioctl_count));

	priv               = drm_alloc(sizeof(*priv), DRM_MEM_FILES);
	memset(priv, 0, sizeof(*priv));
	filp->private_data = priv;
	priv->uid          = current->euid;
	priv->pid          = current->pid;
	priv->minor        = minor;
	priv->dev          = dev;
	priv->ioctl_count  = 0;

	if (!minor || capable(CAP_SYS_ADMIN)) priv->auth = DRM_AUTH_OK;
	else                                  priv->auth = DRM_AUTH_PENDING;

	spin_lock(&drm_meta_lock);
	if (!drm_file_last) {
		priv->next      = NULL;
		priv->prev      = NULL;
		drm_file_first  = priv;
		drm_file_last   = priv;
	} else {
		priv->next           = NULL;
		priv->prev           = drm_file_last;
		drm_file_last->next  = priv;
		drm_file_last        = priv;
	}
	spin_unlock(&drm_meta_lock);

				/* Callback */
	if (drv->funcs && drv->funcs->open) (drv->funcs->open)(inode, filp);
	return 0;
}

/* The drm_read and drm_write_string code (especially that which manages
   the circular buffer), is based on Alessandro Rubini's LINUX DEVICE
   DRIVERS (Cambridge: O'Reilly, 1998), pages 111-113. */

ssize_t drm_read(struct file *filp, char *buf, size_t count, loff_t *off)
{
	drm_file_t    *priv   = filp->private_data;
	drm_device_t  *dev    = priv->dev;
	int           left;
	int           avail;
	int           send;
	int           cur;

	DRM_TRACE("%p, %p\n", dev->buf_rp, dev->buf_wp);
	
	while (dev->buf_rp == dev->buf_wp) {
		DRM_DEBUG("  sleeping\n");
		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		}
		interruptible_sleep_on(&dev->buf_readers);
		if (signal_pending(current)) {
			DRM_DEBUG("  interrupted\n");
			return -ERESTARTSYS;
		}
		DRM_DEBUG("  awake\n");
	}

	left  = (dev->buf_rp + DRM_BSZ - dev->buf_wp) % DRM_BSZ;
	avail = DRM_BSZ - left;
	send  = DRM_MIN(avail, count);

	while (send) {
		if (dev->buf_wp > dev->buf_rp) {
			cur = DRM_MIN(send, dev->buf_wp - dev->buf_rp);
		} else {
			cur = DRM_MIN(send, dev->buf_end - dev->buf_rp);
		}
		copy_to_user_ret(buf, dev->buf_rp, cur, -EINVAL);
		dev->buf_rp += cur;
		if (dev->buf_rp == dev->buf_end) dev->buf_rp = dev->buf;
		send -= cur;
	}
	
	wake_up_interruptible(&dev->buf_writers);
	return DRM_MIN(avail, count);;
}

int drm_write_string(drm_device_t *dev, const char *s)
{
	int left   = (dev->buf_rp + DRM_BSZ - dev->buf_wp) % DRM_BSZ;
	int send   = strlen(s);
	int count;

	DRM_TRACE("%d left, %d to send (%p, %p)\n",
		  left, send, dev->buf_rp, dev->buf_wp);
	
	if (left == 1 || dev->buf_wp != dev->buf_rp) {
		DRM_ERROR("Buffer not empty (%d left, wp = %p, rp = %p)\n",
			  left,
			  dev->buf_wp,
			  dev->buf_rp);
	}

	while (send) {
		if (dev->buf_wp >= dev->buf_rp) {
			count = DRM_MIN(send, dev->buf_end - dev->buf_wp);
			if (count == left) --count; /* Leave a hole */
		} else {
			count = DRM_MIN(send, dev->buf_rp - dev->buf_wp - 1);
		}
		strncpy(dev->buf_wp, s, count);
		dev->buf_wp += count;
		if (dev->buf_wp == dev->buf_end) dev->buf_wp = dev->buf;
		send -= count;
	}

	if (dev->buf_async) kill_fasync(dev->buf_async, SIGIO);
	DRM_DEBUG("waking\n");
	wake_up_interruptible(&dev->buf_readers);
	return 0;
}

int drm_flush(struct file *filp)
{
	drm_file_t    *priv   = filp->private_data;
	drm_device_t  *dev    = priv->dev;

	DRM_TRACE("pid = %d, minor = %d, open_count = %d\n",
		  current->pid, dev->device_minor, dev->open_count);
	return 0;
}

/* drm_release is called whenever a process closes /dev/drm*.  Linux calls
   this only if any mappings have been closed. */

int drm_release(struct inode *inode, struct file *filp)
{
	drm_file_t    *priv   = filp->private_data;
	drm_device_t  *dev    = priv->dev;
	drm_driver_t  *drv    = dev->driver;

	DRM_TRACE("pid = %d, minor = %d, open_count = %d\n",
		  current->pid, dev->device_minor, dev->open_count);

	drm_reclaim_buffers(dev, priv->pid);

	drm_fasync(-1, filp, 0);

	spin_lock(&drm_meta_lock);
	if (priv->prev) priv->prev->next = priv->next;
	else            drm_file_first   = priv->next;
	if (priv->next) priv->next->prev = priv->prev;
	else            drm_file_last    = priv->prev;
	spin_unlock(&drm_meta_lock);
	
	drm_free(priv, sizeof(*priv), DRM_MEM_FILES);
	
				/* Callback */
	if (drv->funcs && drv->funcs->release) {
		DRM_DEBUG("Release callback at %p\n", drv->funcs->release);
		(drv->funcs->release)(inode, filp);
	}

	MOD_DEC_USE_COUNT;
	atomic_inc(&dev->total_close);
	spin_lock(&dev->count_lock);
	--dev->open_count;
	spin_unlock(&dev->count_lock);

	return 0;
}

int drm_fasync(int fd, struct file *filp, int on)
{
	drm_file_t    *priv   = filp->private_data;
	drm_device_t  *dev    = priv->dev;
	int           retcode;
	
	DRM_TRACE("fd = %d, minor = %d\n", fd, dev->device_minor);
	retcode = fasync_helper(fd, filp, on, &dev->buf_async);
	if (retcode < 0) return retcode;
	return 0;
}

unsigned long drm_vm_nopage(struct vm_area_struct *vma,
			    unsigned long address,
			    int write_access)
{
	DRM_TRACE("0x%08lx, %d\n", address, write_access);

	return 0;		/* Disallow mremap */
}

#ifdef MODULE
static struct mm_struct *drm_get_init_mm(void)
{
				/* This routine is based on the
                                   retrieve_init_mm_ptr routine on p. 288
                                   of Alessandro Rubini's Linux Device
                                   Drivers (Cambridge: O'Reilly, 1998).
                                   This will no longer be necessary if the
                                   kernel ever exports init_mm for use by
                                   modules.

				   We should also get the tasklist_lock,
				   but we can't since it isn't
				   exported... */
	struct task_struct *pt;

	/* read_lock(&tasklist_lock); */
	for (pt = current->next_task; pt != current; pt = pt->next_task) {
		if (pt->pid == 0) {
			/* read_unlock(&tasklist_lock); */
			return pt->mm;
		}
	}
	/* read_unlock(&tasklist_lock); */
	return NULL;
}
#endif

unsigned long drm_vm_shm_nopage(struct vm_area_struct *vma,
				unsigned long address,
				int write_access)
{
	static struct mm_struct *drm_init_mm = NULL;
	pgd_t                   *pgd;
	pmd_t                   *pmd;
	pte_t                   *pte;
	unsigned long           physical;
	unsigned long           kernel_virtual;
	
#ifdef MODULE
	if (!drm_init_mm) drm_init_mm = drm_get_init_mm();
#else
	if (!drm_init_mm) drm_init_mm = &init_mm;
#endif
	
	if (!drm_init_mm) {
		DRM_ERROR("Cannot access kernel page tables\n");
		return 0;	/* This will cause a user-space SIGBUS */
	}

	if (address > vma->vm_end) return 0; /* Disallow mremap */

	kernel_virtual = vma->vm_offset + (address - vma->vm_start);

	kernel_virtual = VMALLOC_VMADDR(kernel_virtual);
	pgd            = pgd_offset(drm_init_mm, kernel_virtual);
	pmd            = pmd_offset(pgd,         kernel_virtual);
	pte            = pte_offset(pmd,         kernel_virtual);
	if (pte_present(*pte)) {
		physical = pte_page(*pte);
		atomic_inc(&mem_map[MAP_NR(physical)].count);
	} else {
		physical = 0;	/* Something bad happened; generate SIGBUS */
	}

	DRM_TRACE("0x%08lx => 0x%08lx => 0x%08lx\n",
		  address, kernel_virtual, physical);
	return physical;
}

unsigned long drm_vm_dma_nopage(struct vm_area_struct *vma,
				unsigned long address,
				int write_access)
{
	drm_file_t      *priv   = vma->vm_file->private_data;
	drm_device_t    *dev    = priv->dev;
	unsigned long   physical;
	unsigned long   offset;
	unsigned long   page;
	
	if (address > vma->vm_end) return 0; /* Disallow mremap */
	if (!dev->pagelist)        return 0; /* Nothing allocated */

	offset   = address - vma->vm_start; /* vm_offset should be 0 */
	page     = offset >> PAGE_SHIFT;
	physical = dev->pagelist[page] + (offset & (~PAGE_MASK));
	atomic_inc(&mem_map[MAP_NR(physical)].count); /* Dec. by kernel */

	DRM_TRACE("0x%08lx (page %lu) => 0x%08lx\n", address, page, physical);
	return physical;
}

void drm_vm_open(struct vm_area_struct *vma)
{
	drm_file_t      *priv   = vma->vm_file->private_data;
	drm_device_t    *dev    = priv->dev;
#if DRM_DEBUG_CODE
	drm_vma_entry_t *vma_entry;
#endif

	DRM_TRACE("0x%08lx,0x%08lx\n",
		  vma->vm_start, vma->vm_end - vma->vm_start);
	atomic_inc(&dev->vma_count);
	MOD_INC_USE_COUNT;

#if DRM_DEBUG_CODE
	vma_entry = drm_alloc(sizeof(*vma_entry), DRM_MEM_VMAS);
	if (vma_entry) {
		spin_lock(&dev->struct_lock);
		vma_entry->vma  = vma;
		vma_entry->next = dev->vmalist;
		vma_entry->pid  = current->pid;
		dev->vmalist    = vma_entry;
		spin_unlock(&dev->struct_lock);
	}
#endif
}

void drm_vm_close(struct vm_area_struct *vma)
{
	drm_file_t      *priv   = vma->vm_file->private_data;
	drm_device_t    *dev    = priv->dev;
#if DRM_DEBUG_CODE
	drm_vma_entry_t *pt, *prev;
#endif

	DRM_TRACE("0x%08lx,0x%08lx\n",
		  vma->vm_start, vma->vm_end - vma->vm_start);
	MOD_DEC_USE_COUNT;
	atomic_dec(&dev->vma_count);

#if DRM_DEBUG_CODE
	spin_lock(&dev->struct_lock);
	for (pt = dev->vmalist, prev = NULL; pt; prev = pt, pt = pt->next) {
		if (pt->vma == vma) {
			if (prev) {
				prev->next = pt->next;
			} else {
				dev->vmalist = pt->next;
			}
			drm_free(pt, sizeof(*pt), DRM_MEM_VMAS);
			break;
		}
	}
	spin_unlock(&dev->struct_lock);
#endif
}

int drm_mmap_dma(struct file *filp, struct vm_area_struct *vma)
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	unsigned long   length  = vma->vm_end - vma->vm_start;
	
	DRM_TRACE("start = 0x%lx, end = 0x%lx, offset = 0x%lx\n",
		  vma->vm_start, vma->vm_end, vma->vm_offset);

				/* Length must match exact page count */
	if ((length >> PAGE_SHIFT) != dev->page_count) return -EINVAL;

	vma->vm_ops   = &drm_vm_dma_ops;
	vma->vm_flags |= VM_LOCKED | VM_SHM; /* Don't swap */
	
#if LINUX_VERSION_CODE < 0x020203 /* KERNEL_VERSION(2,2,3) */
				/* In Linux 2.2.3 and above, this is
                                   handled in do_mmap() in mm/mmap.c. */
	++filp->f_count;
#endif
	vma->vm_file  =  filp;	/* Needed for drm_vm_open() */
	drm_vm_open(vma);
	return 0;
}

int drm_mmap(struct file *filp, struct vm_area_struct *vma)
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_map_t       *map    = NULL;
	int             i;
	
	DRM_TRACE("start = 0x%lx, end = 0x%lx, offset = 0x%lx\n",
		  vma->vm_start, vma->vm_end, vma->vm_offset);

	if (!vma->vm_offset) return drm_mmap_dma(filp, vma);

				/* A sequential search of a linked list is
                                   fine here because: 1) there will only be
                                   about 5-10 entries in the list and, 2) a
                                   DRI client only has to do this mapping
                                   once, so it doesn't have to be optimized
                                   for performance, even if the list was a
                                   bit longer. */
	for (i = 0; i < dev->map_count; i++) {
		map = dev->maplist[i];
		if (map->offset == vma->vm_offset) break;
	}
	
	if (i >= dev->map_count) return -EINVAL;
	if (!map || ((map->flags&_DRM_RESTRICTED) && !capable(CAP_SYS_ADMIN)))
		return -EPERM;

				/* Check for valid size. */
	if (map->size != vma->vm_end - vma->vm_start) return -EINVAL;
	

	switch (map->type) {
	case _DRM_FRAME_BUFFER:
	case _DRM_REGISTERS:
		if (vma->vm_offset >= __pa(high_memory)) {
#if defined(__i386__)
			if (boot_cpu_data.x86 > 3) {
				pgprot_val(vma->vm_page_prot) |= _PAGE_PCD;
				pgprot_val(vma->vm_page_prot) &= ~_PAGE_PWT;
			}
#endif
			vma->vm_flags |= VM_IO;	/* not in core dump */
		}
		if (remap_page_range(vma->vm_start,
				     vma->vm_offset,
				     vma->vm_end - vma->vm_start,
				     vma->vm_page_prot))
				return -EAGAIN;
		vma->vm_ops = &drm_vm_ops;
		break;
	case _DRM_SHM:
		vma->vm_ops = &drm_vm_shm_ops;
				/* Don't let this area swap.  Change when
                                   DRM_KERNEL advisory is supported. */
		vma->vm_flags |= VM_LOCKED;
		break;
	default:
		return -EINVAL;	/* This should never happen. */
	}
	vma->vm_flags |= VM_LOCKED | VM_SHM; /* Don't swap */
	if (map->flags & _DRM_READ_ONLY) {
		pgprot_val(vma->vm_page_prot) &= ~_PAGE_RW;
	}

	
#if LINUX_VERSION_CODE < 0x020203 /* KERNEL_VERSION(2,2,3) */
				/* In Linux 2.2.3 and above, this is
                                   handled in do_mmap() in mm/mmap.c. */
	++filp->f_count;
#endif
	vma->vm_file  =  filp;	/* Needed for drm_vm_open() */
	drm_vm_open(vma);
	return 0;
}


void drm_register_driver_internal(const char *name,
				  drm_version_t *version,
				  int ioctl_count,
				  drm_ioctl_desc_t *ioctls,
				  drm_func_desc_t *funcs,
				  int base)
{
	drm_driver_t *pt;
	int          i;

	DRM_TRACE("\n");
	if (!(pt = drm_alloc(sizeof(*pt), DRM_MEM_DRIVER))) {
		DRM_ERROR("Cannot register driver\n");
		return;
	}

	pt->name        = name;
	pt->version     = version;
	pt->funcs       = funcs;
	pt->next        = NULL;
	pt->refcount    = 0;
	pt->base        = base;


	pt->ioctl_count = DRM_MAX(ioctl_count, DRM_BASE_IOCTL_COUNT);
	pt->ioctls = drm_alloc(pt->ioctl_count * sizeof(*pt->ioctls),
			       DRM_MEM_IOCTLS);
	memset(pt->ioctls, 0, pt->ioctl_count * sizeof(*pt->ioctls));
	for (i = 0; i < ioctl_count; i++) {
		if (i < DRM_BASE_IOCTL_COUNT
		    && (drm_base_ioctls[i].dispatch & DRM_BASE)) { /* force */
			pt->ioctls[i].func      = drm_base_ioctls[i].func;
			pt->ioctls[i].root_only = drm_base_ioctls[i].root_only;
			pt->ioctls[i].dispatch  = drm_base_ioctls[i].dispatch;
		} else if (i < ioctl_count
			   && ioctls[i].func) { /* from driver */
			pt->ioctls[i].func      = ioctls[i].func;
			pt->ioctls[i].root_only = ioctls[i].root_only;
			pt->ioctls[i].dispatch  = ioctls[i].dispatch;
		} else if (i < DRM_BASE_IOCTL_COUNT) { /* fallback */
			pt->ioctls[i].func      = drm_base_ioctls[i].func;
			pt->ioctls[i].root_only = drm_base_ioctls[i].root_only;
			pt->ioctls[i].dispatch  = drm_base_ioctls[i].dispatch;
		}
	}

	spin_lock(&drm_meta_lock);
	if (drm_drivers && drm_drivers_tail) {
		drm_drivers_tail->next = pt;
		drm_drivers_tail       = pt;
	} else {
		drm_drivers = drm_drivers_tail = pt;
	}
	spin_unlock(&drm_meta_lock);

				/* Callback */
	if (funcs && funcs->load) (funcs->load)();
}


void drm_register_driver(const char *name,
			 drm_version_t *version,
			 int ioctl_count,
			 drm_ioctl_desc_t *ioctls,
			 drm_func_desc_t *funcs)
{
	drm_register_driver_internal( name, version, ioctl_count,
				      ioctls, funcs, 0);
}


void drm_unregister_driver(const char *name)
{
	drm_driver_t *pt;
	drm_driver_t *prev;
	
	DRM_TRACE("\n");
	
	spin_lock(&drm_meta_lock);

	for (prev = NULL, pt = drm_drivers; pt; prev = pt, pt = pt->next) {
		if (!strcmp(pt->name, name)) {
			if (drm_drivers == pt) {
				/* At head */
				drm_drivers = pt->next;
			} else {
				/* Not at head */
				prev->next = pt->next;
			}
			if (drm_drivers_tail == pt)
				drm_drivers_tail = pt->next;
			spin_unlock(&drm_meta_lock);

				/* Callback */
			if (pt->funcs && pt->funcs->unload)
				(pt->funcs->unload)();
			drm_free(pt, sizeof(*pt), DRM_MEM_DRIVER);
			return;
		}
	}
	spin_unlock(&drm_meta_lock);
	DRM_ERROR("Cannot unregister driver \"%s\"\n", name);
}

static void drm_dma_schedule_wrapper(unsigned long data)
{
	drm_dma_schedule((drm_device_t *)data, 0);
}

/* drm_register_device registers a device on the next available minor
   number.  If the device can be registered, the minor number is returned.
   Otherwise a value < 0 is returned. */

int drm_register_device(const char *name,
			const char *busid,
			int allow_base)
{
	drm_driver_t *drv;
	drm_device_t *dev  = NULL;
	int          minor;
	char         *buf;
	unsigned int buflen;

	spin_lock(&drm_meta_lock);
	
				/* Use first available slot by default */
	for (minor = 0; minor < DRM_MAX_DEVICES; minor++) {
		spin_lock(&drm_devices[minor].count_lock);
		if (!drm_devices[minor].inuse) {
			dev = &drm_devices[minor];
			memset(dev, 0, sizeof(*dev));
			++dev->inuse;
			spin_unlock(&drm_devices[minor].count_lock);
			break;
		}
		spin_unlock(&drm_devices[minor].count_lock);
	}
	
	if (!dev) {
		DRM_ERROR("Cannot load more then %d devices\n",
			  DRM_MAX_DEVICES );
		spin_unlock(&drm_meta_lock);
		return -EBUSY;
	}


	DRM_TRACE("%s %s using minor %d (%d)\n",
		  name, busid, minor, dev->inuse);

	for (drv = drm_drivers; drv; drv = drv->next) {
		if (!allow_base && drv->base) {
			continue; /* don't allow base to load */
		}
		if (!strcmp(drv->version->name, name)) {
			if (drv->refcount) {
				/* FIXME: Allow more than one device with
                                   the same name for 1) multihead support,
                                   and 2) support for recycling the server
                                   when a client refuses to die quickly
                                   (see drm_unregister_device). */
				dev->inuse = 0;
				spin_unlock(&drm_meta_lock);
				DRM_ERROR("Cannot load %s twice -- FIXME\n",
					  name);
				return -EBUSY;
			}
			buflen = (5 + strlen(DRM_NAME)
				  + strlen(name)
				  + strlen(busid));
			buf    = drm_alloc(buflen, DRM_MEM_DRIVER);
			sprintf(buf, "%s %s %s", DRM_NAME, name, busid);

			dev->device_minor = minor;
			dev->driver       = drv;
			dev->busid        = drm_strdup(busid, DRM_MEM_DRIVER);
			dev->devname      = drm_strdup(buf, DRM_MEM_DRIVER);
			dev->count_lock   = SPIN_LOCK_UNLOCKED;
			dev->struct_lock  = SPIN_LOCK_UNLOCKED;
			dev->buf_rp       = dev->buf;
			dev->buf_wp       = dev->buf;
			dev->buf_end      = dev->buf + DRM_BSZ;
			
			init_timer(&dev->timer);
			dev->timer.function = drm_dma_schedule_wrapper;
			dev->timer.data     = (unsigned long)dev;

			drm_free(buf, buflen, DRM_MEM_DRIVER);
			buf               = NULL;

			init_waitqueue_head(&dev->waiting);
			init_waitqueue_head(&dev->context_wait);
			init_waitqueue_head(&dev->buf_readers);
			init_waitqueue_head(&dev->buf_writers);
			init_waitqueue_head(&dev->lock.lock_queue);
			
				/* The kernel's context could be created
                                   here, but is now created in
                                   drm_dma_enqueue.  This is more
                                   resource-efficient for hardware that
                                   does not do DMA, but may mean that
                                   drm_select_queue fails between the time
                                   the interrupt is initialized and the
                                   time the queues are initialized. */
			
			drm_proc_add_device(dev);
			
			++drv->refcount;
			
				/* Callback */
			if (drv->funcs && drv->funcs->reg) {
				int retcode = (drv->funcs->reg)();
				if (retcode) {
					spin_unlock(&drm_meta_lock);
					return retcode;
				}
			}
			spin_unlock(&drm_meta_lock);
			return minor;
		}
	}

	dev->inuse = 0;
	spin_unlock(&drm_meta_lock);
	DRM_ERROR("Cannot load device \"%s\" \"%s\"\n", name, busid);
	return -EINVAL;
}

/* drm_unregister_device unregisters a device, base on minor number.  If
   the device can be unregistered, 0 is returned.  Otherwise a value < 0 is
   returned. */

int drm_unregister_device(int minor)
{
	drm_driver_t      *drv;
	drm_device_t      *dev;
	int               i, j;
	drm_magic_entry_t *pt, *next;
	drm_map_t         *map;
	drm_vma_entry_t   *vma, *vma_next;

	spin_lock(&drm_meta_lock);
	
	DRM_TRACE("minor = %d\n", minor);
	if (minor < 0 || minor >= DRM_MAX_DEVICES) {
		DRM_ERROR("Cannot unload device (%d,%d)"
			  " -- only %d devices possible\n",
			  drm_major, minor, DRM_MAX_DEVICES);
		spin_unlock(&drm_meta_lock);
		return -EINVAL;
	}
	
	dev = &drm_devices[minor];
	drv = drm_devices[minor].driver;
	
	DRM_DEBUG("inuse = %d, open_count = %d, blocked = %d\n",
		  dev->inuse, dev->open_count, dev->blocked );

	spin_lock(&dev->count_lock);
	if (!dev->inuse) {
		spin_unlock(&dev->count_lock);
		spin_unlock(&drm_meta_lock);
		return -EINVAL;
	}
	if (dev->open_count || atomic_read(&dev->ioctl_count) || dev->blocked){
				/* FIXME: Allow flagging for deletion so
                                   that the server can recycle even if a
                                   client refuses to die quickly.  See
                                   comment in drm_register_device. */
		spin_unlock(&dev->count_lock);
		spin_unlock(&drm_meta_lock);
		DRM_ERROR("Device busy: %s at busid %s (%d, %d, %d)\n",
			  dev->driver->name,
			  dev->busid,
			  dev->open_count,
			  atomic_read(&dev->ioctl_count),
			  dev->blocked);
		return -EBUSY;
	}
	dev->inuse = 0;
	spin_unlock(&dev->count_lock);
	
	--drv->refcount;
	drm_proc_del_device(dev);

				/* Unhook interrupt handler */
	if (dev->irq) drm_irq_uninstall(dev);

				/* Delete timer, if any */
	del_timer(&dev->timer);
		
				/* Free busid */
	drm_strfree(dev->busid, DRM_MEM_DRIVER);
	dev->busid = NULL;

				/* Free devname */
	drm_strfree(dev->devname, DRM_MEM_DRIVER);
	dev->devname = NULL;
		
				/* Clear pid list */
	DRM_DEBUG("pidlist\n");
	for (i = 0; i < DRM_HASH_SIZE; i++) {
		for (pt = dev->magiclist[i].head; pt; pt = next) {
			next = pt->next;
			drm_free(pt, sizeof(*pt), DRM_MEM_MAGIC);
		}
		dev->magiclist[i].head = dev->magiclist[i].tail = NULL;
	}
	
				/* Clear vma list (only built for debugging) */
	DRM_DEBUG("vmalist\n");
	for (vma = dev->vmalist; vma; vma = vma_next) {
		vma_next = vma->next;
		drm_free(vma, sizeof(*vma), DRM_MEM_VMAS);
	}
	
		
				/* Clear map area and mtrr information */
	DRM_DEBUG("maplist = %p, %d\n", dev->maplist, dev->map_count);
	if (dev->maplist) {
		for (i = 0; i < dev->map_count; i++) {
			map = dev->maplist[i];
			switch (map->type) {
			case _DRM_REGISTERS:
			case _DRM_FRAME_BUFFER:
#ifdef CONFIG_MTRR
				if (map->mtrr >= 0) {
					int retcode;
					retcode = mtrr_del(map->mtrr,
							   map->offset,
							   map->size);
					DRM_DEBUG("mtrr_del = %d\n", retcode);
				}
#endif
				drm_ioremapfree(map->handle, map->size);
				break;
			case _DRM_SHM:
				drm_vfree(map->handle, map->size);
				break;
			}
			drm_free(map, sizeof(*map), DRM_MEM_MAPS);
		}
		drm_free(dev->maplist,
			 dev->map_count * sizeof(*dev->maplist),
			 DRM_MEM_MAPS);
	}
	
				/* Clear dma buffers */
	for (i = 0; i <= DRM_MAX_ORDER; i++) {
		if (dev->bufs[i].seg_count) {
			DRM_DEBUG("order %d: buf_count = %d,"
				  " seg_count = %d\n",
				  i,
				  dev->bufs[i].buf_count,
				  dev->bufs[i].seg_count);
			for (j = 0; j < dev->bufs[i].seg_count; j++) {
				drm_free_dma(dev->bufs[i].seglist[j],
					     dev->bufs[i].page_order);
			}
			drm_free(dev->bufs[i].buflist,
				 sizeof(*dev->bufs[0].buflist),
				 DRM_MEM_BUFS);
			drm_free(dev->bufs[i].seglist,
				 sizeof(*dev->bufs[0].seglist),
				 DRM_MEM_SEGS);
			drm_freelist_destroy(&dev->bufs[i].freelist);
		}
		dev->bufs[i].buf_count = 0;
		dev->bufs[i].seg_count = 0;
	}
	if (dev->buflist) {
		drm_free(dev->buflist,
			 dev->buf_count * sizeof(*dev->buflist),
			 DRM_MEM_BUFS);
	}
	if (dev->pagelist) {
		drm_free(dev->pagelist,
			 dev->page_count * sizeof(*dev->pagelist),
			 DRM_MEM_PAGES);
	}
	if (dev->queuelist) {
		for (i = 0; i < dev->queue_count; i++) {
			drm_waitlist_destroy(&dev->queuelist[i]->waitlist);
			if (dev->queuelist[i])
				drm_free(dev->queuelist[i],
					 sizeof(*dev->queuelist[0]),
					 DRM_MEM_QUEUES);
		}
		drm_free(dev->queuelist,
			 dev->queue_slots * sizeof(*dev->queuelist),
			 DRM_MEM_QUEUES);
	}
	for (i = 0; i < _DRM_DESC_MAX; i++) {
		if (dev->cmd[i].inst) {
			drm_free(dev->cmd[i].inst,
				 dev->cmd[i].count
				 * _DRM_INST_LENGTH
				 * sizeof(*dev->cmd[i].inst),
				 DRM_MEM_CMDS);
		}
	}

	dev->buf_count       = 0;
	dev->seg_count       = 0;
	dev->page_count      = 0;
	dev->byte_count      = 0;
	dev->buflist         = NULL;
	dev->pagelist        = NULL;
	dev->lock.hw_lock    = NULL; /* SHM removed */
	dev->lock.pid        = 0;
	wake_up_interruptible(&dev->lock.lock_queue);
	
				/* callback */
	DRM_DEBUG("callbacks\n");
	if (drv->funcs && drv->funcs->unreg) (drv->funcs->unreg)();

	spin_unlock(&drm_meta_lock);
	return 0;
}
