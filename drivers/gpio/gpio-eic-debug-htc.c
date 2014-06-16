/* linux/drivers/gpio/gpio-eic-debug-htc.c
 * Copyright (C) 2013 HTC Corporation.
 * Author: Oliver Fu <oliver_fu@htc.com>
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/gpio.h>

#include <mach/hardware.h>
#include <mach/pinmap.h>
#include <mach/gpio.h>

/* 16 GPIO share a group of registers */
#define	GPIO_GROUP_NR			(16)
#define GPIO_GROUP_MASK			(0xFFFF)

#define	GPIO_GROUP_OFFSET		(0x80)
#define	ANA_GPIO_GROUP_OFFSET	(0x40)

/* registers definitions for GPIO controller */
#define REG_GPIO_DATA			(0x0000)
#define REG_GPIO_DMSK			(0x0004)
#define REG_GPIO_DIR			(0x0008)	/* only for gpio */
#define REG_GPIO_IS				(0x000c)	/* only for gpio */
#define REG_GPIO_IBE			(0x0010)	/* only for gpio */
#define REG_GPIO_IEV			(0x0014)
#define REG_GPIO_IE				(0x0018)
#define REG_GPIO_RIS			(0x001c)
#define REG_GPIO_MIS			(0x0020)
#define REG_GPIO_IC				(0x0024)
#define REG_GPIO_INEN			(0x0028)	/* only for gpio */

#define	to_sci_gpio(c)			container_of(c, struct sci_gpio_chip, chip)

#define PIN(name,a,b,c,d) 	{REG_PIN_##name, (int)#name, a, b, c, d}

#define A(x)				(int)(#x)
#define GPIO(x) 			(x)
#define CP0_ARM_GPIO(x)		(x | 0x100)
#define CP1_ARM_GPIO(x)		(x | 0x200)
#define CP2_ARM_GPIO(x)		(x | 0x300)
#define CP0_DSP_GPO(x)		(x | 0x400)
#define CP1_DSP_GPO(x)		(x | 0x500)
#define CP2_DSP_GPO(x)		(x | 0x600)
#define CP0_RFCTL(x)		(x | 0x700)
#define CP1_RFCTL(x)		(x | 0x800)
#define CP2_RFCTL(x)		(x | 0x900)

static int flags = 0;

