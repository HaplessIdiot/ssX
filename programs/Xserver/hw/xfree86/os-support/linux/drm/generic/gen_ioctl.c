/* gen_ioctl.c -- Generic IOCTL and function support -*- linux-c -*-
 * Created: Tue Feb  2 08:37:54 1999 by faith@precisioninsight.com
 * Revised: Fri Jun  4 12:53:53 1999 by faith@precisioninsight.com
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
 * $PI: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/generic/gen_ioctl.c,v 1.38 1999/06/07 13:01:43 faith Exp $
 * $XFree86$
 *
 */

#define __NO_VERSION__
#include "drmP.h"
#include "linux/un.h"

static int drm_gen_registered;

				/* FIXME: this can be made much faster. */
int drm_order(unsigned long size)
{
	int           order;
	unsigned long tmp;

	for (order = 0, tmp = size; tmp >>= 1; ++order);
	if (size & ~(1 << order)) ++order;
	return order;
}

int drm_hash_key(drm_key_t hi, drm_key_t lo)
{
	return (hi ^ lo) & (DRM_HASH_SIZE-1);
}

int drm_check_key(drm_device_t *dev, drm_key_t hi, drm_key_t lo)
{
	drm_key_entry_t *pt;

				/* Always called from inside an IOCTL, so
                                   no locking is needed. */
	for (pt = dev->keylist[drm_hash_key(hi, lo)].head; pt; pt = pt->next) {
		if (pt->hi == hi && pt->lo == lo) {
			return 1;
		}
	}
	return 0;
}

int drm_addkey DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	int             hash;
	drm_auth_t      key;
	drm_key_entry_t *entry;
	
	copy_from_user_ret(&key, (drm_auth_t *)arg, sizeof(key), -EFAULT);
	DRM_TRACE("0x%08x:0x%08x\n", key.hi, key.lo);
	
	hash        = drm_hash_key(key.hi, key.lo);
	entry       = drm_alloc(sizeof(*entry), DRM_MEM_KEYS);
	if (!entry) return -ENOMEM;
	entry->hi   = key.hi;
	entry->lo   = key.lo;
	entry->next = NULL;

	spin_lock(&dev->struct_lock);
	if (dev->keylist[hash].tail) {
		dev->keylist[hash].tail->next = entry;
		dev->keylist[hash].tail       = entry;
	} else {
		dev->keylist[hash].head       = entry;
		dev->keylist[hash].tail       = entry;
	}
	spin_unlock(&dev->struct_lock);
	
	return 0;
}

int drm_remove_key(drm_device_t *dev, drm_key_t hi, drm_key_t lo)
{
	drm_key_entry_t *prev = NULL;
	drm_key_entry_t *pt;
	int             hash;
	
	DRM_TRACE("0x%08x:0x%08x\n", hi, lo);
	hash = drm_hash_key(hi, lo);
	
	spin_lock(&dev->struct_lock);
	for (pt = dev->keylist[hash].head; pt; prev = pt, pt = pt->next) {
		if (pt->lo == lo && pt->hi == hi) {
			if (dev->keylist[hash].head == pt) {
				dev->keylist[hash].head = pt->next;
			}
			if (dev->keylist[hash].tail == pt) {
				dev->keylist[hash].tail = prev;
			}
			if (prev) {
				prev->next = pt->next;
			}
			spin_unlock(&dev->struct_lock);
			return 0;
		}
	}
	spin_unlock(&dev->struct_lock);

	drm_free(pt, sizeof(*pt), DRM_MEM_KEYS);
	
	return -EINVAL;
}

int drm_rmkey DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_auth_t      key;
	
	DRM_TRACE("\n");
	
	copy_from_user_ret(&key, (drm_auth_t *)arg, sizeof(key), -EFAULT);
	return drm_remove_key(dev, key.hi, key.lo);
}

