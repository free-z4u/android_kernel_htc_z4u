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

#ifndef __GPIO_CP5DUG_H__
#define __GPIO_CP5DUG_H__

#ifndef __ASM_ARCH_BOARD_H
#error  "Don't include this file directly, include <mach/board.h>"
#endif


#define GPIO_SENSOR_RESET        75
#define GPIO_MAIN_SENSOR_PWN     72
#define GPIO_SUB_SENSOR_PWN      74

#define USB_OTG_CABLE_DETECT     102

#define EIC_CHARGER_DETECT		(A_EIC_START + 0)
#define EIC_POWER_PBINT2        (A_EIC_START + 1)
#define EIC_POWER_PBINT         (A_EIC_START + 2)
#define EIC_AUD_HEAD_BUTTON     (A_EIC_START + 3)
#define EIC_AUD_HEAD_INST       (A_EIC_START + 4)
#define EIC_AUD_HEAD_INST2      (A_EIC_START + 5)
#define EIC_VCHG_OVI            (A_EIC_START + 6)
#define EIC_VBAT_OVI            (A_EIC_START + 7)

#define EIC_KEY_POWER           (EIC_POWER_PBINT)
#define HEADSET_BUTTON_GPIO		(EIC_AUD_HEAD_BUTTON)
#define HEADSET_DETECT_GPIO		139

#define SPI0_CMMB_CS_GPIO        156
#define SPI1_WIFI_CS_GPIO        44

#define GPIO_BK                  136

#define GPIO_CMMB_RESET         144
#define GPIO_CMMB_INT           143
#define GPIO_CMMB_26M_CLK_EN    197

#define GPIO_BT_RESET       0xFFFF // 194
#define GPIO_BT_POWER       116    // 190
#define GPIO_BT2AP_WAKE     232 // 58
#define GPIO_AP2BT_WAKE     233 // 51

#define GPIO_WIFI_SHUTDOWN	189
#define GPIO_WIFI_IRQ		52

#define GPIO_PLSENSOR_IRQ	213

/* For touch */
#define GPIO_TP_RSTz			81
#define GPIO_TP_ATTz			82
#define GPIO_V_LED_3V3EN		104

/* For NFC */
#define GPIO_NFC_VEN			84
#define GPIO_NFC_IRQ			85
#define GPIO_NFC_DL_MODE		86

/* Used for Spreadtrum board */
#define GPIO_TOUCH_IRQ		141
#define GPIO_TOUCH_RESET	142

#define SHARK_EVM_GPIO_PS_INT 162
#define SHARK_EVM_GPIO_GSENSOR_INT     163

#define TIGER_EVM_LAYOUTS		{ \
		{ { 0,  1, 0}, { 1,  0,  0}, {0, 0, -1} }, \
		{ { 0, -1, 0}, {-1,  0,  0}, {0, 0,  1} }, \
		{ { 1,  0, 0}, { 0, -1,  0}, {0, 0, -1} }, \
		{ { 1,  0, 0}, { 0,  0,  1}, {0, 1,  0} }  \
					}
#define TIGER_EVT_LAYOUTS		{ \
		{ { 0,  1, 0}, { 1,  0,  0}, {0, 0, -1} }, \
		{ { 0, -1, 0}, {-1,  0,  0}, {0, 0,  1} }, \
		{ { 1,  0, 0}, { 0, -1,  0}, {0, 0, -1} }, \
		{ { -1, 0, 0}, { 0,  0, -1}, {0, 1,  0} }  \
					}
#endif

/* used for flashlight */
#define GPIO_FLASH_EN        132
#define GPIO_TORCH_EN        160

/* HEADSET DRIVER BEGIN */
#define GPIO_AUD_UART_OEz		(72)		/* EXTINT1 */
#define GPIO_AUD_HP_INz			(200)	/* KEYIN1 */
#define GPIO_AUD_REMO_PRESz		(118)	/* LCD_D10 */
#define GPIO_AUD_2V85_EN			(117)	/* LCD_D9 */
#define GPIO_CPU_1WIRE_TX			(16)		/* U2TXD */

/* GPIO KEY */
#define GPIO_KEY_POWER      161
#define GPIO_KEY_VOLUMEUP   191
#define GPIO_KEY_VOLUMEDOWN 192
#define GPIO_KEY_IN0 199

/* HW_CHG_LED_OFF,
Amber LED glitter when USB cable in(for dead battery)
L: Enable
*/
#define GPIO_HW_CHG_LED_OFF 175