static int PINS[][6] = {
PIN(TRACECLK       , A(TRACECLK)       , A(SPI1_CLK)       , CP0_DSP_GPO(0)      , GPIO(230) ),
PIN(TRACECTRL      , A(TRACECTRL)      , A(SPI1_DI)        , CP0_DSP_GPO(1)      , GPIO(231) ),
PIN(TRACEDAT0      , A(TRACEDAT0)      , A(SPI1_DO)        , CP0_DSP_GPO(2)      , GPIO(232) ),
PIN(TRACEDAT1      , A(TRACEDAT1)      , A(SPI1_CSN)       , CP0_DSP_GPO(3)      , GPIO(233) ),
PIN(TRACEDAT2      , A(TRACEDAT2)      , A(PWMC)           , CP0_DSP_GPO(4)      , GPIO(234) ),
PIN(TRACEDAT3      , A(TRACEDAT3)      , A(IIS3DI)         , CP0_DSP_GPO(5)      , GPIO(235) ),
PIN(TRACEDAT4      , A(TRACEDAT4)      , A(IIS3DO)         , CP0_DSP_GPO(6)      , GPIO(236) ),
PIN(TRACEDAT5      , A(TRACEDAT5)      , A(IIS3CLK)        , CP0_DSP_GPO(7)      , GPIO(237) ),
PIN(TRACEDAT6      , A(TRACEDAT6)      , A(IIS3LRCK)       , CP0_DSP_GPO(8)      , GPIO(238) ),
PIN(TRACEDAT7      , A(TRACEDAT7)      , A(IIS3MCK)        , CP0_DSP_GPO(9)      , GPIO(239) ),
PIN(U0TXD          , A(U0TXD)          , A(RFSDA2)         , 0                   , GPIO(10)  ),
PIN(U0RXD          , A(U0RXD)          , A(RFSCK2)         , 0                   , GPIO(11)  ),
PIN(U0CTS          , A(U0CTS)          , A(RFSEN2)         , 0                   , GPIO(12)  ),
PIN(U0RTS          , A(U0RTS)          , CP2_RFCTL(2)      , 0                   , GPIO(13)  ),
PIN(U1TXD          , A(U1TXD)          , A(ORP_UTXD)       , A(CP_ARM9_U1_U0TXD) , GPIO(14)  ),
PIN(U1RXD          , A(U1RXD)          , A(ORP_URXD)       , A(CP_ARM9_U1_U0RXD) , GPIO(15)  ),
PIN(U2TXD          , A(U2TXD)          , A(ORP_UTXD)       , A(CP_ARM9_U2_U0TXD) , GPIO(16)  ),
PIN(U2RXD          , A(U2RXD)          , A(ORP_URXD)       , A(CP_ARM9_U2_U0RXD) , GPIO(17)  ),
PIN(U3TXD          , A(U3TXD)          , 0                 , 0                   , GPIO(18)  ),
PIN(U3RXD          , A(U3RXD)          , 0                 , 0                   , GPIO(19)  ),
PIN(U3CTS          , A(U3CTS)          , A(U3RXD)          , 0                   , GPIO(20)  ),
PIN(U3RTS          , A(U3RTS)          , A(U3TXD)          , 0                   , GPIO(21)  ),
PIN(CP2_RFCTL0     , CP2_RFCTL(0)      , 0                 , 0                   , GPIO(27)  ),
PIN(CP2_RFCTL1     , CP2_RFCTL(1)      , 0                 , 0                   , GPIO(28)  ),
PIN(CP2_RFCTL2     , CP2_RFCTL(2)      , 0                 , 0                   , GPIO(29)  ),
PIN(RFSDA0         , A(RFSDA0)         , 0                 , 0                   , GPIO(30)  ),
PIN(RFSCK0         , A(RFSCK0)         , 0                 , 0                   , GPIO(31)  ),
PIN(RFSEN0         , A(RFSEN0)         , 0                 , 0                   , GPIO(32)  ),
PIN(RFSDA1         , A(RFSDA1)         , 0                 , 0                   , GPIO(33)  ),
PIN(RFSCK1         , A(RFSCK1)         , 0                 , 0                   , GPIO(34)  ),
PIN(RFSEN1         , A(RFSEN1)         , 0                 , 0                   , GPIO(35)  ),
PIN(CP1_RFCTL0     , CP1_RFCTL(0)      , 0                 , 0                   , GPIO(36)  ),
PIN(CP1_RFCTL1     , CP1_RFCTL(1)      , 0                 , 0                   , GPIO(37)  ),
PIN(CP1_RFCTL2     , CP1_RFCTL(2)      , 0                 , 0                   , GPIO(38)  ),
PIN(CP1_RFCTL3     , CP1_RFCTL(3)      , A(PWMA)           , 0                   , GPIO(39)  ),
PIN(CP1_RFCTL4     , CP1_RFCTL(4)      , A(PWMB)           , 0                   , GPIO(40)  ),
PIN(CP1_RFCTL5     , CP1_RFCTL(5)      , 0                 , 0                   , GPIO(41)  ),
PIN(CP1_RFCTL6     , CP1_RFCTL(6)      , A(PWMC)           , 0                   , GPIO(42)  ),
PIN(CP1_RFCTL7     , CP1_RFCTL(7)      , 0                 , 0                   , GPIO(43)  ),
PIN(CP1_RFCTL8     , CP1_RFCTL(8)      , CP0_RFCTL(8)      , 0                   , GPIO(44)  ),
PIN(CP1_RFCTL9     , CP1_RFCTL(9)      , CP0_RFCTL(9)      , A(PWMD)             , GPIO(45)  ),
PIN(CP1_RFCTL10    , CP1_RFCTL(10)     , CP0_RFCTL(10)     , 0                   , GPIO(46)  ),
PIN(CP1_RFCTL11    , CP1_RFCTL(11)     , CP0_RFCTL(11)     , 0                   , GPIO(47)  ),
PIN(CP1_RFCTL12    , CP1_RFCTL(12)     , CP0_RFCTL(12)     , A(MIPI_CLK0)        , GPIO(48)  ),
PIN(CP1_RFCTL13    , CP1_RFCTL(13)     , CP0_RFCTL(13)     , A(MIPI_DATA0)       , GPIO(49)  ),
PIN(CP1_RFCTL14    , CP1_RFCTL(14)     , CP0_RFCTL(14)     , A(MIPI_CLK1)        , GPIO(50)  ),
PIN(CP1_RFCTL15    , CP1_RFCTL(15)     , CP0_RFCTL(15)     , A(MIPI_DATA1)       , GPIO(51)  ),
PIN(CP0_RFCTL0     , CP0_RFCTL(0)      , 0                 , 0                   , GPIO(52)  ),
PIN(CP0_RFCTL1     , CP0_RFCTL(1)      , A(PWMD)           , 0                   , GPIO(53)  ),
PIN(CP0_RFCTL2     , CP0_RFCTL(2)      , 0                 , 0                   , GPIO(54)  ),
PIN(CP0_RFCTL3     , CP0_RFCTL(3)      , 0                 , 0                   , GPIO(55)  ),
PIN(CP0_RFCTL4     , CP0_RFCTL(4)      , 0                 , 0                   , GPIO(56)  ),
PIN(CP0_RFCTL5     , CP0_RFCTL(5)      , 0                 , 0                   , GPIO(57)  ),
PIN(CP0_RFCTL6     , CP0_RFCTL(6)      , 0                 , 0                   , GPIO(58)  ),
PIN(CP0_RFCTL7     , CP0_RFCTL(7)      , 0                 , 0                   , GPIO(59)  ),
PIN(XTLEN          , A(XTL_EN)         , A(RFSEN0_1)       , 0                   , GPIO(60)  ),
PIN(SCL3           , A(SCL3)           , A(U4TXD)          , 0                   , GPIO(65)  ),
PIN(SDA3           , A(SDA3)           , A(U4RXD)          , 0                   , GPIO(66)  ),
PIN(SPI0_CSN       , A(SPI0_CSN)       , 0                 , 0                   , GPIO(67)  ),
PIN(SPI0_DO        , A(SPI0_DO)        , 0                 , 0                   , GPIO(68)  ),
PIN(SPI0_DI        , A(SPI0_DI)        , 0                 , 0                   , GPIO(69)  ),
PIN(SPI0_CLK       , A(SPI0_CLK)       , 0                 , 0                   , GPIO(70)  ),
PIN(EXTINT0        , A(EXTINT0)        , 0                 , CP0_DSP_GPO(10)     , GPIO(71)  ),
PIN(EXTINT1        , A(EXTINT1)        , A(USB_DRVVBUS)    , 0                   , GPIO(72)  ),
PIN(SCL1           , A(SCL1)           , 0                 , 0                   , GPIO(73)  ),
PIN(SDA1           , A(SDA1)           , 0                 , 0                   , GPIO(74)  ),
PIN(SIMCLK0        , A(SIMCLK0)        , 0                 , 0                   , GPIO(75)  ),
PIN(SIMDA0         , A(SIMDA0)         , 0                 , 0                   , GPIO(76)  ),
PIN(SIMRST0        , A(SIMRST0)        , 0                 , 0                   , GPIO(77)  ),
PIN(SIMCLK1        , A(SIMCLK1)        , 0                 , 0                   , GPIO(78)  ),
PIN(SIMDA1         , A(SIMDA1)         , 0                 , 0                   , GPIO(79)  ),
PIN(SIMRST1        , A(SIMRST1)        , 0                 , 0                   , GPIO(80)  ),
PIN(SIMCLK2        , A(SIMCLK2)        , A(SCL4)           , A(U0TXD(G3))        , GPIO(81)  ),
PIN(SIMDA2         , A(SIMDA2)         , A(SDA4)           , A(U0RXD(G3))        , GPIO(82)  ),
PIN(SIMRST2        , A(SIMRST2)        , A(CLK_AUX1)       , 0                   , GPIO(83)  ),
PIN(MEMS_MIC_CLK0  , A(MEMS_MIC_CLK0)  , 0                 , 0                   , GPIO(84)  ),
PIN(MEMS_MIC_DATA0 , A(MEMS_MIC_DATA0) , 0                 , 0                   , GPIO(85)  ),
PIN(MEMS_MIC_CLK1  , A(MEMS_MIC_CLK1)  , 0                 , 0                   , GPIO(86)  ),
PIN(MEMS_MIC_DATA1 , A(MEMS_MIC_DATA1) , 0                 , 0                   , GPIO(87)  ),
PIN(SD1_CLK        , A(SD1_CLK)        , A(SPI1_CLK)       , A(WIFI_AGC_GAIN2)   , GPIO(88)  ),
PIN(SD1_CMD        , A(SD1_CMD)        , A(SPI1_DI)        , A(WB_EN_A)          , GPIO(89)  ),
PIN(SD1_D0         , A(SD1_D0)         , A(SPI1_DO)        , A(WB_EN_B)          , GPIO(90)  ),
PIN(SD1_D1         , A(SD1_D1)         , A(SPI1_CSN)       , A(GPS_REAL)         , GPIO(91)  ),
PIN(SD1_D2         , A(SD1_D2)         , 0                 , A(GPS_IMAG)         , GPIO(92)  ),
PIN(SD1_D3         , A(SD1_D3)         , 0                 , A(GPS_CLK)          , GPIO(93)  ),
PIN(SD0_D3         , A(SD0_D3)         , 0                 , 0                   , GPIO(100) ),
PIN(SD0_D2         , A(SD0_D2)         , 0                 , 0                   , GPIO(99)  ),
PIN(SD0_CMD        , A(SD0_CMD)        , 0                 , 0                   , GPIO(96)  ),
PIN(SD0_D0         , A(SD0_D0)         , 0                 , 0                   , GPIO(97)  ),
PIN(SD0_D1         , A(SD0_D1)         , 0                 , 0                   , GPIO(98)  ),
PIN(SD0_CLK1       , A(SD0_CLK1)       , 0                 , 0                   , GPIO(95)  ),
PIN(SD0_CLK0       , A(SD0_CLK0)       , 0                 , 0                   , GPIO(94)  ),
PIN(PTEST          , A(PTEST)          , 0                 , 0                   , 0         ),
PIN(ANA_INT        , A(ANA_INT)        , 0                 , 0                   , 0         ),
PIN(EXT_RST_B      , A(EXT_RST_B)      , 0                 , 0                   , 0         ),
PIN(CHIP_SLEEP     , A(CHIP_SLEEP)     , 0                 , 0                   , 0         ),
PIN(XTL_BUF_EN0    , A(XTL_BUF_EN0)    , 0                 , 0                   , 0         ),
PIN(XTL_BUF_EN1    , A(XTL_BUF_EN1)    , 0                 , 0                   , 0         ),
PIN(XTL_BUF_EN2    , A(XTL_BUF_EN2)    , 0                 , 0                   , 0         ),
PIN(CLK_32K        , A(CLK_32K)        , 0                 , 0                   , 0         ),
PIN(AUD_SCLK       , A(AUD_SCLK)       , 0                 , 0                   , 0         ),
PIN(AUD_ADD0       , A(AUD_ADD0)       , 0                 , 0                   , 0         ),
PIN(AUD_ADSYNC     , A(AUD_ADSYNC)     , 0                 , 0                   , 0         ),
PIN(AUD_DAD1       , A(AUD_DAD1)       , 0                 , 0                   , 0         ),
PIN(AUD_DAD0       , A(AUD_DAD0)       , 0                 , 0                   , 0         ),
PIN(AUD_DASYNC     , A(AUD_DASYNC)     , 0                 , 0                   , 0         ),
PIN(ADI_D          , A(ADI_D)          , 0                 , 0                   , 0         ),
PIN(ADI_SYNC       , A(ADI_SYNC)       , A(SCL)            , 0                   , 0         ),
PIN(ADI_SCLK       , A(ADI_SCLK)       , A(SDA5)           , 0                   , 0         ),
PIN(LCD_CSN1       , A(LCD_CSN1)       , A(CLK_AUX1)       , 0                   , GPIO(101) ),
PIN(LCD_CSN0       , A(LCD_CSN0)       , 0                 , 0                   , GPIO(102) ),
PIN(LCD_RSTN       , A(LCD_RSTN)       , 0                 , 0                   , GPIO(103) ),
PIN(LCD_CD         , A(LCD_CD)         , A(SPI2_CD)        , 0                   , GPIO(104) ),
PIN(LCD_FMARK      , A(LCD_FMARK)      , 0                 , 0                   , GPIO(105) ),
PIN(LCD_WRN        , A(CD_WRN)         , 0                 , 0                   , GPIO(106) ),
PIN(LCD_RDN        , A(CD_RDN)         , 0                 , 0                   , GPIO(107) ),
PIN(LCD_D0         , A(LCD_D0)         , A(TRACEDAT8)      , CP0_ARM_GPIO(4)     , GPIO(108) ),
PIN(LCD_D1         , A(LCD_D1)         , A(TRACEDAT9)      , CP0_ARM_GPIO(5)     , GPIO(109) ),
PIN(LCD_D2         , A(LCD_D2)         , A(TRACEDAT10)     , CP0_ARM_GPIO(6)     , GPIO(110) ),
PIN(LCD_D3         , A(LCD_D3)         , A(TRACEDAT11)     , CP0_ARM_GPIO(7)     , GPIO(111) ),
PIN(LCD_D4         , A(LCD_D4)         , A(TRACEDAT12)     , CP1_ARM_GPIO(4)     , GPIO(112) ),
PIN(LCD_D5         , A(LCD_D5)         , A(TRACEDAT13)     , CP1_ARM_GPIO(5)     , GPIO(113) ),
PIN(LCD_D6         , A(LCD_D6)         , A(TRACEDAT14)     , CP1_ARM_GPIO(6)     , GPIO(114) ),
PIN(LCD_D7         , A(LCD_D7)         , A(TRACEDAT15)     , CP1_ARM_GPIO(7)     , GPIO(115) ),
PIN(LCD_D8         , A(LCD_D8)         , A(TRACEDAT16)     , CP2_ARM_GPIO(4)     , GPIO(116) ),
PIN(LCD_D9         , A(LCD_D9)         , A(TRACEDAT17)     , CP2_ARM_GPIO(5)     , GPIO(117) ),
PIN(LCD_D10        , A(LCD_D10)        , A(TRACEDAT18)     , CP2_ARM_GPIO(6)     , GPIO(118) ),
PIN(LCD_D11        , A(LCD_D11)        , A(TRACEDAT19)     , CP2_ARM_GPIO(7)     , GPIO(119) ),
PIN(LCD_D12        , A(LCD_D12)        , A(TRACEDAT20)     , CP1_DSP_GPO(0)      , GPIO(120) ),
PIN(LCD_D13        , A(LCD_D13)        , A(TRACEDAT21)     , CP1_DSP_GPO(1)      , GPIO(121) ),
PIN(LCD_D14        , A(LCD_D14)        , A(TRACEDAT22)     , CP1_DSP_GPO(2)      , GPIO(122) ),
PIN(LCD_D15        , A(LCD_D15)        , A(TRACEDAT23)     , CP1_DSP_GPO(3)      , GPIO(123) ),
PIN(LCD_D16        , A(LCD_D16)        , A(TRACEDAT24)     , CP1_DSP_GPO(4)      , GPIO(124) ),
PIN(LCD_D17        , A(LCD_D17)        , A(TRACEDAT25)     , CP1_DSP_GPO(5)      , GPIO(125) ),
PIN(LCD_D18        , A(LCD_D18)        , A(TRACEDAT26)     , CP1_DSP_GPO(6)      , GPIO(126) ),
PIN(LCD_D19        , A(LCD_D19)        , A(TRACEDAT27)     , CP1_DSP_GPO(7)      , GPIO(127) ),
PIN(LCD_D20        , A(LCD_D20)        , A(TRACEDAT28)     , CP1_DSP_GPO(8)      , GPIO(128) ),
PIN(LCD_D21        , A(LCD_D21)        , A(TRACEDAT29)     , CP1_DSP_GPO(9)      , GPIO(129) ),
PIN(LCD_D22        , A(LCD_D22)        , A(TRACEDAT30)     , CP1_DSP_GPO(10)     , GPIO(130) ),
PIN(LCD_D23        , A(LCD_D23)        , A(TRACEDAT31)     , A(CLK_AUX1)         , GPIO(131) ),
PIN(SPI2_CSN       , A(SPI2_CSN)       , 0                 , 0                   , GPIO(132) ),
PIN(SPI2_DO        , A(SPI2_DO)        , 0                 , 0                   , GPIO(133) ),
PIN(SPI2_DI        , A(SPI2_DI)        , 0                 , 0                   , GPIO(134) ),
PIN(SPI2_CLK       , A(SPI2_CLK)       , 0                 , 0                   , GPIO(135) ),
PIN(EMMC_CLK       , A(EMMC_CLK)       , 0                 , 0                   , GPIO(136) ),
PIN(EMMC_CMD       , A(EMMC_CMD)       , 0                 , 0                   , GPIO(137) ),
PIN(EMMC_D0        , A(EMMC_D0)        , 0                 , 0                   , GPIO(138) ),
PIN(EMMC_D1        , A(EMMC_D1)        , 0                 , 0                   , GPIO(139) ),
PIN(EMMC_D2        , A(EMMC_D2)        , 0                 , 0                   , GPIO(140) ),
PIN(EMMC_D3        , A(EMMC_D3)        , 0                 , 0                   , GPIO(141) ),
PIN(EMMC_D4        , A(EMMC_D4)        , 0                 , 0                   , GPIO(142) ),
PIN(EMMC_D5        , A(EMMC_D5)        , 0                 , 0                   , GPIO(143) ),
PIN(EMMC_D6        , A(EMMC_D6)        , 0                 , 0                   , GPIO(144) ),
PIN(EMMC_D7        , A(EMMC_D7)        , 0                 , 0                   , GPIO(145) ),
PIN(EMMC_RST       , A(EMMC_RST)       , 0                 , 0                   , GPIO(146) ),
PIN(NFWPN          , A(NFWPN)          , 0                 , A(KEYOUT3)          , GPIO(147) ),
PIN(NFRB           , A(NFRB)           , 0                 , A(KEYOUT4)          , GPIO(148) ),
PIN(NFCLE          , A(NFCLE)          , A(U4TXD)          , A(KEYOUT5)          , GPIO(149) ),
PIN(NFALE          , A(NFALE)          , A(U4RXD)          , A(KEYOUT6)          , GPIO(150) ),
PIN(NFCEN0         , A(NFCEN0)         , 0                 , A(KEYOUT7)          , GPIO(151) ),
PIN(NFCEN1         , A(NFCEN1)         , 0                 , 0                   , GPIO(152) ),
PIN(NFREN          , A(NFREN)          , A(U4CTS)          , 0                   , GPIO(153) ),
PIN(NFWEN          , A(NFWEN)          , A(U4RTS)          , A(KEYIN7)           , GPIO(154) ),
PIN(NFD0           , A(NFD0)           , 0                 , A(KEYIN6)           , GPIO(155) ),
PIN(NFD1           , A(NFD1)           , 0                 , A(KEYIN5)           , GPIO(156) ),
PIN(NFD2           , A(NFD2)           , 0                 , A(KEYIN4)           , GPIO(157) ),
PIN(NFD3           , A(NFD3)           , 0                 , A(KEYIN3)           , GPIO(158) ),
PIN(NFD4           , A(NFD4)           , A(CLK_AUX1)       , A(PWMA)             , GPIO(159) ),
PIN(NFD5           , A(NFD5)           , A(SD2_D7)         , A(IIS3DI)           , GPIO(160) ),
PIN(NFD6           , A(NFD6)           , A(SD2_D6)         , A(IIS3DO)           , GPIO(161) ),
PIN(NFD7           , A(NFD7)           , A(SD2_D5)         , A(IIS3CLK)          , GPIO(162) ),
PIN(NFD8           , A(NFD8)           , A(SD2_D4)         , A(IIS3LRCK)         , GPIO(163) ),
PIN(NFD9           , A(NFD9)           , A(SD2_D3)         , A(IIS3MCK(G))       , GPIO(164) ),
PIN(NFD10          , A(NFD10)          , A(SD2_D2)         , 0                   , GPIO(165) ),
PIN(NFD11          , A(NFD11)          , A(SD2_D1)         , A(IIS2DI)           , GPIO(166) ),
PIN(NFD12          , A(NFD12)          , A(SD2_D0)         , A(IIS2DO)           , GPIO(167) ),
PIN(NFD13          , A(NFD13)          , A(SD2_RST)        , A(IIS2CLK)          , GPIO(168) ),
PIN(NFD14          , A(NFD14)          , A(SD2_CMD)        , A(IIS2LRCK)         , GPIO(169) ),
PIN(NFD15          , A(NFD15)          , A(SD2_CLK)        , A(IIS2MCK)          , GPIO(170) ),
PIN(CCIRCK0        , A(CCIRCK0)        , A(CLK_CP1_DSP)    , CP2_ARM_GPIO(0)     , GPIO(171) ),
PIN(CCIRCK1        , A(CCIRCK1)        , A(CLK_CP0_DSP)    , CP2_ARM_GPIO(1)     , GPIO(172) ),
PIN(CCIRMCLK       , A(CCIRMCLK)       , A(CLK_AUX2)       , CP2_ARM_GPIO(2)     , GPIO(173) ),
PIN(CCIRHS         , A(CCIRHS)         , 0                 , CP2_ARM_GPIO(3)     , GPIO(174) ),
PIN(CCIRVS         , A(CCIRVS)         , 0                 , 0                   , GPIO(175) ),
PIN(CCIRD0         , A(CCIRD0)         , A(CP0_DTDO)       , A(CP_MTDO)          , GPIO(176) ),
PIN(CCIRD1         , A(CCIRD1)         , A(CP0_DTDI)       , A(CP_MTDI)          , GPIO(177) ),
PIN(CCIRD2         , A(CCIRD2)         , A(CP0_DTCK)       , A(CP_MTCK)          , GPIO(178) ),
PIN(CCIRD3         , A(CCIRD3)         , A(CP0_DTMS)       , A(CP_MTMS)          , GPIO(179) ),
PIN(CCIRD4         , A(CCIRD4)         , A(CP0_DRTCK)      , A(CP_MTRST_N)       , GPIO(180) ),
PIN(CCIRD5         , A(CCIRD5)         , A(CP1_DTDO)       , CP1_ARM_GPIO(3)     , GPIO(181) ),
PIN(CCIRD6         , A(CCIRD6)         , A(CP1_DTDI)       , CP0_ARM_GPIO(3)     , GPIO(182) ),
PIN(CCIRD7         , A(CCIRD7)         , A(CP1_DTCK)       , 0                   , GPIO(183) ),
PIN(CCIRD8         , A(CCIRD8)         , 0                 , 0                   , GPIO(184) ),
PIN(CCIRD9         , A(CCIRD9)         , 0                 , 0                   , GPIO(185) ),
PIN(CCIRRST        , A(CCIRRST)        , A(CP1_DTMS)       , 0                   , GPIO(186) ),
PIN(CCIRPD1        , A(CCIRPD1)        , A(CP1_DRTCK)      , 0                   , GPIO(187) ),
PIN(CCIRPD0        , A(CCIRPD0)        , 0                 , 0                   , GPIO(188) ),
PIN(SCL0           , A(SCL0)           , 0                 , 0                   , GPIO(189) ),
PIN(SDA0           , A(SDA0)           , 0                 , 0                   , GPIO(190) ),
PIN(KEYOUT0        , A(KEYOUT0)        , 0                 , CP1_ARM_GPIO(0)     , GPIO(191) ),
PIN(KEYOUT1        , A(KEYOUT1)        , 0                 , CP1_ARM_GPIO(1)     , GPIO(192) ),
PIN(KEYOUT2        , A(KEYOUT2)        , A(VBC_DAC_IISLRCK), CP1_ARM_GPIO(2)     , GPIO(193) ),
PIN(KEYIN0         , A(KEYIN0)         , 0                 , CP0_ARM_GPIO(0)     , GPIO(199) ),
PIN(KEYIN1         , A(KEYIN1)         , 0                 , CP0_ARM_GPIO(1)     , GPIO(200) ),
PIN(KEYIN2         , A(KEYIN2)         , A(PWMB)           , CP0_ARM_GPIO(2)     , GPIO(201) ),
PIN(SCL2           , A(SCL2)           , 0                 , 0                   , GPIO(207) ),
PIN(SDA2           , A(SDA2)           , 0                 , 0                   , GPIO(208) ),
PIN(CLK_AUX0       , A(CLK_AUX0)       , A(PROBE_CLK)      , A(VBC_DAC_IISLRCK)  , GPIO(209) ),
PIN(IIS0DI         , A(IIS0DI)         , A(WIFI_ACTIVE)    , A(FM_RXIQD0)        , GPIO(210) ),
PIN(IIS0DO         , A(IIS0DO)         , A(BT_ACTIVE)      , A(FM_RXIQD1)        , GPIO(211) ),
PIN(IIS0CLK        , A(IIS0CLK)        , A(BT_STATUS)      , A(WIFI_AGC_GAIN0)   , GPIO(212) ),
PIN(IIS0LRCK       , A(IIS0LRCK)       , 0                 , A(WIFI_AGC_GAIN1)   , GPIO(213) ),
PIN(IIS0MCK        , A(IIS0MCK)        , A(CLK_AUX1)       , A(PWMD)             , GPIO(214) ),
PIN(IIS1DI         , A(IIS1DI)         , A(VBC_ADC_IISDI)  , A(ORP_TDO)          , GPIO(215) ),
PIN(IIS1DO         , A(IIS1DO)         , A(VBC_DAC_IISDO)  , A(ORP_TDI)          , GPIO(216) ),
PIN(IIS1CLK        , A(IIS1CLK)        , A(VBC_ADC_IISCLK) , A(ORP_TCK)          , GPIO(217) ),
PIN(IIS1LRCK       , A(IIS1LRCK)       , A(VBC_ADC_IISLRCK), A(ORP_TMS)          , GPIO(218) ),
PIN(IIS1MCK        , A(IIS1MCK)        , A(VBC_DAC_IISCLK) , A(ORP_TRST_N)       , GPIO(219) ),
PIN(MTDO           , A(MTDO)           , A(CP0_DTDO)       , A(CP1_DTDO)         , GPIO(225) ),
PIN(MTDI           , A(MTDI)           , A(CP0_DTDI)       , A(CP1_DTDI)         , GPIO(226) ),
PIN(MTCK           , A(MTCK)           , A(CP0_DTCK)       , A(CP1_DTCK)         , GPIO(227) ),
PIN(MTMS           , A(MTMS)           , A(CP0_DTMS)       , A(CP1_DTMS)         , GPIO(228) ),
PIN(MTRST_N        , A(MTRST_N)        , A(CP0_DRTCK)      , A(CP1_DRTCK)        , GPIO(229) ),
};

