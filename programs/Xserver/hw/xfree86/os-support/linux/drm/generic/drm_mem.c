/* drm_mem.c -- 
 * Created: Thu Feb  4 14:00:34 1999 by faith@precisioninsight.com
 * Revised: Wed May 12 07:42:32 1999 by faith@precisioninsight.com
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
 * $PI: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/generic/drm_mem.c,v 1.12 1999/05/12 11:54:02 faith Exp $
 * $XFree86$
 *
 */

#define __NO_VERSION__
#include "drmP.h"

#define FUDGE 0			/* For debugging (0 for production code) */

void *drm_alloc(size_t size, int area)
{
    void *pt;

    if (!size) {
	DRM_MEM_ERROR(area, "Allocating 0 bytes\n");
	return NULL;
    }

    spin_lock(&drm_mem_lock);
    
    if (!(pt = kmalloc(size + FUDGE, GFP_KERNEL))) {
	++drm_mem_stats[area].fail_count;
	spin_unlock(&drm_mem_lock);
	return NULL;
    }
    ++drm_mem_stats[area].succeed_count;
    drm_mem_stats[area].bytes_allocated += size;
    spin_unlock(&drm_mem_lock);
    
    DRM_MEM_VERB(area, "alloc(%d) = %p\n", size, pt);
    return pt;
}

void *drm_realloc(void *oldpt, size_t oldsize, size_t size, int area)
{
    void *pt;

    if (!(pt = drm_alloc(size, area))) return NULL;
    if (oldpt && oldsize) {
	memcpy(pt, oldpt, oldsize);
	drm_free(oldpt, oldsize, area);
    }
    return pt;
}

char *drm_strdup(const char *s, int area)
{
    char *pt;
    int  length = s ? strlen(s) : 0;

    if (!(pt = drm_alloc(length+1, area))) return NULL;
    strcpy(pt, s);
    return pt;
}

void drm_strfree(const char *s, int area)
{
    unsigned int size;

    if (!s) return;

    size = 1 + (s ? strlen(s) : 0);
    drm_free((void *)s, size, area);
}

void drm_free(void *pt, size_t size, int area)
{
    int alloc_count;
    int free_count;
    
    DRM_MEM_VERB(area, "free(0x%p)\n", pt);
    if (!pt) DRM_MEM_ERROR(area, "Attempt to free NULL pointer\n");
    else     kfree(pt);
    spin_lock(&drm_mem_lock);
    drm_mem_stats[area].bytes_freed += size;
    free_count  = ++drm_mem_stats[area].free_count;
    alloc_count =   drm_mem_stats[area].succeed_count;
    if (free_count > alloc_count) {
	DRM_MEM_ERROR(area, "Excess frees: %d frees, %d allocs\n",
		      free_count, alloc_count);
    }
    spin_unlock(&drm_mem_lock);
}

unsigned long drm_alloc_dma(int order)
{
    unsigned long address;
    unsigned long bytes   = PAGE_SIZE << order;
    unsigned long addr;
    unsigned int  sz;
    
    spin_lock(&drm_mem_lock);
    if (drm_ram_used > +(DRM_RAM_PERCENT * drm_ram_available) / 100) {
	spin_unlock(&drm_mem_lock);
	return 0;
    }
    address = __get_free_pages(GFP_KERNEL, order);
    if (!address) {
	++drm_mem_stats[DRM_MEM_DMA].fail_count;
	spin_unlock(&drm_mem_lock);
	return 0;
    }
    ++drm_mem_stats[DRM_MEM_DMA].succeed_count;
    drm_mem_stats[DRM_MEM_DMA].bytes_allocated += bytes;
    drm_ram_used                               += bytes;
    spin_unlock(&drm_mem_lock);

    
				/* Zero outside the lock */
    memset((void *)address, 0, bytes);
    
				/* Reserve */
    for (addr = address, sz = bytes;
	 sz > 0;
	 addr += PAGE_SIZE, sz -= PAGE_SIZE) {
	mem_map_reserve(MAP_NR(addr));
    }
    
    DRM_MEM_VERB(DRM_MEM_DMA, "alloc(2^%d) = %lu\n", order, address);
    return address;
}

