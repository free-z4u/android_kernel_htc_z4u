/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <asm/uaccess.h>

#include <linux/sipc.h>
#include "sbuf.h"

#ifdef CONFIG_MODEM_SILENT_RESET
#define TEST_SBUF_RESET (sbuf->state == SBUF_STATE_RESET)
#define SBUF_INIT_STATE SBUF_STATE_RESET
#else
#define TEST_SBUF_RESET 0
#define SBUF_INIT_STATE SBUF_STATE_IDLE
#endif

static struct sbuf_mgr *sbufs[SIPC_ID_NR][SMSG_CH_NR];

static int sbuf_thread(void *data)
{
	struct sbuf_mgr *sbuf = data;
	struct smsg mcmd, mrecv;
	int rval, bufid;

restart:
	rval = wait_event_interruptible(sbuf->thread_wait, !TEST_SBUF_RESET);

	LOGI("dst=%d channel=%d sbuf state is %d\n",
			sbuf->dst, sbuf->channel, sbuf->state);

	/* since the channel open may hang, we call it in the sbuf thread */
	rval = smsg_ch_open(sbuf->dst, sbuf->channel, -1);
	if (rval != 0) {
		LOGE("dst=%d channel=%d failed to open\n", sbuf->dst, sbuf->channel);
		if(TEST_SBUF_RESET || rval == -ERESTARTSYS)
			goto restart;
		else
			return rval;
	}

	/* sbuf init done, handle the ring rx events */
	while (!kthread_should_stop()) {
		if(TEST_SBUF_RESET){
			LOGI("sbuf is under reset status\n");
			smsg_ch_close(sbuf->dst, sbuf->channel, 0);
			goto restart;
		}
		/* monitor sbuf rdptr/wrptr update smsg */
		smsg_set(&mrecv, sbuf->channel, 0, 0, 0);
		smsg_recv(sbuf->dst, &mrecv, -1);

		LOGD("dst=%d channel=%d recv msg type=%d flag=0x%04x value=0x%08x\n",
				sbuf->dst, sbuf->channel,
				mrecv.type, mrecv.flag, mrecv.value);

		switch (mrecv.type) {
		case SMSG_TYPE_OPEN:
			/* handle channel recovery */
			LOGI("dst=%d channel=%d recv open msg\n", sbuf->dst, sbuf->channel);
			smsg_open_ack(sbuf->dst, sbuf->channel);
			break;
		case SMSG_TYPE_CLOSE:
			/* handle channel recovery */
			LOGI("dst=%d channel=%d recv close msg\n", sbuf->dst, sbuf->channel);
			smsg_close_ack(sbuf->dst, sbuf->channel);
			/*need to add one mutex to avoid conflict with reset*/
			mutex_lock(&sbuf->statelock);
			if(!TEST_SBUF_RESET){
				sbuf->state = SBUF_STATE_IDLE;
			}
			mutex_unlock(&sbuf->statelock);
			break;
		case SMSG_TYPE_CMD:
			/* respond cmd done for sbuf init */
			LOGI("dst=%d channel=%d recv command msg\n",sbuf->dst, sbuf->channel);
			WARN_ON(mrecv.flag != SMSG_CMD_SBUF_INIT);
			smsg_set(&mcmd, sbuf->channel, SMSG_TYPE_DONE,
					SMSG_DONE_SBUF_INIT, sbuf->smem_addr);
			smsg_send(sbuf->dst, &mcmd, -1);
			LOGI("dst=%d channel=%d set sbuf state ready\n", sbuf->dst, sbuf->channel);
			/*need to add one mutex to avoid conflict with reset*/
			mutex_lock(&sbuf->statelock);
			if(!TEST_SBUF_RESET){
				sbuf->state = SBUF_STATE_READY;
			}
			mutex_unlock(&sbuf->statelock);

			break;
		case SMSG_TYPE_EVENT:
			bufid = mrecv.value;
			WARN_ON(bufid >= sbuf->ringnr);
			switch (mrecv.flag) {
			case SMSG_EVENT_SBUF_RDPTR:
				wake_up_interruptible_all(&(sbuf->rings[bufid].txwait));
				break;
			case SMSG_EVENT_SBUF_WRPTR:
				wake_up_interruptible_all(&(sbuf->rings[bufid].rxwait));
				break;
			default:
				rval = 1;
				break;
			}
			break;
		default:
			rval = 1;
			break;
		};
		sipc_rx_try_wake_unlock(sbuf->dst);
		if (rval) {
			LOGW("non-handled sbuf msg: %d-%d, %d, %d, %d\n",
					sbuf->dst, sbuf->channel,
					mrecv.type, mrecv.flag, mrecv.value);
			rval = 0;
		}
	}

	return 0;
}