int drm_addmap DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_map_t       *map;
	
	DRM_TRACE("TASK_SIZE = 0x%08lx, f_mode = %x\n",
		  TASK_SIZE, filp->f_mode);

	if (!(filp->f_mode & 3)) return -EACCES; /* Require read/write */

	map          = drm_alloc(sizeof(*map), DRM_MEM_MAPS);
	if (!map) return -ENOMEM;
	if (copy_from_user(map, (drm_map_t *)arg, sizeof(*map))) {
		drm_free(map, sizeof(*map), DRM_MEM_MAPS);
		return -EFAULT;
	}

				/* For now, we'll reject non-page-aligned
                                   requests.  If this is a problem, we can
                                   do some rounding later. */
	DRM_DEBUG("offset = 0x%08lx, size = 0x%08lx, type = %d\n",
		  map->offset, map->size, map->type);
	if ((map->offset & (~PAGE_MASK)) || (map->size & (~PAGE_MASK))) {
		drm_free(map, sizeof(*map), DRM_MEM_MAPS);
		return -EINVAL;
	}
	map->mtrr   = -1;
	map->handle = 0;

	switch (map->type) {
	case _DRM_REGISTERS:
	case _DRM_FRAME_BUFFER:	
				/* Allow the client to mmap the frame
                                   buffer.

				   We must provide this mechanism for the
				   client to access the frame buffer and
				   registers because the client is not
				   running as root, and cannot perform a
				   mmap on /dev/mem (in contrast to the X
				   server).

				   Registers must be uncached, so we must
                                   set the PCD attribute on the page to 1.
                                   But some architectures might want
                                   write-combining for fast MMIO access.
				   
				   FIXME: should we allow mappings in the
				   area 0xa0000-0xc0000? */
		
		if (map->offset + map->size < map->offset
		    || map->offset < virt_to_phys(high_memory)) {
			drm_free(map, sizeof(*map), DRM_MEM_MAPS);
			return -EINVAL;
		}
#ifdef CONFIG_MTRR
				/* Try to make the frame buffer use a
                                   write-combinging policy.  For this to
                                   work, we have to make sure that the PCD
                                   and PWT atttributes for the page are not
                                   both 1.  We'd like to set them both to
                                   0.  See 11.5.1 (Precedence of Cache
                                   Controls) of Intel's Pentium Pro Family
                                   Developer's Manual (Volume 3: Operating
                                   System Writer's Manual), 1996, pp. 11-8
                                   to 11-10. */
 
		if (map->type == _DRM_FRAME_BUFFER
		    || (map->flags & _DRM_WRITE_COMBINING)) {
			map->mtrr = mtrr_add(map->offset, map->size,
					     MTRR_TYPE_WRCOMB, 1);
		}
#endif
		map->handle = drm_ioremap(map->offset, map->size);
		break;
			

	case _DRM_SHM:
				/* Create a shared memory mapping between
                                   DR clients.

				   Why don't we just have the X server and
				   the clients use shm* calls?  Because
				   shm* only supports uid/gid-based access.
				   Consider that an X server may have
				   connections from clients with different
				   uids and gids.  To allow these clients
				   to access the SAREA, the permissions on
				   the shm segment would have to allow
				   connections from _any_ uid.  The problem
				   is that this would allow a rogue
				   process, _without_ a connection to the X
				   server, to do a shmat(2) on the segment
				   and to corrupt or destory the
				   information in the segment.

				   By using ioctl and mmap calls on the
				   /dev/drm* devices, we ensure that only
				   processes with an existing connection to
				   the X server can access the SAREA.  This
				   is required so that a rogue client is
				   prevented from circumventing standard X
				   authentication mechanisms. */
		
				/* Right now, we assume DRM_KERNEL is set.
                                   However, in the future, we may make a
                                   decision on where to map the memory
                                   based on the value of DRM_KERNEL. */
		if (!(map->handle = drm_vmalloc(map->size))) {
			drm_free(map, sizeof(*map), DRM_MEM_MAPS);
			return -ENOMEM;
		}
		map->offset = (unsigned long)map->handle;
		if (map->flags & _DRM_CONTAINS_LOCK) {
			dev->lock.hw_lock = map->handle; /* Pointer to lock */
		}
		break;
	default:
		drm_free(map, sizeof(*map), DRM_MEM_MAPS);
		return -EINVAL;
	}

	spin_lock(&dev->struct_lock);
	if (dev->maplist) {
		++dev->map_count;
		dev->maplist = drm_realloc(dev->maplist,
					   (dev->map_count-1)
					   * sizeof(*dev->maplist),
					   dev->map_count
					   * sizeof(*dev->maplist),
					   DRM_MEM_MAPS);
	} else {
		dev->map_count = 1;
		dev->maplist = drm_alloc(dev->map_count*sizeof(*dev->maplist),
					 DRM_MEM_MAPS);
	}
	dev->maplist[dev->map_count-1] = map;
	spin_unlock(&dev->struct_lock);

	copy_to_user_ret((drm_map_t *)arg, map, sizeof(*map), -EFAULT);
	if (map->type != _DRM_SHM) {
				/* Hide kernel-virtual address */
		copy_to_user_ret(&((drm_map_t *)arg)->handle,
				 &map->offset,
				 sizeof(map->offset),
				 -EFAULT);
	}		
	return 0;
}

