#ifndef _YUSHAN_PLATFORM_SPECIFIC_H_
#define _YUSHAN_PLATFORM_SPECIFIC_H_

/* Help compiling in C++ */
#ifdef __cplusplus
extern "C"{
#endif   /*__cplusplus*/

/*******************************************************************************
################################################################################
#                             C STMicroelectronics
#    Reproduction and Communication of this document is strictly prohibited
#      unless specifically authorized in writing by STMicroelectronics.
#------------------------------------------------------------------------------
#                             Imaging Division
################################################################################
File Name:	Yushan_Platform_Specific.h
Author:		Rajdeep Patel
Description:Platform specific header file.
********************************************************************************/
#define SHOW_DEBUG_MSG 0
#include "yushan_registermap.h"
#include "Yushan_API.h"
//#include <plat/gpio-nomadik.h>
//#include <plat/pincfg.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/poll.h>

static bool bInNormalMode = true;
static bool bShowHTCMsg = false;
//#define CDBGE(fmt, args...) printk("[CAM]" fmt, ##args)
#if 0
#if SHOW_DEBUG_MSG
//#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG(fmt, args...) printk("[CAM]" fmt, ##args)
#else
#define CDBG(fmt, args...) /* Nothing */
#endif
#endif

static uint8_t *yushan_spi_write_addr; /* angie free*/

struct yushan_reg_conf {
	uint16_t addr;
	uint8_t  data;
};

struct yushan_reg_t {
	uint16_t pdpcode_first_addr;
	uint8_t  *pdpcode;
	uint16_t pdpcode_size;

	uint16_t pdpclib_first_addr;
	uint8_t *pdpclib;
	uint16_t pdpclib_size;
	
	uint16_t pdpBootAddr;
	uint16_t pdpStartAddr;

	uint16_t dppcode_first_addr;
	uint8_t *dppcode;
	uint16_t dppcode_size;

	uint16_t dppclib_first_addr;
	uint8_t *dppclib;
	uint16_t dppclib_size;

	uint16_t dppBootAddr;
	uint16_t dppStartAddr;
	
	uint16_t dopcode_first_addr;
	uint8_t *dopcode;
	uint16_t dopcode_size;

	uint16_t dopclib_first_addr;
	uint8_t *dopclib;
	uint16_t dopclib_size;

	uint16_t dopBootAddr;
	uint16_t dopStartAddr;
};

/* Yushan Parameters */
typedef struct {
  uint16_t a_gain;
  uint16_t d_gain;
  uint16_t exp_time;
} rawchip_aec_params_t;

typedef struct {
  uint8_t rg_ratio; /* Q6 format */
  uint8_t bg_ratio; /* Q6 format */
} rawchip_awb_params_t;

typedef struct {
  int update;
  rawchip_aec_params_t aec_params;
  rawchip_awb_params_t awb_params;
} rawchip_update_aec_awb_params_t;

typedef struct {
  uint8_t active_number;
  Yushan_AF_ROI_t sYushanAfRoi[5];
} rawchip_af_params_t;

typedef struct {
  int update;
  rawchip_af_params_t af_params;
} rawchip_update_af_params_t;

struct rawchip_ctrl {
	struct msm_camera_rawchip_info *pdata;
	struct cdev   cdev;

	struct mutex raw_ioctl_get_lock;
	struct mutex raw_ioctl_update_lock;
	int rawchip_init;
};

static struct mutex my_wait_lock;
static struct mutex yushan_init_lock;

typedef enum {
  RAWCHIP_NEWFRAME_ACK_NOCHANGE,
  RAWCHIP_NEWFRAME_ACK_ENABLE,
  RAWCHIP_NEWFRAME_ACK_DISABLE,
} rawchip_newframe_ack_enable_t;

extern int rawchip_enable_spi2();

/* Used by Interrupts */
extern uint32_t		udwProtoInterruptList_Pad0[3];
extern uint32_t		udwProtoInterruptList_Pad1[3];
static int rawchip_intr0, rawchip_intr1;
extern atomic_t interrupt, interrupt2;
extern struct yushan_int_t yushan_int;

static uint8_t interrupt_err_count = 0;
static uint8_t nROI_Number = 0;

/* Interrupt functions */
bool_t Yushan_WaitForInterruptEvent(uint8_t,uint32_t);
bool_t Yushan_WaitForInterruptEvent2(uint8_t bInterruptId , uint32_t udwTimeOut);
void	Yushan_Interrupt_Manager_Pad0(void);
void	Yushan_Interrupt_Manager_Pad1(void);
void	Yushan_ISR(void);

int Yushan_sensor_open_init_testing(void);
int Yushan_sensor_open_init(void);
void DumpDOP_Registers();
void DumpDPP_Registers();
void DumpPDP_Registers();
void frame_counter_in_yushan(int);
void Yushan_Power_ON(int);
void Yushan_Power_OFF(void);
void Dump_DxO_Reqiured_Register();
void Dump_DxO_Frame_Count();
void EnablePwr();
void DisablePwr();
void EnableExtClk();
void DisableExtClk();
void EnableRst();
void DisableRst();
void Yushan_dump_all_register(void);
void SPI_Tester(void);
void Yushan_Dump_BB_IPP_Register(void);
void DumpSIARegs();
void Yushan_Init_RawChip(void);
void Yushan_Enable_TX(void);
void Yushan_Disable_TX(void);
void Yushan_Dump_Info(void);
void DoPatternGenerator();
void Yushan_Change_Resolition_To_4X(void);
void Yushan_Change_Resolition_To_Normal(void);

int Yushan_get_AFSU(uint32_t *pAfStatsGreen);
int Yushan_Update_AEC_AWB_Params(rawchip_update_aec_awb_params_t *update_aec_awb_params);
int Yushan_Update_AF_Params(rawchip_update_af_params_t *update_af_params);
int Yushan_Update_3A_Params(rawchip_newframe_ack_enable_t enable_newframe_ack);
static uint8_t *yushan_spi_write_addr; /* angie free*/
static int rawchip_get_interrupt(struct rawchip_ctrl *raw_dev, void __user *arg);
static int rawchip_get_af_status(struct rawchip_ctrl *raw_dev, void __user *arg);
static int rawchip_update_aec_awb_params(struct rawchip_ctrl *raw_dev, void __user *arg);
static int rawchip_update_af_params(struct rawchip_ctrl *raw_dev, void __user *arg);
static int rawchip_update_3a_params(struct rawchip_ctrl *raw_dev, void __user *arg);
static int rawchip_update_debug_level(struct rawchip_ctrl *raw_dev, void __user *arg);
uint8_t Yushan_parse_interrupt(void);
static unsigned int rawchip_fops_poll(struct file *filp, struct poll_table_struct *pll_table);
void ConfigPinsToSuspend();
void ConfigPinsToResume();
static int Yushan_sensor_open_init_worker(void *arg);
static int __init yushan_open_init_thread(void);

/* Structure definitions */
struct msm_AFSU_info {
	Yushan_AF_ROI_t sYushanAfRoi[5];
	uint8_t active_number;
};

struct yushan_spi_ctrl_blk {
	struct spi_device *spi;
	spinlock_t		spinlock;
};

struct yushan_int_t {
	spinlock_t yushan_spin_lock;
	wait_queue_head_t yushan_wait;
	atomic_t frame_count;
};

struct yushan_work {
  struct work_struct work;
};

/* Variable definition */

#define RAWCHIP_GPIO_IRQ  172
#define RAWCHIP_GPIO_IRQ2 174

#define SPI_RAWCHIP_DEVICENAME "spi_rawchip"
#define ENABLE_CLK_GPIO    PIN_CFG(228, ALT_A) /* Enable  clock */
#define DISABLE_CLK_GPIO   PIN_CFG(228, GPIO)  /* Disable clock */

//#if defined(CONFIG_MACH_CSNDUG)
#if 1
#define Power_PIN_1_2V    184
#define Power_PIN_1_8V    185
#else
#define Power_PIN_1_2V AB8500_PIN_GPIO(11)
#define Power_PIN_1_8V AB8500_PIN_GPIO(6)
#endif

#if 1
#define Yushan_Reset_PIN 171
#else
#define Yushan_Reset_PIN PIN_CFG(227, GPIO)
#endif

#define RAW_INT0_SUSPEND PIN_CFG_INPUT(RAWCHIP_GPIO_IRQ, GPIO, PULLDOWN)
#define RAW_INT1_SUSPEND PIN_CFG_INPUT(RAWCHIP_GPIO_IRQ2, GPIO, PULLDOWN)
#define SPI_DO_SUSPEND   PIN_CFG_OUTPUT(215, GPIO, LOW)
#define SPI_CS_SUSPEND   PIN_CFG_OUTPUT(216, GPIO, LOW)
#define SPI_CLK_SUSPEND  PIN_CFG_OUTPUT(217, GPIO, LOW)
#define SPI_DI_SUSPEND   PIN_CFG_OUTPUT(218, GPIO, LOW)

#define SPI_DO_RESUME    PIN_CFG(215, ALT_C)
#define SPI_CS_RESUME    PIN_CFG_OUTPUT(216, GPIO, HIGH)
#define SPI_CLK_RESUME   PIN_CFG(217, ALT_C)
#define SPI_DI_RESUME    PIN_CFG(218, ALT_C)


#define PDP_DISABLE 1
#define PDP_ENABLE 2
#define DPP_DISABLE 3
#define DPP_ENABLE 4
#define DOP_DISABLE 5
#define DOP_PREVIEW_ENABLE 6
#define DOP_VIDEO_ENABLE 7
#define AFSU_DISABLE 8
#define AFSU_ENABLE 9
#define DENOISE_DISABLE 0x11
#define DENOISE_ENABLE 0x12
#define BLACKLEVEL_DISABLE 0x13 //C
#define BLACKLEVEL_ENABLE 0x14 //D
#define DEADPIXEL_DISABLE 0x15
#define DEADPIXEL_ENABLE 0x16
#define RUN_AFSU 0x17
#define ALL_IP_OFF 0x18
#define ALL_IP_ON 0x19

#define Test_GPIO_MODE 0
#define NEW_REWORK_EVM 0

//#ifdef CONFIG_SENSOR_USE_OV5693
#if 1
#if defined (CONFIG_MACH_CP5VEDWG) || defined (CONFIG_MACH_CP5VEDTU)
#define BYPASS_DXO 1
#else
#define BYPASS_DXO 0
#endif
#define USE_SC8830 1
#else
#define BYPASS_DXO 0
#define USE_U8500_3H2 0
#define USE_U8500_4E5 1
#endif

#define USE_NEW_YUSHAN_LAUNCH_WAY    1
#define USE_U8500_4E5_LOW_MIPI_SPEED 0
#define YUSHAN_VERSION 56
#define DxO       1
#define COLOR_BAR 0
#define ENABLE_TEST_PATTERN 0
#define yushan_MAX_ALLOCATE 100000

#define PDPMode 1
#define DPPMode 3
#define DOPMode 1

#define DATA_EMBEDDED_LINE     0x12
/* Sensor Configuration */
#define RAW10_DATA_TYPE        0x2b
#if USE_U8500_3H2
#define PREVIEW_SENSOR_WIDTH   1640
#define PREVIEW_SENSOR_HEIGHT  1232
#define DATA_LANE              2
#define EXT_CLOCK              0x133333
#define SPI_CLOCK              0x140000
#define MIPI_DATA_RATE         0x3ca
#define FRAME_BLANK            1858
#define MIPI_DATA_TYPE_RAW10   0x2b
#define LINE_BLANK             3478
#define PIXEL_FORMAT           0x0A0A
#define IMGCHAR_XEvenInc       0x1
#define IMGCHAR_XOddInc        0x3
#define IMGCHAR_YEvenInc       0x1
#define IMGCHAR_YOddInc        0x1
#define IMGCHAR_BINNING        0x22
#elif USE_U8500_4E5
#define SENSOR_WIDTH_4X        1104
#define LINE_BLANKING_4X       1736
#define SENSOR_HEIGHT_4X       658
#define FRAME_BLANKING_4X      1424
#define PIXEL_FORMAT_4X        0x0A0A
#define IMGCHAR_XEVENINC_4X    0x1
#define IMGCHAR_XOddInc_4X     0x3
#define IMGCHAR_YEvenInc_4X    0x1
#define IMGCHAR_YOddInc_4X     0x3
#define IMGCHAR_BINNING_4X     0x22
#define IMGCHAR_X_START_4X     200
#define IMGCHAR_X_END_4X       2407
#define IMGCHAR_Y_START_4X     322
#define IMGCHAR_Y_END_4X       1637

#define PREVIEW_SENSOR_WIDTH   2608
#define PREVIEW_SENSOR_HEIGHT  1960
#define DATA_LANE              2
#define EXT_CLOCK              0x133333
#define SPI_CLOCK              0x1B0000
//#define SPI_CLOCK              0x00080000
#define MIPI_DATA_RATE         845
#define FRAME_BLANK            122
#define MIPI_DATA_TYPE_RAW10   0x2b
#define LINE_BLANK             976
#define PIXEL_FORMAT           0x0A0A
#define IMGCHAR_XEVENINC       0x1
#define IMGCHAR_XOddInc        0x1
#define IMGCHAR_YEvenInc       0x1
#define IMGCHAR_YOddInc        0x1
#define IMGCHAR_BINNING        0x11
#elif USE_SC8830
#define IMAGE_ORIENTATION_180  3
#define IMAGE_ORIENTATION      0
#define PREVIEW_SENSOR_WIDTH   1640
#define PREVIEW_SENSOR_HEIGHT  1232
#define LINE_BLANK             1800
#define FRAME_BLANK            1280
#define DATA_LANE              4
#define EXT_CLOCK              0x180000
#define SPI_CLOCK              0x180000
//#define MIPI_DATA_RATE         832
#define MIPI_DATA_RATE         648
#define MIPI_DATA_TYPE_RAW10   0x2b
#define PIXEL_FORMAT           0x0A0A
#define IMGCHAR_XEVENINC       0x1
#define IMGCHAR_XOddInc        0x1
#define IMGCHAR_YEvenInc       0x1
#define IMGCHAR_YOddInc        0x1
#define IMGCHAR_BINNING        0x11

#define SENSOR_WIDTH_4X        1296
#define SENSOR_HEIGHT_4X       942
//#define LINE_BLANKING_4X     516
#define LINE_BLANKING_4X       300
#define FRAME_BLANKING_4X      40
#define PIXEL_FORMAT_4X        0x0A0A
#define IMGCHAR_XEVENINC_4X    0x1
#define IMGCHAR_XOddInc_4X     0x7
#define IMGCHAR_YEvenInc_4X    0x1
#define IMGCHAR_YOddInc_4X     0x7
#define IMGCHAR_BINNING_4X     0x44

#define IMGCHAR_X_START_4X     0
#define IMGCHAR_X_END_4X       3279
#define IMGCHAR_Y_START_4X     212
#define IMGCHAR_Y_END_4X       2251
#else
#define PREVIEW_SENSOR_WIDTH   1304
#define PREVIEW_SENSOR_HEIGHT  980
#define DATA_LANE              2
#endif

#define RAWCHIP_INT_TYPE_ERROR (0x01<<0)
#define RAWCHIP_INT_TYPE_NEW_FRAME (0x01<<1)
#define RAWCHIP_INT_TYPE_PDP_EOF_EXECCMD (0x01<<2)
#define RAWCHIP_INT_TYPE_DPP_EOF_EXECCMD (0x01<<3)
#define RAWCHIP_INT_TYPE_DOP_EOF_EXECCMD (0x01<<4)
#define RAWCHIP_INT_TYPE_ERROR_FATAL (0x01<<5)


/* Clock */
/* Variable Definitions */
static Yushan_Init_Struct_t	sInitStruct;
static	Yushan_GainsExpTime_t sGainsExpTime;
static	Yushan_DXO_DPP_Tuning_t sDxoDppTuning;
static	Yushan_DXO_PDP_Tuning_t sDxoPdpTuning;
static	Yushan_DXO_DOP_Tuning_t sDxoDopTuning;

/* help compiling in C++ */
#ifdef __cplusplus
}
#endif   /*__cplusplus*/

#endif /* _YUSHAN_PLATFORM_SPECIFIC_H_ */