int sbuf_create(uint8_t dst, uint8_t channel, uint32_t bufnum,
		uint32_t txbufsize, uint32_t rxbufsize)
{
	struct sbuf_mgr *sbuf;
	volatile struct sbuf_smem_header *smem;
	volatile struct sbuf_ring_header *ringhd;
	int hsize, i;

	sbuf = kzalloc(sizeof(struct sbuf_mgr), GFP_KERNEL);
	if (!sbuf) {
		LOGE("Failed to allocate mgr for sbuf\n");
		return -ENOMEM;
	}

	sbuf->state = SBUF_INIT_STATE;
	sbuf->dst = dst;
	sbuf->channel = channel;
	sbuf->ringnr = bufnum;

	/* allocate smem */
	hsize = sizeof(struct sbuf_smem_header) + sizeof(struct sbuf_ring_header) * bufnum;
	sbuf->smem_size = hsize + (txbufsize + rxbufsize) * bufnum;
	sbuf->smem_addr = smem_alloc(sbuf->smem_size);
	if (!sbuf->smem_addr) {
		LOGE("Failed to allocate smem for sbuf\n");
		kfree(sbuf);
		return -ENOMEM;
	}
	sbuf->smem_virt = ioremap(sbuf->smem_addr, sbuf->smem_size);
	if (!sbuf->smem_virt) {
		LOGE("Failed to map smem for sbuf\n");
		smem_free(sbuf->smem_addr, sbuf->smem_size);
		kfree(sbuf);
		return -EFAULT;
	}

	/* allocate rings description */
	sbuf->rings = kzalloc(sizeof(struct sbuf_ring) * bufnum, GFP_KERNEL);
	if (!sbuf->rings) {
		LOGE("Failed to allocate rings for sbuf\n");
		iounmap(sbuf->smem_virt);
		smem_free(sbuf->smem_addr, sbuf->smem_size);
		kfree(sbuf);
		return -ENOMEM;
	}

	/* initialize all ring bufs */
	smem = (volatile struct sbuf_smem_header *)sbuf->smem_virt;
	smem->ringnr = bufnum;
	for (i = 0; i < bufnum; i++) {
		ringhd = (volatile struct sbuf_ring_header *)&(smem->headers[i]);
		ringhd->txbuf_addr = sbuf->smem_addr + hsize +
				(txbufsize + rxbufsize) * i;
		ringhd->txbuf_size = txbufsize;
		ringhd->txbuf_rdptr = 0;
		ringhd->txbuf_wrptr = 0;
		ringhd->rxbuf_addr = smem->headers[i].txbuf_addr + txbufsize;
		ringhd->rxbuf_size = rxbufsize;
		ringhd->rxbuf_rdptr = 0;
		ringhd->rxbuf_wrptr = 0;

		sbuf->rings[i].header = ringhd;
		sbuf->rings[i].txbuf_virt = sbuf->smem_virt + hsize +
				(txbufsize + rxbufsize) * i;
		sbuf->rings[i].rxbuf_virt = sbuf->rings[i].txbuf_virt + txbufsize;
		init_waitqueue_head(&(sbuf->rings[i].txwait));
		init_waitqueue_head(&(sbuf->rings[i].rxwait));
		mutex_init(&(sbuf->rings[i].txlock));
		mutex_init(&(sbuf->rings[i].rxlock));
	}

	init_waitqueue_head(&(sbuf->thread_wait));
	mutex_init(&(sbuf->statelock));

	sbuf->thread = kthread_create(sbuf_thread, sbuf,
			"sbuf-%d-%d", dst, channel);
	if (IS_ERR(sbuf->thread)) {
		LOGE("Failed to create kthread: sbuf-%d-%d\n", dst, channel);
		kfree(sbuf->rings);
		iounmap(sbuf->smem_virt);
		smem_free(sbuf->smem_addr, sbuf->smem_size);
		kfree(sbuf);
		return PTR_ERR(sbuf->thread);
	}

	sbufs[dst][channel] = sbuf;
	wake_up_process(sbuf->thread);

	return 0;
}

