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
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/timer.h>
#include <linux/rtc.h>

#include <linux/sipc.h>
#include <linux/sipc_priv.h>

#ifdef CONFIG_MODEM_SILENT_RESET
#define TEST_SMSG_RESET (ipc->smsg_state[msg->channel] == SMSG_STATE_RESET)
#else
#define TEST_SMSG_RESET 0
#endif

extern int sipc_suspend;

static struct smsg_ipc *smsg_ipcs[SIPC_ID_NR];

static void smsg_rx_work(struct work_struct * work)
{
	struct smsg_ipc *ipc;
	struct smsg *msg;
	struct smsg_channel *ch;
	uint32_t rxpos, wr;
	struct timespec ts;
	struct rtc_time tm;

	ipc = container_of(work, struct smsg_ipc, smsg_work);

	if (sipc_suspend) {
		getnstimeofday(&ts);
		rtc_time_to_tm(ts.tv_sec, &tm);
		LOGI("at %d-%02d-%02d %02d:%02d:%02d.%09lu UTC\n",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
	}

	mutex_lock(&(ipc->rxlock));

	while (readl(ipc->rxbuf_wrptr) != readl(ipc->rxbuf_rdptr)) {
		rxpos = (readl(ipc->rxbuf_rdptr) & (ipc->rxbuf_size - 1)) *
			sizeof (struct smsg) + ipc->rxbuf_addr;
		msg = (struct smsg *)rxpos;

		LOGD("irq get smsg: dst=%d wrptr=%d, rdptr=%d, rxpos=0x%08x\n",
			ipc->dst, readl(ipc->rxbuf_wrptr), readl(ipc->rxbuf_rdptr), rxpos);
		if (sipc_suspend || msg->type == SMSG_TYPE_OPEN || msg->type == SMSG_TYPE_CMD || msg->type == SMSG_TYPE_CLOSE) {
			LOGI("irq read smsg: dst=%d channel=%d, type=%d, flag=0x%04x, value=0x%08x\n",
				ipc->dst, msg->channel, msg->type, msg->flag, msg->value);
			sipc_suspend = 0;
		}

		if (msg->channel >= SMSG_CH_NR || msg->channel <= SMSG_CH_RPC_CP ||msg->type >= SMSG_TYPE_NR) {
			/* invalid msg */
			LOGE("invalid smsg: dst=%d channel=%d, type=%d, flag=0x%04x, value=0x%08x\n",
				ipc->dst, msg->channel, msg->type, msg->flag, msg->value);

			/* update smsg rdptr */
			writel(readl(ipc->rxbuf_rdptr) + 1, ipc->rxbuf_rdptr);

			continue;
		}

		mutex_lock(&(ipc->chlock));
		ch = ipc->channels[msg->channel];
		if (!ch) {
			if (ipc->states[msg->channel] == CHAN_STATE_UNUSED &&
					msg->type == SMSG_TYPE_OPEN &&
					msg->flag == SMSG_OPEN_MAGIC) {

				ipc->states[msg->channel] = CHAN_STATE_WAITING;
			} else {
				/* drop this bad msg since channel is not opened */
				LOGI("smsg dst=%d channel %d not opened! "
					"drop smsg: type=%d, flag=0x%04x, value=0x%08x\n",
					ipc->dst, msg->channel, msg->type, msg->flag, msg->value);
			}

			/* update smsg rdptr */
			writel(readl(ipc->rxbuf_rdptr) + 1, ipc->rxbuf_rdptr);

			mutex_unlock(&(ipc->chlock));
			continue;
		}
		mutex_unlock(&(ipc->chlock));

		if ((int)(readl(ch->wrptr) - readl(ch->rdptr)) >= SMSG_CACHE_NR) {
			/* msg cache is full, drop this msg */
			LOGI("smsg dst=%d channel %d recv cache is full! "
				"drop smsg: type=%d, flag=0x%04x, value=0x%08x\n",
				ipc->dst, msg->channel, msg->type, msg->flag, msg->value);
		} else {
			/* write smsg to cache */
			sipc_rx_try_wake_lock(ipc->dst);
			wr = readl(ch->wrptr) & (SMSG_CACHE_NR - 1);
			memcpy(&(ch->caches[wr]), msg, sizeof(struct smsg));
			writel(readl(ch->wrptr) + 1, ch->wrptr);
		}

		/* update smsg rdptr */
		writel(readl(ipc->rxbuf_rdptr) + 1, ipc->rxbuf_rdptr);

		wake_up_interruptible_all(&(ch->rxwait));
	}

	mutex_unlock(&(ipc->rxlock));
}

irqreturn_t smsg_irq_handler(int irq, void *dev_id)
{
	struct smsg_ipc *ipc = (struct smsg_ipc *)dev_id;

	if (ipc->rxirq_status())
		ipc->rxirq_clear();

	if (ipc->smsg_wq)
		queue_work(ipc->smsg_wq, &ipc->smsg_work);

	return IRQ_HANDLED;
}

int smsg_ipc_create(uint8_t dst, struct smsg_ipc *ipc)
{
	int rval, i;

	if (!ipc->irq_handler) {
		ipc->irq_handler = smsg_irq_handler;
	}

	spin_lock_init(&(ipc->txspinlock));

	mutex_init(&(ipc->rxlock));
	mutex_init(&(ipc->chlock));
	smsg_ipcs[dst] = ipc;

	ipc->smsg_wq = create_singlethread_workqueue(ipc->name);
	INIT_WORK(&ipc->smsg_work, smsg_rx_work);
	wake_lock_init(&ipc->rx_wakelock, WAKE_LOCK_SUSPEND,
			kasprintf(GFP_KERNEL, "%s-smsg-rx-lock", ipc->name));
	ipc->rx_count = 0;
	spin_lock_init(&ipc->rx_count_plock);

#if 0
	/*
	 * Due to boot modem in os, no irq will be missed.
	 */
	ipc->irq_handler(ipc->irq, ipc);
#endif

	/* register IPI irq */
	rval = request_irq(ipc->irq, ipc->irq_handler,
			IRQF_NO_SUSPEND, ipc->name, ipc);
	if (rval != 0) {
		LOGE("Failed to request irq %s: %d\n",
				ipc->name, ipc->irq);
		return rval;
	}

	init_waitqueue_head(&(ipc->smsg_wait));

	for (i = 0; i < SMSG_CH_NR; i++){
		ipc->smsg_state[i] = SMSG_STATE_IDLE;
	}
	return 0;
}

int smsg_ipc_destroy(uint8_t dst)
{
	struct smsg_ipc *ipc = smsg_ipcs[dst];

	kthread_stop(ipc->thread);
	free_irq(ipc->irq, ipc);
	smsg_ipcs[dst] = NULL;

	return 0;
}

/* ****************************************************************** */

int smsg_ch_open(uint8_t dst, uint8_t channel, int timeout)
{
	struct smsg_ipc *ipc = smsg_ipcs[dst];
	struct smsg_channel *ch;
	struct smsg mopen, mrecv;
	uint32_t rval = 0;

	LOGI("dst=%d channel=%d +++\n", dst, channel);

	ch = kzalloc(sizeof(struct smsg_channel), GFP_KERNEL);
	if (!ch) {
		return -ENOMEM;
	}

	init_waitqueue_head(&(ch->rxwait));
	mutex_init(&(ch->rxlock));

	mutex_lock(&(ipc->chlock));
	ipc->channels[channel] = ch;
	mutex_unlock(&(ipc->chlock));

	smsg_set(&mopen, channel, SMSG_TYPE_OPEN, SMSG_OPEN_MAGIC, 0);
	rval = smsg_send(dst, &mopen, timeout);
	if (rval != 0) {
		goto open_failed;
	}

	/* open msg might be got before */
	if (ipc->states[channel] == CHAN_STATE_WAITING) {
		goto open_done;
	}

	smsg_set(&mrecv, channel, 0, 0, 0);
	rval = smsg_recv(dst, &mrecv, timeout);
	if (rval != 0) {
		sipc_rx_try_wake_unlock(dst);
		goto open_failed;
	}
	sipc_rx_try_wake_unlock(dst);

	if (mrecv.type != SMSG_TYPE_OPEN || mrecv.flag != SMSG_OPEN_MAGIC) {
		LOGE("Got bad open msg on channel %d-%d\n",
				dst, channel);
		rval = -EIO;
		goto open_failed;
	}

open_done:
	LOGI("dst=%d channel=%d ---\n", dst, channel);
	ipc->states[channel] = CHAN_STATE_OPENED;
	return 0;

open_failed:
	mutex_lock(&(ipc->chlock));
	kfree(ch);
	ipc->channels[channel] = NULL;
	mutex_unlock(&(ipc->chlock));
	return rval;
}

int smsg_ch_close(uint8_t dst, uint8_t channel,  int timeout)
{
	struct smsg_ipc *ipc = smsg_ipcs[dst];
	struct smsg mclose;

	if (!ipc) {
		LOGI("dst=%d channel=%d ipc isn't created!\n", dst, channel);
		return -ENODEV;
	}
	smsg_set(&mclose, channel, SMSG_TYPE_CLOSE, SMSG_CLOSE_MAGIC, 0);
	smsg_send(dst, &mclose, timeout);

	mutex_lock(&(ipc->chlock));
	kfree(ipc->channels[channel]);
	ipc->channels[channel] = NULL;
	mutex_unlock(&(ipc->chlock));

	return 0;
}

int smsg_send(uint8_t dst, struct smsg *msg, int timeout)
{
	struct smsg_ipc *ipc = smsg_ipcs[dst];
	unsigned long flags;
	uint32_t txpos;
	int rval = 0;

	if (!ipc->channels[msg->channel]) {
		LOGI("dst %d channel %d not inited!\n", dst, msg->channel);
		return -ENODEV;
	}

	if (ipc->states[msg->channel] != CHAN_STATE_OPENED &&
		msg->type != SMSG_TYPE_OPEN) {
		LOGI("dst %d  channel %d not opened!\n", dst, msg->channel);
		return -EINVAL;
	}

	if (TEST_SMSG_RESET) {
		LOGW("smsg is under reset state, can not send!\n");
		return -EINVAL;
	}

	LOGD("dst=%d, channel=%d, timeout=%d\n",
			dst, msg->channel, timeout);
	LOGD("dst=%d channel=%d, type=%d, flag=0x%04x, value=0x%08x\n",
			dst, msg->channel, msg->type, msg->flag, msg->value);

	spin_lock_irqsave(&(ipc->txspinlock), flags);

	if(TEST_SMSG_RESET) {
		LOGW("smsg is under reset state!\n");
		rval = -EINVAL;
		goto send_failed;
	}

	if ((int)(readl(ipc->txbuf_wrptr) -
				readl(ipc->txbuf_rdptr)) >= ipc->txbuf_size) {
		LOGW("smsg txbuf is full!\n");
		rval = -EBUSY;
		goto send_failed;
	}

	/* calc txpos and write smsg */
	txpos = (readl(ipc->txbuf_wrptr) & (ipc->txbuf_size - 1)) *
	sizeof(struct smsg) + ipc->txbuf_addr;
	memcpy((void *)txpos, msg, sizeof(struct smsg));

	LOGD("write smsg: wrptr=%d, rdptr=%d, txpos=0x%08x\n",
			readl(ipc->txbuf_wrptr),
			readl(ipc->txbuf_rdptr), txpos);

	/* update wrptr */
	writel(readl(ipc->txbuf_wrptr) + 1, ipc->txbuf_wrptr);
	ipc->txirq_trigger();

send_failed:
	spin_unlock_irqrestore(&(ipc->txspinlock), flags);
	return rval;
}

int smsg_recv(uint8_t dst, struct smsg *msg, int timeout)
{
	struct smsg_ipc *ipc = smsg_ipcs[dst];
	struct smsg_channel *ch = ipc->channels[msg->channel];
	uint32_t rd;
	int rval = 0;

	if (!ch) {
		LOGI("dst %d channel %d not opened!\n", dst, msg->channel);
		return -ENODEV;
	}

	LOGD("dst=%d, channel=%d, timeout=%d\n", dst, msg->channel, timeout);

	if (timeout == 0) {
		if (!mutex_trylock(&(ch->rxlock))) {
			LOGI("dst=%d channel=%d recv smsg busy!\n", dst, msg->channel);
			return -EBUSY;
		}

		/* no wait */
		if (readl(ch->wrptr) == readl(ch->rdptr)) {
			LOGW("dst=%d channel=%d smsg rx cache is empty!\n", dst, msg->channel);
			rval = -ENODATA;

			goto recv_failed;
		}
	} else if (timeout < 0) {
		mutex_lock(&(ch->rxlock));

		/* wait forever */
		rval = wait_event_interruptible(ch->rxwait,
				(readl(ch->wrptr) != readl(ch->rdptr)) || TEST_SMSG_RESET );
		if (rval < 0) {
			LOGW("dst=%d channel=%d wait interrupted!\n", dst, msg->channel);

			goto recv_failed;
		}

	} else {
		mutex_lock(&(ch->rxlock));

		/* wait timeout */
		rval = wait_event_interruptible_timeout(ch->rxwait,
			(readl(ch->wrptr) != readl(ch->rdptr)|| TEST_SMSG_RESET), timeout);
		if (rval < 0) {
			LOGW("dst=%d channel=%d wait interrupted!\n", dst, msg->channel);

			goto recv_failed;
		} else if (rval == 0) {
			LOGW("dst=%d channel=%d wait timeout!\n", dst, msg->channel);
			rval = -ETIME;

			goto recv_failed;
		}
	}

	if (TEST_SMSG_RESET){
		LOGW("current channel has been release!\n");
		goto recv_failed;
	}

	/* read smsg from cache */
	rd = readl(ch->rdptr) & (SMSG_CACHE_NR - 1);
	memcpy(msg, &(ch->caches[rd]), sizeof(struct smsg));
	writel(readl(ch->rdptr) + 1, ch->rdptr);

	LOGD("wrptr=%d, rdptr=%d, rd=%d\n", readl(ch->wrptr), readl(ch->rdptr), rd);
	LOGD("dst=%d channel=%d, type=%d, flag=0x%04x, value=0x%08x\n",
			dst, msg->channel, msg->type, msg->flag, msg->value);

recv_failed:
	//maybe it need to remove
	if(TEST_SMSG_RESET){
		writel(0, ch->wrptr);
		writel(0, ch->rdptr);
	}
	mutex_unlock(&(ch->rxlock));

	return rval;
}


int smsg_reset(uint8_t dst, uint8_t channel)
{
	struct smsg_ipc *ipc = smsg_ipcs[dst];
	struct smsg_channel *ch = NULL;
	unsigned long flags;
	int rval = 0;

	if (!ipc) {
		LOGI("dst=%d channel=%d ipc isn't created!\n", dst, channel);
		rval = -ENODEV;
		goto end;
	}

	LOGD("dst=%d channel=%d +++\n", dst, channel);

	if (ipc->smsg_wq != NULL) {
		flush_workqueue(ipc->smsg_wq);
		cancel_work_sync(&ipc->smsg_work);
		destroy_workqueue(ipc->smsg_wq);
		ipc->smsg_wq = NULL;
	}

	ipc->smsg_state[channel] = SMSG_STATE_RESET;

	wake_up_interruptible_all(&(ipc->smsg_wait));

	mutex_lock(&(ipc->chlock));
	ch = ipc->channels[channel];
	if (!ch) {
		LOGI("dst=%d channel=%d channel has been free!\n", dst, channel);
	} else {
		wake_up_interruptible_all(&(ch->rxwait));
		mutex_lock(&(ch->rxlock));
		writel(0, ch->wrptr);
		writel(0, ch->rdptr);
		mutex_unlock(&(ch->rxlock));
	}
	mutex_unlock(&(ipc->chlock));

	spin_lock_irqsave(&(ipc->txspinlock), flags);
	writel(0, ipc->txbuf_wrptr);
	writel(0, ipc->txbuf_rdptr);
	spin_unlock_irqrestore(&(ipc->txspinlock), flags);

	mutex_lock(&(ipc->rxlock));
	writel(0, ipc->rxbuf_wrptr);
	writel(0, ipc->rxbuf_rdptr);
	mutex_unlock(&(ipc->rxlock));

end:
	LOGD("dst=%d channel=%d ---\n", dst, channel);

	ipc->states[channel] = CHAN_STATE_UNUSED;
	return rval;
}

int smsg_restart(uint8_t dst, uint8_t channel)
{
	struct smsg_ipc *ipc = smsg_ipcs[dst];

	LOGD("dst=%d +++\n", dst);

	ipc->smsg_state[channel] = SMSG_STATE_IDLE;
	if (!ipc->smsg_wq) {
		ipc->smsg_wq = create_singlethread_workqueue(ipc->name);
		INIT_WORK(&ipc->smsg_work, smsg_rx_work);
	}

	LOGD("dst=%d ---\n", dst);

	return 0;
}

int sipc_enable_irq_wake(uint8_t dst)
{
	struct smsg_ipc *ipc = smsg_ipcs[dst];

	enable_irq_wake(ipc->irq);

	return 0;
}

int sipc_disable_irq_wake(uint8_t dst)
{
	struct smsg_ipc *ipc = smsg_ipcs[dst];

	disable_irq_wake(ipc->irq);

	return 0;
}

int sipc_rx_try_wake_lock(uint8_t dst)
{
	struct smsg_ipc *ipc = smsg_ipcs[dst];
	unsigned long flags;

	if (ipc->use_wakelock) {
		spin_lock_irqsave(&ipc->rx_count_plock, flags);
		if (ipc->rx_count == 0) {
			wake_lock(&ipc->rx_wakelock);
		}
		++ipc->rx_count;
		spin_unlock_irqrestore(&ipc->rx_count_plock, flags);
	}

	return 0;
}

int sipc_rx_try_wake_unlock(uint8_t dst)
{
	struct smsg_ipc *ipc = smsg_ipcs[dst];
	unsigned long flags;

	if (ipc->use_wakelock) {
		spin_lock_irqsave(&ipc->rx_count_plock, flags);
		if (ipc->rx_count) {
			--ipc->rx_count;
			if (ipc->rx_count == 0)
				wake_unlock(&ipc->rx_wakelock);
		} else {
			LOGE("invalid rx count\n");
		}
		spin_unlock_irqrestore(&ipc->rx_count_plock, flags);
	}

	return 0;
}

int sipc_rx_wake_lock_active(uint8_t dst)
{
	struct smsg_ipc *ipc = smsg_ipcs[dst];
	int has_lock;

	if (wake_lock_active(&ipc->rx_wakelock))
		has_lock = 1;
	else
		has_lock = 0;

	return has_lock;
}

EXPORT_SYMBOL(smsg_ch_open);
EXPORT_SYMBOL(smsg_ch_close);
EXPORT_SYMBOL(smsg_send);
EXPORT_SYMBOL(smsg_recv);
EXPORT_SYMBOL(smsg_reset);
EXPORT_SYMBOL(smsg_restart);
EXPORT_SYMBOL(sipc_enable_irq_wake);
EXPORT_SYMBOL(sipc_disable_irq_wake);
EXPORT_SYMBOL(sipc_rx_try_wake_lock);
EXPORT_SYMBOL(sipc_rx_try_wake_unlock);
EXPORT_SYMBOL(sipc_rx_wake_lock_active);


MODULE_AUTHOR("Chen Gaopeng");
MODULE_DESCRIPTION("SIPC/SMSG driver");
MODULE_LICENSE("GPL");
