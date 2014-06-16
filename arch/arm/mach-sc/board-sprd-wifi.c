/* linux/arch/arm/mach-sc/board-sprd-wifi.c
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/skbuff.h>
#include <linux/wlan_plat.h>
#include <mach/irqs.h>

//#include "pins-db8500.h"
#include "board-sprd-wifi.h"
#include <mach/board.h>
//#include <linux/clk.h>

int sprd_wifi_power(int on);
int sprd_wifi_reset(int on);
int sprd_wifi_set_carddetect(int on);
int sprd_wifi_get_mac_addr(unsigned char *buf);
void wlan_clk_init(void);

#define PREALLOC_WLAN_NUMBER_OF_SECTIONS	4
#define PREALLOC_WLAN_NUMBER_OF_BUFFERS		160
#define PREALLOC_WLAN_SECTION_HEADER		24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 1024)

#define WLAN_SKB_BUF_NUM	16

#define HW_OOB 1

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

typedef struct wifi_mem_prealloc_struct {
	void *mem_ptr;
	unsigned long size;
} wifi_mem_prealloc_t;

static wifi_mem_prealloc_t wifi_mem_array[PREALLOC_WLAN_NUMBER_OF_SECTIONS] = {
	{ NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER) }
};

static void *sprd_wifi_mem_prealloc(int section, unsigned long size)
{
	printk("[WLAN] wifi: prealloc buffer index: %d\n", section);
	if (section == PREALLOC_WLAN_NUMBER_OF_SECTIONS)
		return wlan_static_skb;
	if ((section < 0) || (section > PREALLOC_WLAN_NUMBER_OF_SECTIONS))
		return NULL;
	if (wifi_mem_array[section].size < size)
		return NULL;
	return wifi_mem_array[section].mem_ptr;
}

int __init sprd_init_wifi_mem(void)
{
	int i;

	for (i = 0; (i < WLAN_SKB_BUF_NUM); i++) {
		if (i < (WLAN_SKB_BUF_NUM/2))
			wlan_static_skb[i] = dev_alloc_skb(PAGE_SIZE*2);
		else
			wlan_static_skb[i] = dev_alloc_skb(PAGE_SIZE*4);
	}
	for (i = 0; (i < PREALLOC_WLAN_NUMBER_OF_SECTIONS); i++) {
		wifi_mem_array[i].mem_ptr = kmalloc(wifi_mem_array[i].size,
							GFP_KERNEL);
		if (wifi_mem_array[i].mem_ptr == NULL)
			return -ENOMEM;
	}
	return 0;
}

static struct resource sprd_wifi_resources[] = {
	[0] = {
		.name		= "bcmdhd_wlan_irq",
		//.start		= gpio_to_irq(SPRD_WIFI_IRQ_GPIO),
		//.end		= gpio_to_irq(SPRD_WIFI_IRQ_GPIO),
#ifdef HW_OOB
		.flags          = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
#else
		.flags          = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
#endif
	},
};

static struct wifi_platform_data sprd_wifi_control = {
	.set_power      = sprd_wifi_power,
	.set_reset      = sprd_wifi_reset,
	.set_carddetect = sprd_wifi_set_carddetect,
	.mem_prealloc   = sprd_wifi_mem_prealloc,
	.get_mac_addr	= sprd_wifi_get_mac_addr,
	//.dot11n_enable  = 1,
};

static struct platform_device sprd_wifi_device = {
	.name           = "bcmdhd_wlan",
	.id             = 1,
	.num_resources  = ARRAY_SIZE(sprd_wifi_resources),
	.resource       = sprd_wifi_resources,
	.dev            = {
		.platform_data = &sprd_wifi_control,
	},
};

#if 0
static unsigned sprd_wifi_update_nvs(char *str)
{
#define NVS_LEN_OFFSET		0x0C
#define NVS_DATA_OFFSET		0x40
	unsigned char *ptr;
	unsigned len;

	if (!str)
		return -EINVAL;
	ptr = get_wifi_nvs_ram();
	/* Size in format LE assumed */
	memcpy(&len, ptr + NVS_LEN_OFFSET, sizeof(len));

	/* the last bye in NVRAM is 0, trim it */
	if (ptr[NVS_DATA_OFFSET + len - 1] == 0)
		len -= 1;

	if (ptr[NVS_DATA_OFFSET + len -1] != '\n') {
		len += 1;
		ptr[NVS_DATA_OFFSET + len -1] = '\n';
	}

	strcpy(ptr + NVS_DATA_OFFSET + len, str);
	len += strlen(str);
	memcpy(ptr + NVS_LEN_OFFSET, &len, sizeof(len));
	return 0;
}
#endif

