/* gen_dma.c -- Generic DMA IOCTL and function support -*- linux-c -*-
 * Created: Fri Mar 19 14:30:16 1999 by faith@dict.org
 * Revised: Thu Jun 24 15:49:10 1999 by faith@dict.org
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
 * $PI: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/generic/gen_dma.c,v 1.31 1999/06/24 20:29:42 faith Exp $
 * $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/generic/gen_dma.c,v 1.1 1999/06/14 07:32:05 dawes Exp $
 *
 */

#define __NO_VERSION__
#include "drmP.h"

#include <linux/interrupt.h>	/* For task queue support */

#define DRM_ENGINE_VERBOSE 0	/* Make drm_engine trace verbosely */

#if DRM_DEBUG_CODE > 1
#define CHECK                                                            \
do {                                                                     \
	if (++looping > DRM_LOOPING_LIMIT) {                             \
	        DRM_ERROR("looping = %d, command = %d, inst = %d\n",     \
                          looping, command, i/_DRM_INST_LENGTH);         \
		return -EBUSY;                                           \
	}                                                                \
} while (0)
#else
#define CHECK do { } while (0)
#endif

#define COMPARE(expr)                                           \
do {                                                            \
	if (expr) i  = cmd->inst[i+4] * _DRM_INST_LENGTH;       \
        else      i += _DRM_INST_LENGTH;                        \
} while (0)

#if DRM_DMA_HISTOGRAM
				/* This is slow, but is useful for
                                   debugging. */
int drm_histogram_slot(unsigned long count)
{
	int value = DRM_DMA_HISTOGRAM_INITIAL;
	int slot;

	for (slot = 0;
	     slot < DRM_DMA_HISTOGRAM_SLOTS;
	     ++slot, value = DRM_DMA_HISTOGRAM_NEXT(value)) {
		if (count < value) return slot;
	}
	return DRM_DMA_HISTOGRAM_SLOTS - 1;
}

void drm_histogram_compute(drm_device_t *dev, drm_buf_t *buf)
{
	cycles_t queued_to_dispatched;
	cycles_t dispatched_to_completed;
	cycles_t completed_to_freed;
	int      q2d, d2c, c2f, q2c, q2f;
	
	if (buf->time_queued) {
		queued_to_dispatched    = (buf->time_dispatched
					   - buf->time_queued);
		dispatched_to_completed = (buf->time_completed
					   - buf->time_dispatched);
		completed_to_freed      = (buf->time_freed
					   - buf->time_completed);

		q2d = drm_histogram_slot(queued_to_dispatched);
		d2c = drm_histogram_slot(dispatched_to_completed);
		c2f = drm_histogram_slot(completed_to_freed);

		q2c = drm_histogram_slot(queued_to_dispatched
					 + dispatched_to_completed);
		q2f = drm_histogram_slot(queued_to_dispatched
					 + dispatched_to_completed
					 + completed_to_freed);
		
		atomic_inc(&dev->histo.total);
		atomic_inc(&dev->histo.queued_to_dispatched[q2d]);
		atomic_inc(&dev->histo.dispatched_to_completed[d2c]);
		atomic_inc(&dev->histo.completed_to_freed[c2f]);
		
		atomic_inc(&dev->histo.queued_to_completed[q2c]);
		atomic_inc(&dev->histo.queued_to_freed[q2f]);

	}
	buf->time_queued     = 0;
	buf->time_dispatched = 0;
	buf->time_completed  = 0;
	buf->time_freed      = 0;
}
#endif

void drm_dma_wrapper(void *dev)
{
	drm_dma_schedule(dev, 0);
}

static __inline__ void drm_dma_queue_scheduler(drm_device_t *dev)
{
	queue_task(&dev->tq, &tq_immediate);
	mark_bh(IMMEDIATE_BH);
}

void drm_free_buffer(drm_device_t *dev, drm_buf_t *buf)
{
	if (!buf) return;
	
	buf->waiting  = 0;
	buf->pending  = 0;
	buf->pid      = 0;
	buf->used     = 0;
#if DRM_DMA_HISTOGRAM
	buf->time_completed = get_cycles();
#endif
	if (waitqueue_active(&buf->dma_wait)) {
		wake_up_interruptible(&buf->dma_wait);
	} else {
				/* If processes are waiting, the last one
                                   to wake will put the buffer on the free
                                   list.  If no processes are waiting, we
                                   put the buffer on the freelist here. */
		drm_freelist_put(dev, &dev->bufs[buf->order].freelist, buf);
	}
}

int drm_free_this_buffer(drm_device_t *dev)
{
	if (test_and_set_bit(0, &dev->dma_flag)) {
		atomic_inc(&dev->total_missed_free);
		return 1;
	}

	if (dev->this_buffer) {
		drm_do_command(dev, _DRM_DMA_READY);
		drm_free_buffer(dev, dev->this_buffer);
		dev->this_buffer = NULL;
	}
	clear_bit(0, &dev->dma_flag);
	return 0;
}

void drm_reclaim_buffers(drm_device_t *dev, pid_t pid)
{
	int i;

	for (i = 0; i < dev->buf_count; i++) {
		if (dev->buflist[i]->pid == pid) {
			switch (dev->buflist[i]->list) {
			case DRM_LIST_NONE:
				drm_free_buffer(dev, dev->buflist[i]);
				break;
			case DRM_LIST_WAIT:
				dev->buflist[i]->list = DRM_LIST_RECLAIM;
				break;
			default:
				/* Nothing to do here, buffer already on
                                   hardware. */
				break;
			}
		}
	}
}

