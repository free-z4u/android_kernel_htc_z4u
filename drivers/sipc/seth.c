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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/ipv6.h>
#include <linux/ip.h>
#include <asm/byteorder.h>
#include <linux/tty.h>
#include <linux/platform_device.h>

#include <linux/sipc.h>
#include <linux/seth.h>


#define SETH_BLOCK_NUM	64
#define SETH_BLOCK_SIZE	(ETH_HLEN + ETH_DATA_LEN + NET_IP_ALIGN)

#define DEV_ON 1
#define DEV_OFF 0

/*
 * Device instance data.
 */
typedef struct SEth {
	struct net_device_stats stats;	/* net statistics */
	struct net_device* netdev;	/* Linux net device */
	struct seth_init_data* pdata;	/* platform data */
	struct workqueue_struct*	net_wq;
	struct work_struct	net_work;
	struct sk_buff_head	txhead;
	struct mutex		txlock;
	int state;			/* device state */
	int stopped;			/* sblock indicator */
} SEth;

static struct SEth *seths[SIPC_ID_NR][SMSG_CH_NR]={{NULL}};
/*
 * Tx_ready handler.
 */
static void
seth_tx_ready_handler(void* data)
{
	SEth* seth = (SEth*) data;

	LOGI("seth_tx_ready_handler state %0x\n", seth->state);
	if (seth->state != DEV_ON) {
		seth->state = DEV_ON;
		if (!netif_carrier_ok (seth->netdev)) {
			netif_carrier_on (seth->netdev);
		}
	}
	else {
		seth->state = DEV_OFF;
		if (!netif_carrier_ok (seth->netdev)) {
		netif_carrier_off (seth->netdev);
		/*
		netif_carrier_on (seth->netdev);
		*/
		}
	}
}

static void
seth_rx_handler(void* data)
{
	SEth* seth = (SEth *)data;
	struct seth_init_data *pdata = seth->pdata;
	struct sblock blk;
	struct sk_buff* skb;
	int ret;
	/*int i;*/

	if (seth->state != DEV_ON) {
		LOGE ("rx_handler the state of %s is off!\n", seth->netdev->name);
		seth->stats.rx_errors++;
		return;
	}

	ret = seth_sblock_receive(pdata->dst, pdata->channel, &blk, 0);
	if (ret) {
		LOGE ("receive sblock failed (%d)\n", ret);
		seth->stats.rx_errors++;
		return;
	}
	
	skb = dev_alloc_skb(blk.length + NET_IP_ALIGN); //16 bytes align
	if (!skb) {
		LOGE ("alloc skbuff failed!\n");
		seth->stats.rx_dropped++;
		return;
	}

	skb_reserve(skb, NET_IP_ALIGN);

	memcpy(skb->data, blk.addr, blk.length);

	skb_put (skb, blk.length);

	skb->dev = seth->netdev;
	skb->protocol  = eth_type_trans (skb, seth->netdev);
	skb->ip_summed = CHECKSUM_UNNECESSARY;

	seth->stats.rx_packets++;
	seth->stats.rx_bytes += skb->len;

	netif_rx_ni (skb);

	seth->netdev->last_rx = jiffies;

	ret = sblock_release(pdata->dst, pdata->channel, &blk);
	if (ret) {
		LOGE ("release sblock failed (%d)\n", ret);
	}
}

static void
seth_handler (int event, void* data)
{
	SEth *seth = (SEth *)data;

	switch(event) {
		case SBLOCK_NOTIFY_GET:
			LOGD ("SBLOCK_NOTIFY_GET is received\n");
			break;
		case SBLOCK_NOTIFY_RECV:
			LOGD ("SBLOCK_NOTIFY_RECV is received\n");
			seth_rx_handler(seth);
			break;
		case SBLOCK_NOTIFY_STATUS:
			LOGD ("SBLOCK_NOTIFY_STATUS is received\n");
			seth_tx_ready_handler(seth);
			break;
		default:
			LOGE ("Received event is invalid(event=%d)\n", event);
	}
}