int drm_addbufs DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_buf_desc_t   request;
	int             count;
	int             order;
	int             size;
	int             total;
	int             page_order;
	drm_buf_entry_t *entry;
	unsigned long   page;
	drm_buf_t       *buf;
	int             alignment;
	unsigned long   offset;
	int             i;
	int             byte_count;
	int             page_count;

	copy_from_user_ret(&request,
			   (drm_buf_desc_t *)arg,
			   sizeof(request),
			   -EFAULT);

	count      = request.count;
	order      = drm_order(request.size);
	size       = 1 << order;
	
	DRM_TRACE("count = %d, size = %d (%d), order = %d, queue_count = %d\n",
		  request.count, request.size, size, order, dev->queue_count);

	if (order < DRM_MIN_ORDER || order > DRM_MAX_ORDER) return -EINVAL;
	if (dev->queue_count) return -EBUSY; /* Not while in use */

	alignment  = (request.flags & DRM_PAGE_ALIGN) ? PAGE_ALIGN(size) :size;
	page_order = order - PAGE_SHIFT > 0 ? order - PAGE_SHIFT : 0;
	total      = PAGE_SIZE << page_order;

	spin_lock(&dev->count_lock);
	if (dev->buf_use) {
		spin_unlock(&dev->count_lock);
		return -EBUSY;
	}
	atomic_inc(&dev->buf_alloc);
	spin_unlock(&dev->count_lock);
	
	spin_lock(&dev->struct_lock);
	entry = &dev->bufs[order];
	if (entry->buf_count) {
		spin_unlock(&dev->struct_lock);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;	/* May only call once for each order */
	}
	
	entry->buflist = drm_alloc(count * sizeof(*entry->buflist),
				   DRM_MEM_BUFS);
	if (!entry->buflist) {
		spin_unlock(&dev->struct_lock);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;
	}
	memset(entry->buflist, 0, count * sizeof(*entry->buflist));

	entry->seglist = drm_alloc(count * sizeof(*entry->seglist),
				   DRM_MEM_SEGS);
	if (!entry->seglist) {
		drm_free(entry->buflist,
			 sizeof(*entry->buflist),
			 DRM_MEM_BUFS);
		spin_unlock(&dev->struct_lock);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;
	}
	memset(entry->seglist, 0, count * sizeof(*entry->seglist));

	
				/* We need a sequential list of pages to
                                   service the nopage call to "fault" the
                                   physical pages into user-virtual memory
                                   when they are first accessed.  We make
                                   this list grow with each call since its
                                   final length cannot be predetermined.
                                   Allocate before the next loop, since we
                                   have to fill in this information on the
                                   fly.  Our estimate for the length of
                                   this list is probably too high, but we
                                   cannot do better at this time, since we
                                   cannot predict how much memory the
                                   kernel will give to us.*/
	dev->pagelist = drm_realloc(dev->pagelist,
				    dev->page_count * sizeof(*dev->pagelist),
				    (dev->page_count + (count << page_order))
				    * sizeof(*dev->pagelist),
				    DRM_MEM_PAGES);
	DRM_DEBUG("pagelist: %d entries\n",
		  dev->page_count + (count << page_order));

	
				/* This is the main loop to allocate the
                                   segments of contiguous pages. */
	entry->buf_size   = size;
	entry->page_order = page_order;
	byte_count        = 0;
	page_count        = 0;
	while (entry->buf_count < count) {
		if (!(page = drm_alloc_dma(page_order))) break;
		entry->seglist[entry->seg_count++] = page;
		for (i = 0; i < (1 << page_order); i++) {
			DRM_VERB("page %d @ 0x%08lx\n",
				 dev->page_count + page_count,
				 page + PAGE_SIZE * i);
			dev->pagelist[dev->page_count + page_count++]
				= page + PAGE_SIZE * i;
		}
		for (offset = 0;
		     offset + size <= total && entry->buf_count < count;
		     offset += alignment, ++entry->buf_count) {
			buf          = &entry->buflist[entry->buf_count];
			buf->idx     = dev->buf_count + entry->buf_count;
			buf->total   = alignment;
			buf->order   = order;
			buf->used    = 0;
			buf->offset  = (dev->byte_count + byte_count + offset);
			buf->address = (void *)(page + offset);
			buf->next    = NULL;
			buf->waiting = 0;
			buf->pending = 0;
			buf->dma_wait= NULL;
			buf->pid     = 0;
#if DRM_DMA_HISTOGRAM
			buf->time_queued     = 0;
			buf->time_dispatched = 0;
			buf->time_completed  = 0;
			buf->time_freed      = 0;
#endif

			DRM_VERB("buffer %d @ %p\n",
				 entry->buf_count, buf->address);
		}
		byte_count += PAGE_SIZE << page_order;
	}

				/* We need a sequential list of buffer
                                   pointers in dev->buflist so that we can
                                   have the fastest access time when
                                   dispatching DMA buffers.  We make this
                                   list grow with each call since its final
                                   length cannot be predetermined.
                                   Allocate after the above loop, since we
                                   have an exact count of the number of
                                   buffers used. */
	dev->buflist = drm_realloc(dev->buflist,
				   dev->buf_count * sizeof(*dev->buflist),
				   (dev->buf_count + entry->buf_count)
				   * sizeof(*dev->buflist),
				   DRM_MEM_BUFS);
	for (i = dev->buf_count; i < dev->buf_count + entry->buf_count; i++)
		dev->buflist[i] = &entry->buflist[i - dev->buf_count];

	DRM_VERB("%u =? %u\n", page_count, entry->seg_count << page_order);
	
	dev->buf_count  += entry->buf_count;
	dev->seg_count  += entry->seg_count;
	dev->page_count += entry->seg_count << page_order;
	dev->byte_count += PAGE_SIZE * (entry->seg_count << page_order);
	
	drm_freelist_create(&entry->freelist, entry->buf_count);
	for (i = 0; i < entry->buf_count; i++) {
		drm_freelist_put(dev, &entry->freelist, &entry->buflist[i]);
	}
	
	spin_unlock(&dev->struct_lock);

	request.count = entry->buf_count;
	request.size  = size;

	copy_to_user_ret((drm_buf_desc_t *)arg,
			 &request,
			 sizeof(request),
			 -EFAULT);
	
	atomic_dec(&dev->buf_alloc);
	return 0;
}