void sbuf_destroy(uint8_t dst, uint8_t channel)
{
	struct sbuf_mgr *sbuf = sbufs[dst][channel];

	sbuf->state = SBUF_STATE_IDLE;
	kthread_stop(sbuf->thread);

	kfree(sbuf->rings);
	iounmap(sbuf->smem_virt);
	smem_free(sbuf->smem_addr, sbuf->smem_size);
	kfree(sbuf);

	sbufs[dst][channel] = NULL;
}

int sbuf_write(uint8_t dst, uint8_t channel, uint32_t bufid,
		void *buf, uint32_t len, int timeout)
{
	struct sbuf_mgr *sbuf = sbufs[dst][channel];
	struct sbuf_ring *ring;
	volatile struct sbuf_ring_header *ringhd;
	struct smsg mevt;
	void *txpos;
	int rval, left, tail, txsize, i;

	if (!sbuf) {
		LOGE("%s Fail to find device\n", __func__);
		return -ENODEV;
	}

	if (sbuf->state != SBUF_STATE_READY) {
		LOGE("dst=%d channel=%d ring=%d not ready to write!\n",
				dst, channel, bufid);
		return -ENODEV;
	}

	ring = &(sbuf->rings[bufid]);
	ringhd = ring->header;

	LOGD("dst=%d, channel=%d, bufid=%d, len=%d, timeout=%d\n",
			dst, channel, bufid, len, timeout);
	LOGD("wrptr=%d, rdptr=%d", ringhd->txbuf_wrptr, ringhd->txbuf_rdptr);

	rval = 0;
	left = len;

	if (timeout) {
		mutex_lock(&ring->txlock);
	} else {
		if (!mutex_trylock(&(ring->txlock))) {
			LOGI("dst=%d channel=%d busy!\n", dst, channel);
			return -EBUSY;
		}
	}

	if (timeout == 0) {
		/* no wait */
		if ((int)(ringhd->txbuf_wrptr - ringhd->txbuf_rdptr) >=
				ringhd->txbuf_size) {
			LOGW("dst=%d channel=%d ring=%d txbuf is full!\n",
				dst, channel, bufid);
			rval = -EBUSY;
		}
	} else if (timeout < 0) {
		/* wait forever */
		rval = wait_event_interruptible(ring->txwait,
			((int)(ringhd->txbuf_wrptr - ringhd->txbuf_rdptr) <
			ringhd->txbuf_size) || TEST_SBUF_RESET);
		if (rval < 0) {
			LOGW("dst=%d channel=%d wait interrupted!\n", dst, channel);
		}
	} else {
		/* wait timeout */
		rval = wait_event_interruptible_timeout(ring->txwait,
			((int)(ringhd->txbuf_wrptr - ringhd->txbuf_rdptr) <
			ringhd->txbuf_size) || TEST_SBUF_RESET, timeout);
		if (rval < 0) {
			LOGW("dst=%d channel=%d wait interrupted!\n", dst, channel);
		} else if (rval == 0) {
			LOGW("dst=%d channel=%d wait timeout!\n", dst, channel);
			rval = -ETIME;
		}
	}

	while (left && ((int)(ringhd->txbuf_wrptr - ringhd->txbuf_rdptr) < ringhd->txbuf_size) ) {
		if (TEST_SBUF_RESET) {
			LOGI("dst=%d channel=%d is under reset state, quit!\n", dst, channel);
			break;
		}
		/* calc txpos & txsize */
		txpos = ring->txbuf_virt + ringhd->txbuf_wrptr % ringhd->txbuf_size;
		txsize = ringhd->txbuf_size - (int)(ringhd->txbuf_wrptr - ringhd->txbuf_rdptr);
		txsize = min(txsize, left);

		tail = txpos + txsize - (ring->txbuf_virt + ringhd->txbuf_size);
		if (tail > 0) {
			/* ring buffer is rounded */
			if ((uint32_t)buf > TASK_SIZE) {
				memcpy(txpos, buf, txsize - tail);
				memcpy(ring->txbuf_virt, buf + txsize - tail, tail);
			} else {
				if(copy_from_user(txpos, (void __user *)buf, txsize - tail) ||
				    copy_from_user(ring->txbuf_virt,
				    (void __user *)(buf + txsize - tail), tail)) {
					LOGE("dst=%d channel=%d failed to copy from user!\n",
							 dst, channel);
					rval = -EFAULT;
					break;
				}
			}
		} else {
			if ((uint32_t)buf > TASK_SIZE) {
				memcpy(txpos, buf, txsize);
			} else {
				/* handle the user space address */
				if(copy_from_user(txpos, (void __user *)buf, txsize)) {
					LOGE("dst=%d channel=%d failed to copy from user!\n",
							dst, channel);
					rval = -EFAULT;
					break;
				}
			}
		}


		LOGD("txpos=%p, txsize=%d\n", txpos, txsize);

		/* update tx wrptr */
		ringhd->txbuf_wrptr = ringhd->txbuf_wrptr + txsize;
		if (channel == 5) {
			LOGI("dst=%d slog tx data: ", dst);
			for (i = 0; i < txsize; i++)
				printk("0x%x ", *((char *)txpos + i));
			printk("\n");
			LOGI("dst=%d txbuf_wrptr=0x%x txbuf_rdptr=0x%x\n",
					dst, ringhd->txbuf_wrptr, ringhd->txbuf_rdptr);
		}
		smsg_set(&mevt, channel, SMSG_TYPE_EVENT, SMSG_EVENT_SBUF_WRPTR, bufid);
		smsg_send(dst, &mevt, -1);

		left -= txsize;
		buf += txsize;
	}

	/*maybe the below need to be removed!*/
	if (TEST_SBUF_RESET) {
		LOGI("dst=%d channel=%d sbuf is under reset state, clear tx pointer\n", dst, channel);
		ringhd->txbuf_wrptr = ringhd->txbuf_rdptr = 0;
		rval = -EFAULT;
	}
	mutex_unlock(&ring->txlock);

	LOGD("dst=%d channel=%d len=%d\n", len - left, dst, channel);

	if(rval < 0){
		LOGI("return error code %d dst=%d channel=%d\n",
				rval, dst, channel);
		return rval;
	}

	if (len == left) {
		return rval;
	} else {
		return (len - left);
	}
}