/* use workqueue to xmit data */
static void seth_net_tx_work(struct work_struct * work)
{
	SEth* seth	= container_of(work, struct SEth, net_work);
	struct seth_init_data *pdata = seth->pdata;
	struct sblock blk;
	struct net_device* dev;
	struct sk_buff *skb = NULL;
	int ret;

	mutex_lock(&seth->txlock);
	dev = seth->netdev;
	while ((skb = skb_dequeue(&seth->txhead)) != NULL){
		if (seth->state != DEV_ON) {
			LOGE ("xmit the state of %s is off\n", dev->name);
			//netif_stop_queue (dev);
			netif_carrier_off (dev);
			seth->stats.tx_carrier_errors++;
			dev_kfree_skb_any (skb);
			goto fail;
		}
		/*
		 * Get a free sblock.
		 */

		ret = sblock_get(pdata->dst, pdata->channel, &blk, 0xFFFFFFFF);
		if(ret) {
			LOGE ("Get free sblock failed(%d)\n", ret);
			/* can't stop queue, otherwise it will cause net can't continue to transfer */
			//netif_stop_queue (dev);
			seth->stats.tx_fifo_errors++;
			seth->stopped = 1;
			dev_kfree_skb_any (skb);
			goto fail;
		}

		if(blk.length < skb->len) {
			LOGE ("The size of sblock is so tiny!\n");
			sblock_put(pdata->dst, pdata->channel, &blk);
			seth->stats.tx_fifo_errors++;
			dev_kfree_skb_any (skb);
			goto fail;
		}

		blk.length = skb->len;
		memcpy (blk.addr, skb->data, skb->len);

		ret = seth_sblock_send(pdata->dst, pdata->channel, &blk);
		if(ret) {
			LOGE ("send sblock failed(%d)\n", ret);
			sblock_put(pdata->dst, pdata->channel, &blk);
			seth->stats.tx_fifo_errors++;
			dev_kfree_skb_any (skb);
			goto fail;
		}
		/*
		 * Statistics.
		 */
		seth->stats.tx_bytes += skb->len;
		seth->stats.tx_packets++;
		seth->netdev->trans_start = jiffies;
		dev_kfree_skb_any (skb);
	}

	mutex_unlock(&seth->txlock);
	return;

fail:
	while ((skb = skb_dequeue(&seth->txhead)) != NULL){
		LOGE ("Need release all skb\b");
		dev_kfree_skb_any(skb);
	}

	mutex_unlock(&seth->txlock);
	return;
}

/*
 * Transmit interface
 */
static int
seth_start_xmit (struct sk_buff* skb, struct net_device* dev)
{
	SEth* seth    = netdev_priv (dev);

	if (seth->state != DEV_ON) {
		LOGE ("xmit the state of %s is off\n", dev->name);
		//netif_stop_queue (dev);
		netif_carrier_off (dev);
		seth->stats.tx_carrier_errors++;
		dev_kfree_skb_any (skb);
		return NETDEV_TX_OK;
	}

	if (seth->net_wq != NULL) {
		skb_queue_tail(&seth->txhead, skb);
		queue_work(seth->net_wq, &seth->net_work);
	} else {
		LOGE ("net workqueue has been removed!\n");
		dev_kfree_skb_any(skb);
		return NETDEV_TX_BUSY;
	}

	return NETDEV_TX_OK;
}

int seth_reset(uint8_t dst, uint8_t channel)
{
	SEth* seth = seths[dst][channel];
	struct sk_buff *skb = NULL;

	if (seth == NULL){
		LOGE("dst=%d channel=%d fail to find device\n", dst, channel);
		return -1;
	}

	LOGD("dst=%d channel=%d +++\n", dst, channel);
	seth->state = DEV_OFF;
	if (!netif_carrier_ok (seth->netdev)) {
		netif_carrier_off (seth->netdev);
	}

	if (seth->net_wq != NULL) {
		flush_workqueue(seth->net_wq);
		cancel_work_sync(&seth->net_work);
		destroy_workqueue(seth->net_wq);
		seth->net_wq = NULL;
	}

	while ((skb = skb_dequeue(&seth->txhead)) != NULL){
		LOGE ("Need release all skb if reset\n");
		dev_kfree_skb_any(skb);
	}

	LOGD("dst=%d channel=%d ---\n", dst, channel);

	return 0;
}

int seth_restart(uint8_t dst, uint8_t channel)
{
	SEth* seth = seths[dst][channel];
	struct net_device* dev;

	if (seth == NULL){
		LOGE("dst=%d channel=%d fail to find device\n", dst, channel);
		return -1;
	}

	LOGD("dst=%d channel=%d +++\n", dst, channel);

	dev = seth->netdev;
	if(seth->net_wq == NULL){
		seth->net_wq = create_singlethread_workqueue(dev->name);
		INIT_WORK(&seth->net_work, seth_net_tx_work);
	}

	seth->state = DEV_OFF;
	/* set link as disconnected */
	if (!netif_carrier_ok (seth->netdev)) {
		netif_carrier_off (seth->netdev);
	}else{
		LOGE("test carrier is on, this symptom shouldn't exist\n");
	}

	LOGD("dst=%d channel=%d ---\n", dst, channel);

	return 0;
}
/*
 * Open interface
 */
