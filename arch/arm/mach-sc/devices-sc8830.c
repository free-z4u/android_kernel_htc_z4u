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
#include <linux/platform_device.h>
#include <linux/ion.h>
#include <linux/input.h>
#include <linux/input/matrix_keypad.h>
#include <linux/mmc/sdhci.h>
#include <linux/gpio.h>
#include <linux/persistent_ram.h>

#include <linux/sprd_cproc.h>
#include <linux/sipc.h>
#include <linux/spipe.h>
#include <linux/spool.h>
#include <linux/seth.h>
#include <linux/sprd_thm.h>

#include <sound/saudio.h>

#include <asm/pmu.h>

#include <mach/hardware.h>
#include <mach/sci_glb_regs.h>
#include <mach/irqs.h>
#include <mach/dma.h>
#include <mach/board.h>
#include <mach/kpd.h>
#include "devices.h"


//HTC_WIFI_START
#include <linux/delay.h>
#include <linux/mmc/card.h>  
#include <asm/mach/mmc.h>
#include "board-sprd-wifi.h"
#include <mach/pinmap.h> //NOT FOR SC8830
#include <mach/hardware.h>
#include <linux/clk.h>
#include <linux/module.h>
//#include <linux/regulator/consumer.h>

static struct resource sprd_serial_resources0[] = {
	[0] = {
		.start = SPRD_UART0_BASE,
		.end = SPRD_UART0_BASE + SPRD_UART0_SIZE-1,
		.name = "serial_res",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SER0_INT,
		.end = IRQ_SER0_INT,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device sprd_serial_device0 = {
	.name           = "serial_sprd",
	.id             =  0,
	.num_resources  = ARRAY_SIZE(sprd_serial_resources0),
	.resource       = sprd_serial_resources0,
};

static struct resource sprd_serial_resources1[] = {
	[0] = {
		.start = SPRD_UART1_BASE,
		.end = SPRD_UART1_BASE + SPRD_UART1_SIZE-1,
		.name = "serial_res",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SER1_INT,
		.end = IRQ_SER1_INT,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device sprd_serial_device1 = {
	.name           = "serial_sprd",
	.id             =  1,
	.num_resources  = ARRAY_SIZE(sprd_serial_resources1),
	.resource       = sprd_serial_resources1,
};

static struct resource sprd_serial_resources2[] = {
	[0] = {
		.start = SPRD_UART2_BASE,
		.end = SPRD_UART2_BASE + SPRD_UART2_SIZE - 1,
		.name = "serial_res",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SER2_INT,
		.end = IRQ_SER2_INT,
		.flags = IORESOURCE_IRQ,
	}
};
struct platform_device sprd_serial_device2 = {
	.name           = "serial_sprd",
	.id             =  2,
	.num_resources  = ARRAY_SIZE(sprd_serial_resources2),
	.resource       = sprd_serial_resources2,
};

static struct resource sprd_serial_resources3[] = {
	[0] = {
		.start = SPRD_UART3_BASE,
		.end = SPRD_UART3_BASE + SPRD_UART3_SIZE - 1,
		.name = "serial_res",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SER3_INT,
		.end = IRQ_SER3_INT,
		.flags = IORESOURCE_IRQ,
	}
};
struct platform_device sprd_serial_device3 = {
	.name           = "serial_sprd",
	.id             =  3,
	.num_resources  = ARRAY_SIZE(sprd_serial_resources3),
	.resource       = sprd_serial_resources3,
};

static struct resource sprd_serial_resources4[] = {
	[0] = {
		.start = SPRD_UART4_BASE,
		.end = SPRD_UART4_BASE + SPRD_UART4_SIZE - 1,
		.name = "serial_res",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SER4_INT,
		.end = IRQ_SER4_INT,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device sprd_serial_device4 = {
	.name           = "serial_sprd",
	.id             =  4,
	.num_resources  = ARRAY_SIZE(sprd_serial_resources4),
	.resource       = sprd_serial_resources4,
};

static struct resource resources_rtc[] = {
	[0] = {
		.start	= IRQ_ANA_RTC_INT,
		.end	= IRQ_ANA_RTC_INT,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device sprd_device_rtc= {
	.name	= "sprd_rtc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(resources_rtc),
	.resource	= resources_rtc,
};

static struct eic_gpio_resource sprd_gpio_resource[] = {
       [ENUM_ID_D_GPIO] = {
               .irq            = IRQ_GPIO_INT,
               .base_addr      = CTL_GPIO_BASE,
               .chip_base      = D_GPIO_START,
               .chip_ngpio     = D_GPIO_NR,
       },
       [ENUM_ID_D_EIC] = {
               .irq            = IRQ_EIC_INT,
               .base_addr      = CTL_EIC_BASE,
               .chip_base      = D_EIC_START,
               .chip_ngpio     = D_EIC_NR,
       },
       [ENUM_ID_A_GPIO] = {
               .irq            = IRQ_ANA_GPIO_INT,
               .base_addr      = ANA_CTL_GPIO_BASE,
               .chip_base      = A_GPIO_START,
               .chip_ngpio     = A_GPIO_NR,
       },
       [ENUM_ID_A_EIC] = {
               .irq            = IRQ_ANA_EIC_INT,
               .base_addr      = ANA_CTL_EIC_BASE,
               .chip_base      = A_EIC_START,
               .chip_ngpio     = A_EIC_NR,
       },
};

struct platform_device sprd_eic_gpio_device = {
       .name = "eic-gpio",
       .id = -1,
       .dev = { .platform_data = sprd_gpio_resource },
};

static struct resource sprd_nand_resources[] = {
	[0] = {
		.start	= 7,
		.end = 7,
		.flags	= IORESOURCE_DMA,
	},
	[1] = {
		.start	= SPRD_NFC_BASE,
		.end = SPRD_NFC_BASE + SPRD_NFC_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device sprd_nand_device = {
	.name		= "sprd-nand",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(sprd_nand_resources),
	.resource	= sprd_nand_resources,
};

static struct resource sprd_hwspinlock_resources0[] = {
	[0] = {
		.start	= SPRD_HWLOCK1_BASE,
		.end = SPRD_HWLOCK1_BASE + SPRD_HWLOCK1_SIZE- 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource sprd_hwspinlock_resources1[] = {
	[0] = {
		.start	= SPRD_HWLOCK0_BASE,
		.end = SPRD_HWLOCK0_BASE + SPRD_HWLOCK0_SIZE- 1,
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device sprd_hwspinlock_device0 = {
	.name		= "sci_hwspinlock",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(sprd_hwspinlock_resources0),
	.resource	= sprd_hwspinlock_resources0,
};

struct platform_device sprd_hwspinlock_device1 = {
	.name		= "sci_hwspinlock",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(sprd_hwspinlock_resources1),
	.resource	= sprd_hwspinlock_resources1,
};

static struct resource sprd_lcd_resources[] = {
	[0] = {
		.start = SPRD_LCDC_BASE,
		.end = SPRD_LCDC_BASE + SPRD_LCDC_SIZE - 1,
		.name = "lcd_res",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_DISPC0_INT,
		.end = IRQ_DISPC0_INT,
		.flags = IORESOURCE_IRQ,
	},
	
	[2] = {
		.start = IRQ_DISPC1_INT,
		.end = IRQ_DISPC1_INT,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device sprd_lcd_device0 = {
	.name           = "sprd_fb",
	.id             =  0,
	.num_resources  = ARRAY_SIZE(sprd_lcd_resources),
	.resource       = sprd_lcd_resources,
};

struct platform_device sprd_lcd_device1 = {
	.name           = "sprd_fb",
	.id             =  1,
	.num_resources  = ARRAY_SIZE(sprd_lcd_resources),
	.resource       = sprd_lcd_resources,
};

struct persistent_ram_descriptor sprd_console_desc = {
	.name = "ram_console",
	.size = SPRD_RAM_CONSOLE_SIZE,
};

struct persistent_ram sprd_console_ram = {
	.start = SPRD_RAM_CONSOLE_START,
	.size = SPRD_RAM_CONSOLE_SIZE,
	.num_descs = 1,
	.descs = &sprd_console_desc,
};

struct platform_device sprd_ram_console = {
	.name           = "ram_console",
	.id             =  -1,
};

static struct resource sprd_otg_resource[] = {
	[0] = {
		.start = SPRD_USB_BASE,
		.end   = SPRD_USB_BASE + SPRD_USB_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_USBD_INT,
		.end   = IRQ_USBD_INT,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device sprd_otg_device = {
	.name		= "dwc_otg",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(sprd_otg_resource),
	.resource	= sprd_otg_resource,
};

#if 0
struct platform_device sprd_backlight_device = {
	.name           = "sprd_backlight",
	.id             =  -1,
};
#endif

struct platform_device led_pwm_device = {
	.name		= "lcd-backlight",
};

static struct resource sprd_i2c_resources0[] = {
	[0] = {
		.start = SPRD_I2C0_BASE,
		.end   = SPRD_I2C0_BASE + SPRD_I2C0_SIZE -1,
		.name  = "i2c0_res",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_I2C0_INT,
		.end   = IRQ_I2C0_INT,
		.flags = IORESOURCE_IRQ,
	}
};
struct platform_device sprd_i2c_device0 = {
	.name           = "sprd-i2c",
	.id             =  0,
	.num_resources  = ARRAY_SIZE(sprd_i2c_resources0),
	.resource       = sprd_i2c_resources0,
};


static struct resource sprd_i2c_resources1[] = {
	[0] = {
		.start = SPRD_I2C1_BASE,
		.end   = SPRD_I2C1_BASE + SZ_4K -1,
		.name  = "i2c1_res",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_I2C1_INT,
		.end   = IRQ_I2C1_INT,
		.flags = IORESOURCE_IRQ,
	}
};
struct platform_device sprd_i2c_device1 = {
	.name           = "sprd-i2c",
	.id             =  1,
	.num_resources  = ARRAY_SIZE(sprd_i2c_resources1),
	.resource       = sprd_i2c_resources1,
};


static struct resource sprd_i2c_resources2[] = {
	[0] = {
		.start = SPRD_I2C2_BASE,
		.end   = SPRD_I2C2_BASE + SZ_4K -1,
		.name  = "i2c2_res",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_I2C2_INT,
		.end   = IRQ_I2C2_INT,
		.flags = IORESOURCE_IRQ,
	}
};
struct platform_device sprd_i2c_device2 = {
	.name           = "sprd-i2c",
	.id             =  2,
	.num_resources  = ARRAY_SIZE(sprd_i2c_resources2),
	.resource       = sprd_i2c_resources2,
};


static struct resource sprd_i2c_resources3[] = {
	[0] = {
		.start = SPRD_I2C3_BASE,
		.end   = SPRD_I2C3_BASE + SZ_4K -1,
		.name  = "i2c3_res",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_I2C3_INT,
		.end   = IRQ_I2C3_INT,
		.flags = IORESOURCE_IRQ,
	}
};
struct platform_device sprd_i2c_device3 = {
	.name           = "sprd-i2c",
	.id             =  3,
	.num_resources  = ARRAY_SIZE(sprd_i2c_resources3),
	.resource       = sprd_i2c_resources3,
};

/* 8810 SPI devices.  */
static struct resource spi0_resources[] = {
    [0] = {
        .start = SPRD_SPI0_BASE,
        .end = SPRD_SPI0_BASE + SPRD_SPI0_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = IRQ_SPI0_INT,
        .end = IRQ_SPI0_INT,
        .flags = IORESOURCE_IRQ,
    },
};


static struct resource spi1_resources[] = {
    [0] = {
        .start = SPRD_SPI1_BASE,
        .end = SPRD_SPI1_BASE + SPRD_SPI1_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = IRQ_SPI1_INT,
        .end = IRQ_SPI1_INT,
        .flags = IORESOURCE_IRQ,
    },
};

static struct resource spi2_resources[] = {
	[0] = {
	       .start = SPRD_SPI2_BASE,
	       .end = SPRD_SPI2_BASE + SPRD_SPI2_SIZE - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = IRQ_SPI2_INT,
	       .end = IRQ_SPI2_INT,
	       .flags = IORESOURCE_IRQ,
	       },
};

struct platform_device sprd_spi0_device = {
    .name = "sprd_spi",
    .id = 0,
    .resource = spi0_resources,
    .num_resources = ARRAY_SIZE(spi0_resources),
};

struct platform_device sprd_spi1_device = {
	.name = "sprd_spi",
	.id = 1,
	.resource = spi1_resources,
	.num_resources = ARRAY_SIZE(spi1_resources),
};

struct platform_device sprd_spi2_device = {
	.name = "sprd_spi",
	.id = 2,
	.resource = spi2_resources,
	.num_resources = ARRAY_SIZE(spi2_resources),
};

static struct resource sprd_ahb_bm_res[] = {
	[0] = {
		.start = SPRD_BM0_BASE,
		.end = SPRD_BM2_BASE + SPRD_BM2_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_BM0_INT,
		.end = IRQ_BM0_INT,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_BM1_INT,
		.end = IRQ_BM1_INT,
		.flags = IORESOURCE_IRQ,
	},
	[3] = {
		.start = IRQ_BM2_INT,
		.end = IRQ_BM2_INT,
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource sprd_axi_bm_res[] = {
	[0] = {
		.start =SPRD_AXIBM0_BASE,
		.end = SPRD_AXIBM9_BASE + SPRD_AXIBM9_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_AXI_BM_PUB_INT,
		.end = IRQ_AXI_BM_PUB_INT,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device sprd_ahb_bm_device = {
	.name = "sprd_ahb_busmonitor",
	.id = 0,
	.resource = sprd_ahb_bm_res,
	.num_resources = ARRAY_SIZE(sprd_ahb_bm_res),
};

struct platform_device sprd_axi_bm_device = {
	.name = "sprd_axi_busmonitor",
	.id = 0,
	.resource = sprd_axi_bm_res,
	.num_resources = ARRAY_SIZE(sprd_axi_bm_res),
};

//keypad 
#if defined(CONFIG_MACH_Z4DTG) || defined(CONFIG_MACH_Z4TD) || defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG) || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
#define CUSTOM_KEYPAD_ROWS          (SCI_ROW0 | SCI_ROW1)
#define CUSTOM_KEYPAD_COLS          (SCI_COL0)
#define ROWS	(2)
#define COLS	(1)

static const unsigned int test_keymap[] = {
	KEY(1, 0, KEY_VOLUMEUP),
	KEY(0, 0, KEY_VOLUMEDOWN),
};
#elif defined (CONFIG_MACH_SPX35FPGA) || defined(CONFIG_MACH_SPX35EA)
#define CUSTOM_KEYPAD_ROWS          (SCI_ROW7 | SCI_ROW6 | SCI_ROW5 | SCI_ROW4 | SCI_ROW3 | SCI_ROW2 | SCI_ROW1 | SCI_ROW0)
#define CUSTOM_KEYPAD_COLS          (SCI_COL7 | SCI_COL6 | SCI_COL5 | SCI_COL4 | SCI_COL3 | SCI_COL2 | SCI_COL1 | SCI_COL0)
#define ROWS	(8)
#define COLS	(8)

static const unsigned int test_keymap[] = {
	KEY(0, 0, KEY_F1),

	KEY(0, 3, KEY_COFFEE),
	KEY(0, 2, KEY_QUESTION),
	KEY(2, 3, KEY_CONNECT),
	KEY(1, 2, KEY_SHOP),
	KEY(1, 1, KEY_PHONE),

	KEY(0, 1, KEY_DELETE),
	KEY(2, 2, KEY_PLAY),
	KEY(1, 0, KEY_PAGEUP),
	KEY(1, 3, KEY_PAGEDOWN),
	KEY(2, 0, KEY_EMAIL),
	KEY(2, 1, KEY_STOP),

	KEY(0, 7, KEY_KP1),
	KEY(0, 6, KEY_KP2),
	KEY(0, 5, KEY_KP3),
	KEY(1, 7, KEY_KP4),
	KEY(1, 6, KEY_KP5),
	KEY(1, 5, KEY_KP6),
	KEY(2, 7, KEY_KP7),
	KEY(2, 6, KEY_KP8),
	KEY(2, 5, KEY_KP9),
	KEY(3, 6, KEY_KP0),
	KEY(3, 7, KEY_KPASTERISK),
	KEY(3, 5, KEY_KPDOT),
	KEY(7, 2, KEY_NUMLOCK),
	KEY(7, 1, KEY_KPMINUS),
	KEY(6, 1, KEY_KPPLUS),
	KEY(7, 6, KEY_KPSLASH),
	KEY(6, 0, KEY_ENTER),

	KEY(7, 4, KEY_CAMERA),

	KEY(0, 4, KEY_F2),
	KEY(1, 4, KEY_F3),
	KEY(2, 4, KEY_F4),
	KEY(7, 7, KEY_F5),
	KEY(7, 5, KEY_F6),

	KEY(3, 4, KEY_Q),
	KEY(3, 3, KEY_W),
	KEY(3, 2, KEY_E),
	KEY(3, 1, KEY_R),
	KEY(3, 0, KEY_T),
	KEY(4, 7, KEY_Y),
	KEY(4, 6, KEY_U),
	KEY(4, 5, KEY_I),
	KEY(4, 4, KEY_O),
	KEY(4, 3, KEY_P),
	KEY(4, 2, KEY_A),
	KEY(4, 1, KEY_S),
	KEY(4, 0, KEY_D),
	KEY(5, 7, KEY_F),
	KEY(5, 6, KEY_G),
	KEY(5, 5, KEY_H),
	KEY(5, 4, KEY_J),
	KEY(5, 3, KEY_K),
	KEY(5, 2, KEY_L),
	KEY(5, 1, KEY_Z),
	KEY(5, 0, KEY_X),
	KEY(6, 7, KEY_C),
	KEY(6, 6, KEY_V),
	KEY(6, 5, KEY_B),
	KEY(6, 4, KEY_N),
	KEY(6, 3, KEY_M),
	KEY(6, 2, KEY_SPACE),
	KEY(7, 0, KEY_LEFTSHIFT),
	KEY(7, 3, KEY_LEFTCTRL),
};
#endif

static const struct matrix_keymap_data test_keymap_data = {
	.keymap = test_keymap,
	.keymap_size = ARRAY_SIZE(test_keymap),
};
struct sci_keypad_platform_data sci_keypad_data = {
	.rows_choose_hw = CUSTOM_KEYPAD_ROWS,
	.cols_choose_hw = CUSTOM_KEYPAD_COLS,
	.rows = ROWS,
	.cols = COLS,
	.keymap_data = &test_keymap_data,
	.support_long_key = 1,
	.repeat = 0,
	.debounce_time = 5000,
	.keyup_test_interval = 50,
	.controller_ver = 1,
};

static struct resource sci_keypad_resources[] = {
        {
                .start = IRQ_KPD_INT,
                .end = IRQ_KPD_INT,
                .flags = IORESOURCE_IRQ,
        },
};

struct platform_device sprd_keypad_device = {
	.name = "sci-keypad",
        .id             = -1,
	.dev = {
		.platform_data = &sci_keypad_data,
		},
	.num_resources = ARRAY_SIZE(sci_keypad_resources),
	.resource = sci_keypad_resources,
};

static struct resource sprd_thm_resources[] = {
    [0] = {
        .start = SPRD_THM_BASE,
        .end = SPRD_THM_BASE + SPRD_THM_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = IRQ_THM_INT,
        .end = IRQ_THM_INT,
        .flags = IORESOURCE_IRQ,
    },
};

#define THERMAL_SOLUTION_D
static struct sprd_thm_platform_data sprd_thm_data = {
#ifdef THERMAL_SOLUTION_D
	.trip_points[0] = {
		.temp = 50, //.temp = 80,
		.type = THERMAL_TRIP_ACTIVE,
		.cdev_name = {
			[0] = "thermal-cpufreq-0",
			[1] = "thermal-cpufreq-0-m",
		},
	},
	.trip_points[1] = {
		.temp = 55,//.temp = 85,
		.type = THERMAL_TRIP_ACTIVE,
		.cdev_name = {
			[0] = "thermal-cpufreq-0",
			[1] = "thermal-cpufreq-0-m",
		},
	},
	.trip_points[2] = {
		.temp = 62,//.temp = 95,
		.type = THERMAL_TRIP_ACTIVE,
		.cdev_name = {
			[0] = "thermal-cpufreq-0",
			[1] = "thermal-cpufreq-0-m",
		},
	},
#else
	.trip_points[0] = {
		.temp = 80,
		.type = THERMAL_TRIP_ACTIVE,
		.cdev_name = {
			[0] = "thermal-cpufreq-0",
			[1] = "thermal-cpufreq-0-m",
		},
	},
	.trip_points[1] = {
		.temp = 85,
		.type = THERMAL_TRIP_ACTIVE,
		.cdev_name = {
			[0] = "thermal-cpufreq-0",
			[1] = "thermal-cpufreq-0-m",
		},
	},
	.trip_points[2] = {
		.temp = 95,
		.type = THERMAL_TRIP_ACTIVE,
		.cdev_name = {
			[0] = "thermal-cpufreq-0",
			[1] = "thermal-cpufreq-0-m",
		},
	},
#endif
	.trip_points[3] = {
		.temp = 110,
		.type = THERMAL_TRIP_CRITICAL,
	},
	.num_trips = 4,
};

struct platform_device sprd_thm_device = {
	.name           = "sprd-thermal",
      .id		= 0,
	.resource       = sprd_thm_resources,
	.num_resources  = ARRAY_SIZE(sprd_thm_resources),
	.dev	= {
		.platform_data	= &sprd_thm_data,
	},
};
static struct resource sprd_thm_a_resources[] = {
    [0] = {
        .start = ANA_THM_BASE,
        .end = ANA_THM_BASE + SPRD_THM_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = IRQ_ANA_THM_OTP_INT,
        .end = IRQ_ANA_THM_OTP_INT,
        .flags = IORESOURCE_IRQ,
    },
};

static struct sprd_thm_platform_data sprd_thm_a_data = {
	.trip_points[0] = {
		.temp = 140,
		.type = THERMAL_TRIP_CRITICAL,
	},
	.num_trips = 1,
};

struct platform_device sprd_thm_a_device = {
	.name           = "sprd-thermal",
      .id		= 1,
	.resource       = sprd_thm_a_resources,
	.num_resources  = ARRAY_SIZE(sprd_thm_a_resources),
	.dev	= {
		.platform_data	= &sprd_thm_a_data,
	},
};


struct platform_device sprd_audio_platform_pcm_device = {
	.name           = "sprd-pcm-audio",
	.id             =  -1,
};

struct platform_device sprd_audio_cpu_dai_vaudio_device = {
	.name           = "vaudio",
	.id             =  -1,
};

struct platform_device sprd_audio_cpu_dai_vbc_device = {
	.name           = "vbc",
	.id             =  -1,
};

struct platform_device sprd_audio_codec_sprd_codec_device = {
	.name           = "sprd-codec",
	.id             =  -1,
};

static struct resource sprd_i2s_resources0[] = {
        [0] = {
                .start = SPRD_IIS0_BASE,
                .end   = SPRD_IIS0_BASE + SZ_4K -1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = SPRD_IIS0_PHYS,
                .end   = SPRD_IIS0_PHYS + SZ_4K -1,
                .flags = IORESOURCE_MEM,
        },
        [2] = {
                .start = DMA_IIS_TX,
                .end   = DMA_IIS_RX,
                .flags = IORESOURCE_DMA,
        }
};

struct platform_device sprd_audio_cpu_dai_i2s_device = {
	.name           = "i2s",
	.id             =  0,
        .num_resources  = ARRAY_SIZE(sprd_i2s_resources0),
        .resource       = sprd_i2s_resources0,
};

static struct resource sprd_i2s_resources1[] = {
        [0] = {
                .start = SPRD_IIS1_BASE,
                .end   = SPRD_IIS1_BASE + SZ_4K -1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = SPRD_IIS1_PHYS,
                .end   = SPRD_IIS1_PHYS + SZ_4K -1,
                .flags = IORESOURCE_MEM,
        },
        [2] = {
                .start = DMA_IIS1_TX,
                .end   = DMA_IIS1_RX,
                .flags = IORESOURCE_DMA,
        }
};

struct platform_device sprd_audio_cpu_dai_i2s_device1 = {
	.name           = "i2s",
	.id             =  1,
        .num_resources  = ARRAY_SIZE(sprd_i2s_resources1),
        .resource       = sprd_i2s_resources1,
};

static struct resource sprd_i2s_resources2[] = {
        [0] = {
                .start = SPRD_IIS2_BASE,
                .end   = SPRD_IIS2_BASE + SZ_4K -1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = SPRD_IIS2_PHYS,
                .end   = SPRD_IIS2_PHYS + SZ_4K -1,
                .flags = IORESOURCE_MEM,
        },
        [2] = {
                .start = DMA_IIS2_TX,
                .end   = DMA_IIS2_RX,
                .flags = IORESOURCE_DMA,
        }
};

struct platform_device sprd_audio_cpu_dai_i2s_device2 = {
	.name           = "i2s",
	.id             =  2,
        .num_resources  = ARRAY_SIZE(sprd_i2s_resources2),
        .resource       = sprd_i2s_resources2,
};

static struct resource sprd_i2s_resources3[] = {
        [0] = {
                .start = SPRD_IIS3_BASE,
                .end   = SPRD_IIS3_BASE + SZ_4K -1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = SPRD_IIS3_PHYS,
                .end   = SPRD_IIS3_PHYS + SZ_4K -1,
                .flags = IORESOURCE_MEM,
        },
        [2] = {
                .start = DMA_IIS3_TX,
                .end   = DMA_IIS3_RX,
                .flags = IORESOURCE_DMA,
        }
};

struct platform_device sprd_audio_cpu_dai_i2s_device3 = {
	.name           = "i2s",
	.id             =  3,
        .num_resources  = ARRAY_SIZE(sprd_i2s_resources3),
        .resource       = sprd_i2s_resources3,
};

struct platform_device sprd_audio_codec_null_codec_device = {
	.name           = "null-codec",
	.id             =  -1,
};

#if (defined(CONFIG_SND_SOC_TFA9887_CODEC))
struct platform_device sprd_audio_codec_tfa9887_codec_device = {
	.name           = "tfa9887-codec",
	.id             =  -1,
};
#endif

static struct resource sprd_battery_resources[] = {
        [0] = {
                .start = EIC_CHARGER_DETECT,
                .end = EIC_CHARGER_DETECT,
                .flags = IORESOURCE_IO,
        }
};

struct platform_device sprd_battery_device = {
        .name           = "sprd-battery",
        .id             =  0,
        .num_resources  = ARRAY_SIZE(sprd_battery_resources),
        .resource       = sprd_battery_resources,
};

struct platform_device sprd_vsp_device = {
	.name	= "sprd_vsp",
	.id	= -1,
};

struct platform_device sprd_jpg_device = {
	.name	= "sprd_jpg",
	.id	= -1,
};

#ifdef CONFIG_ION
#ifdef CONFIG_CMA
static struct ion_platform_data ion_pdata = {
        .nr = 2,
        .heaps = {
                {
                        .id     = ION_HEAP_TYPE_CARVEOUT,
                        .type   = ION_HEAP_TYPE_CUSTOM,
                        .name   = "ion_cma_heap",
                        .base   = SPRD_ION_BASE,
                        .size   = SPRD_ION_SIZE,
                },
                {
                        .id     = ION_HEAP_TYPE_CARVEOUT + 1,
                        .type   = ION_HEAP_TYPE_CUSTOM,
                        .name   = "ion_cma_heap_overlay",
                        .base   = SPRD_ION_OVERLAY_BASE,
                        .size   = SPRD_ION_OVERLAY_SIZE,
                },
        }
};
#else
static struct ion_platform_data ion_pdata = {
#if CONFIG_SPRD_ION_OVERLAY_SIZE
        .nr = 2,
#else
        .nr = 1,
#endif
        .heaps = {
                {
                        .id     = ION_HEAP_TYPE_CARVEOUT,
                        .type   = ION_HEAP_TYPE_CARVEOUT,
                        .name   = "ion_carveout_heap",
                        .base   = SPRD_ION_BASE,
                        .size   = SPRD_ION_SIZE,
                },
#if CONFIG_SPRD_ION_OVERLAY_SIZE
                {
                        .id     = ION_HEAP_TYPE_CARVEOUT + 1,
                        .type   = ION_HEAP_TYPE_CARVEOUT,
                        .name   = "ion_carveout_heap_overlay",
                        .base   = SPRD_ION_OVERLAY_BASE,
                        .size   = SPRD_ION_OVERLAY_SIZE,
                },
#endif
        }
};
#endif

struct platform_device sprd_ion_dev = {
	.name = "ion-sprd",
	.id = -1,
	.dev = { .platform_data = &ion_pdata },
};
#endif

static struct resource sprd_dcam_resources[] = {
	{
		.start	= SPRD_ISP_BASE,
		.end	= SPRD_ISP_BASE + SPRD_ISP_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= IRQ_ISP_INT,
		.end	= IRQ_ISP_INT,
		.flags	= IORESOURCE_IRQ,
	},
};
struct platform_device sprd_dcam_device = {
	.name		= "sprd_dcam",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(sprd_dcam_resources),
	.resource	= sprd_dcam_resources,
};
struct platform_device sprd_scale_device = {
	.name	= "sprd_scale",
	.id	= -1,
};

struct platform_device sprd_gsp_device =
{
    .name	= "sprd_gsp",
    .id	= -1,
};
struct platform_device sprd_rotation_device = {
	.name	= "sprd_rotation",
	.id	= -1,
};

struct platform_device sprd_sensor_device = {
	.name	= "sprd_sensor",
	.id	= -1,
};
struct platform_device sprd_isp_device = {
	.name = "sprd_isp",
	.id = -1,
};

struct platform_device sprd_dma_copy_device = {
	.name	= "sprd_dma_copy",
	.id = -1,
};

static struct resource sprd_sdio0_resources[] = {
	[0] = {
		.start = SPRD_SDIO0_BASE,
		.end = SPRD_SDIO0_BASE + SPRD_SDIO0_SIZE-1,
		.name = "sdio0_res",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SDIO0_INT,
		.end = IRQ_SDIO0_INT,
		.flags = IORESOURCE_IRQ,
	}
};

static unsigned int sprd_sd_type = MMC_TYPE_SD;

static struct sprd_host_platdata sprd_sdio0_pdata = {
	.hw_name = "sprd-sdcard",
	.detect_gpio = 71,
	.vdd_name = "vddsd",
	.clk_name = "clk_sdio0",
	.clk_parent = "clk_192m",
	.max_clock = 192000000,
	.enb_bit = BIT_SDIO0_EB,
	.rst_bit = BIT_SDIO0_SOFT_RST,
	.slot_type	= &sprd_sd_type,
};

struct platform_device sprd_sdio0_device = {
	.name           = "sprd-sdhci",
	.id             =  0,
	.num_resources  = ARRAY_SIZE(sprd_sdio0_resources),
	.resource       = sprd_sdio0_resources,
	.dev = { .platform_data = &sprd_sdio0_pdata },
};

static struct resource sprd_sdio1_resources[] = {
	[0] = {
		.start = SPRD_SDIO1_BASE,
		.end = SPRD_SDIO1_BASE + SPRD_SDIO1_SIZE-1,
		.name = "sdio1_res",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SDIO1_INT,
		.end = IRQ_SDIO1_INT,
		.flags = IORESOURCE_IRQ,
	}
};

/*
 * SDIO1 (SDIO WLAN)
 */
//HTC_WIFI_START

/* BCM4329 returns wrong sdio_vsn(1) when we read cccr,
 * we use predefined value (sdio_vsn=2) here to initial sdio driver well
 */
static struct embedded_sdio_data sprd_wifi_emb_data = {
	.cccr   = {
		.sdio_vsn       = 2,
		.multi_block    = 1,
		.low_speed      = 0,
		.wide_bus       = 0,
		.high_power     = 1,
		.high_speed     = 1,
	}
};

static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;

static int
sprd_wifi_status_register(void (*callback)(int card_present, void *dev_id),
		void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;

	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}
static int sprd_wifi_cd;      /* WiFi virtual 'card detect' status */

static unsigned int sprd_wifi_status(struct device *dev)
{
	return sprd_wifi_cd;
}

int sprd_wifi_set_carddetect(int val)
{
	printk(KERN_INFO "[WLAN] %s: %d\n", __func__, val);
	sprd_wifi_cd = val;
	if (wifi_status_cb)
		wifi_status_cb(val, wifi_status_cb_devid);
	else
		printk(KERN_WARNING "[WLAN] %s: Nobody to notify\n", __func__);
	return 0;
}
EXPORT_SYMBOL(sprd_wifi_set_carddetect);


void wlan_clk_init(void)
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
	clk_set_rate(wlan_clk, 32768);
	clk_enable(wlan_clk);
}
EXPORT_SYMBOL(wlan_clk_init);

#if 0
static void power_on_cmmb1v8(void)
{
      //u32 value;
	  int enable;
      struct regulator* cmmb_regulator_1v8;
	  cmmb_regulator_1v8 = regulator_get(NULL, "vddcmmb1p8");

	  enable = regulator_is_enabled(cmmb_regulator_1v8);

      printk("[WIFI] power_on_cmmb1v8 is %d\n",enable);
	  
      regulator_set_voltage(cmmb_regulator_1v8, 1800000, 1800000);
      //regulator_disable(cmmb_regulator_1v8);
      //msleep(3);
      // value = cmmb_regulator_1v8->rdev->use_count;
      //printk("[WIFI] power_on_cmmb1v8 is %d\n",value);
      regulator_set_mode(cmmb_regulator_1v8, REGULATOR_MODE_STANDBY);
      regulator_enable(cmmb_regulator_1v8);
      msleep(5);
}
#endif

//config wifi gpio
typedef struct {
	uint32_t reg;
	uint32_t val;
} pinmap_t;

#define SDIO1_DATA3_WIFI   ( 0x01dc )
#define SDIO1_DATA2_WIFI   ( 0x01d8 )
#define SDIO1_DATA1_WIFI   ( 0x01d4 )
#define SDIO1_DATA0_WIFI   ( 0x01d0 )
#define SDIO1_CMD_WIFI      ( 0x01cc )
#define SDIO1_CLK_WIFI       ( 0x01c8 )

#define WL_HOST_WAKE        ( 0x0428 )

static pinmap_t  wifi_on_pinmap[] = {
	{SDIO1_DATA3_WIFI,           BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{SDIO1_DATA2_WIFI,           BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{SDIO1_DATA1_WIFI,           BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{SDIO1_DATA0_WIFI,           BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{SDIO1_CMD_WIFI,              BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{SDIO1_CLK_WIFI,               BIT_PIN_NULL|BITS_PIN_DS(2)|BITS_PIN_AF(0)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{WL_HOST_WAKE,                BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_IE},
};

static pinmap_t  wifi_off_pinmap[] = {
	{SDIO1_DATA3_WIFI,           BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{SDIO1_DATA2_WIFI,           BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{SDIO1_DATA1_WIFI,           BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{SDIO1_DATA0_WIFI,           BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{SDIO1_CMD_WIFI,              BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{SDIO1_CLK_WIFI,               BIT_PIN_NULL|BITS_PIN_DS(2)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{WL_HOST_WAKE,                BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_WPD|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
};

static void  wifi_pin_config(pinmap_t *pinmap, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		__raw_writel(pinmap[i].val, CTL_PIN_BASE + pinmap[i].reg);
	}
}

static void  wifi_pin_config_driection(void)
{
	gpio_request(WIFI_SDIO1_DATA0,"gpio_data0");
	gpio_direction_input(WIFI_SDIO1_DATA0);
	gpio_free(WIFI_SDIO1_DATA0);

	gpio_request(WIFI_SDIO1_DATA1,"gpio_data1");
	gpio_direction_input(WIFI_SDIO1_DATA1);
	gpio_free(WIFI_SDIO1_DATA1);

	gpio_request(WIFI_SDIO1_DATA2,"gpio_data2");
	gpio_direction_input(WIFI_SDIO1_DATA2);
	gpio_free(WIFI_SDIO1_DATA2);

	gpio_request(WIFI_SDIO1_DATA3,"gpio_data3");
	gpio_direction_input(WIFI_SDIO1_DATA3);
	gpio_free(WIFI_SDIO1_DATA3);
}

//we will adjust later
int sprd_wifi_power(int on)
{
	printk(KERN_INFO "[WLAN] %s: %d\n", __func__, on);

	if (on)
	{
		wifi_pin_config(wifi_on_pinmap,ARRAY_SIZE(wifi_on_pinmap));
		mdelay(1); /* Delay 1 ms, Recommand by Hardware */
	    gpio_request(SPRD_WIFI_PMENA_GPIO,"wifi_pwd");
        gpio_direction_output(SPRD_WIFI_PMENA_GPIO, on);
	    gpio_free(SPRD_WIFI_PMENA_GPIO);
	    mdelay(120);
	}
	else
	{
		gpio_request(SPRD_WIFI_PMENA_GPIO,"wifi_pwd");
    	gpio_direction_output(SPRD_WIFI_PMENA_GPIO, on);
		gpio_free(SPRD_WIFI_PMENA_GPIO);
		mdelay(120);
	    wifi_pin_config(wifi_off_pinmap,ARRAY_SIZE(wifi_off_pinmap));
		mdelay(1);
		//config GPIO direction
		wifi_pin_config_driection();
	}	
	
	return 0;
}
EXPORT_SYMBOL(sprd_wifi_power);

int sprd_wifi_reset(int on)
{
	printk(KERN_INFO "%s: do nothing\n", __func__);
	return 0;
}
//HTC_WIFI_END
static unsigned int sprd_wifislot_type = MMC_TYPE_SDIO_WIFI;

static struct sprd_host_platdata sprd_sdio1_pdata = {
	.hw_name = "sprd-sdio1",
	.clk_name = "clk_sdio1",
	//.clk_parent = "clk_sdio_src", //need modify by wifi
	.clk_parent = "clk_96m",
	.max_clock = 96000000,
	//.vdd_name = "vddsd1",       //we will power on it on init
	.enb_bit = BIT_SDIO1_EB,
	.rst_bit = BIT_SDIO1_SOFT_RST,
	.regs.is_valid = 1,
	//HTC_WIFI_START
	.status                 = sprd_wifi_status,
	.register_status_notify = sprd_wifi_status_register,
	.embedded_sdio          = &sprd_wifi_emb_data,  //We didnot use on cp2
	.slot_type	= &sprd_wifislot_type,
    //HTC_WIFI_END
};

struct platform_device sprd_sdio1_device = {
	.name           = "sprd-sdhci",
	.id             =  1,
	.num_resources  = ARRAY_SIZE(sprd_sdio1_resources),
	.resource       = sprd_sdio1_resources,
	.dev = { .platform_data = &sprd_sdio1_pdata },
};

static struct resource sprd_sdio2_resources[] = {
	[0] = {
	       .start = SPRD_SDIO2_BASE,
	       .end = SPRD_SDIO2_BASE + SPRD_SDIO2_SIZE - 1,
	       .name = "sdio2_res",
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = IRQ_SDIO2_INT,
	       .end = IRQ_SDIO2_INT,
	       .flags = IORESOURCE_IRQ,
	       }
};

static unsigned int sprd_cbpslot_type = MMC_TYPE_SDIO_CBP;
static struct sprd_host_platdata sprd_sdio2_pdata = {
	.hw_name	= "sprd-sdio2",
	.clk_name	= "clk_sdio2",
	.clk_parent 	= "clk_96m",
	.max_clock 	= 96000000,
	//.vdd_name     = "vddsd",
	.enb_bit	= BIT_SDIO2_EB,
	.rst_bit	= BIT_SDIO2_SOFT_RST,
	.slot_type	= &sprd_cbpslot_type,
};

struct platform_device sprd_sdio2_device = {
	.name = "sprd-sdhci",
	.id = 2,
	.num_resources = ARRAY_SIZE(sprd_sdio2_resources),
	.resource = sprd_sdio2_resources,
	.dev = { .platform_data = &sprd_sdio2_pdata },
};
static struct resource sprd_emmc_resources[] = {
	[0] = {
	       .start = SPRD_EMMC_BASE,
	       .end = SPRD_EMMC_BASE + SPRD_EMMC_SIZE - 1,
	       .name = "sdio3_res",
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = IRQ_EMMC_INT,
	       .end = IRQ_EMMC_INT,
	       .flags = IORESOURCE_IRQ,
	       }
};

static struct sprd_host_platdata sprd_emmc_pdata = {
	.hw_name = "sprd-emmc",
	.vdd_ext_name = "vddemmccore",
	.clk_name = "clk_emmc",
	.clk_parent = "clk_192m",
	.max_clock = 192000000,
	.enb_bit = BIT_EMMC_EB,
	.rst_bit = BIT_EMMC_SOFT_RST,
	.regs.is_valid = 1,
};

struct platform_device sprd_emmc_device = {
	.name = "sprd-sdhci",
	.id = 3,
	.num_resources = ARRAY_SIZE(sprd_emmc_resources),
	.resource = sprd_emmc_resources,
	.dev = { .platform_data = &sprd_emmc_pdata },
};

static struct resource sprd_pmu_resource[] = {
	[0] = {
		.start		= IRQ_NPMUIRQ0_INT,
		.end		= IRQ_NPMUIRQ0_INT,
		.flags		= IORESOURCE_IRQ,
	},
	[1] = {
		.start		= IRQ_NPMUIRQ1_INT,
		.end		= IRQ_NPMUIRQ1_INT,
		.flags		= IORESOURCE_IRQ,
	},
	[2] = {
		.start		= IRQ_NPMUIRQ2_INT,
		.end		= IRQ_NPMUIRQ2_INT,
		.flags		= IORESOURCE_IRQ,
	},
	[3] = {
		.start		= IRQ_NPMUIRQ3_INT,
		.end		= IRQ_NPMUIRQ3_INT,
		.flags		= IORESOURCE_IRQ,
	},
};

struct platform_device sprd_a7_pmu_device = {
	.name		= "arm-pmu",
	.id		= ARM_PMU_DEVICE_CPU,
	.resource = sprd_pmu_resource,
	.num_resources	= ARRAY_SIZE(sprd_pmu_resource),
};


struct platform_device sprd_peer_state_device = {
        .name           = "peer_state",
        .id             = -1,
};
#if defined(CONFIG_MACH_Z4DTG) || defined(CONFIG_MACH_Z4TD) || defined(CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)  || defined(CONFIG_MACH_CP5VEDTU) || defined(CONFIG_MACH_CP5VEDWG)
struct sysdump_mem sprd_dump_mem[] = {
#ifdef LINUX_BASE_ADDR1
	{
		.paddr		= LINUX_BASE_ADDR1,
		.vaddr		= LINUX_VIRT_ADDR1,
		.soff		= 0xffffffff,
		.size		= LINUX_SIZE_ADDR1,
		.type	 	= SYSDUMP_RAM,
		.name		= "linux_base_addr1",
	},
#endif

#ifdef TD_BASE_ADDR
	{
		.paddr		= TD_BASE_ADDR,
		.vaddr		= TD_VIRT_ADDR,
		.soff		= 0xffffffff,
		.size		= TD_SIZE_ADDR,
#ifdef CONFIG_SIPC_TD
		.type		= SYSDUMP_MODEM,
#else
		.type		= SYSDUMP_RAM,
#endif
		.name		= "td_base_addr",
	},
#endif

#ifdef TDS_BASE_ADDR
	{
		.paddr		= TDS_BASE_ADDR,
		.vaddr		= TDS_VIRT_ADDR,
		.soff		= 0xffffffff,
		.size		= TDS_SIZE_ADDR,
#ifdef CONFIG_SIPC_TD
		.type		= SYSDUMP_MODEM,
#else
		.type		= SYSDUMP_RAM,
#endif
		.name		= "tds_base_addr",
	},
#endif

#ifdef LINUX_BASE_ADDR2
	{
		.paddr		= LINUX_BASE_ADDR2,
		.vaddr		= LINUX_VIRT_ADDR2,
		.soff		= 0xffffffff,
		.size		= LINUX_SIZE_ADDR2,
		.type		= SYSDUMP_RAM,
		.name		= "linux_base_addr2",
	},
#endif

#ifdef LINUXS_BASE_ADDR2
	{
		.paddr		= LINUXS_BASE_ADDR2,
		.vaddr		= LINUXS_VIRT_ADDR2,
		.soff		= 0xffffffff,
		.size		= LINUXS_SIZE_ADDR2,
		.type		= SYSDUMP_RAM,
		.name		= "linuxs_base_addr2",
	},
#endif

#ifdef W_BASE_ADDR
	{
		.paddr		= W_BASE_ADDR,
		.vaddr		= W_VIRT_ADDR,
		.soff		= 0xffffffff,
		.size		= W_SIZE_ADDR,
#ifdef CONFIG_SIPC_WCDMA
		.type		= SYSDUMP_MODEM,
#else
		.type		= SYSDUMP_RAM,
#endif
		.name		= "w_base_addr",
	},
#endif

#ifdef WS_BASE_ADDR
	{
		.paddr		= WS_BASE_ADDR,
		.vaddr		= WS_VIRT_ADDR,
		.soff		= 0xffffffff,
		.size		= WS_SIZE_ADDR,
#ifdef CONFIG_SIPC_WCDMA
		.type		= SYSDUMP_MODEM,
#else
		.type		= SYSDUMP_RAM,
#endif
		.name		= "ws_base_addr",
	},
#endif

#ifdef LINUX_BASE_ADDR3
	{
		.paddr		= LINUX_BASE_ADDR3,
		.vaddr		= LINUX_VIRT_ADDR3,
		.soff		= 0xffffffff,
		.size		= LINUX_SIZE_ADDR3,
		.type		= SYSDUMP_RAM,
		.name		= "linux_base_addr3",
	},
#endif

#ifdef LINUXS_BASE_ADDR3
	{
		.paddr		= LINUXS_BASE_ADDR3,
		.vaddr		= LINUXS_VIRT_ADDR3,
		.soff		= 0xffffffff,
		.size		= LINUXS_SIZE_ADDR3,
		.type		= SYSDUMP_RAM,
		.name		= "linuxs_base_addr3",
	},
#endif

	{
		.paddr		= SPRD_AHB_PHYS,
		.vaddr		= SPRD_AHB_BASE,
		.soff		= 0x0,
		.size		= SPRD_AHB_SIZE,
		.type		= SYSDUMP_IOMEM,
		.name		= "sprd_ahb_phys",
	},
	{
		.paddr		= SPRD_INTC0_PHYS,
		.vaddr		= SPRD_INTC0_BASE,
		.soff		= 0x0,
		.size		= SPRD_INTC0_SIZE,
		.type		= SYSDUMP_IOMEM,
		.name		= "sprd_intc0_phys",
	},
	{
		.paddr		= SPRD_GPTIMER0_PHYS,
		.vaddr		= SPRD_GPTIMER0_BASE,
		.soff		= 0x0,
		.size		= SPRD_GPTIMER0_SIZE,
		.type		= SYSDUMP_IOMEM,
		.name		= "sprd_gptimer0_phys",
	},
	{
		.paddr		= SPRD_ADI_PHYS,
		.vaddr		= SPRD_ADI_BASE,
		.soff		= 0x0,
		.size		= SPRD_ADI_SIZE,
		.type		= SYSDUMP_IOMEM,
		.name		= "sprd_adi_phys",
	},
	{
		.paddr		= SPRD_GPIO_PHYS,
		.vaddr		= SPRD_GPIO_BASE,
		.soff		= 0x0,
		.size		= SPRD_GPIO_SIZE,
		.type		= SYSDUMP_IOMEM,
		.name		= "sprd_gpio_phys",
	},
	{
		.paddr		= SPRD_EIC_PHYS,
		.vaddr		= SPRD_EIC_BASE,
		.soff		= 0x0,
		.size		= SPRD_EIC_SIZE,
		.type		= SYSDUMP_IOMEM,
		.name		= "sprd_eic_phys",
	},
	{
		.paddr		= SPRD_GREG_PHYS,
		.vaddr		= SPRD_GREG_BASE,
		.soff		= 0x0,
		.size		= SPRD_GREG_SIZE,
		.type		= SYSDUMP_IOMEM,
		.name		= "sprd_greg_phys",
	},
};
#else
struct sysdump_mem sprd_dump_mem[] = {
	{
		.paddr		= CONFIG_PHYS_OFFSET,
		.vaddr		= PAGE_OFFSET,
		.soff		= 0xffffffff,
		.size		= CPT_START_ADDR - CONFIG_PHYS_OFFSET,
		.type	 	= SYSDUMP_RAM,
	},
	{
		.paddr		= CPT_START_ADDR,
		.vaddr		= PAGE_OFFSET +
					(CPT_START_ADDR - CONFIG_PHYS_OFFSET),
		.soff		= 0xffffffff,
		.size		= CPT_TOTAL_SIZE,
#ifdef CONFIG_SIPC_TD
		.type		= SYSDUMP_MODEM,
#else
		.type		= SYSDUMP_RAM,
#endif
	},
	{
		.paddr		= CPT_START_ADDR + CPT_TOTAL_SIZE,
		.vaddr		= PAGE_OFFSET +
					(CPT_START_ADDR + CPT_TOTAL_SIZE - CONFIG_PHYS_OFFSET),
		.soff		= 0xffffffff,
		.size		= SPRD_RAM_CONSOLE_START - (CPT_START_ADDR + CPT_TOTAL_SIZE),
		.type		= SYSDUMP_RAM,
	},
	{
		.paddr		= CPW_START_ADDR,
		.vaddr		= PAGE_OFFSET +
					(CPW_START_ADDR - CONFIG_PHYS_OFFSET),
		.soff		= 0xffffffff,
		.size		= CPW_TOTAL_SIZE,
#ifdef CONFIG_SIPC_WCDMA
		.type		= SYSDUMP_MODEM,
#else
		.type		= SYSDUMP_RAM,
#endif
	},
	{
		.paddr		= WCN_START_ADDR,
		.vaddr		= PAGE_OFFSET +
					(WCN_START_ADDR - CONFIG_PHYS_OFFSET),
		.soff		= 0xffffffff,
		.size		= WCN_TOTAL_SIZE,
//#ifdef CONFIG_SIPC_WCN
		.type		= SYSDUMP_MODEM,
//#else
//		.type		= SYSDUMP_RAM,
//#endif
	},
	{
		.paddr		= WCN_START_ADDR + WCN_TOTAL_SIZE,
		.vaddr		= PAGE_OFFSET +
					(WCN_START_ADDR + WCN_TOTAL_SIZE - CONFIG_PHYS_OFFSET),
		.soff		= 0xffffffff,
		.size		= 0, /* fill this dynamically according to real ram size */
		.type		= SYSDUMP_RAM,
	},
	{
		.paddr		= SPRD_AHB_PHYS,
		.vaddr		= SPRD_AHB_BASE,
		.soff		= 0x0,
		.size		= SPRD_AHB_SIZE,
		.type		= SYSDUMP_IOMEM,
	},
	{
		.paddr		= SPRD_INTC0_PHYS,
		.vaddr		= SPRD_INTC0_BASE,
		.soff		= 0x0,
		.size		= SPRD_INTC0_SIZE,
		.type		= SYSDUMP_IOMEM,
	},
	{
		.paddr		= SPRD_GPTIMER0_PHYS,
		.vaddr		= SPRD_GPTIMER0_BASE,
		.soff		= 0x0,
		.size		= SPRD_GPTIMER0_SIZE,
		.type		= SYSDUMP_IOMEM,
	},
	{
		.paddr		= SPRD_ADI_PHYS,
		.vaddr		= SPRD_ADI_BASE,
		.soff		= 0x0,
		.size		= SPRD_ADI_SIZE,
		.type		= SYSDUMP_IOMEM,
	},
	{
		.paddr		= SPRD_GPIO_PHYS,
		.vaddr		= SPRD_GPIO_BASE,
		.soff		= 0x0,
		.size		= SPRD_GPIO_SIZE,
		.type		= SYSDUMP_IOMEM,
	},
	{
		.paddr		= SPRD_EIC_PHYS,
		.vaddr		= SPRD_EIC_BASE,
		.soff		= 0x0,
		.size		= SPRD_EIC_SIZE,
		.type		= SYSDUMP_IOMEM,
	},
	{
		.paddr		= SPRD_GREG_PHYS,
		.vaddr		= SPRD_GREG_BASE,
		.soff		= 0x0,
		.size		= SPRD_GREG_SIZE,
		.type		= SYSDUMP_IOMEM,
	},
};
#endif
int sprd_dump_mem_num = ARRAY_SIZE(sprd_dump_mem);