#if 0
static void wlan_clk_init(void)
{
	struct clk *wlan_clk;
	struct clk *clk_parent;
/*8825ea/openphone use aux0 garda use aux1*/
#ifdef CONFIG_MACH_GARDA
	wlan_clk = clk_get(NULL, "clk_aux1");
	if (IS_ERR(wlan_clk)) {
		printk("clock: failed to get clk_aux1\n");
	}
#else
	wlan_clk = clk_get(NULL, "clk_aux0");
	if (IS_ERR(wlan_clk)) {
		printk("clock: failed to get clk_aux0\n");
	}
#endif
	clk_parent = clk_get(NULL, "ext_32k");
	if (IS_ERR(clk_parent)) {
		printk("failed to get parent ext_32k\n");
	}

	clk_set_parent(wlan_clk, clk_parent);
	//clk_set_rate(wlan_clk, 32000);
	clk_enable(wlan_clk);
}
#endif

#if 0
#ifdef HW_OOB
static unsigned strip_nvs_param(char* param)
{
        unsigned char *nvs_data;

        unsigned param_len;
        int start_idx, end_idx;

        unsigned char *ptr;
        unsigned len;

        if (!param)
                return -EINVAL;

        ptr = get_wifi_nvs_ram();
        /* Size in format LE assumed */
        memcpy(&len, ptr + NVS_LEN_OFFSET, sizeof(len));

        /* the last bye in NVRAM is 0, trim it */
        if (ptr[NVS_DATA_OFFSET + len -1] == 0)
                len -= 1;

        nvs_data = ptr + NVS_DATA_OFFSET;

        param_len = strlen(param);

        /* search param */
        for (start_idx = 0; start_idx < len - param_len; start_idx++) {
                if (memcmp(&nvs_data[start_idx], param, param_len) == 0) {
                        break;
                }
        }

        end_idx = 0;
        if (start_idx < len - param_len) {
                /* search end-of-line */
                for (end_idx = start_idx + param_len; end_idx < len; end_idx++) {
                        if (nvs_data[end_idx] == '\n' || nvs_data[end_idx] == 0) {
                                break;
                        }
                }
        }

        if (start_idx < end_idx) {
                /* move the remain data forward */
                for (; end_idx + 1 < len; start_idx++, end_idx++) {
                        nvs_data[start_idx] = nvs_data[end_idx+1];
                }
                len = len - (end_idx - start_idx + 1);
                memcpy(ptr + NVS_LEN_OFFSET, &len, sizeof(len));
        }
        return 0;
}
#endif
#endif

#define WIFI_MAC_PARAM_STR     "macaddr="
#define WIFI_MAX_MAC_LEN       17 /* XX:XX:XX:XX:XX:XX */