int sbuf_read(uint8_t dst, uint8_t channel, uint32_t bufid,
		void *buf, uint32_t len, int timeout)
{
	struct sbuf_mgr *sbuf = sbufs[dst][channel];
	struct sbuf_ring *ring;
	volatile struct sbuf_ring_header *ringhd;
	struct smsg mevt;
	void *rxpos;
	int rval, left, tail, rxsize;

	if (!sbuf) {
		LOGE("dst=%d channel=%d fail to find device\n", dst, channel);
		return -ENODEV;
	}

	if (sbuf->state != SBUF_STATE_READY) {
		LOGE("dst=%d channel=%d ring=%d not ready to read!\n",
				dst, channel, bufid);
		return -ENODEV;
	}

	ring = &(sbuf->rings[bufid]);
	ringhd = ring->header;

	LOGD("dst=%d channel=%d ring=%d len=%d timeout=%d\n",
			dst, channel, bufid, len, timeout);
	LOGD("wrptr=%d rdptr=%d", ringhd->rxbuf_wrptr, ringhd->rxbuf_rdptr);

	rval = 0;
	left = len;

	if (timeout) {
		mutex_lock(&ring->rxlock);
	} else {
		if (!mutex_trylock(&(ring->rxlock))) {
			LOGI("dst=%d channel=%d busy!\n", dst, channel);
			return -EBUSY;
		}
	}

	if (ringhd->rxbuf_wrptr == ringhd->rxbuf_rdptr) {
		if (timeout == 0) {
			/* no wait */
			LOGW("dst=%d channel=%d ring=%d rxbuf is empty!\n",
					dst, channel, bufid);
			rval = -ENODATA;
		} else if (timeout < 0) {
			/* wait forever */
			rval = wait_event_interruptible(ring->rxwait,
				(ringhd->rxbuf_wrptr != ringhd->rxbuf_rdptr) || TEST_SBUF_RESET);
			if (rval < 0) {
				LOGW("dst=%d channel=%d wait interrupted!\n",
						dst, channel);
			}
		} else {
			/* wait timeout */
			rval = wait_event_interruptible_timeout(ring->rxwait,
				((ringhd->rxbuf_wrptr != ringhd->rxbuf_rdptr) || TEST_SBUF_RESET), timeout);
			if (rval < 0) {
				LOGW("dst=%d channel=%d wait interrupted!\n",
					dst, channel);
			} else if (rval == 0) {
				LOGW("dst=%d channel=%d wait timeout!\n",
					dst, channel);
				rval = -ETIME;
			}
		}
	}

	while (left && (ringhd->rxbuf_wrptr != ringhd->rxbuf_rdptr)) {
		if (TEST_SBUF_RESET) {
			LOGI("sbuf is under reset state, quit!\n");
			break;
		}

		/* calc rxpos & rxsize */
		rxpos = ring->rxbuf_virt + ringhd->rxbuf_rdptr % ringhd->rxbuf_size;
		rxsize = (int)(ringhd->rxbuf_wrptr - ringhd->rxbuf_rdptr);
		/* check overrun */
		WARN_ON(rxsize > ringhd->rxbuf_size);
		rxsize = min(rxsize, left);

		LOGD("buf=%p, rxpos=%p, rxsize=%d\n", buf, rxpos, rxsize);

		tail = rxpos + rxsize - (ring->rxbuf_virt + ringhd->rxbuf_size);
		if (tail > 0) {
			/* ring buffer is rounded */
			if ((uint32_t)buf > TASK_SIZE) {
				memcpy(buf, rxpos, rxsize - tail);
				memcpy(buf + rxsize - tail, ring->rxbuf_virt, tail);
			} else {
				/* handle the user space address */
				if(copy_to_user((void __user *)buf, rxpos, rxsize - tail) ||
				    copy_to_user((void __user *)(buf + rxsize - tail),
				    ring->rxbuf_virt, tail)) {
					LOGE("dst=%d channel=%d failed to copy to user!\n",
							dst, channel);
					rval = -EFAULT;
					break;
				}
			}
		} else {
			if ((uint32_t)buf > TASK_SIZE) {
				memcpy(buf, rxpos, rxsize);
			} else {
				/* handle the user space address */
				if (copy_to_user((void __user *)buf, rxpos, rxsize)) {
					LOGE("dst=%d channel=%d failed to copy to user!\n",
							dst, channel);
					rval = -EFAULT;
					break;
				}
			}
		}

		/* update rx rdptr */
		ringhd->rxbuf_rdptr = ringhd->rxbuf_rdptr + rxsize;
		smsg_set(&mevt, channel, SMSG_TYPE_EVENT, SMSG_EVENT_SBUF_RDPTR, bufid);
		smsg_send(dst, &mevt, -1);

		left -= rxsize;
		buf += rxsize;
	}
       /*maybe the below need to be removed!*/
	if (TEST_SBUF_RESET) {
		LOGI("sbuf is under reset state, clear rx pointer\n");
		ringhd->rxbuf_wrptr = ringhd->rxbuf_rdptr = 0;
		rval = -EFAULT;
	}
	mutex_unlock(&ring->rxlock);

	LOGD("dst=%d channel=%d len=%d\n", len - left, dst, channel);

	if(rval < 0){
		LOGI("return error code 0x%x dst=%d channel=%d\n",
				rval, dst, channel);
		return rval;
	}

	if (len == left) {
		return rval;
	} else {
		return (len - left);
	}
}