int drm_infobufs DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_buf_info_t  request;
	int             i;
	int             count;

	spin_lock(&dev->count_lock);
	if (atomic_read(&dev->buf_alloc)) {
		spin_unlock(&dev->count_lock);
		return -EBUSY;
	}
	++dev->buf_use;		/* Can't allocate more after this call */
	spin_unlock(&dev->count_lock);

	copy_from_user_ret(&request,
			   (drm_buf_info_t *)arg,
			   sizeof(request),
			   -EFAULT);

	for (i = 0, count = 0; i < DRM_MAX_ORDER+1; i++) {
		if (dev->bufs[i].buf_count) ++count;
	}
	
	DRM_TRACE("count = %d\n", count);
	
	if (request.count >= count) {
		for (i = 0, count = 0; i < DRM_MAX_ORDER+1; i++) {
			if (dev->bufs[i].buf_count) {
				copy_to_user_ret(&request.list[count].count,
						 &dev->bufs[i].buf_count,
						 sizeof(dev->bufs[0]
							.buf_count),
						 -EFAULT);
				copy_to_user_ret(&request.list[count].size,
						 &dev->bufs[i].buf_size,
						 sizeof(dev->bufs[0].buf_size),
						 -EFAULT);
				copy_to_user_ret(&request.list[count].low_mark,
						 &dev->bufs[i]
						 .freelist.low_mark,
						 sizeof(dev->bufs[0]
							.freelist.low_mark),
						 -EFAULT);
				copy_to_user_ret(&request.list[count]
						 .high_mark,
						 &dev->bufs[i]
						 .freelist.high_mark,
						 sizeof(dev->bufs[0]
							.freelist.high_mark),
						 -EFAULT);
				DRM_DEBUG("%d %d %d %d %d\n",
					  i,
					  dev->bufs[i].buf_count,
					  dev->bufs[i].buf_size,
					  dev->bufs[i].freelist.low_mark,
					  dev->bufs[i].freelist.high_mark);
				++count;
			}
		}
	}
	request.count = count;

	copy_to_user_ret((drm_buf_info_t *)arg,
			 &request,
			 sizeof(request),
			 -EFAULT);
	
	return 0;
}

int drm_markbufs DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_buf_desc_t  request;
	int             order;
	drm_buf_entry_t *entry;

	copy_from_user_ret(&request,
			   (drm_buf_desc_t *)arg,
			   sizeof(request),
			   -EFAULT);

	DRM_TRACE("%d, %d, %d\n",
		  request.size, request.low_mark, request.high_mark);
	order = drm_order(request.size);
	if (order < DRM_MIN_ORDER || order > DRM_MAX_ORDER) return -EINVAL;
	entry = &dev->bufs[order];

	if (request.low_mark < 0 || request.low_mark > entry->buf_count)
		return -EINVAL;
	if (request.high_mark < 0 || request.high_mark > entry->buf_count)
		return -EINVAL;

	entry->freelist.low_mark  = request.low_mark;
	entry->freelist.high_mark = request.high_mark;
	
	return 0;
}

int drm_freebufs DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_buf_free_t  request;
	int             i;
	int             idx;
	drm_buf_t       *buf;

	copy_from_user_ret(&request,
			   (drm_buf_free_t *)arg,
			   sizeof(request),
			   -EFAULT);

	DRM_TRACE("%d\n", request.count);
	for (i = 0; i < request.count; i++) {
		copy_from_user_ret(&idx,
				   &request.list[i],
				   sizeof(idx),
				   -EFAULT);
		if (idx < 0 || idx >= dev->buf_count) {
			DRM_ERROR("Index %d (of %d max)\n",
				  idx, dev->buf_count - 1);
			return -EINVAL;
		}
		buf = dev->buflist[idx];
		if (buf->pid != current->pid) {
			DRM_ERROR("Process %d freeing buffer owned by %d\n",
				  current->pid, buf->pid);
			return -EINVAL;
		}
		drm_free_buffer(dev, buf);
	}
	
	return 0;
}

int drm_mapbufs DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	int             retcode = 0;
	const int       zero    = 0;
	unsigned long   virtual;
	unsigned long   address;
	drm_buf_map_t   request;
	int             i;

	DRM_TRACE("\n");

	spin_lock(&dev->count_lock);
	if (atomic_read(&dev->buf_alloc)) {
		spin_unlock(&dev->count_lock);
		return -EBUSY;
	}
	++dev->buf_use;		/* Can't allocate more after this call */
	spin_unlock(&dev->count_lock);

	copy_from_user_ret(&request,
			   (drm_buf_map_t *)arg,
			   sizeof(request),
			   -EFAULT);

	if (request.count >= dev->buf_count) {
		virtual = do_mmap(filp, 0, dev->byte_count,
				  PROT_READ|PROT_WRITE, MAP_SHARED, 0);
		if (virtual > -1024UL) {
				/* Real error */
			retcode = (signed long)virtual;
			goto done;
		}
		request.virtual = (void *)virtual;

		for (i = 0; i < dev->buf_count; i++) {
			if (copy_to_user(&request.list[i].idx,
					 &dev->buflist[i]->idx,
					 sizeof(request.list[0].idx))) {
				retcode = -EFAULT;
				goto done;
			}
			if (copy_to_user(&request.list[i].total,
					 &dev->buflist[i]->total,
					 sizeof(request.list[0].total))) {
				retcode = -EFAULT;
				goto done;
			}
			if (copy_to_user(&request.list[i].used,
					 &zero,
					 sizeof(zero))) {
				retcode = -EFAULT;
				goto done;
			}
			address = virtual + dev->buflist[i]->offset;
			if (copy_to_user(&request.list[i].address,
					 &address,
					 sizeof(address))) {
				retcode = -EFAULT;
				goto done;
			}
		}
	}
done:
	request.count = dev->buf_count;
	DRM_DEBUG("%d buffers, retcode = %d\n", request.count, retcode);

	copy_to_user_ret((drm_buf_map_t *)arg,
			 &request,
			 sizeof(request),
			 -EFAULT);

	return retcode;
}

int drm_reg( void )
{
	DRM_TRACE("\n");
	if (drm_gen_registered) return -EBUSY;
	++drm_gen_registered;
	return 0;
}