static int seth_open (struct net_device *dev)
{
	SEth* seth = netdev_priv(dev);

	/* Reset stats */
	memset(&seth->stats, 0, sizeof(seth->stats));
	
	/*
	seth->state = DEV_ON;
	*/

	netif_start_queue(dev);

	return 0;
}

/*
 * Close interface
 */
static int seth_close (struct net_device *dev)
{
	/*SEth* seth = netdev_priv(dev);*/

	netif_stop_queue(dev);

	/*
	seth->state = DEV_OFF;
	*/

	return 0;
}

static struct net_device_stats * seth_get_stats(struct net_device *dev)
{
	SEth * seth = netdev_priv(dev);
	return &(seth->stats);
}

static void seth_tx_timeout(struct net_device *dev)
{
	LOGI ("seth_tx_timeout()\n");
	netif_wake_queue(dev);
}

/*
 * Cleanup Ethernet device driver.
 */
static int __devexit seth_remove (struct platform_device *pdev)
{
	struct SEth* seth = platform_get_drvdata(pdev);
	struct seth_init_data *pdata = seth->pdata;

	if (seth->net_wq != NULL) {
		cancel_work_sync(&seth->net_work);
		destroy_workqueue(seth->net_wq);
		seth->net_wq = NULL;
	}

	sblock_destroy(pdata->dst, pdata->channel);

	unregister_netdev(seth->netdev);
	free_netdev(seth->netdev);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct net_device_ops seth_ops = {
	.ndo_open = seth_open,
	.ndo_stop = seth_close,
	.ndo_start_xmit = seth_start_xmit,
	.ndo_get_stats = seth_get_stats,
	.ndo_tx_timeout = seth_tx_timeout,
};

static int __devinit seth_probe(struct platform_device *pdev)
{
	struct seth_init_data *pdata = pdev->dev.platform_data;
	struct net_device* netdev;
	SEth* seth;
	char ifname[IFNAMSIZ];
	int ret;

	if(pdata->name[0])
		strlcpy(ifname, pdata->name, IFNAMSIZ);
	else
		strcpy(ifname, "veth%d");

	netdev = alloc_netdev (sizeof (SEth), ifname, ether_setup);
	if (!netdev) {
		LOGE ("alloc_netdev() failed.\n");
		return -ENOMEM;
	}
	seth = netdev_priv (netdev);
	seth->pdata = pdata;
	seth->netdev = netdev;
	seth->state = DEV_OFF;
	seth->stopped = 0;

	netdev->netdev_ops = &seth_ops;
	netdev->watchdog_timeo = 100*HZ;
	netdev->irq = 0;
	netdev->dma = 0;

	random_ether_addr(netdev->dev_addr);

	seths[pdata->dst][pdata->channel] = seth;
	ret = sblock_create(pdata->dst, pdata->channel,
		SETH_BLOCK_NUM, SETH_BLOCK_SIZE,
		SETH_BLOCK_NUM, SETH_BLOCK_SIZE);
	if (ret) {
		LOGE ("create sblock failed (%d)\n", ret);
		free_netdev(netdev);
		return ret;
	}

	ret = sblock_register_notifier(pdata->dst, pdata->channel, seth_handler, seth);
	if (ret) {
		LOGE ("regitster notifier failed (%d)\n", ret);
		free_netdev(netdev);
		sblock_destroy(pdata->dst, pdata->channel);
		return ret;
	}

	/* register new Ethernet interface */
	if ((ret = register_netdev (netdev))) {
		LOGE ("register_netdev() failed (%d)\n", ret);
		free_netdev(netdev);
		sblock_destroy(pdata->dst, pdata->channel);
		return ret;
	}

	mutex_init(&(seth->txlock));
	skb_queue_head_init(&seth->txhead);

	seth->net_wq = create_singlethread_workqueue(netdev->name);
	INIT_WORK(&seth->net_work, seth_net_tx_work);

	/* set link as disconnected */
	netif_carrier_off (netdev);

	platform_set_drvdata(pdev, seth);

	return 0;
}

static struct platform_driver seth_driver = {
	.probe = seth_probe,
	.remove = __devexit_p(seth_remove),
	.driver = {
		.owner = THIS_MODULE,
		.name = "seth",
	},
};

static int __init seth_init(void)
{
	return platform_driver_register(&seth_driver);
}

static void __exit seth_exit(void)
{
	platform_driver_unregister(&seth_driver);
}

EXPORT_SYMBOL(seth_reset);
EXPORT_SYMBOL(seth_restart);

module_init (seth_init);
module_exit (seth_exit);

MODULE_DESCRIPTION ("Spreadtrum Ethernet device driver");
MODULE_LICENSE ("GPL");