/* drm_engine executes the script passed from the X server via
   DRM_IOCTL_CONTROL.  For performance reasons, no argument checking is
   performed.  However, we observe that the X server has all of the regions
   mapped, and that it can write to those regions directly -- there is no
   violation of the security policy by trusting the information from the X
   server. */

int drm_engine(drm_device_t *dev,
	       drm_desc_t command,
	       unsigned long address,
	       unsigned long length)
{
	int           i;
	unsigned long val;
	unsigned long accumulator = 0;
	drm_command_t *cmd        = &dev->cmd[command];
	int           retcode     = 0;
#if DRM_DEBUG_CODE > 1
	int           looping     = 0;
#endif
	
#define BASE    ((unsigned long)dev->maplist[cmd->inst[i+1]]->handle)
#define ADDRESS ((BASE) + cmd->inst[i+2])
#define DEREF   *(__volatile__ int *)ADDRESS
#define CMD     _DRM_E_CMD(cmd->inst[i])
#define TYPE    _DRM_E_TYPE(cmd->inst[i])
#define MOD     _DRM_E_MOD(cmd->inst[i])
#define MODVAL  _DRM_E_MODVAL(cmd->inst[i])
#define COND    _DRM_E_COND(cmd->inst[i])

#if DRM_ENGINE_VERBOSE
	DRM_TRACE("0x%08lx (0x%08lx bytes); %d instructions for %d\n",
		  address, length, cmd->count, command);
#endif

	if (!cmd->count || !cmd->inst) return -EINVAL;

	for (i = 0; i < cmd->count * _DRM_INST_LENGTH; /* MUST UPDATE i */) {
		CHECK;
		switch (TYPE) {
		case _DRM_T_LENGTH:
			val = length;
			break;
		case _DRM_T_ADDRESS:
			val = virt_to_phys((void *)address);
			break;
		case _DRM_T_ACC:
			val = accumulator;
			break;
		case _DRM_T_IMM:
		default:
			val = cmd->inst[i+3];
			break;
		}
		
		if (MOD == _DRM_V_RSHIFT) val >>= MODVAL;
		if (MOD == _DRM_V_LSHIFT) val <<= MODVAL;
		
		switch (CMD) {
		case _DRM_M_WRITE:
#if DRM_ENGINE_VERBOSE
			DRM_VERB("  (%d,0x%04x)=0x%lx <= 0x%08lx\n",
				 cmd->inst[i+1],
				 cmd->inst[i+2],
				 ADDRESS,
				 val);
#endif
			DEREF = val;
			i += _DRM_INST_LENGTH;
			break;
		case _DRM_M_READ:
			accumulator = DEREF;
			DRM_VERB("  0x%08lx\n", accumulator);
			i += _DRM_INST_LENGTH;
			break;
		case _DRM_M_WHILE:
#if DRM_ENGINE_VERBOSE
			DRM_VERB("  while (%d,0x%04x)=0x%lx %s %lu\n",
				 cmd->inst[i+1],
				 cmd->inst[i+2],
				 ADDRESS,
				 COND == _DRM_C_NE ? "!=" : "?",
				 val);
#endif
			switch (COND) {
			case _DRM_C_EQ: while (DEREF == val) CHECK; break;
			case _DRM_C_NE: while (DEREF != val) CHECK; break;
			case _DRM_C_LT: while (DEREF <  val) CHECK; break;
			case _DRM_C_GT: while (DEREF >  val) CHECK; break;
			case _DRM_C_LE: while (DEREF <= val) CHECK; break;
			case _DRM_C_GE: while (DEREF >= val) CHECK; break;
			}
			i += _DRM_INST_LENGTH;
			break;
		case _DRM_M_IF:
#if DRM_ENGINE_VERBOSE
			DRM_VERB("  if (%d,0x%04x)=0x%lx [%lu %s %u]"
				 " goto %d\n",
				 cmd->inst[i+1],
				 cmd->inst[i+2],
				 ADDRESS,
				 val,
				 COND == _DRM_C_NE ? "!=" : "?",
				 DEREF,
				 cmd->inst[i+4]);
#endif
			switch (COND) {
			case _DRM_C_EQ:  COMPARE(DEREF == val);     break;
			case _DRM_C_NE:  COMPARE(DEREF != val);     break;
			case _DRM_C_LT:  COMPARE(DEREF <  val);     break;
			case _DRM_C_GT:  COMPARE(DEREF >  val);     break;
			case _DRM_C_LE:  COMPARE(DEREF <= val);     break;
			case _DRM_C_GE:  COMPARE(DEREF >= val);     break;
			case _DRM_C_BIT: COMPARE(DEREF & (1<<val)); break;
			}
			break;
		case _DRM_M_TEST:
#if DRM_ENGINE_VERBOSE
			DRM_VERB("  test (%lu) %s %lu goto %d\n",
				 accumulator,
				 COND == _DRM_C_NE ? "!=" : "?",
				 val,
				 cmd->inst[i+4]);
#endif
			switch (COND) {
			case _DRM_C_EQ:  COMPARE(accumulator == val);    break;
			case _DRM_C_NE:  COMPARE(accumulator != val);    break;
			case _DRM_C_LT:  COMPARE(accumulator <  val);    break;
			case _DRM_C_GT:  COMPARE(accumulator >  val);    break;
			case _DRM_C_LE:  COMPARE(accumulator <= val);    break;
			case _DRM_C_GE:  COMPARE(accumulator >= val);    break;
			case _DRM_C_BIT: COMPARE(accumulator & (1<<val));break;
			}
			break;
		case _DRM_M_GOTO:
#if DRM_ENGINE_VERBOSE
			DRM_VERB("  goto %d\n", cmd->inst[i+4]);
#endif
			i = cmd->inst[i+4] * _DRM_INST_LENGTH;
			break;
		case _DRM_M_NOOP:
#if DRM_ENGINE_VERBOSE
			DRM_VERB("  noop\n");
#endif
			i += _DRM_INST_LENGTH;
			break;
		case _DRM_M_RETURN:
#if DRM_ENGINE_VERBOSE
			DRM_VERB("  return %ld\n", val);
#endif
			retcode = val;
			i = cmd->count * _DRM_INST_LENGTH; /* End */
			break;
		case _DRM_M_DO:
#if DRM_ENGINE_VERBOSE
			DRM_VERB("  do %d\n", cmd->inst[i+4]);
#endif
			switch (cmd->inst[i+4]) {
			case _DRM_F_CLEAR:
				drm_free_this_buffer(dev);
				break;
			case _DRM_F_DMA:
				drm_dma_queue_scheduler(dev);
				break;
			}
			i += _DRM_INST_LENGTH;
			break;


			
				/* WARNING: If you add any more commands
                                   here, be sure to set i to the next
                                   instruction value!  Otherwise, you will
                                   enter an infinite loop. */
			
		default:
			DRM_ERROR("Unknown command %d\n", CMD);
			i += _DRM_INST_LENGTH;
			break;
		}
	}
#if DRM_ENGINE_VERBOSE
	DRM_VERB("  exit %d\n", retcode);
#endif
	return retcode;
}

				     
int drm_block DRM_IOCTL_ARGS
{
	DRM_TRACE("\n");
	return 0;
}