int drm_unreg( void )
{
	DRM_TRACE("\n");
	if (!drm_gen_registered) return -EBUSY;
	--drm_gen_registered;
	return 0;
}

int drm_lock_take(__volatile__ unsigned int *lock, unsigned int context)
{
	unsigned int old;
	unsigned int new;
	char         failed;

	DRM_TRACE("%d attempts\n", context);
	do {
		old = *lock;
		if (old & _DRM_LOCK_HELD) new = old | _DRM_LOCK_CONT;
		else                      new = context | _DRM_LOCK_HELD;
		_DRM_CAS(lock, old, new, failed);
	} while (failed);
	if (_DRM_LOCKING_CONTEXT(old) == context) {
		if (old & _DRM_LOCK_HELD) {
			if (context != DRM_KERNEL_CONTEXT) {
				DRM_ERROR("%d holds heavyweight lock\n",
					  context);
			}
			return 0;
		}
	}
	if (new == (context | _DRM_LOCK_HELD)) {
				/* Have lock */
		DRM_LOCK_VERB("%d\n", context);
		return 1;
	}
	DRM_LOCK_VERB("%d unable to get lock held by %d\n",
		      context, _DRM_LOCKING_CONTEXT(old));
	return 0;
}

/* This takes a lock forcibly and hands it to context.  Should ONLY be used
   inside drm_unlock to give lock to kernel before calling
   drm_dma_schedule. */
static int drm_lock_transfer(__volatile__ unsigned int *lock,
			     unsigned int context)
{
	unsigned int old;
	unsigned int new;
	char         failed;

	do {
		old = *lock;
		new = context | _DRM_LOCK_HELD;
		_DRM_CAS(lock, old, new, failed);
	} while (failed);
	DRM_LOCK_VERB("%d => %d\n", _DRM_LOCKING_CONTEXT(old), context);
	return 1;
}

int drm_lock_free(drm_device_t *dev,
		  __volatile__ unsigned int *lock, unsigned int context)
{
	unsigned int old;
	unsigned int new;
	char         failed;

	DRM_TRACE("%d\n", context);
	do {
		old = *lock;
		new = 0;
		_DRM_CAS(lock, old, new, failed);
	} while (failed);
	if (_DRM_LOCK_IS_HELD(old) && _DRM_LOCKING_CONTEXT(old) != context) {
		DRM_ERROR("%d freed heavyweight lock held by %d\n",
			  context,
			  _DRM_LOCKING_CONTEXT(old));
		return 1;
	}
	dev->lock.pid = 0;
	wake_up_interruptible(&dev->lock.lock_queue);
	return 0;
}

static int drm_flush_queue(drm_device_t *dev, int context)
{
	struct wait_queue entry = { current, NULL };
	int               ret   = 0;
	drm_queue_t       *q    = dev->queuelist[context];
	
	DRM_TRACE("\n");
	
	atomic_inc(&q->use_count);
	if (atomic_read(&q->use_count) > 1) {
		atomic_inc(&q->block_write);
		current->state = TASK_INTERRUPTIBLE;
		add_wait_queue(&q->flush_queue, &entry);
		atomic_inc(&q->block_count);
		for (;;) {
			if (!DRM_BUFCOUNT(&q->waitlist)) break;
			schedule();
			if (signal_pending(current)) {
				ret = -EINTR;
				break;
			}
		}
		atomic_dec(&q->block_count);
		current->state = TASK_RUNNING;
		remove_wait_queue(&q->flush_queue, &entry);
	}
	atomic_dec(&q->use_count);
	atomic_inc(&q->total_flushed);
		
				/* NOTE: block_write is still incremented!
				   Use drm_flush_unlock_queue to decrement. */
	return ret;
}

static int drm_flush_unblock_queue(drm_device_t *dev, int context)
{
	drm_queue_t       *q    = dev->queuelist[context];
	
	DRM_TRACE("\n");
	
	atomic_inc(&q->use_count);
	if (atomic_read(&q->use_count) > 1) {
		if (atomic_read(&q->block_write)) {
			atomic_dec(&q->block_write);
			wake_up_interruptible(&q->write_queue);
		}
	}
	atomic_dec(&q->use_count);
	return 0;
}

static int drm_flush_block_and_flush(drm_device_t *dev, int context,
				     drm_lock_flags_t flags)
{
	int ret = 0;
	int i;
	
	DRM_TRACE("\n");
	
	if (flags & _DRM_LOCK_FLUSH) {
		ret = drm_flush_queue(dev, DRM_KERNEL_CONTEXT);
		if (!ret) ret = drm_flush_queue(dev, context);
	}
	if (flags & _DRM_LOCK_FLUSH_ALL) {
		read_lock(&dev->queue_lock);
		for (i = 0; !ret && i < dev->queue_count; i++) {
			ret = drm_flush_queue(dev, i);
		}
		read_unlock(&dev->queue_lock);
	}
	return ret;
}

static int drm_flush_unblock(drm_device_t *dev, int context,
			     drm_lock_flags_t flags)
{
	int ret = 0;
	int i;
	
	DRM_TRACE("\n");
	
	if (flags & _DRM_LOCK_FLUSH) {
		ret = drm_flush_unblock_queue(dev, DRM_KERNEL_CONTEXT);
		if (!ret) ret = drm_flush_unblock_queue(dev, context);
	}
	if (flags & _DRM_LOCK_FLUSH_ALL) {
		read_lock(&dev->queue_lock);
		for (i = 0; !ret && i < dev->queue_count; i++) {
			ret = drm_flush_unblock_queue(dev, i);
		}
		read_unlock(&dev->queue_lock);
	}
		
	return ret;
}