#if 0
static uint
get_mac_from_wifi_nvs_ram(char* buf, unsigned int buf_len)
{
	unsigned char *nvs_ptr;
	unsigned char *mac_ptr;
	uint len = 0;

	if (!buf || !buf_len) {
		return 0;
	}

	nvs_ptr = get_wifi_nvs_ram();
	if (nvs_ptr) {
		nvs_ptr += NVS_DATA_OFFSET;
	}

	mac_ptr = strstr(nvs_ptr, WIFI_MAC_PARAM_STR);
	if (mac_ptr) {
		mac_ptr += strlen(WIFI_MAC_PARAM_STR);

		/* skip vigoring space */
		while (mac_ptr[0] == ' ') {
			mac_ptr++;
		}

		/* locate end-of-line */
		len = 0;
		while (mac_ptr[len] != '\r' && mac_ptr[len] != '\n' &&
			mac_ptr[len] != '\0') {
			len++;
		}

		if (len > buf_len) {
			len = buf_len;
		}
		memcpy(buf, mac_ptr, len);
	}

	return len;
}
#endif

#define ETHER_ADDR_LEN 6
int sprd_wifi_get_mac_addr(unsigned char *buf)
{
	static u8 ether_mac_addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0xFF};
	//char mac[WIFI_MAX_MAC_LEN];
	//unsigned mac_len;
	//unsigned int macpattern[ETHER_ADDR_LEN];
	//int i;
#if 0	//liushoubin temp remove
	mac_len = get_mac_from_wifi_nvs_ram(mac, WIFI_MAX_MAC_LEN);
	if (mac_len > 0) {
		//Mac address to pattern
		sscanf( mac, "%02x:%02x:%02x:%02x:%02x:%02x",
		&macpattern[0], &macpattern[1], &macpattern[2],
		&macpattern[3], &macpattern[4], &macpattern[5]
		);

		for(i = 0; i < ETHER_ADDR_LEN; i++) {
			ether_mac_addr[i] = (u8)macpattern[i];
		}
	}
#endif
	memcpy(buf, ether_mac_addr, sizeof(ether_mac_addr));

	printk("sprd_wifi_get_mac_addr = %02x %02x %02x %02x %02x %02x \n",
		ether_mac_addr[0],ether_mac_addr[1],ether_mac_addr[2],ether_mac_addr[3],ether_mac_addr[4],ether_mac_addr[5]);

	return 0;
}

//we will adjust wifi on/off gpio config later
static int __init sprd_wifi_init(void)
{
	int ret = 0;

	printk(KERN_INFO "[WLAN] %s: start\n", __func__);

#ifdef HW_OOB
	//strip_nvs_param("sd_oobonly"); //liushoubin temp remove
#else
	sprd_wifi_update_nvs("sd_oobonly=1\n");
#endif
	//sprd_wifi_update_nvs("btc_params80=0\n"); //liushoubin temp remove
	//sprd_wifi_update_nvs("btc_params6=30\n"); //liushoubin temp remove
	sprd_init_wifi_mem();

#if defined(CONFIG_MACH_Z4DTG) || defined(CONFIG_MACH_Z4TD)
	wlan_clk_init();
#endif

#if 1//def CONFIG_MACH_SP8825EA_TIGER_EVM
        gpio_request(SPRD_WIFI_IRQ_GPIO, "oob_irq");
	gpio_direction_input(SPRD_WIFI_IRQ_GPIO);
	gpio_free(SPRD_WIFI_IRQ_GPIO);

	sprd_wifi_resources[0].start = gpio_to_irq(SPRD_WIFI_IRQ_GPIO);
	sprd_wifi_resources[0].end = gpio_to_irq(SPRD_WIFI_IRQ_GPIO);

	gpio_request(SPRD_WIFI_PMENA_GPIO,"wifi_pwd");
	gpio_direction_output(SPRD_WIFI_PMENA_GPIO, 0);
	gpio_free(SPRD_WIFI_PMENA_GPIO);
	
	ret = platform_device_register(&sprd_wifi_device);
#endif

	printk(KERN_INFO "[WLAN] %s: end\n", __func__);

	return ret;
}

late_initcall(sprd_wifi_init);

//MODULE_DESCRIPTION("Broadcomm wlan driver");
//MODULE_LICENSE("GPL");

