/* drm_ioctl.c -- IOCTL processing for DRM -*- linux-c -*-
 * Created: Fri Jan  8 09:01:26 1999 by faith@precisioninsight.com
 * Revised: Fri Jun 18 09:49:12 1999 by faith@precisioninsight.com
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
 * $PI: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/generic/drm_ioctl.c,v 1.17 1999/06/21 14:31:21 faith Exp $
 * $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/generic/drm_ioctl.c,v 1.1 1999/06/14 07:32:04 dawes Exp $
 *
 */

#define __NO_VERSION__
#include "drmP.h"

int drm_version DRM_IOCTL_ARGS
{
	drm_file_t    *priv   = filp->private_data;
	drm_device_t  *dev    = priv->dev;
	drm_driver_t  *drv    = dev->driver;
	drm_version_t version;
	drm_version_t *vp;
	int           len;

	copy_from_user_ret(&version,
			   (drm_version_t *)arg,
			   sizeof(version),
			   -EFAULT);


				/* It is essential to zero the lengths for
                                   information that is not returned -- a
                                   user-level program may depend on the
                                   lengths to allocate the buffers. */
#define DRM_COPY(name,value)                                 \
	len = name##_len;                                    \
	name##_len = value##_len;                            \
	if (len > name##_len) len = name##_len;              \
	if (len && value##_len && name) {                    \
		copy_to_user_ret(name, value, len, -EFAULT); \
	}

	vp                         = drv->version;
	version.version_major      = vp->version_major;
	version.version_minor      = vp->version_minor;
	version.version_patchlevel = vp->version_patchlevel;

	DRM_COPY(version.name, vp->name);
	DRM_COPY(version.date, vp->date);
	DRM_COPY(version.desc, vp->desc);

	copy_to_user_ret((drm_version_t *)arg,
			 &version,
			 sizeof(version),
			 -EFAULT);
	return 0;
}

int drm_list DRM_IOCTL_ARGS
{
	drm_list_t                    list;
	drm_version_t                 version;
	int                           count = 0;
	drm_driver_t                  *pt;
	int                           len;

	DRM_TRACE("\n");
	copy_from_user_ret(&list,
			   (drm_list_t *)arg,
			   sizeof(list),
			   -EFAULT);

	for (pt = drm_drivers; pt; pt = pt->next) {
		if (pt->base) continue;
		if (count < list.count) {
			copy_from_user_ret(&version,
					   &list.version[count],
					   sizeof(version),
					   -EFAULT);
			version.version_major = pt->version->version_major;
			version.version_minor = pt->version->version_minor;
			version.version_patchlevel
				= pt->version->version_patchlevel;
			
			DRM_COPY(version.name, pt->version->name);
			DRM_COPY(version.date, pt->version->date);
			DRM_COPY(version.desc, pt->version->desc);
			copy_to_user_ret(&list.version[count],
					 &version,
					 sizeof(version),
					 -EFAULT);
		}
		++count;
	}
	
	list.count = count;

	copy_to_user_ret((drm_list_t *)arg,
			 &list,
			 sizeof(list),
			 -EFAULT);
	return 0;
}

int drm_create DRM_IOCTL_ARGS
{
	drm_request_t  request;
	char           name[DRM_MAX_NAMELEN+2];
	char           busid[DRM_MAX_NAMELEN+2];
	int            retcode;

	copy_from_user_ret(&request,
			   (drm_request_t *)arg,
			   sizeof(request),
			   -EFAULT);
	
	if (!request.device_name)      return -ENOENT;
	if (!request.device_busid)     return -ENOENT;
	retcode = strncpy_from_user(name,
				    request.device_name,
				    DRM_MAX_NAMELEN+1);
	if (retcode < 0)               return retcode;
	if (!retcode)                  return -ENOENT;
	if (retcode > DRM_MAX_NAMELEN) return -ENAMETOOLONG;
	
	retcode = strncpy_from_user(busid,
				    request.device_busid,
				    DRM_MAX_NAMELEN+1);
	if (retcode < 0)               return retcode;
	if (!retcode)                  return -ENOENT;
	if (retcode > DRM_MAX_NAMELEN) return -ENAMETOOLONG;
	
	name[DRM_MAX_NAMELEN] = '\0';
	busid[DRM_MAX_NAMELEN] = '\0';

	DRM_TRACE("%s %s\n", name, busid);
	
	retcode = drm_register_device(name, busid, 0);
	if (retcode < 0) return retcode;
	request.device_major = drm_major;
	request.device_minor = retcode;

	copy_to_user_ret((drm_request_t *)arg,
			 &request,
			 sizeof(request),
			 -EFAULT);
	
	return 0;
}

int drm_destroy DRM_IOCTL_ARGS
{
	drm_file_t    *priv   = filp->private_data;
	drm_device_t  *dev    = priv->dev;
	drm_request_t request;
	char          busid[DRM_MAX_NAMELEN+2];
	int           retcode = 0;

	copy_from_user_ret(&request,
			   (drm_request_t *)arg,
			   sizeof(request),
			   -EFAULT);
	
	if (!request.device_busid) return -ENOENT;
	
	retcode = strncpy_from_user(busid,
				    request.device_busid,
				    DRM_MAX_NAMELEN+1);
	if (retcode < 0)               return retcode;
	if (!retcode)                  return -ENOENT;
	if (retcode > DRM_MAX_NAMELEN) return -ENAMETOOLONG;
	
	busid[DRM_MAX_NAMELEN] = '\0';

	DRM_TRACE("%s (%d,%d)\n",
		  busid, request.device_major, request.device_minor);
	
	if (request.device_major != drm_major) return -ENOENT;

	if (strcmp(dev->busid, busid)) {
		DRM_DEBUG("busid = %s != %s\n", dev->busid, busid);
		retcode = -ENOENT; /* Busid mismatch */
	}
	return drm_unregister_device(request.device_minor);
}

int drm_irq_busid DRM_IOCTL_ARGS
{
	drm_irq_busid_t p;
	struct pci_dev  *dev;

	copy_from_user_ret(&p, (drm_irq_busid_t *)arg, sizeof(p), -EFAULT);
	dev = pci_find_slot(p.busnum, PCI_DEVFN(p.devnum, p.funcnum));
	if (dev) p.irq = dev->irq;
	else     p.irq = 0;
	DRM_TRACE("%d:%d:%d => IRQ %d\n",
		  p.busnum, p.devnum, p.funcnum, p.irq);
	copy_to_user_ret((drm_irq_busid_t *)arg, &p, sizeof(p), -EFAULT);
	return 0;
}


/* drm_ioctl is called whenever a process performs an ioctl on /dev/drm. */

int drm_ioctl DRM_IOCTL_ARGS
{
	int              nr      = DRM_IOCTL_NR(cmd);
	drm_file_t       *priv   = filp->private_data;
	drm_device_t     *dev    = priv->dev;
	drm_driver_t     *drv    = dev->driver;
	int              retcode = 0;
	drm_ioctl_desc_t *ioctl;
	drm_ioctl_t      *func;

	atomic_inc(&dev->ioctl_count);
	atomic_inc(&dev->total_ioctl);
	++priv->ioctl_count;
	
	DRM_TRACE("cmd = 0x%02x, nr = 0x%02x, dev (%d,%d), auth = %d\n",
		  cmd, nr, MAJOR(inode->i_rdev), dev->device_minor,
		  priv->auth);

	if (priv->auth && cmd != DRM_IOCTL_GET_MAGIC) {
		retcode = -EACCES;
	} else if (nr >= drv->ioctl_count) {
		retcode = -EINVAL;
	} else {
		ioctl     = &drv->ioctls[nr];
		func      = ioctl->func;

		if (!func) {
			retcode = -EINVAL;
		} else 	if (ioctl->root_only && !capable(CAP_SYS_ADMIN)) {
			retcode = -EACCES;
		} else {
			retcode = (func)(inode, filp, cmd, arg);
		}
	}
	
	atomic_dec(&dev->ioctl_count);
	return retcode;
}