int drm_lock DRM_IOCTL_ARGS
{
	drm_file_t        *priv   = filp->private_data;
	drm_device_t      *dev    = priv->dev;
	struct wait_queue entry = { current, NULL };
	int               ret   = 0;
	drm_lock_t        lock;
	drm_queue_t       *q;
#if DRM_DMA_HISTOGRAM
	cycles_t          start;

	dev->lck_start = start = get_cycles();
#endif

	copy_from_user_ret(&lock, (drm_lock_t *)arg, sizeof(lock), -EFAULT);

	if (lock.context == DRM_KERNEL_CONTEXT) {
		DRM_ERROR("Process %d using kernel context %d\n",
			  current->pid, lock.context);
		return -EINVAL;
	}

	DRM_TRACE("%d requests lock, flags = 0x%08x\n",
		  lock.context, lock.flags);

	if (lock.context < 0 || lock.context >= dev->queue_count)
		return -EINVAL;
	q = dev->queuelist[lock.context];
	
	ret = drm_flush_block_and_flush(dev, lock.context, lock.flags);

	if (!ret) {
		if (_DRM_LOCKING_CONTEXT(dev->lock.hw_lock->lock)
		    != lock.context) {
			long j = jiffies - dev->lock.lock_time;

			if (j > 0 && j <= DRM_LOCK_SLICE) {
				/* Can't take lock if we just had it and
                                   there is contention. */
				current->state = TASK_INTERRUPTIBLE;
				schedule_timeout(j);
			}
		}
		add_wait_queue(&dev->lock.lock_queue, &entry);
		for (;;) {
			if (drm_lock_take(&dev->lock.hw_lock->lock,
					  lock.context)) {
				dev->lock.pid       = current->pid;
				dev->lock.lock_time = jiffies;
				atomic_inc(&dev->total_locks);
				atomic_inc(&q->total_locks);
				break;  /* Got lock */
			}
			
				/* Contention */
			atomic_inc(&dev->total_sleeps);
			current->state = TASK_INTERRUPTIBLE;
			schedule();
			if (signal_pending(current)) {
				ret = -ERESTARTSYS;
				break;
			}
		}
		current->state = TASK_RUNNING;
		remove_wait_queue(&dev->lock.lock_queue, &entry);
	}

				/* Ok to unblock here, since we either have
                                   the lock now, in which case more DMA
                                   won't be sent; or ret != 0, in which
                                   case we don't have the lock and we won't
                                   be getting the lock. */
	drm_flush_unblock(dev, lock.context, lock.flags); /* cleanup phase */
	
	if (!ret) {
		if (lock.flags & _DRM_LOCK_READY)
			drm_do_command(dev, _DRM_DMA_READY);
		if (lock.flags & _DRM_LOCK_QUIESCENT)
			drm_do_command(dev, _DRM_DMA_QUIESCENT);
	}
	DRM_DEBUG("%d %s\n", lock.context, ret ? "interrupted" : "has lock");

#if DRM_DMA_HISTOGRAM
	atomic_inc(&dev->histo.lacq[drm_histogram_slot(get_cycles() - start)]);
#endif
	
	return ret;
}

int drm_unlock DRM_IOCTL_ARGS
{
	drm_file_t        *priv   = filp->private_data;
	drm_device_t      *dev    = priv->dev;
	drm_lock_t        lock;

	copy_from_user_ret(&lock, (drm_lock_t *)arg, sizeof(lock), -EFAULT);
	
	if (lock.context == DRM_KERNEL_CONTEXT) {
		DRM_ERROR("Process %d using kernel context %d\n",
			  current->pid, lock.context);
		return -EINVAL;
	}

	DRM_TRACE("%d frees lock (%d holds)\n",
		  lock.context,
		  _DRM_LOCKING_CONTEXT(dev->lock.hw_lock->lock));
	atomic_inc(&dev->total_unlocks);
	if (_DRM_LOCK_IS_CONT(dev->lock.hw_lock->lock))
		atomic_inc(&dev->total_contends);
	drm_lock_transfer(&dev->lock.hw_lock->lock, DRM_KERNEL_CONTEXT);
	drm_dma_schedule(dev, 1);
	if (!dev->context_flag) {
				/* Don't release lock if context switch in
                                   progress. */
		if (drm_lock_free(dev, &dev->lock.hw_lock->lock,
				  DRM_KERNEL_CONTEXT)) {
			DRM_ERROR("\n");
		}
	}
#if DRM_DMA_HISTOGRAM
	atomic_inc(&dev->histo.lhld[drm_histogram_slot(get_cycles()
						       - dev->lck_start)]);
#endif
	
	return 0;
}

static int drm_init_queue(drm_device_t *dev, drm_queue_t *q, drm_ctx_t *ctx)
{
	DRM_TRACE("\n");
	
	if (atomic_read(&q->use_count) != 1
	    || atomic_read(&q->finalization)
	    || atomic_read(&q->block_count)) {
		DRM_ERROR("New queue is already in use: u%d f%d b%d\n",
			  atomic_read(&q->use_count),
			  atomic_read(&q->finalization),
			  atomic_read(&q->block_count));
	}
		  
	atomic_set(&q->finalization,  0);
	atomic_set(&q->block_count,   0);
	atomic_set(&q->block_read,    0);
	atomic_set(&q->block_write,   0);
	atomic_set(&q->total_queued,  0);
	atomic_set(&q->total_flushed, 0);
	atomic_set(&q->total_locks,   0);

	q->write_queue = NULL;
	q->read_queue  = NULL;
	q->flush_queue = NULL;

	q->flags = ctx->flags;

	drm_waitlist_create(&q->waitlist, dev->buf_count);

	return 0;
}