int drm_unblock DRM_IOCTL_ARGS
{
	DRM_TRACE("\n");
	return 0;
}

static void drm_interrupt_handler(int irq, void *dev, struct pt_regs *regs)
{
	atomic_inc(&((drm_device_t *)dev)->total_irq);
	drm_do_command(dev, _DRM_IH_SERVICE);
}


int drm_irq_install(drm_device_t *dev, int irq)
{
	int retcode;

	if (!irq)     return -EINVAL;
	
	spin_lock(&dev->struct_lock);
	if (dev->irq) {
		spin_unlock(&dev->struct_lock);
		return -EBUSY;
	}
	dev->irq = irq;
	spin_unlock(&dev->struct_lock);
	
	DRM_TRACE("%d\n", irq);

	dev->context_flag   = 0;
	dev->interrupt_flag = 0;
	dev->dma_flag       = 0;
	dev->next_buffer    = NULL;
	dev->next_queue     = NULL;
	dev->this_buffer    = NULL;

	dev->tq.next        = NULL;
	dev->tq.sync        = 0;
	dev->tq.routine     = drm_dma_wrapper;
	dev->tq.data        = dev;

	drm_do_command(dev, _DRM_IH_PRE_INST); /* Optional */
	if ((retcode = request_irq(dev->irq,
				   drm_interrupt_handler,
				   0, // SA_INTERRUPT,
				   dev->devname,
				   dev))) {
		spin_lock(&dev->struct_lock);
		dev->irq = 0;
		spin_unlock(&dev->struct_lock);
		return retcode;
	}
	drm_do_command(dev, _DRM_IH_POST_INST); /* Optional */

	return 0;
}

int drm_irq_uninstall(drm_device_t *dev)
{
	int irq;

	spin_lock(&dev->struct_lock);
	irq      = dev->irq;
	dev->irq = 0;
	spin_unlock(&dev->struct_lock);
	
	if (!irq) return -EINVAL;
	
	DRM_TRACE("%d\n", irq);
	
	drm_do_command(dev, _DRM_IH_PRE_UNINST); /* Optional */
	free_irq(irq, dev);
	drm_do_command(dev, _DRM_IH_POST_UNINST); /* Optional */

	return 0;
}