struct sci_gpio_chip {
	struct gpio_chip chip;

	uint32_t base_addr;
	uint32_t group_offset;

	 uint32_t(*read_reg) (uint32_t addr);
	void (*write_reg) (uint32_t value, uint32_t addr);
	void (*set_bits) (uint32_t bits, uint32_t addr);
	void (*clr_bits) (uint32_t bits, uint32_t addr);
};

static u32 dump_enable = 0;

static int sci_gpio_read(struct gpio_chip *chip, uint32_t offset, uint32_t reg)
{
	struct sci_gpio_chip *sci_gpio = to_sci_gpio(chip);
	int group = offset / GPIO_GROUP_NR;
	int bitof = offset & (GPIO_GROUP_NR - 1);
	int addr = sci_gpio->base_addr + sci_gpio->group_offset * group + reg;
	int value = sci_gpio->read_reg(addr) & GPIO_GROUP_MASK;

	return (value >> bitof) & 0x1;
}

bool sc_suspend_dump_enabled(void)
{
        return dump_enable != 0;
}

#define SEQ_PRINTF(s,  fmt, args...) \
	do {					\
		if( NULL == s)		\
			printk(fmt, ##args);		\
		else			\
			seq_printf(s, fmt, ##args);	\
	}while(0)


int gpio_show_all(struct seq_file *s, void *v)
{
	int i;

	SEQ_PRINTF(s, "ADDR PIN             MODE WPUS DS WP SLP     GPIO    DIR VAL IE NOTE\n");

	for (i=0; i < ARRAY_SIZE(PINS); i++)
	{
		unsigned int val = __raw_readl( CTL_PIN_BASE + PINS[i][0] );

		unsigned int wpus     = (val & BIT(12)) >> 12;
		unsigned int drv      = (val & ((BIT(9) | BIT(8)))) >> 8;
		unsigned int func_wpu = (val & BIT(7)) >> 7;
		unsigned int func_wpd = (val & BIT(6)) >> 6;

		unsigned int slp_with = (val & (BIT(13) | BIT(14) | BIT(15) | BIT(16)));

		unsigned int sel = (val & ((BIT(5) | BIT(4)))) >> 4;
		unsigned int wpu = (val & BIT(3)) >> 3;
		unsigned int wpd = (val & BIT(2)) >> 2;
		unsigned int ie  = (val & BIT(1)) >> 1;
		unsigned int oe  = val & BIT(0);

		unsigned int v = PINS[i][sel + 2];

		if ((flags == 1) && (sel == 0) && (v > 0xff)) continue;

		// ADDR PINNAME MODE WPUS DS WP[WPU|WPD] SLP[WPU|WPD|NUL IE|OE|Z]
		SEQ_PRINTF(s, "%04X %-16s [%u]    %c %u  %2s %c %2s %2s",
			(unsigned short)PINS[i][0], (const char *)PINS[i][1], sel,
			wpus ? 'Y' : ' ',
			drv,
			(func_wpu ? "PU" : func_wpd ? "PD" : ""),
			(slp_with == BIT_PIN_SLP_AP ? 'A' : slp_with == BIT_PIN_SLP_CP0 ? '0' :
				slp_with == BIT_PIN_SLP_CP1 ? '1' : slp_with == BIT_PIN_SLP_CP2 ? '2' :
				slp_with == 0 ? ' ' : 'x'),
			(wpu ? "PU" : wpd ? "PD" : ""),
			(ie ? "IE" : oe ? "OE" : ""));

		if (v > 0 && v <= 0xff) {
			SEQ_PRINTF(s, " GPIO%-3u ", v);

			if (gpio_is_valid(v)) {
				struct gpio_chip *chip = gpio_to_chip(v);
				if (v >= chip->base)
				{
					unsigned offset = v - chip->base;
					int dir, dat, ie;

					// DIR VALUE IE NOTE
					dir = sci_gpio_read(chip, offset, REG_GPIO_DIR);
					SEQ_PRINTF(s, "%s ", dir ? "out" : "in ");

					dat = sci_gpio_read(chip, offset, REG_GPIO_DATA);
					SEQ_PRINTF(s, "%s  ", dat ? "hi" : "lo");

					ie = sci_gpio_read(chip, offset, REG_GPIO_IE);
					SEQ_PRINTF(s, ie ? "Y  " : "   ");

					SEQ_PRINTF(s, "%s", gpiochip_is_requested(chip, offset));
				}
			}

#define ENTRY(a, b) \
		} else if (v >= (a) && v <= ((a) | 0xff)) { \
			SEQ_PRINTF(s, " " #b "%u", v & 0xff)

		ENTRY(0x100, CP0_ARM_GPIO);
		ENTRY(0x200, CP1_ARM_GPIO);
		ENTRY(0x300, CP2_ARM_GPIO);
		ENTRY(0x400, CP0_DSP_GPO);
		ENTRY(0x500, CP1_DSP_GPO);
		ENTRY(0x600, CP2_DSP_GPO);
		ENTRY(0x700, CP0_RFCTL);
		ENTRY(0x800, CP1_RFCTL);
		ENTRY(0x900, CP2_RFCTL);

		} else {
			SEQ_PRINTF(s, " %s", (const char *)v);
		}

		SEQ_PRINTF(s, "\n");
	}

	return 0;
}

static int gpio_open(struct inode *inode, struct file *file)
{
	flags = (int)inode->i_private;

	return single_open(file, gpio_show_all, inode->i_private);
}

static const struct file_operations gpio_fops = {
	.open		= gpio_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init gpio_debugfs_init(void)
{
	static struct dentry *debug_root = NULL;

	debug_root = debugfs_create_dir("htc_gpio", NULL);

	if (IS_ERR_OR_NULL(debug_root)) {
		pr_err("%s return %p\n", __FUNCTION__, debug_root);
		return PTR_ERR(debug_root);
	}

	debugfs_create_file("all", S_IRUSR | S_IRGRP, debug_root, (void *)0, &gpio_fops);
	debugfs_create_file("gpio", S_IRUSR | S_IRGRP, debug_root, (void *)1, &gpio_fops);
	debugfs_create_bool("dump", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP , debug_root, &dump_enable);

	return 0;
}

module_init(gpio_debugfs_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver Fu <oliver_fu@htc.com>");
MODULE_DESCRIPTION("gpio debugfs");