static int drm_alloc_queue(drm_device_t *dev)
{
	int         i;
	drm_queue_t *queue;
	int         oldslots;
	int         newslots;
	
	/* PRE: 1) dev->queuelist[0..dev->queue_count] is allocated and
       	           will not disappear (so all deallocation must be done
       	           after IOCTLs are off)
		2) dev->queue_count < dev->queue_slots
		3) dev->queuelist[i].use_count == 0
		   and dev->queuelist[i].finalization == 0 if i not in use
		   
	   POST: 1) dev->queuelist[i].use_count == 1
	         2) dev->queue_count < dev->queue_slots
	*/
	        
				/* Check for a free queue */
	for (i = 0; i < dev->queue_count; i++) {
		atomic_inc(&dev->queuelist[i]->use_count);
		if (atomic_read(&dev->queuelist[i]->use_count) == 1
		    && !atomic_read(&dev->queuelist[i]->finalization)) {
			DRM_TRACE("%d (free)\n", i);
			return i;
		}
		atomic_dec(&dev->queuelist[i]->use_count);
	}
				/* Allocate a new queue */
	spin_lock(&dev->struct_lock);
	
	queue = drm_alloc(sizeof(*queue), DRM_MEM_QUEUES);
	memset(queue, 0, sizeof(*queue));
	atomic_set(&queue->use_count, 1);
	
	++dev->queue_count;
	if (dev->queue_count >= dev->queue_slots) {
		oldslots = dev->queue_slots * sizeof(*dev->queuelist);
		if (!dev->queue_slots) dev->queue_slots = 1;
		dev->queue_slots *= 2;
		newslots = dev->queue_slots * sizeof(*dev->queuelist);

		dev->queuelist = drm_realloc(dev->queuelist,
					     oldslots,
					     newslots,
					     DRM_MEM_QUEUES);
		if (!dev->queuelist) {
			spin_unlock(&dev->struct_lock);
			DRM_TRACE("out of memory\n");
			return -ENOMEM;
		}
	}
	dev->queuelist[dev->queue_count-1] = queue;
	
	spin_unlock(&dev->struct_lock);
	DRM_TRACE("%d (new)\n", dev->queue_count - 1);
	return dev->queue_count - 1;
}

int drm_resctx DRM_IOCTL_ARGS
{
	drm_ctx_res_t   res;
	drm_ctx_t       ctx;
	int             i;

	DRM_TRACE("%d\n", DRM_RESERVED_CONTEXTS);
	copy_from_user_ret(&res, (drm_ctx_res_t *)arg, sizeof(res), -EFAULT);
	if (res.count >= DRM_RESERVED_CONTEXTS) {
		memset(&ctx, 0, sizeof(ctx));
		for (i = 0; i < DRM_RESERVED_CONTEXTS; i++) {
			ctx.handle = i;
			copy_to_user_ret(&res.contexts[i],
					 &i,
					 sizeof(i),
					 -EFAULT);
		}
	}
	res.count = DRM_RESERVED_CONTEXTS;
	copy_to_user_ret((drm_ctx_res_t *)arg, &res, sizeof(res), -EFAULT);
	return 0;
}


int drm_addctx DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_ctx_t       ctx;

	copy_from_user_ret(&ctx, (drm_ctx_t *)arg, sizeof(ctx), -EFAULT);
	if ((ctx.handle = drm_alloc_queue(dev)) == DRM_KERNEL_CONTEXT) {
				/* Init kernel's context and get a new one. */
		drm_init_queue(dev, dev->queuelist[ctx.handle], &ctx);
		ctx.handle = drm_alloc_queue(dev);
	}
	drm_init_queue(dev, dev->queuelist[ctx.handle], &ctx);
	DRM_TRACE("%d\n", ctx.handle);
	copy_to_user_ret((drm_ctx_t *)arg, &ctx, sizeof(ctx), -EFAULT);
	return 0;
}

int drm_modctx DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_ctx_t       ctx;
	drm_queue_t     *q;
		
	copy_from_user_ret(&ctx, (drm_ctx_t *)arg, sizeof(ctx), -EFAULT);
	
	DRM_TRACE("%d\n", ctx.handle);
	
	if (ctx.handle < 0 || ctx.handle >= dev->queue_count) return -EINVAL;
	q = dev->queuelist[ctx.handle];
	
	atomic_inc(&q->use_count);
	if (atomic_read(&q->use_count) == 1) {
				/* No longer in use */
		atomic_dec(&q->use_count);
		return -EINVAL;
	}
				/* You have to drm_finish before doing
                                   modifications.  Otherwise, the DMA
                                   engine could become very confused
                                   (although, in the intitial
                                   implementation, it won't). */
	if (DRM_BUFCOUNT(&q->waitlist)) {
		atomic_dec(&q->use_count);
		return -EBUSY;
	}
	
				/* Add other possible modificaitons here. */
	q->flags = ctx.flags;
	
	atomic_dec(&q->use_count);
	return 0;
}