int drm_control DRM_IOCTL_ARGS
{
	drm_file_t      *priv   = filp->private_data;
	drm_device_t    *dev    = priv->dev;
	drm_control_t   ctl;
	drm_command_t   *command;
	int             retcode;
	
	copy_from_user_ret(&ctl, (drm_control_t *)arg, sizeof(ctl), -EFAULT);
	DRM_TRACE("func = %d\n", ctl.func);
	
	if (ctl.desc < 0 || ctl.desc >= _DRM_DESC_MAX) {
		return -EINVAL;
	}
	command = &dev->cmd[ctl.desc];
	
	switch (ctl.func) {
	case DRM_ADD_COMMAND:
		if (command->inst) {
			return -EBUSY;
		}
		DRM_DEBUG("count = %d, %d bytes, desc = %d\n",
			  ctl.count,
			  ctl.count
			  * _DRM_INST_LENGTH
			  * sizeof(*command->inst),
			  ctl.desc );
		command->count = ctl.count;
		command->inst  = drm_alloc(command->count
					   * _DRM_INST_LENGTH
					   * sizeof(*command->inst),
					   DRM_MEM_CMDS);
		if (copy_from_user(command->inst, ctl.inst,
				   ctl.count
				   * _DRM_INST_LENGTH
				   * sizeof(*command->inst))) {
			return -EFAULT;
		}
		break;    
	case DRM_RM_COMMAND:
		if (command->inst) {
			drm_free(command->inst,
				 command->count
				 * _DRM_INST_LENGTH
				 * sizeof(*command->inst),
				 DRM_MEM_CMDS);
		}
		command->inst  = NULL;
		command->count = 0;
		break;
	case DRM_INST_HANDLER:
		if ((retcode = drm_irq_install(dev, ctl.irq))) {
			return retcode;
		}
		break;
	case DRM_UNINST_HANDLER:
		if ((retcode = drm_irq_uninstall(dev))) {
			return retcode;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

int drm_context_switch(drm_device_t *dev, int old, int new)
{
	char        buf[64];
	drm_queue_t *q;

	atomic_inc(&dev->total_ctx);

	if (test_and_set_bit(0, &dev->context_flag)) {
		DRM_ERROR("Reentering -- FIXME\n");
		return -EBUSY;
	}

#if DRM_DMA_HISTOGRAM
	dev->ctx_start = get_cycles();
#endif
	
	DRM_TRACE("Context switch from %d to %d\n", old, new);

	if (new >= dev->queue_count) {
		clear_bit(0, &dev->context_flag);
		return -EINVAL;
	}

	if (new == dev->last_context) {
		clear_bit(0, &dev->context_flag);
		return 0;
	}
	
	q = dev->queuelist[new];
	atomic_inc(&q->use_count);
	if (atomic_read(&q->use_count) == 1) {
		atomic_dec(&q->use_count);
		clear_bit(0, &dev->context_flag);
		return -EINVAL;
	}

	if (drm_flags & DRM_FLAG_NOCTX) {
		drm_context_switch_complete(dev, new);
	} else {
		sprintf(buf, "C %d %d\n", old, new);
		drm_write_string(dev, buf);
	}
	
	atomic_dec(&q->use_count);
	
	return 0;
}

int drm_context_switch_complete(drm_device_t *dev, int new)
{
	dev->last_context = new;  /* PRE/POST: This is the _only_ writer. */
	dev->last_switch  = jiffies;
	
	if (!_DRM_LOCK_IS_HELD(dev->lock.hw_lock->lock)) {
		DRM_ERROR("Lock isn't held after context switch\n");
	}
	if (!(dev->next_buffer && dev->next_buffer->while_locked)) {
		if (drm_lock_free(dev, &dev->lock.hw_lock->lock,
				  DRM_KERNEL_CONTEXT)) {
			DRM_ERROR("\n");
		}
	}
	
#if DRM_DMA_HISTOGRAM
	atomic_inc(&dev->histo.ctx[drm_histogram_slot(get_cycles()
		                                      - dev->ctx_start)]);
		   
#endif
	clear_bit(0, &dev->context_flag);
	wake_up_interruptible(&dev->context_wait);
	
	return 0;
}

static void drm_clear_next_buffer(drm_device_t *dev)
{
	dev->next_buffer = NULL;
	if (dev->next_queue && !DRM_BUFCOUNT(&dev->next_queue->waitlist)) {
		wake_up_interruptible(&dev->next_queue->flush_queue);
	}
	dev->next_queue  = NULL;
}


/* Only called by drm_dma_schedule. */
int drm_do_dma(drm_device_t *dev, int locked)
{
	unsigned long address;
	unsigned long length;
	drm_buf_t     *buf;
	int           retcode;
#if DRM_DMA_HISTOGRAM
	cycles_t      dma_start, dma_stop;
#endif

	DRM_TRACE("\n");

	if (test_and_set_bit(0, &dev->dma_flag)) {
		atomic_inc(&dev->total_missed_dma);
		return -EBUSY;
	}
	
#if DRM_DMA_HISTOGRAM
	dma_start = get_cycles();
#endif

	if (!dev->next_buffer) {
		DRM_ERROR("No next_buffer\n");
		clear_bit(0, &dev->dma_flag);
		return -EINVAL;
	}

	buf     = dev->next_buffer;
	address = (unsigned long)buf->address;
	length  = buf->used;

	DRM_DEBUG("context %d, buffer %d (%ld bytes)\n",
		  buf->context, buf->idx, length);

	if (buf->list == DRM_LIST_RECLAIM) {
		drm_clear_next_buffer(dev);
		drm_free_buffer(dev, buf);
		clear_bit(0, &dev->dma_flag);
		return -EINVAL;
	}

	if (!length) {
		DRM_ERROR("0 length buffer\n");
		drm_clear_next_buffer(dev);
		drm_free_buffer(dev, buf);
		clear_bit(0, &dev->dma_flag);
		return 0;
	}
	
	if (!drm_do_command(dev, _DRM_DMA_IS_READY)) {
		clear_bit(0, &dev->dma_flag);
		return -EBUSY;
	}

	if (buf->while_locked) {
		if (!_DRM_LOCK_IS_HELD(dev->lock.hw_lock->lock)) {
			DRM_ERROR("Dispatching buffer %d from pid %d"
				  " \"while locked\", but no lock held\n",
				  buf->idx, buf->pid);
		}
	} else {
		if (!locked && !drm_lock_take(&dev->lock.hw_lock->lock,
					      DRM_KERNEL_CONTEXT)) {
			atomic_inc(&dev->total_missed_lock);
			clear_bit(0, &dev->dma_flag);
			return -EBUSY;
		}
	}

	if (dev->last_context != buf->context
	    && !(dev->queuelist[buf->context]->flags
		 & _DRM_CONTEXT_PRESERVED)) {
				/* PRE: dev->last_context != buf->context */
		if (drm_context_switch(dev, dev->last_context, buf->context)) {
			drm_clear_next_buffer(dev);
			drm_free_buffer(dev, buf);
		}
		retcode = -EBUSY;
		goto cleanup;
			
				/* POST: we will wait for the context
                                   switch and will dispatch on a later call
                                   when dev->last_context == buf->context.
                                   NOTE WE HOLD THE LOCK THROUGHOUT THIS
                                   TIME! */
	}

	drm_clear_next_buffer(dev);
	buf->pending     = 1;
	buf->waiting     = 0;
	buf->list        = DRM_LIST_PEND;
#if DRM_DMA_HISTOGRAM
	buf->time_dispatched = get_cycles();
#endif
	if (dev->cmd[_DRM_DMA_DISPATCH].f) {
		retcode = dev->cmd[_DRM_DMA_DISPATCH].f(dev,
							_DRM_DMA_DISPATCH,
							address,
							length);
        } else {
		retcode = drm_engine(dev, _DRM_DMA_DISPATCH, address, length);
	}
	drm_free_buffer(dev, dev->this_buffer);
	dev->this_buffer = buf;

	atomic_add(length, &dev->total_bytes);
	atomic_inc(&dev->total_dmas);

	if (!buf->while_locked && !dev->context_flag && !locked) {
		if (drm_lock_free(dev, &dev->lock.hw_lock->lock,
				  DRM_KERNEL_CONTEXT)) {
			DRM_ERROR("\n");
		}
	}
cleanup:

	clear_bit(0, &dev->dma_flag);

#if DRM_DMA_HISTOGRAM
	dma_stop = get_cycles();
	atomic_inc(&dev->histo.dma[drm_histogram_slot(dma_stop - dma_start)]);
#endif

	return retcode;
}

static int drm_select_queue(drm_device_t *dev)
{
	int        i;
	int        candidate = -1;
	int        j         = jiffies;

	if (!dev) {
		DRM_ERROR("No device\n");
		return -1;
	}
	if (!dev->queuelist || !dev->queuelist[DRM_KERNEL_CONTEXT]) {
				/* This only happens between the time the
                                   interrupt is initialized and the time
                                   the queues are initialized. */
		return -1;
	}

				/* Doing "while locked" DMA? */
	if (DRM_WAITCOUNT(dev, DRM_KERNEL_CONTEXT)) {
		return DRM_KERNEL_CONTEXT;
	}

				/* If there are buffers on the last_context
                                   queue, and we have not been executing
                                   this context very long, continue to
                                   execute this context. */
	if (dev->last_switch <= j
	    && dev->last_switch + DRM_TIME_SLICE > j
	    && DRM_WAITCOUNT(dev, dev->last_context)) {
		return dev->last_context;
	}

				/* Otherwise, find a candidate */
	for (i = dev->last_checked + 1; i < dev->queue_count; i++) {
		if (DRM_WAITCOUNT(dev, i)) {
			candidate = dev->last_checked = i;
			break;
		}
	}

	if (candidate < 0) {
		for (i = 0; i < dev->queue_count; i++) {
			if (DRM_WAITCOUNT(dev, i)) {
				candidate = dev->last_checked = i;
				break;
			}
		}
	}

	if (candidate >= 0
	    && candidate != dev->last_context
	    && dev->last_switch <= j
	    && dev->last_switch + DRM_TIME_SLICE > j) {
		if (dev->timer.expires != dev->last_switch + DRM_TIME_SLICE) {
			del_timer(&dev->timer);
			dev->timer.expires  = dev->last_switch+DRM_TIME_SLICE;
			add_timer(&dev->timer);
		}
		return -1;
	}

	return candidate;
}


int drm_dma_schedule(drm_device_t *dev, int locked)
{
	int         next;
	drm_queue_t *q;
	drm_buf_t   *buf;
	int         retcode   = 0;
	int         processed = 0;
	int         missed;
	int         expire    = 20;
#if DRM_DMA_HISTOGRAM
	cycles_t    schedule_start;
#endif

	if (test_and_set_bit(0, &dev->interrupt_flag)) {
				/* Not reentrant */
		atomic_inc(&dev->total_missed_sched);
		return -EBUSY;
	}
	missed = atomic_read(&dev->total_missed_sched);

#if DRM_DMA_HISTOGRAM
	schedule_start = get_cycles();
#endif

again:
	if (dev->context_flag) {
		clear_bit(0, &dev->interrupt_flag);
		return -EBUSY;
	}
	if (dev->next_buffer) {
				/* Unsent buffer that was previously
                                   selected, but that couldn't be sent
                                   because the lock could not be obtained
                                   or the DMA engine wasn't ready.  Try
                                   again. */
		atomic_inc(&dev->total_tried);
		if (!(retcode = drm_do_dma(dev, locked))) {
			atomic_inc(&dev->total_hit);
			++processed;
		}
	} else {
		do {
			next = drm_select_queue(dev);
			if (next >= 0) {
				q   = dev->queuelist[next];
				buf = drm_waitlist_get(&q->waitlist);
				dev->next_buffer = buf;
				dev->next_queue  = q;
				if (buf && buf->list == DRM_LIST_RECLAIM) {
					drm_clear_next_buffer(dev);
					drm_free_buffer(dev, buf);
				}
			}
		} while (next >= 0 && !dev->next_buffer);
		if (dev->next_buffer) {
			if (!(retcode = drm_do_dma(dev, locked))) {
				++processed;
			}
		}
	}

	if (--expire) {
		if (missed != atomic_read(&dev->total_missed_sched)) {
			atomic_inc(&dev->total_lost);
			if (drm_do_command(dev, _DRM_DMA_IS_READY)) goto again;
		}
		if (processed && drm_do_command(dev, _DRM_DMA_IS_READY)) {
			atomic_inc(&dev->total_lost);
			processed = 0;
			goto again;
		}
	}
	
	clear_bit(0, &dev->interrupt_flag);
	
#if DRM_DMA_HISTOGRAM
	atomic_inc(&dev->histo.schedule[drm_histogram_slot(get_cycles()
							   - schedule_start)]);
#endif
	return retcode;
}

int drm_dma_enqueue(drm_device_t *dev, drm_dma_t *dma)
{
	int               i;
	drm_queue_t       *q;
	drm_buf_t         *buf;
	int               idx;
	int               while_locked = 0;
	DECLARE_WAITQUEUE(entry, current);

 	DRM_TRACE("%d\n", dma->send_count);

	if (dma->flags & _DRM_DMA_WHILE_LOCKED) {
		int context = dev->lock.hw_lock->lock;
		
		if (!_DRM_LOCK_IS_HELD(context)) {
			DRM_ERROR("No lock held during \"while locked\""
				  " request\n");
			return -EINVAL;
		}
		if (dma->context != _DRM_LOCKING_CONTEXT(context)
		    && _DRM_LOCKING_CONTEXT(context) != DRM_KERNEL_CONTEXT) {
			DRM_ERROR("Lock held by %d while %d makes"
				  " \"while locked\" request\n",
				  _DRM_LOCKING_CONTEXT(context),
				  dma->context);
			return -EINVAL;
		}
		q = dev->queuelist[DRM_KERNEL_CONTEXT];
		while_locked = 1;
	} else {
		q = dev->queuelist[dma->context];
	}


	atomic_inc(&q->use_count);
	if (atomic_read(&q->block_write)) {
		current->state = TASK_INTERRUPTIBLE;
		add_wait_queue(&q->write_queue, &entry);
		atomic_inc(&q->block_count);
		for (;;) {
			if (!atomic_read(&q->block_write)) break;
			schedule();
			if (signal_pending(current)) {
				atomic_dec(&q->use_count);
				return -EINTR;
			}
		}
		atomic_dec(&q->block_count);
		current->state = TASK_RUNNING;
		remove_wait_queue(&q->write_queue, &entry);
	}
	
	for (i = 0; i < dma->send_count; i++) {
		idx = dma->send_indices[i];
		if (idx < 0 || idx >= dev->buf_count) {
			atomic_dec(&q->use_count);
			DRM_ERROR("Index %d (of %d max)\n",
				  dma->send_indices[i], dev->buf_count - 1);
			return -EINVAL;
		}
		buf = dev->buflist[ idx ];
		if (buf->pid != current->pid) {
			atomic_dec(&q->use_count);
			DRM_ERROR("Process %d using buffer owned by %d\n",
				  current->pid, buf->pid);
			return -EINVAL;
		}
		if (buf->list != DRM_LIST_NONE) {
			atomic_dec(&q->use_count);
			DRM_ERROR("Process %d using buffer %d on list %d\n",
				  current->pid, buf->idx, buf->list);
		}
		buf->used         = dma->send_sizes[i];
		buf->while_locked = while_locked;
		buf->context      = dma->context;
		if (!buf->used) {
			DRM_ERROR("Queueing 0 length buffer\n");
		}
		if (buf->pending) {
			atomic_dec(&q->use_count);
			DRM_ERROR("Queueing pending buffer:"
				  " buffer %d, offset %d\n",
				  dma->send_indices[i], i);
			return -EINVAL;
		}
		if (buf->waiting) {
			atomic_dec(&q->use_count);
			DRM_ERROR("Queueing waiting buffer:"
				  " buffer %d, offset %d\n",
				  dma->send_indices[i], i);
			return -EINVAL;
		}
		buf->waiting = 1;
		if (atomic_read(&q->use_count) == 1
		    || atomic_read(&q->finalization)) {
			drm_free_buffer(dev, buf);
		} else {
			drm_waitlist_put(&q->waitlist, buf);
			atomic_inc(&q->total_queued);
		}
	}
	atomic_dec(&q->use_count);
	
	return 0;
}

static int drm_dma_priority(drm_device_t *dev, drm_dma_t *dma)
{
	unsigned long     address;
	unsigned long     length;
	int               must_free = 0;
	int               retcode   = 0;
	int               i;
	int               idx;
	drm_buf_t         *buf;
	drm_buf_t         *last_buf = NULL;
	DECLARE_WAITQUEUE(entry, current);

				/* Turn off interrupt handling */
	while (test_and_set_bit(0, &dev->interrupt_flag)) {
		schedule();
		if (signal_pending(current)) return -EINTR;
	}
	if (!(dma->flags & _DRM_DMA_WHILE_LOCKED)) {
		while (!drm_lock_take(&dev->lock.hw_lock->lock,
				      DRM_KERNEL_CONTEXT)) {
			schedule();
			if (signal_pending(current)) {
				clear_bit(0, &dev->interrupt_flag);
				return -EINTR;
			}
		}
		++must_free;
	}
	atomic_inc(&dev->total_prio);

	for (i = 0; i < dma->send_count; i++) {
		idx = dma->send_indices[i];
		if (idx < 0 || idx >= dev->buf_count) {
			DRM_ERROR("Index %d (of %d max)\n",
				  dma->send_indices[i], dev->buf_count - 1);
			continue;
		}
		buf = dev->buflist[ idx ];
		if (buf->pid != current->pid) {
			DRM_ERROR("Process %d using buffer owned by %d\n",
				  current->pid, buf->pid);
			retcode = -EINVAL;
			goto cleanup;
		}
		if (buf->list != DRM_LIST_NONE) {
			DRM_ERROR("Process %d using %d's buffer on list %d\n",
				  current->pid, buf->pid, buf->list);
			retcode = -EINVAL;
			goto cleanup;
		}
				/* This isn't a race condition on
                                   buf->list, since our concern is the
                                   buffer reclaim during the time the
                                   process closes the /dev/drm? handle, so
                                   it can't also be doing DMA. */
		buf->list         = DRM_LIST_PRIO;
		buf->used         = dma->send_sizes[i];
		buf->context      = dma->context;
		buf->while_locked = dma->flags & _DRM_DMA_WHILE_LOCKED;
		address           = (unsigned long)buf->address;
		length            = buf->used;
		if (!length) {
			DRM_ERROR("0 length buffer\n");
		}
		if (buf->pending) {
			DRM_ERROR("Sending pending buffer:"
				  " buffer %d, offset %d\n",
				  dma->send_indices[i], i);
			retcode = -EINVAL;
			goto cleanup;
		}
		if (buf->waiting) {
			DRM_ERROR("Sending waiting buffer:"
				  " buffer %d, offset %d\n",
				  dma->send_indices[i], i);
			retcode = -EINVAL;
			goto cleanup;
		}
		buf->pending = 1;
		
		if (dev->last_context != buf->context
		    && !(dev->queuelist[buf->context]->flags
			 & _DRM_CONTEXT_PRESERVED)) {
			add_wait_queue(&dev->context_wait, &entry);
			current->state = TASK_INTERRUPTIBLE;
				/* PRE: dev->last_context != buf->context */
			drm_context_switch(dev, dev->last_context,
					   buf->context);
				/* POST: we will wait for the context
                                   switch and will dispatch on a later call
                                   when dev->last_context == buf->context.
                                   NOTE WE HOLD THE LOCK THROUGHOUT THIS
                                   TIME! */
			schedule();
			current->state = TASK_RUNNING;
			remove_wait_queue(&dev->context_wait, &entry);
			if (signal_pending(current)) {
				retcode = -EINTR;
				goto cleanup;
			}
			if (dev->last_context != buf->context) {
				DRM_ERROR("Context mismatch: %d %d\n",
					  dev->last_context,
					  buf->context);
			}
		}

#if DRM_DMA_HISTOGRAM
		buf->time_queued     = get_cycles();
		buf->time_dispatched = buf->time_queued;
#endif
		if (dev->cmd[_DRM_DMA_DISPATCH].f) {
			retcode = dev
				->cmd[_DRM_DMA_DISPATCH].f(dev,
							   _DRM_DMA_DISPATCH,
							   address,
							   length);
		} else {
			retcode = drm_engine(dev, _DRM_DMA_DISPATCH,
					     address, length);
		}
		atomic_add(length, &dev->total_bytes);
		atomic_inc(&dev->total_dmas);
		
		if (last_buf) {
			drm_free_buffer(dev, last_buf);
		}
		last_buf = buf;
	}


cleanup:
	if (last_buf) {
		drm_do_command(dev, _DRM_DMA_READY);
		drm_free_buffer(dev, last_buf);
	}
	
	if (must_free && !dev->context_flag) {
		if (drm_lock_free(dev, &dev->lock.hw_lock->lock,
				  DRM_KERNEL_CONTEXT)) {
			DRM_ERROR("\n");
		}
	}
	clear_bit(0, &dev->interrupt_flag);
	return retcode;
}

static int drm_dma_send_buffers(drm_device_t *dev, drm_dma_t *dma)
{
	DECLARE_WAITQUEUE(entry, current);
	drm_buf_t         *last_buf = NULL;
	int               retcode   = 0;

	if (dma->flags & _DRM_DMA_BLOCK) {
		last_buf = dev->buflist[dma->send_indices[dma->send_count-1]];
		add_wait_queue(&last_buf->dma_wait, &entry);
	}
	
	if ((retcode = drm_dma_enqueue(dev, dma))) {
		if (dma->flags & _DRM_DMA_BLOCK)
			remove_wait_queue(&last_buf->dma_wait, &entry);
		return retcode;
	}
	
	drm_dma_schedule(dev, 0);
	
	if (dma->flags & _DRM_DMA_BLOCK) {
		DRM_DEBUG("%d waiting\n", current->pid);
		current->state = TASK_INTERRUPTIBLE;
		for (;;) {
			if (!last_buf->waiting
			    && !last_buf->pending)
				break; /* finished */
			schedule();
			if (signal_pending(current)) {
				/* We can't handle a restart of a
				   DMA request, since we have no
				   idea if it has finished or
				   not. */
				retcode = -EINTR;
				break;
			}
		}
		current->state = TASK_RUNNING;
		DRM_DEBUG("%d running\n", current->pid);
		remove_wait_queue(&last_buf->dma_wait, &entry);
		if (!retcode
		    || (last_buf->list==DRM_LIST_PEND && !last_buf->pending)) {
			if (!waitqueue_active(&last_buf->dma_wait)) {
				drm_free_buffer(dev, last_buf);
			}
		}
		if (retcode) {
			DRM_ERROR("ctx%d w%d p%d c%d i%d l%d %d/%d\n",
				  dma->context,
				  last_buf->waiting,
				  last_buf->pending,
				  DRM_WAITCOUNT(dev, dma->context),
				  last_buf->idx,
				  last_buf->list,
				  last_buf->pid,
				  current->pid);
		}
	}
	return retcode;
}

static int drm_dma_get_buffers_of_order(drm_device_t *dev, drm_dma_t *dma,
					int order)
{
	int               i;
	drm_buf_t         *buf;
	
	for (i = dma->granted_count; i < dma->request_count; i++) {
		buf = drm_freelist_get(&dev->bufs[order].freelist,
				       dma->flags & _DRM_DMA_WAIT);
		if (!buf) break;
		if (buf->pending || buf->waiting) {
			DRM_ERROR("Free buffer %d in use by %d (w%d, p%d)\n",
				  buf->idx,
				  buf->pid,
				  buf->waiting,
				  buf->pending);
		}
		buf->pid     = current->pid;
		copy_to_user_ret(&dma->request_indices[i],
				 &buf->idx,
				 sizeof(buf->idx),
				 -EFAULT);
		copy_to_user_ret(&dma->request_sizes[i],
				 &buf->total,
				 sizeof(buf->total),
				 -EFAULT);
		++dma->granted_count;
	}
	return 0;
}


static int drm_dma_get_buffers(drm_device_t *dev, drm_dma_t *dma)
{
	int               order;
	int               retcode = 0;
	int               tmp_order;
	
	order = drm_order(dma->request_size);

	dma->granted_count = 0;
	retcode            = drm_dma_get_buffers_of_order(dev, dma, order);

	if (dma->granted_count < dma->request_count
	    && (dma->flags & _DRM_DMA_SMALLER_OK)) {
		for (tmp_order = order - 1;
		     !retcode
			     && dma->granted_count < dma->request_count
			     && tmp_order >= DRM_MIN_ORDER;
		     --tmp_order) {
			
			retcode = drm_dma_get_buffers_of_order(dev, dma,
							       tmp_order);
		}
	}

	if (dma->granted_count < dma->request_count
	    && (dma->flags & _DRM_DMA_LARGER_OK)) {
		for (tmp_order = order + 1;
		     !retcode
			     && dma->granted_count < dma->request_count
			     && tmp_order <= DRM_MAX_ORDER;
		     ++tmp_order) {
			
			retcode = drm_dma_get_buffers_of_order(dev, dma,
							       tmp_order);
		}
	}
	return 0;
}

int drm_dma DRM_IOCTL_ARGS
{
	drm_file_t        *priv     = filp->private_data;
	drm_device_t      *dev      = priv->dev;
	int               retcode   = 0;
	drm_dma_t         dma;

	copy_from_user_ret(&dma, (drm_dma_t *)arg, sizeof(dma), -EFAULT);
	DRM_DEBUG("%d %d: %d send, %d req\n",
		  current->pid, dma.context, dma.send_count,
		  dma.request_count);

	if (dma.context == DRM_KERNEL_CONTEXT
	    || dma.context >= dev->queue_slots) {
		DRM_ERROR("Process %d using context %d\n",
			  current->pid, dma.context);
		return -EINVAL;
	}
	if (dma.send_count < 0 || dma.send_count > dev->buf_count) {
		DRM_ERROR("Process %d trying to send %d buffers (of %d max)\n",
			  current->pid, dma.send_count, dev->buf_count);
		return -EINVAL;
	}
	if (dma.request_count < 0 || dma.request_count > dev->buf_count) {
		DRM_ERROR("Process %d trying to get %d buffers (of %d max)\n",
			  current->pid, dma.request_count, dev->buf_count);
		return -EINVAL;
	}

	if (dma.send_count) {
		if (dma.flags & _DRM_DMA_PRIORITY)
			retcode = drm_dma_priority(dev, &dma);
		else 
			retcode = drm_dma_send_buffers(dev, &dma);
	}

	dma.granted_count = 0;

	if (!retcode && dma.request_count) {
		retcode = drm_dma_get_buffers(dev, &dma);
	}

	DRM_DEBUG("%d returning, granted = %d\n",
		  current->pid, dma.granted_count);
	copy_to_user_ret((drm_dma_t *)arg, &dma, sizeof(dma), -EFAULT);

	return retcode;
}
