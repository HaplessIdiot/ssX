/* drm_proc.c -- /proc support for DRM -*- linux-c -*-
 * Created: Mon Jan 11 09:48:47 1999 by faith@precisioninsight.com
 * Revised: Sun May 16 22:37:45 1999 by faith@precisioninsight.com
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
 * $PI: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/generic/drm_proc.c,v 1.26 1999/05/17 03:27:05 faith Exp $
 * $XFree86$
 *
 */

#define __NO_VERSION__
#include "drmP.h"

static struct proc_dir_entry *drm_root = NULL;

#define DRM_PROC_LIMIT (PAGE_SIZE-80)
#define DRM_PROC_PRINT(fmt, arg...)        \
   len += sprintf(&buf[len], fmt , ##arg); \
   if (len > DRM_PROC_LIMIT) return len;

#define DRM_PROC_PRINT_RET(ret, fmt, arg...)        \
   len += sprintf(&buf[len], fmt , ##arg);          \
   if (len > DRM_PROC_LIMIT) { ret; return len; }

/* drm_proc_init is called from drm_init at module initialization time. */

int drm_proc_init(void)
{
	struct proc_dir_entry *ent;

	drm_root = create_proc_entry(DRM_NAME, S_IFDIR, NULL);
	if (!drm_root) {
		DRM_ERROR("Cannot create /proc/%s entry\n", DRM_NAME);
		return -1;
	}
    
	ent = create_proc_entry(DRM_DRIVERS, S_IFREG|S_IRUGO, drm_root);
	if (!ent) {
		DRM_ERROR("Cannot create /proc/%s/%s entry\n",
			  DRM_NAME, DRM_DRIVERS);
		remove_proc_entry(DRM_NAME, NULL);
		return -1;
	}
	ent->read_proc = drm_driver_info;

	ent = create_proc_entry(DRM_DEVICES, S_IFREG|S_IRUGO, drm_root);
	if (!ent) {
		DRM_ERROR("Cannot create /proc/%s/%s entry\n",
			  DRM_NAME, DRM_DEVICES);
		remove_proc_entry(DRM_DRIVERS, drm_root);
		remove_proc_entry(DRM_NAME, NULL);
		return -1;
	}
	ent->read_proc = drm_device_info;
	
	ent = create_proc_entry(DRM_MEM, S_IFREG|S_IRUGO, drm_root);
	if (!ent) {
		DRM_ERROR("Cannot create /proc/%s/%s entry\n",
			  DRM_NAME, DRM_MEM);
		remove_proc_entry(DRM_DEVICES, drm_root);
		remove_proc_entry(DRM_DRIVERS, drm_root);
		remove_proc_entry(DRM_NAME, NULL);
		return -1;
	}
	ent->read_proc = drm_mem_info;

	ent = create_proc_entry(DRM_CLIENTS, S_IFREG|S_IRUGO, drm_root);
	if (!ent) {
		DRM_ERROR("Cannot create /proc/%s/%s entry\n",
			  DRM_NAME, DRM_CLIENTS);
		remove_proc_entry(DRM_MEM, drm_root);
		remove_proc_entry(DRM_DEVICES, drm_root);
		remove_proc_entry(DRM_DRIVERS, drm_root);
		remove_proc_entry(DRM_NAME, NULL);
		return -1;
	}
	ent->read_proc = drm_clients_info;

	return 0;
}

/* drm_proc_add_device is called from drm_register_device. */

int drm_proc_add_device(drm_device_t *dev)
{
	char                  buf[32];
	struct proc_dir_entry *ent;

	sprintf(buf, "%d", dev->device_minor);
	dev->root = create_proc_entry(buf, S_IFDIR, drm_root);
	if (!dev->root) {
		DRM_ERROR("Cannot create /proc/%s/%s entry\n",
			  DRM_NAME, buf);
		return -1;
	}

	ent = create_proc_entry(DRM_MEMINFO, S_IFREG|S_IRUGO, dev->root);
	if (!ent) {
		DRM_ERROR("Cannot create /proc/%s/%s/%s entry\n",
			  DRM_NAME, buf, DRM_MEMINFO);
		remove_proc_entry(buf, dev->root);
		dev->root = NULL;
		return -1;
	}
	ent->read_proc = drm_meminfo_info;
	ent->data      = dev;
	
	ent = create_proc_entry(DRM_QUEUES, S_IFREG|S_IRUGO, dev->root);
	if (!ent) {
		DRM_ERROR("Cannot create /proc/%s/%s/%s entry\n",
			  DRM_NAME, buf, DRM_QUEUES);
		remove_proc_entry(buf, dev->root);
		dev->root = NULL;
		return -1;
	}
	ent->read_proc = drm_queues_info;
	ent->data      = dev;
	
	ent = create_proc_entry(DRM_BUFS, S_IFREG|S_IRUGO, dev->root);
	if (!ent) {
		DRM_ERROR("Cannot create /proc/%s/%s/%s entry\n",
			  DRM_NAME, buf, DRM_BUFS);
		remove_proc_entry(buf, dev->root);
		dev->root = NULL;
		return -1;
	}
	ent->read_proc = drm_bufs_info;
	ent->data      = dev;

#if DRM_DEBUG_CODE
	ent = create_proc_entry(DRM_VMAINFO, S_IFREG|S_IRUGO, dev->root);
	if (!ent) {
		DRM_ERROR("Cannot create /proc/%s/%s/%s entry\n",
			  DRM_NAME, buf, DRM_VMAINFO);
		remove_proc_entry(buf, dev->root);
		dev->root = NULL;
		return -1;
	}
	ent->read_proc = drm_vmainfo_info;
	ent->data      = dev;
#endif

#if DRM_DMA_HISTOGRAM
	ent = create_proc_entry(DRM_HISTO, S_IFREG|S_IRUGO, dev->root);
	if (!ent) {
		DRM_ERROR("Cannot create /proc/%s/%s/%s entry\n",
			  DRM_NAME, buf, DRM_HISTO);
		remove_proc_entry(buf, dev->root);
		dev->root = NULL;
		return -1;
	}
	ent->read_proc = drm_histo_info;
	ent->data      = dev;
#endif
	
	return 0;
}

/* drm_proc_del_device is called from drm_unregister_device. */

int drm_proc_del_device(drm_device_t *dev)
{
	char buf[32];

	if (dev->root && drm_root) {
#if DRM_DMA_HISTOGRAM
		remove_proc_entry(DRM_HISTO, dev->root);
#endif		
#if DRM_DEBUG_CODE
		remove_proc_entry(DRM_VMAINFO, dev->root);
#endif
		remove_proc_entry(DRM_BUFS,    dev->root);
		remove_proc_entry(DRM_QUEUES,  dev->root);
		remove_proc_entry(DRM_MEMINFO, dev->root);
		sprintf(buf, "%d", dev->device_minor);
		remove_proc_entry(buf, drm_root);
		dev->root = NULL;
	}
	return 0;
}

/* drm_proc_cleanup is called from drm_cleanup at module cleanup time. */

int drm_proc_cleanup(void)
{
	if (drm_root) {
		remove_proc_entry(DRM_CLIENTS, drm_root);
		remove_proc_entry(DRM_MEM, drm_root);
		remove_proc_entry(DRM_DRIVERS, drm_root);
		remove_proc_entry(DRM_DEVICES, drm_root);
		remove_proc_entry(DRM_NAME, NULL);
	}
	drm_root = NULL;

	return 0;
}

/* drm_driver_info is called whenever a process reads /dev/drm/drivers.
   FIXME: should use locks. */

int drm_driver_info(char *buf, char **start, off_t offset, int len,
		    int *eof, void *data)
{
	drm_driver_t *pt;
	
	if (offset > 0) return 0; /* no partial requests */
	len  = 0;
	*eof = 1;
	DRM_PROC_PRINT("b name       major minor patch       date desc\n\n");
	for (pt = drm_drivers; pt; pt = pt->next) {
		DRM_PROC_PRINT("%c %-10s %5d %5d %5d \"%s\" \"%s\"\n",
			       pt->base ? '*' : ' ',
			       pt->name ?: "?",
			       pt->version->version_major,
			       pt->version->version_minor,
			       pt->version->version_patchlevel,
			       pt->version->date ?: "",
			       pt->version->desc ?: "");
	}
	
	return len;
}

/* drm_device_info is called whenever a process reads /dev/drm/devices.
   FIXME: should use locks. */

int drm_device_info(char *buf, char **start, off_t offset, int len,
		    int *eof, void *data)
{
	int          i;

	if (offset > 0) return 0; /* no partial requests */
	len  = 0;
	*eof = 1;
	for (i = 0; i < DRM_MAX_DEVICES; i++) {
		if (!drm_devices[i].inuse) continue;
		DRM_PROC_PRINT("%3d %s %s\n",
			       i,
			       drm_devices[i].driver->name,
			       drm_devices[i].busid);
	}
	
	return len;
}

/* drm_mem_info is called whenever a process reads /dev/drm/mem. */

int drm_mem_info(char *buf, char **start, off_t offset, int len,
		 int *eof, void *data)
{
	drm_mem_stats_t *pt;

	if (offset > 0) return 0; /* no partial requests */
	len  = 0;
	*eof = 1;
	DRM_PROC_PRINT("                  total counts                  "
		       " |    outstanding  \n");
	DRM_PROC_PRINT("type       alloc freed fail     bytes      freed"
		       " | allocs      bytes\n\n");
	DRM_PROC_PRINT("%-9.9s %5d %5d %4d %10lu            |\n",
		       "system", 0, 0, 0, drm_ram_available);
	DRM_PROC_PRINT("%-9.9s %5d %5d %4d %10lu            |\n",
		       "locked", 0, 0, 0, drm_ram_used);
	DRM_PROC_PRINT("\n");
	for (pt = drm_mem_stats; pt->name; pt++) {
		DRM_PROC_PRINT("%-9.9s %5d %5d %4d %10lu %10lu | %6d %10ld\n",
			       pt->name,
			       pt->succeed_count,
			       pt->free_count,
			       pt->fail_count,
			       pt->bytes_allocated,
			       pt->bytes_freed,
			       pt->succeed_count - pt->free_count,
			       (long)pt->bytes_allocated
			       - (long)pt->bytes_freed);
	}
	
	return len;
}

/* drm_meminfo_info is called whenever a process reads
   /dev/drm/<dev>/meminfo.  FIXME: should use locks. */

int drm_meminfo_info(char *buf, char **start, off_t offset, int len,
		     int *eof, void *data)
{
	drm_device_t *dev = (drm_device_t *)data;
	drm_map_t    *map;
	const char   *types[] = { "FB", "REG", "SHM" };
	const char   *type;
	int          i;

	if (offset > 0) return 0; /* no partial requests */
	len  = 0;
	*eof = 1;
	DRM_PROC_PRINT("vma use count: %d\n", dev->vma_count.counter);
	DRM_PROC_PRINT("kB used for dma buffers: %lu\n",
		       dev->byte_count / 1024);
	for (i = 0; i < DRM_MAX_ORDER+1; i++) {
		if (dev->bufs[i].seg_count) {
			DRM_PROC_PRINT("order: %2d; size: %6d;"
				       " bufs: %4d; segs: %4d;"
				       " pages: %4d; kB: %4lu\n",
				       i,
				       dev->bufs[i].buf_size,
				       dev->bufs[i].buf_count,
				       dev->bufs[i].seg_count,
				       dev->bufs[i].seg_count
				       *(1 << dev->bufs[i].page_order),
				       (dev->bufs[i].seg_count
					* (1 << dev->bufs[i].page_order))
				       * PAGE_SIZE / 1024);
		}
	}
	DRM_PROC_PRINT("\nslot     offset       size type flags    "
		       "address mtrr\n\n");
	for (i = 0; i < dev->map_count; i++) {
		map = dev->maplist[i];
		if (map->type < 0 || map->type > 2) type = "??";
		else                                type = types[map->type];
		DRM_PROC_PRINT("%4d 0x%08lx 0x%08lx %4.4s  0x%02x 0x%08lx ",
			       i,
			       map->offset,
			       map->size,
			       type,
			       map->flags,
			       (unsigned long)map->handle);
		if (map->mtrr < 0) {
			DRM_PROC_PRINT("none\n");
		} else {
			DRM_PROC_PRINT("%4d\n", map->mtrr);
		}
	}

	return len;
}

/* drm_queues_info is called whenever a process reads
   /dev/drm/<dev>/queues. */

int drm_queues_info(char *buf, char **start, off_t offset, int len,
		     int *eof, void *data)
{
	drm_device_t *dev = (drm_device_t *)data;
	int          i;
	drm_queue_t  *q;

	if (offset > 0) return 0; /* no partial requests */
	len  = 0;
	*eof = 1;
	DRM_PROC_PRINT("  ctx/flags   use   fin"
		       "   blk/rw/rwf  wait    flushed     queued"
		       "      locks\n\n");
	for (i = 0; i < dev->queue_count; i++) {
		q = dev->queuelist[i];
		atomic_inc(&q->use_count);
		DRM_PROC_PRINT_RET(atomic_dec(&q->use_count),
				   "%5d/0x%03x %5d %5d"
				   " %5d/%c%c/%c%c%c %5d %10d %10d %10d\n",
				   i,
				   q->flags,
				   atomic_read(&q->use_count),
				   atomic_read(&q->finalization),
				   atomic_read(&q->block_count),
				   atomic_read(&q->block_read) ? 'r' : '-',
				   atomic_read(&q->block_write) ? 'w' : '-',
				   waitqueue_active(&q->read_queue) ? 'r':'-',
				   waitqueue_active(&q->write_queue) ? 'w':'-',
				   waitqueue_active(&q->flush_queue) ? 'f':'-',
				   DRM_BUFCOUNT(&q->waitlist),
				   atomic_read(&q->total_flushed),
				   atomic_read(&q->total_queued),
				   atomic_read(&q->total_locks));
		atomic_dec(&q->use_count);
	}
	
	return len;
}

/* drm_bufs_info is called whenever a process reads
   /dev/drm/<dev>/bufs.  FIXME: should use locks. */

int drm_bufs_info(char *buf, char **start, off_t offset, int len,
		  int *eof, void *data)
{
	drm_device_t *dev  = (drm_device_t *)data;
	int          i;

	if (offset > 0) return 0; /* no partial requests */
	len  = 0;
	*eof = 1;
	DRM_PROC_PRINT(" o     size  count  free\n\n");
	for (i = 0; i <= DRM_MAX_ORDER; i++) {
		if (dev->bufs[i].buf_count)
			DRM_PROC_PRINT("%2d %8d %5d %5d\n",
				       i,
				       dev->bufs[i].buf_size,
				       dev->bufs[i].buf_count,
				       atomic_read(&dev->bufs[i]
						   .freelist.count));
	}
	DRM_PROC_PRINT("\n");
	for (i = 0; i < dev->buf_count; i++) {
		if (i && !(i%32)) DRM_PROC_PRINT("\n");
		DRM_PROC_PRINT(" %d", dev->buflist[i]->list);
	}
	DRM_PROC_PRINT("\n");

	return len;
}

/* drm_clients_info is called whenever a process reads
   /dev/drm/<dev>/clients.  FIXME: should use locks. */

int drm_clients_info(char *buf, char **start, off_t offset, int len,
		     int *eof, void *data)
{
	drm_file_t   *priv;
	const char   auths[] = { 'a', 'p', 'u' };

	if (offset > 0) return 0; /* no partial requests */
	len  = 0;
	*eof = 1;
	DRM_PROC_PRINT("a dev   pid    uid                  key"
		       "     ioctls\n\n");
	for (priv = drm_file_first; priv; priv = priv->next) {
		DRM_PROC_PRINT("%c %3d %5d %5d 0x%08x:0x%08x %10lu\n",
			       priv->auth > 2 ? '?' : auths[priv->auth],
                               priv->minor,
			       priv->pid,
			       priv->uid,
			       priv->hi,
			       priv->lo,
			       priv->ioctl_count);
	}

	return len;
}


#if DRM_DEBUG_CODE
/* drm_vmainfo_info is called whenever a process reads
   /dev/drm/<dev>/vmainfo.  FIXME: should use locks. */

int drm_vmainfo_info(char *buf, char **start, off_t offset, int len,
		     int *eof, void *data)
{
	drm_device_t          *dev = (drm_device_t *)data;
	drm_vma_entry_t       *pt;
	pgd_t                 *pgd;
	pmd_t                 *pmd;
	pte_t                 *pte;
	unsigned long         i;
	struct vm_area_struct *vma;
	unsigned long         address;
#if defined(__i386__)
	unsigned int          pgprot;
#endif

	if (offset > 0) return 0; /* no partial requests */
	len  = 0;
	*eof = 1;
	DRM_PROC_PRINT("vma use count: %d, high_memory = %p, 0x%08lx\n",
		       dev->vma_count.counter,
		       high_memory, virt_to_phys(high_memory));
	for (pt = dev->vmalist; pt; pt = pt->next) {
		if (!(vma = pt->vma)) continue;
		DRM_PROC_PRINT("\n%5d 0x%08lx-0x%08lx %c%c%c%c%c%c 0x%08lx",
			       pt->pid,
			       vma->vm_start,
			       vma->vm_end,
			       vma->vm_flags & VM_READ     ? 'r' : '-',
			       vma->vm_flags & VM_WRITE    ? 'w' : '-',
			       vma->vm_flags & VM_EXEC     ? 'x' : '-',
			       vma->vm_flags & VM_MAYSHARE ? 's' : 'p',
			       vma->vm_flags & VM_LOCKED   ? 'l' : '-',
			       vma->vm_flags & VM_IO       ? 'i' : '-',
			       vma->vm_offset );
#if defined(__i386__)
		pgprot = pgprot_val(vma->vm_page_prot);
		DRM_PROC_PRINT(" %c%c%c%c%c%c%c%c%c",
			       pgprot & _PAGE_PRESENT  ? 'p' : '-',
			       pgprot & _PAGE_RW       ? 'w' : 'r',
			       pgprot & _PAGE_USER     ? 'u' : 's',
			       pgprot & _PAGE_WT       ? 't' : 'b',
			       pgprot & _PAGE_PCD      ? 'u' : 'c',
			       pgprot & _PAGE_ACCESSED ? 'a' : '-',
			       pgprot & _PAGE_DIRTY    ? 'd' : '-',
			       pgprot & _PAGE_4M       ? 'm' : 'k',
			       pgprot & _PAGE_GLOBAL   ? 'g' : 'l' );
#endif		
		DRM_PROC_PRINT("\n");
		for (i = vma->vm_start; i < vma->vm_end; i += PAGE_SIZE) {
			pgd = pgd_offset(vma->vm_mm, i);
			pmd = pmd_offset(pgd, i);
			pte = pte_offset(pmd, i);
			if (pte_present(*pte)) {
				address = __pa(pte_page(*pte))
					+ (i & (PAGE_SIZE-1));
				DRM_PROC_PRINT("      0x%08lx -> 0x%08lx"
					       " %c%c%c%c%c\n",
					       i,
					       address,
					       pte_read(*pte)  ? 'r' : '-',
					       pte_write(*pte) ? 'w' : '-',
					       pte_exec(*pte)  ? 'x' : '-',
					       pte_dirty(*pte) ? 'd' : '-',
					       pte_young(*pte) ? 'a' : '-' );
			} else {
				DRM_PROC_PRINT("      0x%08lx\n", i);
			}
		}
	}
	
	return len;
}
#endif


#if DRM_DMA_HISTOGRAM
int drm_histo_info(char *buf, char **start, off_t offset, int len,
		   int *eof, void *data)
{
	drm_device_t  *dev = (drm_device_t *)data;
	int           i;
	unsigned long slot_value = DRM_DMA_HISTOGRAM_INITIAL;
	unsigned long prev_value = 0;
	drm_buf_t     *buffer;

	if (offset > 0) return 0; /* no partial requests */
	len  = 0;
	*eof = 1;

	DRM_PROC_PRINT("total  %10u\n", atomic_read(&dev->histo.total));
	DRM_PROC_PRINT("open   %10u\n", atomic_read(&dev->total_open));
	DRM_PROC_PRINT("close  %10u\n", atomic_read(&dev->total_close));
	DRM_PROC_PRINT("ioctl  %10u\n", atomic_read(&dev->total_ioctl));
	DRM_PROC_PRINT("irq    %10u\n", atomic_read(&dev->total_irq));
	DRM_PROC_PRINT("prio   %10u\n", atomic_read(&dev->total_prio));
	DRM_PROC_PRINT("ctx    %10u\n", atomic_read(&dev->total_ctx));
	DRM_PROC_PRINT("bytes  %10u\n", atomic_read(&dev->total_bytes));
	DRM_PROC_PRINT("dmas   %10u\n", atomic_read(&dev->total_dmas));

	DRM_PROC_PRINT("\nmissed:\n");
	DRM_PROC_PRINT(" dma   %10u\n", atomic_read(&dev->total_missed_dma));
	DRM_PROC_PRINT(" lock  %10u\n", atomic_read(&dev->total_missed_lock));
	DRM_PROC_PRINT(" free  %10u\n", atomic_read(&dev->total_missed_free));
	DRM_PROC_PRINT(" sched %10u\n", atomic_read(&dev->total_missed_sched));

	DRM_PROC_PRINT("tried  %10u\n", atomic_read(&dev->total_tried));
	DRM_PROC_PRINT("hit    %10u\n", atomic_read(&dev->total_hit));
	DRM_PROC_PRINT("lost   %10u\n", atomic_read(&dev->total_lost));

	DRM_PROC_PRINT("\nlock statistics:\n");
	DRM_PROC_PRINT("locks    %10u\n", atomic_read(&dev->total_locks));
	DRM_PROC_PRINT("unlocks  %10u\n", atomic_read(&dev->total_unlocks));
	DRM_PROC_PRINT("contends %10u\n", atomic_read(&dev->total_contends));
	DRM_PROC_PRINT("sleeps   %10u\n", atomic_read(&dev->total_sleeps));
	

	if (!dev->device_minor) return len;

	DRM_PROC_PRINT("\n");
	DRM_PROC_PRINT("lock           0x%08x\n", dev->lock.hw_lock->lock);
	DRM_PROC_PRINT("context_flag   0x%08x\n", dev->context_flag);
	DRM_PROC_PRINT("interrupt_flag 0x%08x\n", dev->interrupt_flag);
	DRM_PROC_PRINT("dma_flag       0x%08x\n", dev->dma_flag);
	
	buffer = dev->next_buffer;
	if (buffer) {
		DRM_PROC_PRINT("next_buffer    %10d\n", buffer->idx);
	} else {
		DRM_PROC_PRINT("next_buffer          none\n");
	}
	buffer = dev->this_buffer;
	if (buffer) {
		DRM_PROC_PRINT("this_buffer    %10d\n", buffer->idx);
	} else {
		DRM_PROC_PRINT("this_buffer          none\n");
	}

	
	DRM_PROC_PRINT("queue_count    %10d\n",  dev->queue_count);
	DRM_PROC_PRINT("last_context   %10d\n",  dev->last_context);
	DRM_PROC_PRINT("last_switch    %10lu\n", dev->last_switch);
	DRM_PROC_PRINT("last_checked   %10d\n",  dev->last_checked);
		
	
	DRM_PROC_PRINT("\n                     q2d        d2c        c2f"
		       "        q2c        q2f        dma        sch"
		       "        ctx       lacq       lhld\n\n");
	for (i = 0; i < DRM_DMA_HISTOGRAM_SLOTS; i++) {
		DRM_PROC_PRINT("%s %10lu %10u %10u %10u %10u %10u"
			       " %10u %10u %10u %10u %10u\n",
			       i == DRM_DMA_HISTOGRAM_SLOTS - 1 ? ">=" : "< ",
			       i == DRM_DMA_HISTOGRAM_SLOTS - 1
			       ? prev_value : slot_value ,
			       
			       atomic_read(&dev->histo
					   .queued_to_dispatched[i]),
			       atomic_read(&dev->histo
					   .dispatched_to_completed[i]),
			       atomic_read(&dev->histo
					   .completed_to_freed[i]),
			       
			       atomic_read(&dev->histo
					   .queued_to_completed[i]),
			       atomic_read(&dev->histo
					   .queued_to_freed[i]),
			       atomic_read(&dev->histo.dma[i]),
			       atomic_read(&dev->histo.schedule[i]),
			       atomic_read(&dev->histo.ctx[i]),
			       atomic_read(&dev->histo.lacq[i]),
			       atomic_read(&dev->histo.lhld[i]));
		prev_value = slot_value;
		slot_value = DRM_DMA_HISTOGRAM_NEXT(slot_value);
	}
	return len;
}
#endif