int drm_getctx DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_ctx_t       ctx;
	drm_queue_t     *q;
		
	copy_from_user_ret(&ctx, (drm_ctx_t *)arg, sizeof(ctx), -EFAULT);
	
	DRM_TRACE("%d\n", ctx.handle);
	
	if (ctx.handle >= dev->queue_count) return -EINVAL;
	q = dev->queuelist[ctx.handle];
	
	atomic_inc(&q->use_count);
	if (atomic_read(&q->use_count) == 1) {
				/* No longer in use */
		atomic_dec(&q->use_count);
		return -EINVAL;
	}
	
				/* Add other possible modificaitons here. */
	ctx.flags = q->flags;
	atomic_dec(&q->use_count);
	
	copy_to_user_ret((drm_ctx_t *)arg, &ctx, sizeof(ctx), -EFAULT);
	
	return 0;
}

int drm_switchctx DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_ctx_t       ctx;

	copy_from_user_ret(&ctx, (drm_ctx_t *)arg, sizeof(ctx), -EFAULT);
	
	DRM_TRACE("%d\n", ctx.handle);

				/* All the validity checking is done in
                                   drm_context_switch. */
	return drm_context_switch(dev, dev->last_context, ctx.handle);
}

int drm_newctx DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_ctx_t       ctx;

	copy_from_user_ret(&ctx, (drm_ctx_t *)arg, sizeof(ctx), -EFAULT);
	
	DRM_TRACE("%d\n", ctx.handle);
	drm_context_switch_complete(dev, ctx.handle);

	return 0;
}

int drm_rmctx DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_ctx_t       ctx;
	drm_queue_t     *q;
	drm_buf_t       *buf;

	copy_from_user_ret(&ctx, (drm_ctx_t *)arg, sizeof(ctx), -EFAULT);
	DRM_DEBUG("%d\n", ctx.handle);
	
	if (ctx.handle >= dev->queue_count) return -EINVAL;
	q = dev->queuelist[ctx.handle];
	
	atomic_inc(&q->use_count);
	if (atomic_read(&q->use_count) == 1) {
				/* No longer in use */
		atomic_dec(&q->use_count);
		return -EINVAL;
	}
	
	atomic_inc(&q->finalization); /* Mark queue in finalization state */
	atomic_sub(2, &q->use_count); /* Mark queue as unused (pending
					 finalization) */
		
				/* If there are buffers waiting, then we
                                   have a problem, because we guarantee
                                   that drm_dma_schedule is the only reader
                                   of the waitlist, and that it is not
                                   reentrant.  While it could check
                                   finalization, there is no easy way to
                                   prevent a race condition.  So, we have
                                   to use the (heavy-handed) dma_flag flag.
                                   (The freelist allows multiple writers,
                                   so it isn't a problem.) */

	while (test_and_set_bit(0, &dev->interrupt_flag)) {
		schedule();
		if (signal_pending(current)) {
			clear_bit(0, &dev->interrupt_flag);
			return -EINTR;
		}
	}
				/* Remove queued buffers */
	while ((buf = drm_waitlist_get(&q->waitlist))) {
		drm_free_buffer(dev, buf);
	}
	clear_bit(0, &dev->interrupt_flag);
	
				/* Wakeup blocked processes */
	wake_up_interruptible(&q->read_queue);
	wake_up_interruptible(&q->write_queue);
	wake_up_interruptible(&q->flush_queue);
	
				/* Finalization over.  Queue is made
                                   available when both use_count and
                                   finalization become 0, which won't
                                   happen until all the waiting processes
                                   stop waiting. */
	atomic_dec(&q->finalization);
	return 0;
}

int drm_adddraw DRM_IOCTL_ARGS
{
	drm_draw_t draw;

	draw.handle = 0;	/* NOOP */
	DRM_TRACE("%d\n", draw.handle);
	copy_to_user_ret((drm_draw_t *)arg, &draw, sizeof(draw), -EFAULT);
	return 0;
}

int drm_rmdraw DRM_IOCTL_ARGS
{
	return 0;		/* NOOP */
}

int drm_auth DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_auth_t      auth;
	
	if (priv->auth == DRM_AUTH_FAILED) return -EACCES; /* One try */

	copy_from_user_ret(&auth, (drm_auth_t *)arg, sizeof(auth), -EFAULT);
	DRM_TRACE("0x%08x:0x%08x\n", auth.hi, auth.lo);
	if (drm_check_key(dev, auth.hi, auth.lo)) {
		priv->auth = DRM_AUTH_OK;
		priv->hi   = auth.hi;
		priv->lo   = auth.lo;
				/* Must remove key, since used keys are
                                   made available via the /proc/drm/clients
                                   interface. */
		drm_remove_key(dev, priv->hi, priv->lo);
		return 0;
	}
	priv->auth = DRM_AUTH_FAILED;
	return -EACCES;
}

int drm_finish DRM_IOCTL_ARGS
{
	drm_file_t        *priv   = filp->private_data;
	drm_device_t      *dev    = priv->dev;
	int               ret     = 0;
	drm_lock_t        lock;

	DRM_TRACE("\n");

	copy_from_user_ret(&lock, (drm_lock_t *)arg, sizeof(lock), -EFAULT);
	ret = drm_flush_block_and_flush(dev, lock.context, lock.flags);
	drm_flush_unblock(dev, lock.context, lock.flags);
	return ret;
}