int sbuf_poll_wait(uint8_t dst, uint8_t channel, uint32_t bufid,
		struct file *filp, poll_table *wait)
{
	struct sbuf_mgr *sbuf = sbufs[dst][channel];
	struct sbuf_ring *ring;
	volatile struct sbuf_ring_header *ringhd;
	unsigned int mask = 0;

	if (!sbuf) {
		LOGE("dst=%d channel=%d fail to find device\n", dst, channel);
		goto poll_wait_err;
	}

	if (sbuf->state != SBUF_STATE_READY) {
		LOGE("dst=%d channel=%d not ready to poll !\n", dst, channel);
		goto poll_wait_err;
	}

	ring = &(sbuf->rings[bufid]);
	ringhd = ring->header;

	poll_wait(filp, &ring->txwait, wait);
	poll_wait(filp, &ring->rxwait, wait);

	if (ringhd->rxbuf_wrptr != ringhd->rxbuf_rdptr) {
		mask |= POLLIN | POLLRDNORM;
	}

	if (ringhd->txbuf_wrptr - ringhd->txbuf_rdptr < ringhd->txbuf_size) {
		mask |= POLLOUT | POLLWRNORM;
	}

poll_wait_err:
	return mask;
}

int sbuf_status(uint8_t dst, uint8_t channel)
{
	struct sbuf_mgr *sbuf = sbufs[dst][channel];

	if (!sbuf) {
		return -ENODEV;
	}

	if (sbuf->state != SBUF_STATE_READY) {
		return -ENODEV;
	}

	return 0;
}