void drm_free_dma(unsigned long address, int order)
{
    unsigned long bytes = PAGE_SIZE << order;
    int           alloc_count;
    int           free_count;
    unsigned long addr;
    unsigned int  sz;
    
    DRM_MEM_VERB(DRM_MEM_DMA, "free(0x%08lx, %d)\n", address, order);
    if (!address) {
	DRM_MEM_ERROR(DRM_MEM_DMA, "Attempt to free address 0\n");
    } else {
				/* Unreserve */
	for (addr = address, sz = bytes;
	     sz > 0;
	     addr += PAGE_SIZE, sz -= PAGE_SIZE) {
	    mem_map_unreserve(MAP_NR(addr));
	}
	free_pages(address, order);
    }
    
    spin_lock(&drm_mem_lock);
    free_count  = ++drm_mem_stats[DRM_MEM_DMA].free_count;
    alloc_count =   drm_mem_stats[DRM_MEM_DMA].succeed_count;
    drm_mem_stats[DRM_MEM_DMA].bytes_freed += bytes;
    drm_ram_used                           -= bytes;
    spin_unlock(&drm_mem_lock);
    if (free_count > alloc_count) {
	DRM_MEM_ERROR(DRM_MEM_DMA, "Excess frees: %d frees, %d allocs\n",
		      free_count, alloc_count);
    }
}

void *drm_vmalloc(unsigned int size)
{
    void          *pt;

    if (!size) {
	DRM_MEM_ERROR(DRM_MEM_SAREA, "Allocating 0 bytes in kernel-virtual\n");
	return NULL;
    }

    spin_lock(&drm_mem_lock);
    if (!(pt = vmalloc(size))) {
	++drm_mem_stats[DRM_MEM_SAREA].fail_count;
	spin_unlock(&drm_mem_lock);
	return NULL;
    }
    ++drm_mem_stats[DRM_MEM_SAREA].succeed_count;
    drm_mem_stats[DRM_MEM_SAREA].bytes_allocated += size;
    spin_unlock(&drm_mem_lock);

				/* Zero outside the lock */
    memset(pt, 0, size);
    DRM_MEM_VERB(DRM_MEM_SAREA, "alloc(%u) = %p\n", size, pt);
    return pt;
}

void drm_vfree(void *pt, unsigned int size)
{
    int           alloc_count;
    int           free_count;
	
    DRM_MEM_VERB(DRM_MEM_SAREA, "free(0x%p)\n", pt);
    if (!pt) {
	DRM_MEM_ERROR(DRM_MEM_SAREA, "Attempt to free NULL pointer\n");
    } else {
	vfree(pt);
    }
    
    spin_lock(&drm_mem_lock);
    drm_mem_stats[DRM_MEM_SAREA].bytes_freed += size;
    free_count  = ++drm_mem_stats[DRM_MEM_SAREA].free_count;
    alloc_count =   drm_mem_stats[DRM_MEM_SAREA].succeed_count;
    spin_unlock(&drm_mem_lock);
    if (free_count > alloc_count) {
	DRM_MEM_ERROR(DRM_MEM_SAREA, "Excess frees: %d frees, %d allocs\n",
		      free_count, alloc_count);
    }
}

void *drm_ioremap(unsigned long offset, unsigned long size)
{
    void *pt;

    if (!size) {
	DRM_MEM_ERROR(DRM_MEM_MAPPINGS,
		      "Mapping 0 bytes at 0x%08lx\n", offset);
	return NULL;
    }
		      
    spin_lock(&drm_mem_lock);
    if (!(pt = ioremap(offset, size))) {
	++drm_mem_stats[DRM_MEM_MAPPINGS].fail_count;
	spin_unlock(&drm_mem_lock);
	return NULL;
    }
    ++drm_mem_stats[DRM_MEM_MAPPINGS].succeed_count;
    drm_mem_stats[DRM_MEM_MAPPINGS].bytes_allocated += size;
    spin_unlock(&drm_mem_lock);
    DRM_MEM_VERB(DRM_MEM_MAPPINGS, "map(%lu,%lu) = %p\n", offset, size, pt);
    return pt;
}

void drm_ioremapfree(void *pt, unsigned long size)
{
    int alloc_count;
    int free_count;
	
    DRM_MEM_VERB(DRM_MEM_MAPPINGS, "free(0x%p)\n", pt);
    if (!pt) DRM_MEM_ERROR(DRM_MEM_MAPPINGS, "Attempt to free NULL pointer\n");
    else     vfree(pt);
    spin_lock(&drm_mem_lock);
    drm_mem_stats[DRM_MEM_MAPPINGS].bytes_freed += size;
    free_count  = ++drm_mem_stats[DRM_MEM_MAPPINGS].free_count;
    alloc_count =   drm_mem_stats[DRM_MEM_MAPPINGS].succeed_count;
    spin_unlock(&drm_mem_lock);
    if (free_count > alloc_count) {
	DRM_MEM_ERROR(DRM_MEM_MAPPINGS, "Excess frees: %d frees, %d allocs\n",
		      free_count, alloc_count);
    }
}