int sbuf_has_data(uint8_t dst, uint8_t channel, uint32_t bufid)
{
	struct sbuf_mgr *sbuf = sbufs[dst][channel];
	struct sbuf_ring *ring;
	volatile struct sbuf_ring_header *ringhd;

	if (!sbuf) {
		LOGE("dst=%d channel=%d fail to find device\n", dst, channel);
		return -ENODEV;
	}

	if (sbuf->state != SBUF_STATE_READY) {
		LOGE("dst=%d channel=%d ring=%d not ready to read!\n",
				dst, channel, bufid);
		return -ENODEV;
	}

	ring = &(sbuf->rings[bufid]);
	ringhd = ring->header;
	if (!mutex_trylock(&(ring->rxlock))) {
		LOGI("dst=%d channel=%d busy!\n", dst, channel);
		return 1;
	} else {
		if (ringhd->rxbuf_wrptr != ringhd->rxbuf_rdptr) {
			LOGI("dst=%d channel=%d has data!\n", dst, channel);
			mutex_unlock(&(ring->rxlock));
			return 1;
		}
		mutex_unlock(&(ring->rxlock));
	}

	return 0;
}

int sbuf_reset(uint8_t dst, uint8_t channel)
{
	int i = 0;
	struct sbuf_mgr *sbuf = sbufs[dst][channel];

	if (!sbuf) {
		LOGE("dst=%d channel=%d fail to find device\n", dst, channel);
		return -ENODEV;
	}

	LOGD("dst=%d channel=%d +++\n", dst, channel);

	//mutex for thread
	mutex_lock(&(sbuf->statelock));
	sbuf->state = SBUF_STATE_RESET;
	mutex_unlock(&(sbuf->statelock));

	smsg_reset(dst, channel);

	for (i = 0; i < sbuf->ringnr; i++){
		struct sbuf_ring *ring = &(sbuf->rings[i]);
		volatile struct sbuf_ring_header *ringhd = ring->header;

		wake_up_interruptible_all(&(ring->rxwait));
		mutex_lock(&ring->rxlock);
		ringhd->rxbuf_wrptr = ringhd->rxbuf_rdptr = 0;
		mutex_unlock(&ring->rxlock);

		wake_up_interruptible_all(&(ring->txwait));
		mutex_lock(&ring->txlock);
		ringhd->txbuf_wrptr = ringhd->txbuf_rdptr = 0;
		mutex_unlock(&ring->txlock);
	}

	LOGD("dst=%d channel=%d ---\n", dst, channel);

	return 0;
}

int sbuf_restart(uint8_t dst, uint8_t channel)
{
	struct sbuf_mgr *sbuf = sbufs[dst][channel];

	if (!sbuf) {
		LOGE("%s fail to find device\n", __func__);
		return -ENODEV;
	}

	LOGD("dst=%d channel=%d +++\n", dst, channel);

	sbuf->state = SBUF_STATE_IDLE;
	smsg_restart(dst, channel);
	wake_up_interruptible(&sbuf->thread_wait);

	LOGD("dst=%d channel=%d ---\n", dst, channel);

	return 0;
}


EXPORT_SYMBOL(sbuf_create);
EXPORT_SYMBOL(sbuf_destroy);
EXPORT_SYMBOL(sbuf_write);
EXPORT_SYMBOL(sbuf_read);
EXPORT_SYMBOL(sbuf_poll_wait);
EXPORT_SYMBOL(sbuf_status);
EXPORT_SYMBOL(sbuf_has_data);
EXPORT_SYMBOL(sbuf_reset);
EXPORT_SYMBOL(sbuf_restart);

MODULE_AUTHOR("Chen Gaopeng");
MODULE_DESCRIPTION("SIPC/SBUF driver");
MODULE_LICENSE("GPL");
