/*******************************************************************************
################################################################################
#                             C STMicroelectronics
#    Reproduction and Communication of this document is strictly prohibited
#      unless specifically authorized in writing by STMicroelectronics.
#------------------------------------------------------------------------------
#                             Imaging Division
################################################################################
File Name:	Yushan_API.c
Author:		Rajdeep Patel
Description: Contains API functions to be used by host to Initialize Yushan clocks,
			 regsiters and DXO parameters. Also BOOT the DXO IPs.
********************************************************************************/
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include "yushan_registermap.h"
#include "DxODOP_regMap.h"
#include "DxODPP_regMap.h"
#include "DxOPDP_regMap.h"
#include "Yushan_API.h"
#include "Yushan_Platform_Specific.h"
#include "Yushan_HTC_Specific.h"
#include <linux/mutex.h>

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>

/* Add by Kueihung */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/gpio.h>
//#include <linux/mmio.h>
//#include <linux/gpio/nomadik.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/vmalloc.h>
#include <asm/mach-types.h>
//#include <plat/pincfg.h>
#include <mach/gpio.h>
//#include <mach/devices.h>
#include <mach/hardware.h>
//#include <linux/mfd/ab8500/gpio.h>

#include <linux/spi/spi.h> 
#include <linux/spi/rawchip_spi.h> 
#include <linux/gpio.h>
#include <mach/gpio.h>
//#include <plat/pincfg.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/Yushan_IOCTL_Code.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <asm/mach-types.h>
#include <mach/gpio.h>
#include <linux/kthread.h>
#include <media/rawchip_interface.h>

uint8_t yushan_spi_read(uint16_t reg);
struct msm_AFSU_info afsu_info;
static struct spi_device *rawchip_dev;
int logSPI;
static int rawchip_power_status = 0;

struct yushan_spi_ctrl_blk *yushan_spi_ctrl;
Yushan_Init_Dxo_Struct_t	sDxoStruct;
/* end */

atomic_t interrupt, interrupt2;
int AF_STAT_INT = 0;
struct yushan_int_t yushan_int;

Yushan_ImageChar_t sImageChar;
uint32_t	*udwListOfInterrupts;
//ChenC add for dynamic resolution configuration
Yushan_New_Context_Config_t* 	sYushanAEContextConfig;
Yushan_New_Context_Config_t 	sYushan4E1PreviewContextConfig;
/* Used by Interrupts */
uint32_t	udwProtoInterruptList[3];
uint32_t check_yushan_id;

uint8_t i;
uint8_t pData = 1;
uint32_t yushan_mode = 0;
Yushan_AF_ROI_t sYushanAfRoi[5];
uint32_t pAfStatsGreen[20];
uint8_t active_number = 1;
int32_t M;
volatile bool bIsYushan_Init = false;
static struct rawchip_ctrl *rawchipCtrl;
volatile static bool bYushan_init_function_done = false;
uint32_t error_cnt = 0;
bool bCanDoSnapshot = true;
// for fix run in test first camera fail
extern int board_mfg_mode(void);
static int camera_times = 0;
//end
static uint32_t bypass_dxo = BYPASS_DXO;

static int32_t Yushan_spi_write_table( uint16_t uwIndex , uint16_t uwCount , uint8_t * pData);
void yushan_send_clk(void);
#define CDBG(fmt, args...) if (0) printk("[CAM]" fmt, ##args)
#define CDBGE(fmt, args...) printk("[CAM] error" fmt, ##args)


static uint32_t v_image_orientation = IMAGE_ORIENTATION;
static uint32_t v_preview_sensor_width = PREVIEW_SENSOR_WIDTH;
static uint32_t v_preview_sensor_height = PREVIEW_SENSOR_HEIGHT;
static uint32_t v_line_blank = LINE_BLANK;
static uint32_t v_frame_blank = FRAME_BLANK;
static uint32_t v_data_line = DATA_LANE;
static uint32_t v_ext_clock = EXT_CLOCK;
static uint32_t v_mipi_data_rate = MIPI_DATA_RATE;
static uint32_t v_spi_clock = SPI_CLOCK;

static uint32_t v_imgchar_xeveninc = IMGCHAR_XEVENINC;
static uint32_t v_imgchar_xoddinc = IMGCHAR_XOddInc;
static uint32_t v_imgchar_yeveninc = IMGCHAR_YEvenInc;
static uint32_t v_imgchar_yoddinc = IMGCHAR_YOddInc;
static uint32_t v_imgchar_binning = IMGCHAR_BINNING;
static uint32_t v_pixel_format = PIXEL_FORMAT;

/*for gain setting*/
static uint32_t v_uwAnalogGainCodeGR		= 0x20;
static uint32_t v_uwAnalogGainCodeR		= 0x20;
static uint32_t v_uwAnalogGainCodeB		= 0x20;
static uint32_t v_uwPreDigGainGR		= 0x100;
static uint32_t v_uwPreDigGainR			= 0x100;
static uint32_t v_uwPreDigGainB			= 0x100;
static uint32_t v_uwExposureTime		= 0x200;
static uint32_t v_bRedGreenRatio		= 0x40;
static uint32_t v_bBlueGreenRatio		= 0x40;

static uint32_t v_uwFlashPreflashRating 	= 0;   
static uint32_t v_bFocalInfo			= 0;   

static uint32_t v_bDeadPixelCorrectionLowGain	= 0x80;
static uint32_t v_bDeadPixelCorrectionMedGain	= 0x80;
static uint32_t v_bDeadPixelCorrectionHiGain	= 0x80;

static uint32_t v_bEstimationMode		= 1;
static uint32_t v_bSharpness			= 0x10;
static uint32_t v_bDenoisingLowGain		= 0x70;
static uint32_t v_bDenoisingMedGain		= 0x70;
static uint32_t v_bDenoisingHiGain		= 0x70;
static uint32_t v_bNoiseVsDetailsLowGain	= 0x90;
static uint32_t v_bNoiseVsDetailsMedGain	= 0x90;
static uint32_t v_bNoiseVsDetailsHiGain		= 0x90;
static uint32_t v_bTemporalSmoothing		= 0x26;

static uint32_t v_imgchar_uwxaddrstart			= 0;
static uint32_t v_imgchar_uwyaddrstart			= 0;
static uint32_t v_imgchar_uwxaddrend			= PREVIEW_SENSOR_WIDTH - 1;
static uint32_t v_imgchar_uwyaddrend			= PREVIEW_SENSOR_HEIGHT - 1;

static struct yushan_reg_t * yushan_regs = NULL;
int front_main = 0; //main 0; front 1
static int last_width = 0;
static int yushan_init_in_progress = 0;

DEFINE_MUTEX(spi_rw_lock);
/*******************************************************************
Yushan_WaitForInterruptEvent:
Waiting for Interrupt and adding the same in the Interrupt list.
*******************************************************************/
bool_t Yushan_WaitForInterruptEvent(uint8_t bInterruptId , uint32_t udwTimeOut)
{
	int					 counterLimit;
	/* int counter = 0;*/
	bool_t				fStatus = 0, INTStatus = 0;
#ifdef API_DUAL_PIN
	bool_t		bInterruptPad = 0;
	uint32_t	*pudwProtoInterruptList;
#endif

	switch (udwTimeOut) {
	case TIME_5MS :
		counterLimit = 100 ;
		break;
	case TIME_10MS :
		counterLimit = 200 ;
		break;
	case TIME_20MS :
		counterLimit = 400 ;
		break;
	case TIME_50MS :
		counterLimit = 1000 ;
		break;
/* 	case TIME_200MS :
		counterLimit = 4000 ;
		break; */
	default :
		counterLimit = 50 ;
		break;
	}

	/* init_waitqueue_head(&yushan_int.yushan_wait);*/
	wait_event_timeout(yushan_int.yushan_wait,
	atomic_read(&interrupt),
		counterLimit/200);/* counterLimit/200 */
	CDBG(" %s end interrupt: 0x%x; interrupt id:0x%x wait\n", __func__, atomic_read(&interrupt), bInterruptId);
	if (atomic_read(&interrupt)) {
		INTStatus = 1;
		atomic_set(&interrupt, 0);
	}

#ifdef API_DUAL_PIN
		/* First check which Pad, the current interrupt belongs to. */
		bInterruptPad = Yushan_Check_Pad_For_IntrID(bInterruptId);

		if(bInterruptPad==INTERRUPT_PAD_0)
		{
			/* ISR will create a fresh list of asserted event (for Host) and update the local ProtoIntrList. */
			Yushan_Interrupt_Manager_Pad0();
			
			pudwProtoInterruptList	= (uint32_t *)udwProtoInterruptList_Pad0;
		}
		else
		{
			/* ISR will create a fresh list of asserted event (for Host) and update the local ProtoIntrList. */
			Yushan_Interrupt_Manager_Pad1();
			
			pudwProtoInterruptList	= (uint32_t *)udwProtoInterruptList_Pad1;
		}
	fStatus = Yushan_CheckForInterruptIDInList(bInterruptId, pudwProtoInterruptList);		/* Work on ProtoIntrList */
	CDBG(" %s Yushan_CheckForInterruptIDInList:%d \n", __func__, fStatus);
	/* Interrupt has been served, so remove the same from the InterruptList. */
	Yushan_AddnRemoveIDInList(bInterruptId, pudwProtoInterruptList, DEL_INTR_FROM_LIST);
#else
	Yushan_ISR();
	fStatus = Yushan_CheckForInterruptIDInList(bInterruptId, udwProtoInterruptList);		/* Work on ProtoIntrList */
	/* Interrupt has been served, so remove the same from the InterruptList. */
	Yushan_AddnRemoveIDInList(bInterruptId, udwProtoInterruptList, DEL_INTR_FROM_LIST);
#endif 

	if ((fStatus) || (INTStatus))
		return SUCCESS;
	else
		return FAILURE;
}

bool_t Yushan_WaitForInterruptEvent2(uint8_t bInterruptId , uint32_t udwTimeOut)
{
	int					 counterLimit;
	/* int counter = 0;*/
	bool_t				fStatus = 0, INTStatus = 0;
#ifdef API_DUAL_PIN
	bool_t			 bInterruptPad = 0;
	uint32_t			*pudwProtoInterruptList;
#endif

	switch (udwTimeOut) {
	case TIME_5MS :
		counterLimit = 100 ;
		break;
	case TIME_10MS :
		counterLimit = 200 ;
		break;
	case TIME_20MS :
		counterLimit = 400 ;
		break;
	case TIME_50MS :
		counterLimit = 1000 ;
		break;
/* 	case TIME_200MS :
		counterLimit = 4000 ;
		break; */
	default :
		counterLimit = 50 ;
		break;
	}

	CDBG(" %s begin interrupt wait\n", __func__);
	/* init_waitqueue_head(&yushan_int.yushan_wait);*/
	wait_event_timeout(yushan_int.yushan_wait,
	atomic_read(&interrupt),
		counterLimit/200);/* counterLimit/200 */
	CDBG(" %s end interrupt: %d; interrupt id:%d wait\n", __func__, atomic_read(&interrupt), bInterruptId);
	if (atomic_read(&interrupt)) {
		INTStatus = 1;
		atomic_set(&interrupt, 0);
	}

#ifdef API_DUAL_PIN
		/* First check which Pad, the current interrupt belongs to. */
		bInterruptPad = Yushan_Check_Pad_For_IntrID(bInterruptId);

		if(bInterruptPad==INTERRUPT_PAD_0)
		{
			/* ISR will create a fresh list of asserted event (for Host) and update the local ProtoIntrList. */
			Yushan_Interrupt_Manager_Pad0();
			
			pudwProtoInterruptList	= (uint32_t *)udwProtoInterruptList_Pad0;
		}
		else
		{
			/* ISR will create a fresh list of asserted event (for Host) and update the local ProtoIntrList. */
			Yushan_Interrupt_Manager_Pad1();
			
			pudwProtoInterruptList	= (uint32_t *)udwProtoInterruptList_Pad1;
		}
	fStatus = Yushan_CheckForInterruptIDInList(bInterruptId, pudwProtoInterruptList);		/* Work on ProtoIntrList */
	CDBG(" %s Yushan_CheckForInterruptIDInList:%d \n", __func__, fStatus);
	/* Interrupt has been served, so remove the same from the InterruptList. */
	Yushan_AddnRemoveIDInList(bInterruptId, pudwProtoInterruptList, DEL_INTR_FROM_LIST);
#else
	Yushan_Interrupt_Manager_Pad1();
	fStatus = Yushan_CheckForInterruptIDInList(bInterruptId, udwProtoInterruptList);		/* Work on ProtoIntrList */
	CDBG(" %s Yushan_CheckForInterruptIDInList:%d \n", __func__, fStatus);
	/* Interrupt has been served, so remove the same from the InterruptList. */
	Yushan_AddnRemoveIDInList(bInterruptId, udwProtoInterruptList, DEL_INTR_FROM_LIST);
#endif 

	CDBG(" %s Del Yushan_CheckForInterruptIDInList:%d \n", __func__, fStatus);
	if ((fStatus) || (INTStatus))
		return SUCCESS;
	else
		return FAILURE;
}

void Yushan_Interrupt_Manager_Pad1()
{
	uint8_t		bCurrentInterruptID = 0;
	uint8_t		bAssertOrDeassert=0, bInterruptWord = 0;
	// The List consists of the Current asserted events. Have to be assignmed memory by malloc.
	// and set every element to 0.
	uint32_t	*udwListOfInterrupts;

	//
	udwListOfInterrupts	= (uint32_t *) kmalloc(12, GFP_KERNEL);

	// Call Yushan_Intr_Status_Read  for interrupts and add the same to the udwListOfInterrupts.
	// bInterruptID = Yushan_ReadIntr_Status();
	Yushan_Intr_Status_Read ((uint8_t *)udwListOfInterrupts, INTERRUPT_PAD_1);
	
	// Clear InterruptStatus
	Yushan_Intr_Status_Clear((uint8_t *) udwListOfInterrupts);

	// Adding the Current Interrupt asserted to the Proto List
	while(bCurrentInterruptID<TOTAL_INTERRUPT_COUNT)
	{
		bAssertOrDeassert = ((udwListOfInterrupts[bInterruptWord])>>(bCurrentInterruptID%32))&0x01;

		if(bAssertOrDeassert)
			Yushan_AddnRemoveIDInList((uint8_t)(bCurrentInterruptID+1), udwProtoInterruptList, ADD_INTR_TO_LIST);

			CDBG(" %s:bCurrentInterruptID:%d\n",__func__, bCurrentInterruptID+1);

		bCurrentInterruptID++;

		if(bCurrentInterruptID%32==0)
			bInterruptWord++;

	}	
	kfree(udwListOfInterrupts);
}

static irqreturn_t yushan_irq_handler(int irq, void *dev_id){
	unsigned long flags;
  
  if (bIsYushan_Init) {
		spin_lock_irqsave(&yushan_int.yushan_spin_lock,flags);
		atomic_set(&interrupt, 1);
		CDBG("kuei, %s after detect gpio%d INT, interrupt:%d\n",__func__, RAWCHIP_GPIO_IRQ, atomic_read(&interrupt));
		wake_up(&yushan_int.yushan_wait);
		spin_unlock_irqrestore(&yushan_int.yushan_spin_lock,flags);
	} else {
		atomic_set(&interrupt, 1);
	}
	return IRQ_HANDLED;
}

static irqreturn_t yushan_irq_handler2(int irq, void *dev_id){
	unsigned long flags;
	
	if (bIsYushan_Init) {
		spin_lock_irqsave(&yushan_int.yushan_spin_lock,flags);
		atomic_set(&interrupt2, 1);
		CDBG("kuei, %s after detect gpio%d INT, interrupt:%d \n",__func__, RAWCHIP_GPIO_IRQ2, atomic_read(&interrupt));
		wake_up(&yushan_int.yushan_wait);
		spin_unlock_irqrestore(&yushan_int.yushan_spin_lock,flags);
	} else {
		atomic_set(&interrupt2, 1);
	}	
	return IRQ_HANDLED;
}

static int yushan_create_irq(void){
	unsigned int irq;
	irq = (unsigned int)gpio_to_irq(RAWCHIP_GPIO_IRQ);
	CDBG("kuei, yushan_create_irq\n");
	init_waitqueue_head(&yushan_int.yushan_wait);
	// udwListOfInterrupts	= (uint32_t *) kmalloc(96, GFP_KERNEL);//malloc(96); 
 	return request_irq(irq, yushan_irq_handler, IRQF_TRIGGER_RISING, "yushan_irq", 0);
}

static int yushan_create_irq2(void){
	CDBG("kuei, yushan_create_irq2\n");
	unsigned int irq2;
	irq2 = (unsigned int)gpio_to_irq(RAWCHIP_GPIO_IRQ2);
	init_waitqueue_head(&yushan_int.yushan_wait);
 	return  request_irq(irq2, yushan_irq_handler2, IRQF_TRIGGER_RISING, "yushan_irq2", 0);
}

static int PDP_enable(uint8_t	enable)
{
	uint8_t	bSpiData;
	bool_t fStatus = SUCCESS;

	CDBG("[kuei]%s:%d\n",__func__, enable); 

	if(enable)
		bSpiData = 0x01;
	else
		bSpiData = 0x00;
			
	fStatus = SPI_Write(DXO_PDP_BASE_ADDR + DxOPDP_mode_7_0, 1, (uint8_t*)(&bSpiData));
	SPI_Write(DXO_PDP_BASE_ADDR + DxOPDP_execCmd, 1, &pData);	
	return fStatus;
}

static int black_level_enable(uint8_t	enable)
{
	uint8_t	bSpiData;
	bool_t fStatus = SUCCESS;

	CDBG("kuei, %s:%d\n",__func__, enable); 

	SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_mode_7_0,1,&bSpiData);

	if(enable == 0)
		bSpiData = bSpiData |(1<<3); 
	else if (enable == 1)
		bSpiData = bSpiData &(~ (1<<3));			
	fStatus = SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_mode_7_0, 1, (uint8_t*)(&bSpiData));
	SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_execCmd, 1, &pData);	
	return fStatus;
}

static int dead_pixel_enable(uint8_t	enable) {
	uint8_t	bSpiData;
	bool_t fStatus = SUCCESS;

	CDBG("[kuei]%s:%d\n",__func__, enable); 

	SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_mode_7_0,1,&bSpiData);

	if(enable == 0)
		bSpiData = bSpiData |(1<<4); 
	else if (enable == 1)
		bSpiData = bSpiData &(~ (1<<4));				
	fStatus = SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_mode_7_0, 1, (uint8_t*)(&bSpiData));
	SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_execCmd, 1, &pData);	
	return fStatus;
}

static int DPP_enable(uint8_t	enable) {
	uint8_t	bSpiData;
	bool_t fStatus = SUCCESS;
	uint32_t udwSpiData;	

	CDBG("[kuei]%s:%d\n",__func__, enable); 

	if(enable)
		bSpiData = 0x03;
	else
		bSpiData = 0x00;
	
	udwSpiData = 0x10000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);
	/* Setting DXO mode */
	SPI_Write(DXO_DPP_BASE_ADDR+DxODPP_mode_7_0-0x8000, 1, &bSpiData);
	udwSpiData = 0x18000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);
	/* Giving executing command, so that the programmed mode will be executed. */
	SPI_Write(DXO_DPP_BASE_ADDR+DxODPP_execCmd-0x10000, 1, &pData);

	/* Restoring the SpiBaseAddress to 0x08000. */
	udwSpiData = 0x8000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);
  
	return fStatus;
}

static int DOP_enable(uint8_t	enable) {
	uint8_t	bSpiData;
	bool_t fStatus = SUCCESS;

	CDBG("[kuei]%s:%d\n",__func__, enable); 

	if(enable == 1)
		bSpiData = 0x01; /* still or preview mode */
	else if (enable == 2 ) 		
		bSpiData = 0x02; /* video recording mode */
	else if (enable == 0)
		bSpiData = 0x00;				
	fStatus = SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_mode_7_0, 1, (uint8_t*)(&bSpiData));
	SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_execCmd, 1, &pData);	
	msleep(20);
	fStatus = SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_mode_7_0, 1, (uint8_t*)(&bSpiData));
	SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_execCmd, 1, &pData);		
	return fStatus;
}

static int AFSU_enable(uint8_t enable) {
	uint8_t	bSpiData;
	bool_t fStatus = SUCCESS;

	CDBG("[kuei]%s:%d\n",__func__, enable); 

	SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_mode_7_0,1,&bSpiData);

	if(enable == 0)
		bSpiData = bSpiData |(1<<2); 
	else if (enable == 1)
		bSpiData = bSpiData &(~ (1<<2)); 			
	fStatus = SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_mode_7_0, 1, (uint8_t*)(&bSpiData));
	SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_execCmd, 1, &pData);	
	return fStatus;
}

static int Denoise_enable(uint8_t enable)
{
	uint8_t	bSpiData;
	bool_t fStatus = SUCCESS;

	CDBG("[kuei]%s:%d\n",__func__, enable); 

	SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_mode_7_0,1,&bSpiData);

	if(enable == 0) {
		/* bSpiData = bSpiData |(1<<4); */
		bSpiData = 0x11;
	}		
	else if (enable == 1) {
		/* bSpiData = bSpiData &(~ (1<<4)); */
		bSpiData = 0x1;
	}
	fStatus = SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_mode_7_0, 1, (uint8_t*)(&bSpiData));
	SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_execCmd, 1, &pData);
	msleep(1000);
	return fStatus;
}

void select_mode(uint8_t mode)
{
	/* uint16_t	uwSpiData = 0; */
	switch ( mode )
	{
		case PDP_DISABLE :
#if 0
			uwSpiData = 0xFFFF;
			SPI_Write(YUSHAN_IOR_NVM_INTR_STATUS, 2, (uint8_t *)(&uwSpiData));
#endif
			PDP_enable(0);
			break;
		case PDP_ENABLE :			
			PDP_enable(1) ;
			break;
		case DPP_DISABLE :
			DPP_enable(0);
			break;
		case DPP_ENABLE :
			DPP_enable(1);
			break;
		case DOP_DISABLE :
			DOP_enable(0) ;
			break;
		case DOP_PREVIEW_ENABLE :
			DOP_enable(1) ;
			break;
		case DOP_VIDEO_ENABLE :
			DOP_enable(2);
			break;
		case AFSU_DISABLE :
			AFSU_enable(0);
			break;			
		case AFSU_ENABLE :
			AFSU_enable(1) ;
			break;
		case DENOISE_DISABLE :
			Denoise_enable(0);
			break;
		case DENOISE_ENABLE:
			Denoise_enable(1);
			break;
		case BLACKLEVEL_DISABLE :
			black_level_enable(0);
			break;
		case BLACKLEVEL_ENABLE:
			black_level_enable(1);
			break;
		case DEADPIXEL_DISABLE :
			dead_pixel_enable(0);
			break;
		case DEADPIXEL_ENABLE:
			dead_pixel_enable(1);
			break;	
		  case ALL_IP_OFF:
			Yushan_Update_Commit(0,0,0);
		  	break;
		  case ALL_IP_ON:
			Yushan_Update_Commit(1,3,1);
		  	break;		  	
		default :
			CDBG("kuei,yushan_mode error= %d\n", yushan_mode);			
			break;		
	}	
	msleep(600);
}
#if 0
static ssize_t yushan_mode_change(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	uint32_t tmp = 0;
	tmp = buf[0] - 0x30; /* only get the first char */

#if 0
	if (strcmp(current->comm,"com.android.camera")!=0){
		CDBG("kuei, No permission : not camera ap\n");
		return -EINVAL;
	}
#endif

	yushan_mode = tmp;
	CDBG("kuei, yushan_mode = 0x%x\n", yushan_mode);
	logSPI=0;
	select_mode	(yushan_mode);
	logSPI=0;
	return count;
}
#endif

static ssize_t yushan_mode_change(struct device *dev,
		                              struct device_attribute *attr,
		                              const char *buf, size_t count)
{
	return 0;
}


extern int _Sensor_K_SetMCLK(uint32_t mclk);
#define GPIO_SET(port, value) { gpio_request(port, "main camera"); gpio_direction_output(port, value); gpio_free(port);}
static ssize_t yushan_mode_get(struct device *dev,
		                           struct device_attribute *attr,
		                           const char *buf, size_t count)
{
	GPIO_SET(183, 1);
	GPIO_SET(184, 1);
	GPIO_SET(185, 1);
	_Sensor_K_SetMCLK(24);
	rawchip_poweron();

	rawchip_powerdown();
	_Sensor_K_SetMCLK(0);
	GPIO_SET(183, 0);
	GPIO_SET(184, 0);
	GPIO_SET(185, 0);
	return sprintf(buf, "0x0%x\n", check_yushan_id);
}

static DEVICE_ATTR(rawchip_mode, 644, yushan_mode_get, yushan_mode_change);

static struct kobject *android_yushan = NULL;

static int yushan_sysfs_init(void) {
  int ret = 0;
  CDBG("kuei,%s \n", __func__);
  CDBG("kuei,yushan:kobject creat and add\n");
  android_yushan = kobject_create_and_add("android_yushan", NULL);
  if (android_yushan == NULL) {
    CDBG("kuei,yushan_sysfs_init: subsystem_register failed\n");
    ret = -ENOMEM;
    return ret ;
  }
  CDBG("kuei,yushan:sysfs_create_file\n");

  ret = sysfs_create_file(android_yushan, &dev_attr_rawchip_mode.attr);
  if (ret) {
    CDBG("kuei,yushan_sysfs_init: sysfs_create_file failed\n");
    ret = -EFAULT;
    goto error;
  }

  return ret;

error:
  kobject_del(android_yushan);
  return ret;
}

struct clk* spiClk = NULL;
struct clk* spiParentClk = NULL;

static   struct rawchip_platform_data  *prawchipplat =NULL; 
extern int read_id();
int spi_rawchip_probe(struct spi_device *rawchip) {
	unsigned long check_yushan_id;
	struct resource *res;
	int rc;
	prawchipplat = rawchip->dev.platform_data;
	printk("clean rawchip probe is called\n");
	//printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	//printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	//printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	//printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	//printk("build time %s^^^^^^^^\n", __TIME__);
	//printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	//printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	//printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

	CDBG(" kuei, spi_rawchip_probe\n");
	rawchip_dev = rawchip;
	
	ConfigPinsToSuspend();

	//from yushan
	yushan_spi_ctrl = kzalloc(sizeof(*yushan_spi_ctrl), GFP_KERNEL);
	if (!yushan_spi_ctrl)
		return -ENOMEM;

	yushan_spi_ctrl->spi = rawchip;

	if (yushan_spi_ctrl->spi == NULL) {
		CDBG(KERN_ERR "kuei, yushan_spi_ctrl->spi == NULL !!!\n");
	}
	
	if(yushan_spi_write_addr == NULL) {
		/* 3 bytes one for Read(0x61), Write(0x60), two bytes for read/write address */
		yushan_spi_write_addr = kcalloc(yushan_MAX_ALLOCATE + 3, sizeof(uint8_t), GFP_KERNEL); 
	}
	
	spin_lock_init(&yushan_spi_ctrl->spinlock);
	spin_lock_init(&yushan_int.yushan_spin_lock);
	spi_set_drvdata(rawchip, yushan_spi_ctrl);

	gpio_request(RAWCHIP_GPIO_IRQ, "rawchip");
	gpio_direction_input(RAWCHIP_GPIO_IRQ);
	//gpio_free(RAWCHIP_GPIO_IRQ);
	gpio_request(RAWCHIP_GPIO_IRQ2, "rawchip");
	gpio_direction_input(RAWCHIP_GPIO_IRQ2);
	//gpio_free(RAWCHIP_GPIO_IRQ2);

	rawchip_poweron();//Yushan_Power_ON();
	yushan_sysfs_init();
	rawchip_powerdown();
	//read_id();
	return 0;
}

static int spi_rawchip_suspend(struct spi_device *rawchip, pm_message_t pmsg) {
	CDBG("%s\n", __func__);
	ConfigPinsToSuspend();
	return 0;
}

static int spi_rawchip_resume(struct spi_device *rawchip) {
	CDBG("%s\n", __func__);
	ConfigPinsToResume();
	return 0;
}

static int spi_rawchip_remove(struct spi_device *rawchip) {
	CDBG("%s\n", __func__);
	if (android_yushan != NULL)
		sysfs_remove_file(android_yushan, &dev_attr_rawchip_mode.attr);
	return 0;
}

static struct spi_driver spi_rawchip = {
	.driver = {
		.name = SPI_RAWCHIP_DEVICENAME,
		.owner = THIS_MODULE,
	},
	.probe = spi_rawchip_probe,
	.suspend = spi_rawchip_suspend,
	.resume = spi_rawchip_resume,
	.remove = spi_rawchip_remove,
};

void ConfigPinsToSuspend()
{
	//nmk_config_pins(SUSPEND_GPIOS, ARRAY_SIZE(SUSPEND_GPIOS));
}

void ConfigPinsToResume()
{
	//nmk_config_pins(RESUME_GPIOS, ARRAY_SIZE(RESUME_GPIOS));
}

static int raw_dbf_setup(void);
int spi_rawchip_init(void) {
	int ret = 0;

	raw_dbf_setup();
	printk("\nclean, spi_rawchip_init\n");
	ret = spi_register_driver(&spi_rawchip);
	printk("clean register raw chip, ret = %d\n", ret);
	if (ret < 0) {
		printk("clean,%s:failed to register spi driver(%d) for camera\n", __func__, ret);
		return ret;
	}
	printk("clean, spi_rawchip_init\n");
	return 1;
}
module_init(spi_rawchip_init);

/*********************************************************************
				Interrupt Functions Starts Here
**********************************************************************/
void Reset_Yushan(void)
{
	uint8_t	bSpiData;
	CDBG("[kuei]%s\n",__func__);
	Yushan_Assert_Reset(0x001F0F10, RESET_MODULE);
	bSpiData =1;
	Yushan_DXO_Sync_Reset_Dereset(bSpiData);
	Yushan_Assert_Reset(0x001F0F10, DERESET_MODULE);
	bSpiData = 0;
	Yushan_DXO_Sync_Reset_Dereset(bSpiData);	
	Yushan_Init_Dxo(&sDxoStruct, 1);
	msleep(10);
	Yushan_sensor_open_init();
}

/******************************************************************************
API Internal Function:	Interrupt Service Routine: Look for Interrupts asserted.
						Add the ID to the list only once. Usually ISR provoked 
						by any associated Intr, but this function has to be called
						whenever interrupts are expected.						
Input:					None
Return:					InterruptID or FAILURE				
*******************************************************************************/
void Yushan_ISR()
{

	uint8_t		bCurrentInterruptID = 0;
	uint8_t		bAssertOrDeassert=0, bInterruptWord = 0;
	/* The List consists of the Current asserted events. Have to be assignmed memory by malloc.
	*  and set every element to 0. */
	uint32_t	*udwListOfInterrupts;
	uint8_t	bSpiData;
	uint32_t udwSpiBaseIndex;
	uint8_t interrupt_type = 0;

	udwListOfInterrupts	= (uint32_t *) kmalloc(96, GFP_KERNEL);

	/* Call Yushan_Intr_Status_Read  for interrupts and add the same to the udwListOfInterrupts. */
	/* bInterruptID = Yushan_ReadIntr_Status(); */
	Yushan_Intr_Status_Read((uint8_t *)udwListOfInterrupts, INTERRUPT_PAD_0);
	
	/* Clear InterruptStatus */
	Yushan_Intr_Status_Clear((uint8_t *) udwListOfInterrupts);

	/* Adding the Current Interrupt asserted to the Proto List */
	while (bCurrentInterruptID < (TOTAL_INTERRUPT_COUNT + 1)) {
		bAssertOrDeassert = ((udwListOfInterrupts[bInterruptWord])>>(bCurrentInterruptID%32))&0x01;

		if (bAssertOrDeassert) {
			Yushan_AddnRemoveIDInList((uint8_t)(bCurrentInterruptID+1), udwProtoInterruptList, ADD_INTR_TO_LIST);

			CDBG(" %s:bCurrentInterruptID:%d\n",__func__, bCurrentInterruptID+1);
			switch (bCurrentInterruptID + 1) {
				case EVENT_PDP_EOF_EXECCMD :
					CDBG(" %s:[AF_INT]EVENT_PDP_EOF_EXECCMD\n", __func__);
					interrupt_type |= RAWCHIP_INT_TYPE_PDP_EOF_EXECCMD;
					break;

				case EVENT_DPP_EOF_EXECCMD :
					CDBG(" %s:[AF_INT]EVENT_DPP_EOF_EXECCMD\n", __func__);
					interrupt_type |= RAWCHIP_INT_TYPE_DPP_EOF_EXECCMD;
					break;

				case EVENT_DOP7_EOF_EXECCMD :
					CDBG(" %s:[AF_INT]EVENT_DOP7_EOF_EXECCMD\n", __func__);
					interrupt_type |= RAWCHIP_INT_TYPE_DOP_EOF_EXECCMD;
					break;

				case EVENT_DXODOP7_NEWFRAMEPROC_ACK :
					CDBG(" %s:[AF_INT]EVENT_DXODOP7_NEWFRAMEPROC_ACK\n", __func__);
					interrupt_type |= RAWCHIP_INT_TYPE_NEW_FRAME;
					break;

				case EVENT_CSI2RX_ECC_ERR :
					CDBG(" %s:[ERR]EVENT_CSI2RX_ECC_ERR\n", __func__);
					interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_CSI2RX_CHKSUM_ERR :
					CDBG(" %s:[ERR]EVENT_CSI2RX_CHKSUM_ERR\n", __func__);
					interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_CSI2RX_SYNCPULSE_MISSED :
					CDBG(" %s:[ERR]EVENT_CSI2RX_SYNCPULSE_MISSED\n", __func__);
					interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_DXOPDP_NEWFRAME_ERR :
					SPI_Read(DXO_PDP_BASE_ADDR + DxOPDP_error_code_7_0, 1, &bSpiData);
					CDBG(" %s:[ERR]EVENT_DXOPDP_NEWFRAME_ERR, error code =%d\n", __func__, bSpiData);
					/* Reset_Yushan(); */
					interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_DXODPP_NEWFRAME_ERR :
					udwSpiBaseIndex = 0x010000;
					SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

					SPI_Read(DXO_DPP_BASE_ADDR + DxODPP_error_code_7_0-0x8000, 1, &bSpiData);
					CDBG(" %s:[ERR]EVENT_DXODPP_NEWFRAME_ERR, error code =%d\n", __func__, bSpiData);

					udwSpiBaseIndex = 0x08000;
					SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
					/* Reset_Yushan();*/
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_DXODOP7_NEWFRAME_ERR :
					SPI_Read(DXO_DOP_BASE_ADDR + DxODOP_error_code_7_0, 1, &bSpiData);
					CDBG(" %s:[ERR]EVENT_DXODOP7_NEWFRAME_ERR, error code =%d\n", __func__, bSpiData);
					/* Reset_Yushan(); */
					interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_CSI2TX_SP_ERR :
					CDBG(" %s:[ERR]EVENT_CSI2TX_SP_ERR\n", __func__);
					interrupt_type |= RAWCHIP_INT_TYPE_ERROR_FATAL;
					break;

				case EVENT_CSI2TX_LP_ERR :
					CDBG(" %s:[ERR]EVENT_CSI2TX_LP_ERR\n", __func__);
					interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_CSI2TX_DATAINDEX_ERR :
					CDBG(" %s:[ERR]EVENT_CSI2TX_DATAINDEX_ERR\n", __func__);
					interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_SOT_SOFT_DL1 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_SOT_HARD_DL1 :
				CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_EOT_DL1 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_ESC_DL1 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_CTRL_DL1 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_SOT_SOFT_DL2 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_SOT_HARD_DL2 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_EOT_DL2 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_ESC_DL2 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_CTRL_DL2 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_SOT_SOFT_DL3 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_SOT_HARD_DL3 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_EOT_DL3 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_ESC_DL3 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_CTRL_DL3:
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_SOT_SOFT_DL4 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_SOT_HARD_DL4 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_EOT_DL4:
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_ESC_DL4:
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_PHY_ERR_CTRL_DL4 :
					CDBG(" %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_TXPHY_CTRL_ERR_D1 :
					CDBG(" %s:[ERR]EVENT_TXPHY_CTRL_ERR_D1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_TXPHY_CTRL_ERR_D2 :
					CDBG(" %s:[ERR]EVENT_TXPHY_CTRL_ERR_D2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_TXPHY_CTRL_ERR_D3 :
					CDBG(" %s:[ERR]EVENT_TXPHY_CTRL_ERR_D3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_TXPHY_CTRL_ERR_D4 :
					CDBG(" %s:[ERR]EVENT_TXPHY_CTRL_ERR_D4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_UNMATCHED_IMAGE_SIZE_ERROR :
					CDBG(" %s:[ERR]EVENT_UNMATCHED_IMAGE_SIZE_ERROR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case PRE_DXO_WRAPPER_PROTOCOL_ERR :
					CDBG(" %s:[ERR]PRE_DXO_WRAPPER_PROTOCOL_ERR\n", __func__);
					/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case PRE_DXO_WRAPPER_FIFO_OVERFLOW :
					CDBG(" %s:[ERR]PRE_DXO_WRAPPER_FIFO_OVERFLOW\n", __func__);
					/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_BAD_FRAME_DETECTION :
					CDBG(" %s:[ERR]EVENT_BAD_FRAME_DETECTION\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_TX_DATA_FIFO_OVERFLOW :
					CDBG(" %s:[ERR]EVENT_TX_DATA_FIFO_OVERFLOW\n", __func__);
					/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_TX_INDEX_FIFO_OVERFLOW :
					CDBG(" %s:[ERR]EVENT_TX_INDEX_FIFO_OVERFLOW\n", __func__);
					/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_CHAR_COLOR_BAR_0_ERR :
					CDBG(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_0_ERR\n", __func__);
				  // interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_CHAR_COLOR_BAR_1_ERR :
					CDBG(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_1_ERR\n", __func__);
				  // interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_CHAR_COLOR_BAR_2_ERR :
					CDBG(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_2_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_CHAR_COLOR_BAR_3_ERR :
					CDBG(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_3_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_CHAR_COLOR_BAR_4_ERR :
					CDBG(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_4_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_CHAR_COLOR_BAR_5_ERR :
					CDBG(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_5_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_CHAR_COLOR_BAR_6_ERR :
					CDBG(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_6_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_RX_CHAR_COLOR_BAR_7_ERR :
					CDBG(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_7_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_POST_DXO_WRAPPER_PROTOCOL_ERR :
					CDBG(" %s:[ERR]EVENT_POST_DXO_WRAPPER_PROTOCOL_ERR\n",__func__);
					/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_POST_DXO_WRAPPER_FIFO_OVERFLOW :
					CDBG(" %s:[ERR]EVENT_POST_DXO_WRAPPER_FIFO_OVERFLOW\n", __func__);
					/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_TX_DATA_UNDERFLOW :
					CDBG(" %s:[ERR]EVENT_TX_DATA_UNDERFLOW\n", __func__);
					/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;

				case EVENT_TX_INDEX_UNDERFLOW :
					CDBG(" %s:[ERR]EVENT_TX_INDEX_UNDERFLOW\n", __func__);
					/* Reset_Yushan(); */
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
					break;
				case EVENT_DOP7_BOOT:
					CDBG(" %s:EVENT_DOP7_BOOT\n", __func__);
					break;
			}
		}
		bCurrentInterruptID++;

		if (interrupt_type & RAWCHIP_INT_TYPE_ERROR) {
			 error_cnt++;
			 if (bCanDoSnapshot) {
			 	 bCanDoSnapshot = false;
			 	 Yushan_Status_Snapshot();
		         }
		}

		if (bCurrentInterruptID%32 == 0)
			bInterruptWord++;
	}

	kfree(udwListOfInterrupts);

	if (gpio_get_value(rawchip_intr0) == 1) {
		atomic_set(&interrupt, 1);
		wake_up(&yushan_int.yushan_wait);
	}
	if (gpio_get_value(rawchip_intr1) == 1) {
		atomic_set(&interrupt2, 1);
		wake_up(&yushan_int.yushan_wait);
	}
}

void Yushan_ISR2() /* for DOP7 */
{

	uint8_t		bCurrentInterruptID = 16;
	uint8_t		bAssertOrDeassert = 0, bInterruptWord = 0;
	/* The List consists of the Current asserted events. Have to be assignmed memory by malloc.
	*  and set every element to 0. */
	uint32_t	*udwListOfInterrupts;
	uint8_t	bSpiData;

	udwListOfInterrupts	= (uint32_t *) kmalloc(96, GFP_KERNEL);

	/* Call Yushan_Intr_Status_Read  for interrupts and add the same to the udwListOfInterrupts. */
	/* bInterruptID = Yushan_ReadIntr_Status();*/
	Yushan_Intr_Status_Read((uint8_t *)udwListOfInterrupts, INTERRUPT_PAD_1);

#if 1

	/* Adding the Current Interrupt asserted to the Proto List */
	while(bCurrentInterruptID<21)  
	{		
		bAssertOrDeassert = ((udwListOfInterrupts[bInterruptWord])>>(bCurrentInterruptID%32))&0x01;

		if(bAssertOrDeassert)
		{				
			Yushan_AddnRemoveIDInList((uint8_t)(bCurrentInterruptID+1), udwProtoInterruptList, ADD_INTR_TO_LIST);
#if 1
			CDBG("[CAM] %s:bCurrentInterruptID:%d\n", __func__, bCurrentInterruptID+1);
			switch(bCurrentInterruptID+1)
			{
			case EVENT_DXODOP7_NEWFRAMEPROC_ACK :
				CDBG("[CAM] %s:[AF_INT]EVENT_DXODOP7_NEWFRAMEPROC_ACK\n", __func__);
				break;
			case EVENT_DXODOP7_NEWFRAME_ERR :
				{
					SPI_Read(DXO_DOP_BASE_ADDR + DxODOP_error_code_7_0, 1, &bSpiData);
						CDBG(" %s:[ERR]EVENT_DXODOP7_NEWFRAME_ERR, error code =%d\n",__func__, bSpiData);
					/* Reset_Yushan(); */
					break;
				}
			}
#endif
		}			
		bCurrentInterruptID++;

		if(bCurrentInterruptID % 32 == 0)
			bInterruptWord++;			
	}
	/* Clear InterruptStatus */
	Yushan_Intr_Status_Clear((uint8_t *) udwListOfInterrupts);
#endif

	kfree(udwListOfInterrupts);
}

/******************************************************************
API Internal Function:	Function for reading exact interrupt event.
Input:					Asserted IntrSetID.
Return:					InterruptEventID as define in Yushan_API.h	

/**********************************************************************
Function:	To remove (clear  bit corresponding to) interrupt
			mentioned in input.
Input:		Interrupt ID to be removed from the list
Return:
***********************************************************************/
void Yushan_ClearInterruptEvent(uint8_t bInterruptID, uint32_t *udwProtoInterruptList)
{
	Yushan_AddnRemoveIDInList(bInterruptID, udwProtoInterruptList, DEL_INTR_FROM_LIST);
}

/*******************************************************************************
						Yushan related FUNCTIONS
*******************************************************************************/
void Yushan_Dump_Info(void)
{
	int i = 0;
	//Yushan_Status_Snapshot();
	//for (i = 0; i < 20; i++) {
		//printk("kuei, Yushan_Dump_Info index = %d\n", i);
		frame_counter_in_yushan(1);
		//msleep(100);
	//}
	//count++;
	//if ((count > 20) % 2)
	//	frame_counter_in_yushan(1);
#if 0			
			frame_counter_in_yushan(1);
			count++;
			
			if (bIsYushan_Init == true || count == 30)
			{
				Yushan_Status_Snapshot();
				bIsYushan_Init = false;
				/*
				CDBG("kuei, IOCTL_DUMP_YUSHAN_REGISTERS \n");
				Yushan_Status_Snapshot();
				Yushan_ISR();
				Yushan_dump_all_register();
				frame_counter_in_yushan(1);
				bIsYushan_Init = false;
				//DumpSIARegs();
				*/
			}
			else
			{
				frame_counter_in_yushan(1);
			}
#endif
}

void Yushan_Change_Resolition_To_4X(void)
{
	uint8_t bCurrentStreamingMode;
	uint32_t udwSpiData = 0;
	uint8_t  bUpdate_4X_Status;
	
	Yushan_New_Context_Config_t sYushanNewContextConfig;
	Yushan_ImageChar_t snewImageChar;
	
	CDBG("> kuei, Yushan_Change_Resolition_To_4X\n");
	bCurrentStreamingMode = Yushan_GetCurrentStreamingMode();
	if(bCurrentStreamingMode==YUSHAN_FRAME_FORMAT_STILL_MODE)
	{
		CDBG("kuei, Yushan_Change_Resolition_To_4X, Switch to VF mode\n");
		// Current streaming mode is STILL, so selected mode will be VF
		sYushanNewContextConfig.bSelectStillVfMode = YUSHAN_FRAME_FORMAT_VF_MODE;
	}
	else
	{
		CDBG("kuei, Yushan_Change_Resolition_To_4X, Switch to STILL mode\n");
		// Current streaming mode is VF, so selected mode will be STILL
		sYushanNewContextConfig.bSelectStillVfMode = YUSHAN_FRAME_FORMAT_STILL_MODE;
	}

	sYushanNewContextConfig.uwActivePixels 			 = SENSOR_WIDTH_4X;
	sYushanNewContextConfig.uwLineBlank					 = LINE_BLANKING_4X;
	sYushanNewContextConfig.uwActiveFrameLength  = SENSOR_HEIGHT_4X;
	sYushanNewContextConfig.uwPixelFormat 			 = PIXEL_FORMAT_4X;
	Yushan_Context_Config_Update(&sYushanNewContextConfig);
	
	snewImageChar.bImageOrientation  = v_image_orientation;
	snewImageChar.uwXAddrStart		   = IMGCHAR_X_START_4X;
	snewImageChar.uwYAddrStart		   = IMGCHAR_Y_START_4X;
	snewImageChar.uwXAddrEnd		   = IMGCHAR_X_END_4X;
	snewImageChar.uwYAddrEnd		   = IMGCHAR_Y_END_4X;
	snewImageChar.uwXEvenInc		   = IMGCHAR_XEVENINC_4X;
	snewImageChar.uwXOddInc                    = IMGCHAR_XOddInc_4X;
	snewImageChar.uwYEvenInc                   = IMGCHAR_YEvenInc_4X;
	snewImageChar.uwYOddInc	                   = IMGCHAR_YOddInc_4X;
	snewImageChar.bBinning           = IMGCHAR_BINNING_4X;
	
	Yushan_Update_SensorParameters(&sGainsExpTime);
	Yushan_Update_ImageChar(&snewImageChar); // snewImageChar previously initialized
	Yushan_Update_DxoDpp_TuningParameters(&sDxoDppTuning);
	Yushan_Update_DxoDop_TuningParameters(&sDxoDopTuning);
	Yushan_Update_DxoPdp_TuningParameters(&sDxoPdpTuning);
	
	bUpdate_4X_Status = Yushan_Update_Commit(PDPMode,DPPMode,DOPMode);
	if (bUpdate_4X_Status) {
		CDBG("\n< kuei, Yushan_Change_Resolition_To_4X, Update success\n");
	} else {
		CDBG("\n< kuei, Yushan_Change_Resolition_To_4X, Update fail\n");
	}	
	CDBG("< kuei, Yushan_Change_Resolition_To_4X\n");
}

void Yushan_Change_Resolition_To_Normal(void)
{
	uint8_t bCurrentStreamingMode;
	uint8_t bUpdate_Normal_Status;
	
	Yushan_New_Context_Config_t sYushanNewContextConfig;
	Yushan_ImageChar_t snewImageChar;
	
	bCurrentStreamingMode = Yushan_GetCurrentStreamingMode();
	if(bCurrentStreamingMode==YUSHAN_FRAME_FORMAT_STILL_MODE)
	{
		// Current streaming mode is STILL, so selected mode will be VF
		sYushanNewContextConfig.bSelectStillVfMode = YUSHAN_FRAME_FORMAT_VF_MODE;
	}
	else
	{
		// Current streaming mode is VF, so selected mode will be STILL
		sYushanNewContextConfig.bSelectStillVfMode = YUSHAN_FRAME_FORMAT_STILL_MODE;
	}

	sYushanNewContextConfig.uwActivePixels 			 = v_preview_sensor_width;
	sYushanNewContextConfig.uwLineBlank					 = v_line_blank;
	sYushanNewContextConfig.uwActiveFrameLength  = v_preview_sensor_height;
	sYushanNewContextConfig.uwPixelFormat 			 = v_pixel_format;
	Yushan_Context_Config_Update(&sYushanNewContextConfig);
	
	snewImageChar.bImageOrientation  = v_image_orientation;
	snewImageChar.uwXAddrStart		   = 0;
	snewImageChar.uwYAddrStart		   = 0;
	snewImageChar.uwXAddrEnd		     = v_preview_sensor_width  - 1;
	snewImageChar.uwYAddrEnd		     = v_preview_sensor_height - 1;
	snewImageChar.uwXEvenInc		     = v_imgchar_xeveninc;
	snewImageChar.uwXOddInc          = v_imgchar_xoddinc;
	snewImageChar.uwYEvenInc         = v_imgchar_yeveninc;
	snewImageChar.uwYOddInc	         = v_imgchar_yoddinc;
	snewImageChar.bBinning           = v_imgchar_binning;
	
	Yushan_Update_SensorParameters(&sGainsExpTime);
	Yushan_Update_ImageChar(&snewImageChar); // snewImageChar previously initialized
	Yushan_Update_DxoDpp_TuningParameters(&sDxoDppTuning);
	Yushan_Update_DxoDop_TuningParameters(&sDxoDopTuning);
	Yushan_Update_DxoPdp_TuningParameters(&sDxoPdpTuning);
	bUpdate_Normal_Status = Yushan_Update_Commit(PDPMode,DPPMode,DOPMode);
	
	if (bUpdate_Normal_Status) {
		CDBG("\n< kuei, Yushan_Change_Resolition_To_Normal, Update success\n");
	} else {
		CDBG("\n< kuei, Yushan_Change_Resolition_To_Normal, Update fail\n");
	}
	
	CDBG("< kuei, Yushan_Change_Resolition_To_Normal\n");
}

void Yushan_Enable_TX(void)
{
	uint32_t udwSpiData = 0;
	udwSpiData = 0x33;
	SPI_Write(YUSHAN_MIPI_TX_ENABLE, 1, (unsigned char *)&udwSpiData);
}

void Yushan_Disable_TX(void)
{
	uint32_t udwSpiData = 0;
	udwSpiData = 0x31;
	SPI_Write(YUSHAN_MIPI_TX_ENABLE, 1, (unsigned char *)&udwSpiData);
}

void rawchip_stop(void)
{
	//Yushan_Enable_Tx_HsClock(false);
}

void Yushan_Init_RawChip()
{
	CDBG("> kuei, Yushan_Init_RawChip");
	#if USE_NEW_YUSHAN_LAUNCH_WAY
		if(!bIsYushan_Init) {
			while (!bYushan_init_function_done) {
				CDBG("kuei, bYushan_init_function_done = %d\n", bYushan_init_function_done);
				msleep(1);
			}
			Yushan_Enable_Tx_HsClock(false);
			Yushan_Enable_Tx_HsClock(true);			
		}
		bIsYushan_Init = true;
	#else
	if(!bIsYushan_Init)
		Yushan_sensor_open_init();
	bIsYushan_Init = true;
	#endif
	CDBG("< kuei, Yushan_Init_RawChip");
}

void rawchip_powerdown(void)
{
	unsigned int nCount = 0;

	if(!rawchip_power_status){
		printk("double rawchip power down, ignore!!!\n");
		return;
	}
	CDBG("> Yushan_Power_OFF\n");
	CDBG(KERN_ERR "> Kuei, Yushan_Power_OFF \n");
	while (yushan_init_in_progress && nCount < 100) {
		nCount++;
		msleep(10);
	}
#ifndef CONFIG_SENSOR_USE_OV5693
	DisablePwr();
	DisableRst();
#else
	DisableRst();
	mdelay(1);
	DisableExtClk();	
	//DisablePwr();
#endif
	gPllLocked = 0;
	bIsYushan_Init = false;	
	bYushan_init_function_done = false;
	last_width = 0;
		
	ConfigPinsToSuspend();	
	CDBG("< Yushan_Power_OFF\n");
	CDBG(KERN_ERR "< Kuei, Yushan_Power_OFF \n");
	/*
	if( (prawchipplat!=NULL) && (prawchipplat->set_spi_pin_input!=NULL) ){
		prawchipplat->set_spi_pin_input();
	}
	*///keep spi enabled always for easy debug
	rawchip_power_status = 0;
	printk("rawchip power down end\n");
}

void switch_size(int size)
{
	uint8_t bCurrentStreamingMode;
	uint8_t bUpdate_Normal_Status;
	
	Yushan_New_Context_Config_t sYushanNewContextConfig;
	Yushan_ImageChar_t snewImageChar;
	
	bCurrentStreamingMode = Yushan_GetCurrentStreamingMode();
	if(bCurrentStreamingMode==YUSHAN_FRAME_FORMAT_STILL_MODE)
	{
		// Current streaming mode is STILL, so selected mode will be VF
		sYushanNewContextConfig.bSelectStillVfMode = YUSHAN_FRAME_FORMAT_VF_MODE;
	}
	else
	{
		// Current streaming mode is VF, so selected mode will be STILL
		sYushanNewContextConfig.bSelectStillVfMode = YUSHAN_FRAME_FORMAT_STILL_MODE;
	}

	sYushanNewContextConfig.uwActivePixels 			 = v_preview_sensor_width;
	sYushanNewContextConfig.uwLineBlank					 = v_line_blank;
	sYushanNewContextConfig.uwActiveFrameLength  = v_preview_sensor_height;
	sYushanNewContextConfig.uwPixelFormat 			 = v_pixel_format;
	Yushan_Context_Config_Update(&sYushanNewContextConfig);
	
	snewImageChar.bImageOrientation  = v_image_orientation;
	snewImageChar.uwXEvenInc	= v_imgchar_xeveninc;
	snewImageChar.uwYEvenInc         = v_imgchar_yeveninc;
	snewImageChar.uwXAddrStart	= v_imgchar_uwxaddrstart;
	snewImageChar.uwYAddrStart	= v_imgchar_uwyaddrstart;
	snewImageChar.uwXAddrEnd 	= v_imgchar_uwxaddrend;
	snewImageChar.uwYAddrEnd 	= v_imgchar_uwyaddrend;

	if(size == 820){
       snewImageChar.uwXOddInc      = IMGCHAR_XOddInc_4X;
	   snewImageChar.uwYOddInc	    = IMGCHAR_YOddInc_4X;
	   snewImageChar.bBinning       = IMGCHAR_BINNING_4X;
	}
	else{
       snewImageChar.uwXOddInc      = v_imgchar_xoddinc;
	   snewImageChar.uwYOddInc	    = v_imgchar_yoddinc;
	   snewImageChar.bBinning       = v_imgchar_binning;
	}
	
	Yushan_Update_SensorParameters(&sGainsExpTime);
	Yushan_Update_ImageChar(&snewImageChar); // snewImageChar previously initialized
	Yushan_Update_DxoDpp_TuningParameters(&sDxoDppTuning);
	Yushan_Update_DxoDop_TuningParameters(&sDxoDopTuning);
	Yushan_Update_DxoPdp_TuningParameters(&sDxoPdpTuning);
	bUpdate_Normal_Status = Yushan_Update_Commit(PDPMode,DPPMode,DOPMode);
	
	if (bUpdate_Normal_Status) {
		CDBG("\n< kuei, Yushan_Change_Resolition_To_Normal, Update success\n");
	} else {
		CDBG("\n< kuei, Yushan_Change_Resolition_To_Normal, Update fail\n");
	}
}
	
extern struct yushan_reg_t yushan_regs_imx175;
extern struct yushan_reg_t yushan_regs_s5k6a2ya;
extern struct yushan_reg_t yushan_regs_ov2722;
void rawchip_start(int width)
{
	int32_t rc = 0;
	//uint32_t check_yushan_id;
	int index2 = 0;

	if(width == last_width){
		printk("rawchip: same width doing nothing\n");
		Yushan_Enable_Tx_HsClock(false);
		Yushan_Enable_Tx_HsClock(true);
		return;
	}
	SPI_Read(0x5c04, 4, (uint8_t*)(&check_yushan_id));
	CDBG(KERN_ERR "> kuei, Yushan_common_init, Yushan ID = 0x%x\n", check_yushan_id);

	if (check_yushan_id != 0x02030200) {
		CDBG(KERN_ERR "> kuei, Yushan_common_init, Fail to get raw chip ID\n");
		return;
	}
	CDBG(KERN_ERR "> kuei, Yushan_common_init, Get correct raw chip ID\n");


	printk("---------------------\n");
	printk("---------------------\n");
	printk("---------------------\n");
	printk("---------------------\n");
	printk("---------------------\n");
	printk("---------------------\n");
	printk("---------------------\n");
	printk("---------------------\n");
	printk("---------------------\n");
	printk("---------------------\n");
	printk("---------------------\n");
	printk("---------------------\n");
	printk("---------------------\n");
	printk("---------------------\n");
	printk("clean image size is %d\n", width);
	switch (width)
	{
	    case 820: //Sony 8M slowmotion
			v_image_orientation = IMAGE_ORIENTATION;
			v_preview_sensor_width = 820;
			v_preview_sensor_height = 510;
			v_line_blank = 2620;
			v_frame_blank = 110;//746;
			v_data_line = 4;
			v_ext_clock = EXT_CLOCK;
			v_mipi_data_rate = 640;//648;
			v_spi_clock = SPI_CLOCK;
			v_imgchar_uwxaddrstart	= 0;
			v_imgchar_uwyaddrstart	= 212;
			v_imgchar_uwxaddrend	= 3279;//v_preview_sensor_width  - 1;
			v_imgchar_uwyaddrend	= 2251;//v_preview_sensor_height - 1;
			yushan_regs = &yushan_regs_imx175;

			/*gain setting*/
			v_uwAnalogGainCodeGR		= 0x20;//0x0 10x=>140; 1x=>20
			v_uwAnalogGainCodeR		= 0x20;
			v_uwAnalogGainCodeB		= 0x20;
			v_uwPreDigGainGR		= 0x100;//0x100
			v_uwPreDigGainR			= 0x100;
			v_uwPreDigGainB			= 0x100;
			v_uwExposureTime		= 0x200;
			v_bRedGreenRatio		= 0x40;
			v_bBlueGreenRatio		= 0x40;

			v_uwFlashPreflashRating 	= 0;
			v_bFocalInfo			= 0;

			v_bDeadPixelCorrectionLowGain	= 0x80;
			v_bDeadPixelCorrectionMedGain	= 0x80;
			v_bDeadPixelCorrectionHiGain	= 0x80;

			v_bEstimationMode		= 1;
			v_bSharpness			= 0x10;
			v_bDenoisingLowGain		= 0x70;//0xFF for de-noise verify, original:0x1
			v_bDenoisingMedGain		= 0x70;
			v_bDenoisingHiGain		= 0x70;
			v_bNoiseVsDetailsLowGain	= 0x90;
			v_bNoiseVsDetailsMedGain	= 0x90;
			v_bNoiseVsDetailsHiGain		= 0x90;
			v_bTemporalSmoothing		= 0x26;
			break;
		case 1280: //4e1
			v_image_orientation = IMAGE_ORIENTATION;
			v_preview_sensor_width = 1280;
			v_preview_sensor_height = 960;
			v_line_blank = 104;
			v_frame_blank = 40;
			v_data_line = 2;
			v_ext_clock = EXT_CLOCK;
			v_mipi_data_rate = 404;
			v_spi_clock = SPI_CLOCK;
			v_imgchar_uwxaddrstart	= 0;
			v_imgchar_uwyaddrstart	= 0;
			v_imgchar_uwxaddrend	= v_preview_sensor_width  - 1;
			v_imgchar_uwyaddrend	= v_preview_sensor_height - 1;
			yushan_regs = &yushan_regs_imx175; 
			break;
		case 1640: //Sony 8M quator
			v_image_orientation = IMAGE_ORIENTATION;
			v_preview_sensor_width = 1640;
			v_preview_sensor_height = 1232;
			v_line_blank = 1800;
			v_frame_blank = 1280;
			v_data_line = 4;
			v_ext_clock = EXT_CLOCK;
			v_mipi_data_rate = 648;
			v_spi_clock = SPI_CLOCK;
			v_imgchar_uwxaddrstart	= 0;
			v_imgchar_uwyaddrstart	= 0;
			v_imgchar_uwxaddrend	= v_preview_sensor_width  - 1;
			v_imgchar_uwyaddrend	= v_preview_sensor_height - 1;
			yushan_regs = &yushan_regs_imx175; 
			break;
		case 1472: //6a2
			v_image_orientation = IMAGE_ORIENTATION;
			v_preview_sensor_width = 1472;
			v_preview_sensor_height = 1104;
			v_line_blank = 192;
			v_frame_blank = 136;
			v_data_line = 1;
			v_ext_clock = EXT_CLOCK;
			v_mipi_data_rate = 620;
			v_spi_clock = SPI_CLOCK;
			v_imgchar_uwxaddrstart	= 0;
			v_imgchar_uwyaddrstart	= 0;
			v_imgchar_uwxaddrend	= v_preview_sensor_width  - 1;
			v_imgchar_uwyaddrend	= v_preview_sensor_height - 1;
			yushan_regs = &yushan_regs_s5k6a2ya; 

			/*gain setting*/
			v_uwAnalogGainCodeGR		= 0x20;//0x0 10x=>140; 1x=>20
			v_uwAnalogGainCodeR		= 0x20;
			v_uwAnalogGainCodeB		= 0x20;
			v_uwPreDigGainGR		= 0x100;//0x100
			v_uwPreDigGainR			= 0x100;
			v_uwPreDigGainB			= 0x100;
			v_uwExposureTime		= 0x200;
			v_bRedGreenRatio		= 0x40;
			v_bBlueGreenRatio		= 0x40;

			v_uwFlashPreflashRating 	= 0;
			v_bFocalInfo			= 0;

			v_bDeadPixelCorrectionLowGain	= 0x80;
			v_bDeadPixelCorrectionMedGain	= 0x80;
			v_bDeadPixelCorrectionHiGain	= 0x80;

			v_bEstimationMode		= 1;
			v_bSharpness			= 0x10;
			v_bDenoisingLowGain		= 0x70;//0xFF for de-noise verify, original:0x1
			v_bDenoisingMedGain		= 0x70;
			v_bDenoisingHiGain		= 0x70;
			v_bNoiseVsDetailsLowGain	= 0x90;
			v_bNoiseVsDetailsMedGain	= 0x90;
			v_bNoiseVsDetailsHiGain		= 0x90;
			v_bTemporalSmoothing		= 0x26;

			break;

		case 1928: //ov2722
			v_image_orientation = IMAGE_ORIENTATION;
			v_preview_sensor_width = 1928;
			v_preview_sensor_height = 1088;
			v_line_blank = 212;
			v_frame_blank = 48;
			v_data_line = 1;
			v_ext_clock = EXT_CLOCK;
			v_mipi_data_rate = 720;
			v_spi_clock = SPI_CLOCK;
			v_imgchar_uwxaddrstart	= 0;
			v_imgchar_uwyaddrstart	= 0;
			v_imgchar_uwxaddrend	= v_preview_sensor_width  - 1;
			v_imgchar_uwyaddrend	= v_preview_sensor_height - 1;
			yushan_regs = &yushan_regs_ov2722; 

			/*gain setting*/
			v_uwAnalogGainCodeGR		= 0x20;//0x0 10x=>140; 1x=>20
			v_uwAnalogGainCodeR		= 0x20;
			v_uwAnalogGainCodeB		= 0x20;
			v_uwPreDigGainGR		= 0x100;//0x100
			v_uwPreDigGainR			= 0x100;
			v_uwPreDigGainB			= 0x100;
			v_uwExposureTime		= 0x200;
			v_bRedGreenRatio		= 0x40;
			v_bBlueGreenRatio		= 0x40;

			v_uwFlashPreflashRating 	= 0;
			v_bFocalInfo			= 0;

			v_bDeadPixelCorrectionLowGain	= 0x80;
			v_bDeadPixelCorrectionMedGain	= 0x80;
			v_bDeadPixelCorrectionHiGain	= 0x80;

			v_bEstimationMode		= 1;
			v_bSharpness			= 0x10;
			v_bDenoisingLowGain		= 0x70;//0xFF for de-noise verify, original:0x1
			v_bDenoisingMedGain		= 0x70;
			v_bDenoisingHiGain		= 0x70;
			v_bNoiseVsDetailsLowGain	= 0x90;
			v_bNoiseVsDetailsMedGain	= 0x90;
			v_bNoiseVsDetailsHiGain		= 0x90;
			v_bTemporalSmoothing		= 0x26;

			break;

	     case 3084:
			v_image_orientation = IMAGE_ORIENTATION;
			v_preview_sensor_width = 3084;
			v_preview_sensor_height = 1736;
			v_line_blank = 1716;
			v_frame_blank = 54;
			v_data_line = 4;
			v_ext_clock = EXT_CLOCK;
			v_mipi_data_rate = 640;
			v_spi_clock = SPI_CLOCK;
			v_imgchar_uwxaddrstart	= 98;
			v_imgchar_uwyaddrstart	= 364;
			v_imgchar_uwxaddrend	= v_imgchar_uwxaddrstart + v_preview_sensor_width  - 1;
			v_imgchar_uwyaddrend	= v_imgchar_uwyaddrstart + v_preview_sensor_height - 1;
			yushan_regs = &yushan_regs_imx175; 

			/*gain setting*/
			v_uwAnalogGainCodeGR		= 0x20;//0x0 10x=>140; 1x=>20
			v_uwAnalogGainCodeR		= 0x20;
			v_uwAnalogGainCodeB		= 0x20;
			v_uwPreDigGainGR		= 0x100;//0x100
			v_uwPreDigGainR			= 0x100;
			v_uwPreDigGainB			= 0x100;
			v_uwExposureTime		= 0x200;
			v_bRedGreenRatio		= 0x40;
			v_bBlueGreenRatio		= 0x40;

			v_uwFlashPreflashRating 	= 0;
			v_bFocalInfo			= 0;

			v_bDeadPixelCorrectionLowGain	= 0x80;
			v_bDeadPixelCorrectionMedGain	= 0x80;
			v_bDeadPixelCorrectionHiGain	= 0x80;

			v_bEstimationMode		= 1;
			v_bSharpness			= 0x10;
			v_bDenoisingLowGain		= 0x70;//0xFF for de-noise verify, original:0x1
			v_bDenoisingMedGain		= 0x70;
			v_bDenoisingHiGain		= 0x70;
			v_bNoiseVsDetailsLowGain	= 0x90;
			v_bNoiseVsDetailsMedGain	= 0x90;
			v_bNoiseVsDetailsHiGain		= 0x90;
			v_bTemporalSmoothing		= 0x26;
            		break;
					
		case 3264: //Sony 8M
			v_image_orientation = IMAGE_ORIENTATION;
			v_preview_sensor_width = 3264;
			v_preview_sensor_height = 2448;
#if defined (CONFIG_MACH_CP5VEDWG) || defined (CONFIG_MACH_CP5VEDTU)
			v_line_blank = 616;
			v_frame_blank = 26;
			//v_frame_blank = 52;
			v_mipi_data_rate = 576;
#else
			v_line_blank = 176;
			v_frame_blank = 64;
			//v_frame_blank = 52;
			v_mipi_data_rate = 648;
#endif
			v_data_line = 4;
			v_ext_clock = EXT_CLOCK;
			//v_mipi_data_rate = 796; 
			v_spi_clock = SPI_CLOCK;
			v_imgchar_uwxaddrstart	= 0;
			v_imgchar_uwyaddrstart	= 0;
			v_imgchar_uwxaddrend	= v_preview_sensor_width  - 1;
			v_imgchar_uwyaddrend	= v_preview_sensor_height - 1;
			yushan_regs = &yushan_regs_imx175; 

			/*gain setting*/
			v_uwAnalogGainCodeGR		= 0x20;//0x0 10x=>140; 1x=>20
			v_uwAnalogGainCodeR		= 0x20;
			v_uwAnalogGainCodeB		= 0x20;
			v_uwPreDigGainGR		= 0x100;//0x100
			v_uwPreDigGainR			= 0x100;
			v_uwPreDigGainB			= 0x100;
			v_uwExposureTime		= 0x200;
			v_bRedGreenRatio		= 0x40;
			v_bBlueGreenRatio		= 0x40;

			v_uwFlashPreflashRating 	= 0;
			v_bFocalInfo			= 0;

			v_bDeadPixelCorrectionLowGain	= 0x80;
			v_bDeadPixelCorrectionMedGain	= 0x80;
			v_bDeadPixelCorrectionHiGain	= 0x80;

			v_bEstimationMode		= 1;
			v_bSharpness			= 0x10;
			v_bDenoisingLowGain		= 0x70;//0xFF for de-noise verify, original:0x1
			v_bDenoisingMedGain		= 0x70;
			v_bDenoisingHiGain		= 0x70;
			v_bNoiseVsDetailsLowGain	= 0x90;
			v_bNoiseVsDetailsMedGain	= 0x90;
			v_bNoiseVsDetailsHiGain		= 0x90;
			v_bTemporalSmoothing		= 0x26;

			break;
		 case 3280: //Sony 8M
#if defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DTU)
		    v_image_orientation = IMAGE_ORIENTATION_180;
#else
		    v_image_orientation = IMAGE_ORIENTATION;
#endif
			v_preview_sensor_width = 3280;
			v_preview_sensor_height = 2464;
			v_line_blank = 816;
			v_frame_blank = 140;
			v_data_line = 4;
			v_ext_clock = EXT_CLOCK;
#if defined(CONFIG_MACH_Z4DTG) || defined(CONFIG_MACH_Z4TD)
			v_mipi_data_rate = 640;
#else
			v_mipi_data_rate = 640;
#endif
			v_spi_clock = SPI_CLOCK;
			v_imgchar_uwxaddrstart	= 0;
			v_imgchar_uwyaddrstart	= 0;
			v_imgchar_uwxaddrend	= v_preview_sensor_width  - 1;
			v_imgchar_uwyaddrend	= v_preview_sensor_height - 1;
			yushan_regs = &yushan_regs_imx175; 

			/*gain setting*/
			v_uwAnalogGainCodeGR		= 0x20;//0x0 10x=>140; 1x=>20
			v_uwAnalogGainCodeR		= 0x20;
			v_uwAnalogGainCodeB		= 0x20;
			v_uwPreDigGainGR		= 0x100;//0x100
			v_uwPreDigGainR			= 0x100;
			v_uwPreDigGainB			= 0x100;
			v_uwExposureTime		= 0x200;
			v_bRedGreenRatio		= 0x40;
			v_bBlueGreenRatio		= 0x40;

			v_uwFlashPreflashRating 	= 0;
			v_bFocalInfo			= 0;

			v_bDeadPixelCorrectionLowGain	= 0x80;
			v_bDeadPixelCorrectionMedGain	= 0x80;
			v_bDeadPixelCorrectionHiGain	= 0x80;

			v_bEstimationMode		= 1;
			v_bSharpness			= 0x10;
			v_bDenoisingLowGain		= 0x70;//0xFF for de-noise verify, original:0x1
			v_bDenoisingMedGain		= 0x70;
			v_bDenoisingHiGain		= 0x70;
			v_bNoiseVsDetailsLowGain	= 0x90;
			v_bNoiseVsDetailsMedGain	= 0x90;
			v_bNoiseVsDetailsHiGain		= 0x90;
			v_bTemporalSmoothing		= 0x26;
            		break;
	}

	CDBG("rawchip set orientation :%d\n", v_image_orientation);
	if (!bIsYushan_Init) {
		#if USE_NEW_YUSHAN_LAUNCH_WAY
		yushan_init_in_progress = 1;
		//yushan_open_init_thread();
		Yushan_sensor_open_init();
	
		yushan_init_in_progress = 0;
		bYushan_init_function_done = true;
		#endif
		/*
		rawchip_update_af_params_t update_af_params;
		update_af_params.af_params.sYushanAfRoi[0].bXStart = 6;
		update_af_params.af_params.sYushanAfRoi[0].bXEnd = 10;
		update_af_params.af_params.sYushanAfRoi[0].bYStart = 4;
		update_af_params.af_params.sYushanAfRoi[0].bYEnd = 8;
		update_af_params.af_params.active_number = 1;
		Yushan_Update_AF_Params(&update_af_params);
		*/
		Yushan_Enable_Tx_HsClock(false);
		Yushan_Enable_Tx_HsClock(true);
	}
	else{
		mdelay(100);
		Yushan_Enable_Tx_HsClock(false);
		mdelay(100);
		switch_size(width);
		Yushan_Enable_Tx_HsClock(true);
	}
	last_width = width;

	
#if 0
	// fix first time preview fail in R/I test -- Clean
	if(1 == board_mfg_mode()  && camera_times++ < 5){
		printk("delay for fixing MFG first camera fail %d times\n", camera_times);
		msleep(500);
	}
	// end
#endif
}

void rawchip_poweron(void) //power on
{
	interrupt_err_count = 0;
	if(rawchip_power_status){
		printk("double rawchip power up, ignored!!!\n");
		return;
	}
	if( (prawchipplat!=NULL) && (prawchipplat->restore_spi_pin_cfg!=NULL) ){
		prawchipplat->restore_spi_pin_cfg();
	}

	ConfigPinsToResume();
	//rawchip_enable_spi2();
	if( (prawchipplat!=NULL) && (prawchipplat->restore_spi_pin_cfg!=NULL) ){
		prawchipplat->restore_spi_pin_cfg();
	}
	
	error_cnt = 0;
        bCanDoSnapshot = true;
	bIsYushan_Init = false;
	bYushan_init_function_done = false;
  mdelay(1);
  //EnablePwr();
  mdelay(1);
  EnableExtClk();
  mdelay(1);
  EnableRst();
  mdelay(1);

	yushan_spi_write(0x0008, 0x7f);
//	msleep(1);

	SPI_Read(0x5c04, 4, (uint8_t*)(&check_yushan_id));
	printk("clean in send clk, Yushan_common_init, Yushan ID = 0x%x\n", check_yushan_id);

	if (check_yushan_id == 0x02030200) {
		printk("clean Yushan_common_init, Get correct raw chip ID\n");
	} else {
		printk("clean Yushan_common_init, Fail to get raw chip ID\n");
	}

	printk("clk send enabled\n");
	rawchip_power_status = 1;
	printk("rawchip power on end\n");
}

void DisablePwr(void) {
	/* disable 1.2 V */
	gpio_request(Power_PIN_1_2V, NULL);
	gpio_direction_output(Power_PIN_1_2V, 0);
	gpio_free(Power_PIN_1_2V);
	
	/* disable 1.8 V */
	gpio_request(Power_PIN_1_8V, NULL);
	gpio_direction_output(Power_PIN_1_8V, 0);
	gpio_free(Power_PIN_1_8V);
}

void EnablePwr(void) {
	/* Enable 1.8 V */
	//gpio_set_value(Power_PIN_1_8V, 1);
	gpio_request(Power_PIN_1_8V, NULL);
	gpio_direction_output(Power_PIN_1_8V, 1);
	gpio_free(Power_PIN_1_8V);
	
	/* Enable 1.2 V */
	gpio_request(Power_PIN_1_2V, NULL);
	gpio_direction_output(Power_PIN_1_2V, 1);
	gpio_free(Power_PIN_1_2V);
}

void EnableExtClk() {
	/* Enable ext clk */
	//nmk_config_pins(RawChip_Clock_pins_high, ARRAY_SIZE(RawChip_Clock_pins_high));
	printk("RAW chip enable clk but not applied in sprd platform\n");
}

void DisableExtClk() {
	/* Disable ext clock */
	//nmk_config_pins(RawChip_Clock_pins_low, ARRAY_SIZE(RawChip_Clock_pins_low));
}

void EnableRst() {
	/* Enable reset */
	gpio_request(Yushan_Reset_PIN, NULL);
	gpio_direction_output(Yushan_Reset_PIN, 0);
	mdelay(3);
	gpio_direction_output(Yushan_Reset_PIN, 1);
	gpio_free(Yushan_Reset_PIN);
	printk("reset high of rawchip\n");
}

void DisableRst() {
	/* Enable reset */
	gpio_request(Yushan_Reset_PIN, NULL);
	gpio_direction_output(Yushan_Reset_PIN, 0);
	gpio_free(Yushan_Reset_PIN);
	printk("reset low of rawchip\n");
}

struct platform_device *yushan_pdev;
static struct task_struct *init_rawchip_tsk;

#if USE_NEW_YUSHAN_LAUNCH_WAY
static int __init yushan_open_init_thread(void)
{
    int ret = 0;
//		mutex_lock(&yushan_init_lock);
		if (!bIsYushan_Init) {
    	init_rawchip_tsk = kthread_create(Yushan_sensor_open_init_worker, NULL, "rawchip_init_worker");
    	wake_up_process(init_rawchip_tsk);
  	}
//		mutex_unlock(&yushan_init_lock);
    return 0;
}	

static int Yushan_sensor_open_init_worker(void *arg)
{
    if(!bIsYushan_Init)
			Yushan_sensor_open_init();
	
		bYushan_init_function_done = true;
    return 0;
}
#endif

int Yushan_sensor_open_init(void)//(const struct msm_camera_sensor_info *data)
{
	/* color bar test */
#if COLOR_BAR	
	int32_t rc = 0;  
	ASIC_Test();
#else	

 	bool_t	bBypassDxoUpload = 0;
	if(bypass_dxo)
		bBypassDxoUpload = 1;
	else
		bBypassDxoUpload = 0;
	
	Yushan_Version_Info_t sYushanVersionInfo;
	uint8_t			bPixelFormat = RAW10;
	/*
	Yushan_GainsExpTime_t sGainsExpTime;
	Yushan_DXO_DPP_Tuning_t sDxoDppTuning;
	Yushan_DXO_PDP_Tuning_t sDxoPdpTuning;
	Yushan_DXO_DOP_Tuning_t sDxoDopTuning;
	*/
  Yushan_SystemStatus_t			sSystemStatus;
	uint8_t bPdpMode = 0, bDppMode = 0, bDopMode = 0;
	uint8_t bStatus;
	uint32_t uWordCount   = 0;

	int32_t rc = 0;  
	//uint32_t		udwIntrMask[] = {0x00130C30, 0x00000000, 0x00030000};
	//uint32_t		udwIntrMask[] = {0x7DE38E3B,0xFC7C7C7C,0x001B7FFB};
	uint32_t		udwIntrMask[] = {0x7DE38E3B,0xFC3C3C7C,0x001B7FFB};

	uint16_t          uwSpiData = 0x1E00;
	
	CDBG("kuei, Yushan_sensor_open_init start\n");
	/* If the data type is Raw10, we need to convert the pixel count to word count.
	   Line length * 10 / 8 */
	if (MIPI_DATA_TYPE_RAW10 == RAW10_DATA_TYPE)
		uWordCount = v_preview_sensor_width * 10 / 8;

    SPI_Write(0x1018, 2, (unsigned char*)&uwSpiData);

#endif
  	 //yushan_sysfs_init();
#if COLOR_BAR
#else

	CDBG("kuei, YUSHAN_VERSION = %d\n", YUSHAN_VERSION);
	#if USE_NEW_YUSHAN_LAUNCH_WAY
	CDBG("kuei, USE_NEW_YUSHAN_LAUNCH_WAY\n");
	#else
	CDBG("kuei, USE_ORIGINAL_YUSHAN_LAUNCH_WAY\n");
	#endif

	// Default Values
	sInitStruct.bNumberOfLanes		=	v_data_line;
	sInitStruct.fpExternalClock		=	v_ext_clock;
	sInitStruct.uwPixelFormat		  =	v_pixel_format;
	
	if ((sInitStruct.uwPixelFormat&0x0F) == 0x0A) {
	  	bPixelFormat = RAW10;
	} else if((sInitStruct.uwPixelFormat&0x0F)==0x08) {
		if(((sInitStruct.uwPixelFormat>>8)&0x0F)==0x08)
				bPixelFormat= RAW8;
		else // 10 to 8 case
			bPixelFormat = RAW10_8;
	}
#endif
  
  rc = yushan_create_irq();
  if(rc < 0)
  	CDBG("kuei, yushan_create_irq() fail\n");
  else
  	CDBG("kuei, yushan_create_irq() success\n");
  	
	rc = yushan_create_irq2();
  if(rc < 0)
  	CDBG("kuei, yushan_create_irq2() fail");

  rawchip_intr0 = RAWCHIP_GPIO_IRQ;
  rawchip_intr1 = RAWCHIP_GPIO_IRQ2;
#if COLOR_BAR	  
#else
	sDxoStruct.pDxoPdpRamImage[0]= (uint8_t *)yushan_regs->pdpcode;
	sDxoStruct.pDxoDppRamImage[0]= (uint8_t *)yushan_regs->dppcode;
	sDxoStruct.pDxoDopRamImage[0]= (uint8_t *)yushan_regs->dopcode;
	sDxoStruct.pDxoPdpRamImage[1]= (uint8_t *)yushan_regs->pdpclib;
	sDxoStruct.pDxoDppRamImage[1]= (uint8_t *)yushan_regs->dppclib;
	sDxoStruct.pDxoDopRamImage[1]= (uint8_t *)yushan_regs->dopclib;

	sDxoStruct.uwDxoPdpRamImageSize[0]= yushan_regs->pdpcode_size;
	sDxoStruct.uwDxoDppRamImageSize[0]= yushan_regs->dppcode_size;
	sDxoStruct.uwDxoDopRamImageSize[0]= yushan_regs->dopcode_size;
	sDxoStruct.uwDxoPdpRamImageSize[1]= yushan_regs->pdpclib_size;
	sDxoStruct.uwDxoDppRamImageSize[1]= yushan_regs->dppclib_size;
	sDxoStruct.uwDxoDopRamImageSize[1]= yushan_regs->dopclib_size;

	sDxoStruct.uwBaseAddrPdpMicroCode[0] = yushan_regs->pdpcode_first_addr;
	sDxoStruct.uwBaseAddrDppMicroCode[0] = yushan_regs->dppcode_first_addr;
	sDxoStruct.uwBaseAddrDopMicroCode[0] = yushan_regs->dopcode_first_addr;
	sDxoStruct.uwBaseAddrPdpMicroCode[1] = yushan_regs->pdpclib_first_addr;
	sDxoStruct.uwBaseAddrDppMicroCode[1] = yushan_regs->dppclib_first_addr;
	sDxoStruct.uwBaseAddrDopMicroCode[1] = yushan_regs->dopclib_first_addr;
	
	sDxoStruct.uwDxoPdpBootAddr = yushan_regs->pdpBootAddr;
	sDxoStruct.uwDxoDppBootAddr = yushan_regs->dppBootAddr;
	sDxoStruct.uwDxoDopBootAddr = yushan_regs->dopBootAddr;

	sDxoStruct.uwDxoPdpStartAddr = yushan_regs->pdpStartAddr;
	sDxoStruct.uwDxoDppStartAddr = yushan_regs->dppStartAddr;
	sDxoStruct.uwDxoDopStartAddr = yushan_regs->dopStartAddr;

	sImageChar.bImageOrientation = v_image_orientation;
	sImageChar.uwXAddrStart	= v_imgchar_uwxaddrstart;
	sImageChar.uwYAddrStart	= v_imgchar_uwyaddrstart;
	sImageChar.uwXAddrEnd	= v_imgchar_uwxaddrend;
	sImageChar.uwYAddrEnd	= v_imgchar_uwyaddrend;
	sImageChar.uwXEvenInc	     = v_imgchar_xeveninc;
	sImageChar.uwXOddInc         = v_imgchar_xoddinc;
	sImageChar.uwYEvenInc        = v_imgchar_yeveninc;
	sImageChar.uwYOddInc	       = v_imgchar_yoddinc;
	sImageChar.bBinning          = v_imgchar_binning;

	memset(sInitStruct.sFrameFormat , 0 , sizeof (Yushan_Frame_Format_t)*15);

	// HTC_CAM_START, Sensor configuration
	sInitStruct.bNumberOfLanes				      = v_data_line;
	sInitStruct.uwPixelFormat               = v_pixel_format;
	sInitStruct.uwBitRate                   = v_mipi_data_rate;
	sInitStruct.fpExternalClock             = v_ext_clock;
	sInitStruct.fpSpiClock                  = v_spi_clock;
	sInitStruct.uwActivePixels				      = v_preview_sensor_width;
	sInitStruct.uwLineBlankVf		            = v_line_blank;
	sInitStruct.uwLineBlankStill		        = v_line_blank;
	sInitStruct.uwLines			                = v_preview_sensor_height;
	sInitStruct.uwFrameBlank		            = v_frame_blank;
	sInitStruct.bUseExternalLDO 			      = 1; 
	sInitStruct.bDxoSettingCmdPerFrame 		  = 1; 

	sInitStruct.sFrameFormat[0].uwWordcount 		   = uWordCount;
	sInitStruct.sFrameFormat[0].bDatatype	  		   = RAW10_DATA_TYPE;
	sInitStruct.sFrameFormat[0].bActiveDatatype 	 = 1;
	sInitStruct.sFrameFormat[0].bSelectStillVfMode = YUSHAN_FRAME_FORMAT_STILL_MODE;

	sInitStruct.sFrameFormat[1].uwWordcount 		   = 820 * 10 / 8;     //slow motion
	sInitStruct.sFrameFormat[1].bDatatype	  		   = RAW10_DATA_TYPE;
	sInitStruct.sFrameFormat[1].bActiveDatatype 	 = 1;
	sInitStruct.sFrameFormat[1].bSelectStillVfMode = YUSHAN_FRAME_FORMAT_VF_MODE;
	
	sInitStruct.sFrameFormat[2].uwWordcount        = 1472 * 10 / 8;
	sInitStruct.sFrameFormat[2].bActiveDatatype    = 1;
	sInitStruct.sFrameFormat[2].bDatatype	  		   = RAW10_DATA_TYPE;
	sInitStruct.sFrameFormat[2].bSelectStillVfMode = YUSHAN_FRAME_FORMAT_NORMAL_MODE;
/*
	sInitStruct.sFrameFormat[3].uwWordcount        = SENSOR_WIDTH_4X * 10 / 8;
	sInitStruct.sFrameFormat[3].bActiveDatatype    = 0;
	sInitStruct.sFrameFormat[3].bDatatype	  		   = DATA_EMBEDDED_LINE;
	sInitStruct.sFrameFormat[3].bSelectStillVfMode = YUSHAN_FRAME_FORMAT_NORMAL_MODE;
*/
	sInitStruct.bValidWCEntries = 3;

  // HTC_CAM_END, Sensor configuration

	sGainsExpTime.uwAnalogGainCodeGR		= v_uwAnalogGainCodeGR		;
	sGainsExpTime.uwAnalogGainCodeR			= v_uwAnalogGainCodeR		;
	sGainsExpTime.uwAnalogGainCodeB			= v_uwAnalogGainCodeB		;
	sGainsExpTime.uwPreDigGainGR			= v_uwPreDigGainGR		;
	sGainsExpTime.uwPreDigGainR			= v_uwPreDigGainR		;
	sGainsExpTime.uwPreDigGainB			= v_uwPreDigGainB		;
	sGainsExpTime.uwExposureTime			= v_uwExposureTime		;
	sGainsExpTime.bRedGreenRatio			= v_bRedGreenRatio		;
	sGainsExpTime.bBlueGreenRatio			= v_bBlueGreenRatio		;

	sDxoDppTuning.uwFlashPreflashRating 		= v_uwFlashPreflashRating 	;
	sDxoDppTuning.bFocalInfo			= v_bFocalInfo			;

	sDxoPdpTuning.bDeadPixelCorrectionLowGain	= v_bDeadPixelCorrectionLowGain	;
	sDxoPdpTuning.bDeadPixelCorrectionMedGain	= v_bDeadPixelCorrectionMedGain	;
	sDxoPdpTuning.bDeadPixelCorrectionHiGain	= v_bDeadPixelCorrectionHiGain	;

	sDxoDopTuning.bEstimationMode			= v_bEstimationMode		;
	sDxoDopTuning.bSharpness			= v_bSharpness			;
	sDxoDopTuning.bDenoisingLowGain			= v_bDenoisingLowGain		;
	sDxoDopTuning.bDenoisingMedGain			= v_bDenoisingMedGain		;
	sDxoDopTuning.bDenoisingHiGain			= v_bDenoisingHiGain		;
	sDxoDopTuning.bNoiseVsDetailsLowGain		= v_bNoiseVsDetailsLowGain	;
	sDxoDopTuning.bNoiseVsDetailsMedGain		= v_bNoiseVsDetailsMedGain	;
	sDxoDopTuning.bNoiseVsDetailsHiGain		= v_bNoiseVsDetailsHiGain	;
	sDxoDopTuning.bTemporalSmoothing		= v_bTemporalSmoothing		;

	gPllLocked = 0;
	CDBG("Yushan_Init_Clocks\n");
	bStatus=Yushan_Init_Clocks(&sInitStruct, &sSystemStatus, udwIntrMask) ;
	if (bStatus != 1)
	{
		CDBG("kuei, Yushan_common_init, ->->->->->-> Clock Init FAILED <-<-<-<-<-<-\n");
		CDBG("->->->->->-> Clock Init FAILED <-<-<-<-<-<-");
		CDBG("Yushan_common_init Yushan_Init_Clocks=%d\n", bStatus);
    CDBG("Min Value Required %d\n", sSystemStatus.udwDxoConstraintsMinValue);	
		CDBG("Error Code : %d \n",sSystemStatus.bDxoConstraints);
		return FAILURE;
	}
	else
	{
		CDBG("kuei, Yushan_common_init, Clock Init Done\n");
		CDBG("Clock Init Done \n");
	}

	/* Pll Locked. Done to allow auto incremental SPI transactions. */
	gPllLocked = 1;

	CDBG("kuei, Yushan_common_init >>>> Yushan_Init\n");
	CDBG("kuei, sInitStruct.sFrameFormat[1].uwWordcount = %d\n", sInitStruct.sFrameFormat[1].uwWordcount);
	bStatus = Yushan_Init(&sInitStruct);
	CDBG("kuei, Yushan_common_init Yushan_Init = %d\n", bStatus);
	if(bypass_dxo){
		CDBG("kuei, Bypass mode\n");
		Yushan_DXO_DTFilter_Bypass();    // Enter DxO IP but no image processing
//		Yushan_DXO_Lecci_Bypass();     // Don't enter DxO IP
	}
	else{
		CDBG("kuei, NOT bypass mode\n");
	}

	// Initialize DCPX and CPX of Yushan
	if (bPixelFormat == RAW10_8) {
		Yushan_DCPX_CPX_Enable(); 
	}

	if (bStatus == 0)
	{
		CDBG("kuei, Yushan_common_init, ->->->->->-> Yushan Init FAILED <-<-<-<-<-<-\n");
		return 0;
	}
	else
	{
		CDBG("kuei, Yushan_common_init, Yushan Init success\n");
	}


	if(bypass_dxo){
		CDBG("kuei, Yushan_common_init, Bypass DxO releated init functions \n");		
	}
	else{
	#if DxO	
		CDBG("kuei, Yushan_common_init Yushan_Init_Dxo\n");
	
		bStatus= Yushan_Init_Dxo(&sDxoStruct, bBypassDxoUpload);
		CDBG("kuei, Yushan_common_init Yushan_Init_Dxo = %d\n", bStatus);
		if ( bStatus == 1 ) {
			CDBG("kuei, DXO Upload and Init Done\n");
		} else {
			CDBG("kuei, DXO Upload and Init FAILED\n");
			/* return -1; */
		}
	
		//DumpPDP_Registers();
		CDBG("[CAM] Yushan_common_init Yushan_Get_Version_Information\n");
	
		bStatus=Yushan_Get_Version_Information(&sYushanVersionInfo );
		#if 1
			CDBG("Yushan_common_init Yushan_Get_Version_Information=%d\n", bStatus);
		
			CDBG("kuei, API Version : %d.%d \n", sYushanVersionInfo.bApiMajorVersion, sYushanVersionInfo.bApiMinorVersion);
			CDBG("kuei, DxO Pdp Version : %x \n", sYushanVersionInfo.udwPdpVersion);
			CDBG("kuei, DxO Dpp Version : %x \n", sYushanVersionInfo.udwDppVersion);
			CDBG("kuei, DxO Dop Version : %x \n", sYushanVersionInfo.udwDopVersion);
			CDBG("kuei, DxO Pdp Calibration Version : %x \n", sYushanVersionInfo.udwPdpCalibrationVersion);
			CDBG("kuei, DxO Dpp Calibration Version : %x \n", sYushanVersionInfo.udwDppCalibrationVersion);
			CDBG("kuei, DxO Dop Calibration Version : %x \n", sYushanVersionInfo.udwDopCalibrationVersion);
		#endif
	#endif
	}

	/* Updating DXO */
	#if ENABLE_TEST_PATTERN
		Yushan_PatternGenerator(&sInitStruct, 1, 1);
	#endif
	Yushan_Update_ImageChar(&sImageChar);


	if(bypass_dxo){
	}
	else{
		Yushan_Update_SensorParameters(&sGainsExpTime);
		Yushan_Update_DxoDpp_TuningParameters(&sDxoDppTuning);
		Yushan_Update_DxoDop_TuningParameters(&sDxoDopTuning);
		Yushan_Update_DxoPdp_TuningParameters(&sDxoPdpTuning);
		
		bStatus = Yushan_Update_Commit(PDPMode,DPPMode,DOPMode);
		
		if (bStatus == 1) {
			CDBG("kuei, Yushan_Update_Commit Done\n");
		}
		else {
			CDBG("kuei, Yushan_Update_Commit FAILED\n");
		}
	}

#if 0
	bPdpMode = 1; bDppMode = 3; bDopMode = 1;
	bStatus = Yushan_Update_Commit(bPdpMode,bDppMode,bDopMode);
	CDBG("Yushan_common_init ROI, Yushan_Update_Commit = %d\n", bStatus);

	if ( bStatus == 1 ) 
		CDBG("kuei, ROI, DXO Commit Done\n");
	else {
		CDBG("kuei, ROI, DXO Commit FAILED\n");
		/* return -1; */
	}
#endif
	CDBG("----**** Start the sensor ****---- \n"); 
	CDBG("kuei, Yushan_common_init, Start the sensor\n");
#if 0
	// disable extra INT
	bSpiData = 0;
	SPI_Write(YUSHAN_SMIA_FM_EOF_INT_EN, 1,  &bSpiData);	
#endif	

		CDBG("kuei, Yushan_sensor_open_init end\n");
		bIsYushan_Init = true;

	  return bStatus;
#endif

	  return 1;
}

/*********************************************************************
Function to programm AF_ROI and Check the stats too
**********************************************************************/
bool_t Yushan_Dxo_Dop_Af_Run(Yushan_AF_ROI_t	*sYushanAfRoi, uint32_t *pAfStatsGreen, uint8_t	bRoiActiveNumber)
{
	uint8_t		bStatus = SUCCESS;
	uint8_t		bPdpMode=0, bDppMode=0, bDopMode=0;

	/* Recommended by DXO */
	bPdpMode = 1; bDppMode=3; bDopMode = 1;
	
	if (bRoiActiveNumber)
	{
		/* Program the AF_ROI */
		bStatus = Yushan_AF_ROI_Update(&sYushanAfRoi[0], bRoiActiveNumber);

		bStatus = Yushan_Update_Commit(PDPMode,DPPMode,DOPMode);

		/* Reading the AF Statistics */
		bStatus &= Yushan_WaitForInterruptEvent(EVENT_DXODOP7_NEWFRAMEPROC_ACK, TIME_20MS);
	}

	DumpDOP_Registers();

	return SUCCESS;
}

/*********************************************************************
Function: Context Update Wrapper
**********************************************************************/
Yushan_ImageChar_t	sImageChar_context;
bool_t	Yushan_ContextUpdate_Wrapper(Yushan_New_Context_Config_t	*sYushanNewContextConfig)
{
	
	bool_t	bStatus = SUCCESS, bReconfigRequired = 1;
	uint8_t	bPdpMode=0, bDppMode=0, bDopMode=0;

CDBG("> kuei, Yushan_ContextUpdate_Wrapper\n");	

	if (bReconfigRequired)
	{	
		CDBG("Reconfiguration starts\n");
		CDBG("kuei, Yushan_ContextUpdate_Wrapper\n");
		bStatus = Yushan_Context_Config_Update(sYushanNewContextConfig);
		sYushanAEContextConfig = (Yushan_New_Context_Config_t*)&sYushanNewContextConfig;
	}
	else
		return SUCCESS;

	// Now update the DXO
	sImageChar_context.bImageOrientation = v_image_orientation;
	sImageChar_context.uwXAddrStart      = 0;
	sImageChar_context.uwYAddrStart      = 0;
	sImageChar_context.uwXEvenInc        = 1;
	sImageChar_context.uwXOddInc         = 1;
	sImageChar_context.uwYEvenInc        = 1;
	sImageChar_context.uwYOddInc         = 1;
	sImageChar_context.bBinning          = 0x11;

	sImageChar_context.uwXAddrEnd = sYushanNewContextConfig->uwActivePixels - 1;
	sImageChar_context.uwYAddrEnd = sYushanNewContextConfig->uwActiveFrameLength -1;

	Yushan_Update_ImageChar(&sImageChar_context);
	bStatus = Yushan_Update_Commit(PDPMode,DPPMode,DOPMode);	

	if (bStatus) 
	{
		CDBG("->->->->->-> DXO Commit, Post Context Reconfigration, Done <-<-<-<-<-<-");
	}
	else
	{
		CDBG("->->->->->-> DXO Commit, Post Context Reconfigration, FAILED <-<-<-<-<-<-");
	}
	CDBG("< kuei, Yushan_ContextUpdate_Wrapper\n");

	return bStatus;
}

void Yushan_Write_Exp_Time_Gain(uint16_t yushan_line, uint16_t yushan_gain)
{
	Yushan_GainsExpTime_t sGainsExpTime;

//CDBG("kuei, >> Yushan_Write_Exp_Time_Gain\n");

	//CDBG("<ChenC> Yushan_Write_Exp_Time_Gain, yushan_gain:%d, yushan_line:%d", yushan_gain, yushan_line);

	sGainsExpTime.uwAnalogGainCodeGR= yushan_gain;
	sGainsExpTime.uwAnalogGainCodeR=yushan_gain;
	sGainsExpTime.uwAnalogGainCodeB=yushan_gain;
	sGainsExpTime.uwPreDigGainGR= 0x100;
	sGainsExpTime.uwPreDigGainR= 0x100;
	sGainsExpTime.uwPreDigGainB= 0x100;
	//sGainsExpTime.uwExposureTime= yushan_line * (sYushanAEContextConfig->uwActivePixels + sYushanAEContextConfig->uwPixelBlank) /182400000;//*200/912; ;
	sGainsExpTime.uwExposureTime= yushan_line * (1304 + 1900) /182400000;//*200/912; ;
	sGainsExpTime.bRedGreenRatio=0x40;
	sGainsExpTime.bBlueGreenRatio=0x40;

	Yushan_Update_SensorParameters(&sGainsExpTime);	
//CDBG("kuei, << Yushan_Write_Exp_Time_Gain\n");
}

//Add raw chip as one device
#define MSM_RAWCHIP_NAME "rawchip"
struct msm_rawchip_device {
	struct platform_device *pdev;
	struct resource        *mem;
	int                     irq;
	void                   *base;

	struct device *device;
	struct cdev   cdev;
	struct mutex  lock;
	char	  open_count;
	uint8_t       op_mode;
};

static int msm_rawchip_open(struct inode *inode, struct file *filp)
{
	int rc;

	struct msm_rawchip_device *pgmn_dev = container_of(inode->i_cdev,
		struct msm_rawchip_device, cdev);
	filp->private_data = pgmn_dev;

//CDBG("> kuei, msm_rawchip_open\n");

	//CDBG("%s:%d]\n", __func__, __LINE__);

	rc = 0;//Yushan_sensor_open_init();//__msm_rawchip_open(pgmn_dev);

	//CDBG(KERN_INFO "%s:%d] %s open_count = %d\n", __func__, __LINE__,
	//	filp->f_path.dentry->d_name.name, pgmn_dev->open_count);

	return rc;
}

static int msm_rawchip_release(struct inode *inode, struct file *filp)
{
	int rc;

	unsigned int irq;
	struct msm_rawchip_device *pgmn_dev = filp->private_data;

	CDBG(KERN_INFO "%s:%d]\n", __func__, __LINE__);

	//rc = __msm_rawchip_release(pgmn_dev);

	CDBG(KERN_INFO "%s:%d] %s open_count = %d\n", __func__, __LINE__,
		filp->f_path.dentry->d_name.name, pgmn_dev->open_count);
	irq = (unsigned int)gpio_to_irq(RAWCHIP_GPIO_IRQ);
	free_irq(irq, NULL);
	irq = (unsigned int)gpio_to_irq(RAWCHIP_GPIO_IRQ2);
	free_irq(irq, NULL);
	
	return rc;
}

#define RX_FRAME_TH 5
uint32_t count = 0;
uint32_t frame_drop_cnt_for_4x = 0;
static bool bBypassIOTCL = false;

static long msm_rawchip_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	struct rawchip_ctrl *raw_dev = filp->private_data;
	void __user *argp = (void __user *)arg;
	uint32_t		udwSpiData = 0;
	
	mutex_lock(&my_wait_lock);
	switch (cmd) {
		case IOCTL_POWER_ON_RAWCHIP:
			//Yushan_Power_ON(0);
			break;

		case IOCTL_POWER_OFF_RAWCHIP:
			//Yushan_Power_OFF();
			break;

		case IOCTL_INIT_RAWCHIP:
			Yushan_Init_RawChip();

				rawchip_update_af_params_t update_af_params;
/*				
				update_af_params.af_params.sYushanAfRoi[0].bXStart = 7;
				update_af_params.af_params.sYushanAfRoi[0].bXEnd = 9;
				update_af_params.af_params.sYushanAfRoi[0].bYStart = 5;
				update_af_params.af_params.sYushanAfRoi[0].bYEnd = 7;
*/
				update_af_params.af_params.sYushanAfRoi[0].bXStart = 6;
				update_af_params.af_params.sYushanAfRoi[0].bXEnd = 10;
				update_af_params.af_params.sYushanAfRoi[0].bYStart = 4;
				update_af_params.af_params.sYushanAfRoi[0].bYEnd = 8;
				update_af_params.af_params.active_number = 1;
				Yushan_Update_AF_Params(&update_af_params);

			break;

		case IOCTL_ENABLE_YUSHAN_TX:
			//mutex_lock(&raw_dev->raw_ioctl_update_lock);
			CDBG("> kuei, IOCTL_ENABLE_YUSHAN_TX:");
			Yushan_Enable_Tx_HsClock(true);
			//mutex_unlock(&raw_dev->raw_ioctl_get_lock);
			break;
		case IOCTL_DISABLE_YUSHAN_TX:
			//mutex_lock(&raw_dev->raw_ioctl_update_lock);
			CDBG("> kuei, IOCTL_DISABLE_YUSHAN_TX:");
			Yushan_Enable_Tx_HsClock(false);
			//mutex_unlock(&raw_dev->raw_ioctl_get_lock);
			break;

		case IOCTL_DUMP_YUSHAN_REGISTERS:
			//mutex_lock(&raw_dev->raw_ioctl_update_lock);
			Yushan_Dump_Info();
			//mutex_unlock(&raw_dev->raw_ioctl_get_lock);
			break;
		case IOCTL_CHANGE_RESOLUTION_TO_4X:
			//mutex_lock(&raw_dev->raw_ioctl_update_lock);
			if (!bInNormalMode) {
				rawchip_update_af_params_t update_af_params;
				update_af_params.af_params.sYushanAfRoi[0].bXStart = 0;
				update_af_params.af_params.sYushanAfRoi[0].bXEnd = 15;
				update_af_params.af_params.sYushanAfRoi[0].bYStart = 0;
				update_af_params.af_params.sYushanAfRoi[0].bYEnd = 11;				
				update_af_params.af_params.active_number = 1;
				Yushan_Update_AF_Params(&update_af_params);
				
				CDBG("\n> kuei, IOCTL_CHANGE_RESOLUTION_TO_4X:\n");
				Yushan_Change_Resolition_To_4X();			
				CDBG("< kuei, IOCTL_CHANGE_RESOLUTION_TO_4X:\n");
				bInNormalMode = true;
				frame_drop_cnt_for_4x = 0;
			}
			//mutex_unlock(&raw_dev->raw_ioctl_get_lock);
			break;
			
		case IOCTL_CHANGE_RESOLUTION_TO_NORMAL:
			if (bInNormalMode) {
				//mutex_lock(&raw_dev->raw_ioctl_update_lock);
				CDBG("> kuei, IOCTL_CHANGE_RESOLUTION_TO_NORMAL:\n");
				rawchip_update_af_params_t update_af_params;
				update_af_params.af_params.sYushanAfRoi[0].bXStart = 6;
				update_af_params.af_params.sYushanAfRoi[0].bXEnd = 10;
				update_af_params.af_params.sYushanAfRoi[0].bYStart = 4;
				update_af_params.af_params.sYushanAfRoi[0].bYEnd = 8;				
				update_af_params.af_params.active_number = 1;
				Yushan_Update_AF_Params(&update_af_params);
				Yushan_Change_Resolition_To_Normal();
				CDBG("< kuei, IOCTL_CHANGE_RESOLUTION_TO_NORMAL:\n");
				//mutex_unlock(&raw_dev->raw_ioctl_get_lock);
				bInNormalMode = false;
			}
			break;

		case RAWCHIP_IOCTL_GET_INT:
			if(!bIsYushan_Init) break;
			//mutex_lock(&raw_dev->raw_ioctl_get_lock);
			CDBG(", kuei, RAWCHIP_IOCTL_GET_INT\n");
			rawchip_get_interrupt(raw_dev, argp);
			//mutex_unlock(&raw_dev->raw_ioctl_get_lock);
			break;
		case RAWCHIP_IOCTL_GET_AF_STATUS:
			if(!bIsYushan_Init) break;
			//mutex_lock(&raw_dev->raw_ioctl_get_lock);
			CDBG("RAWCHIP_IOCTL_GET_AF_STATUS\n");
			rawchip_get_af_status(raw_dev, argp);
			//mutex_unlock(&raw_dev->raw_ioctl_get_lock);
			break;
		case RAWCHIP_IOCTL_UPDATE_AEC_AWB:
			if(!bIsYushan_Init) break;
			//mutex_lock(&raw_dev->raw_ioctl_update_lock);
			CDBG("RAWCHIP_IOCTL_UPDATE_AEC\n");
			rawchip_update_aec_awb_params(raw_dev, argp);
			//mutex_unlock(&raw_dev->raw_ioctl_update_lock);
			break;
		case RAWCHIP_IOCTL_UPDATE_AF:
			if(!bIsYushan_Init) break;
			//mutex_lock(&raw_dev->raw_ioctl_update_lock);
			CDBG("RAWCHIP_IOCTL_UPDATE_AF\n");
			rawchip_update_af_params(raw_dev, argp);
			//mutex_unlock(&raw_dev->raw_ioctl_update_lock);
			break;
		case RAWCHIP_IOCTL_UPDATE_3A:
#if 0
			//if(!bIsYushan_Init || bBypassIOTCL) break;
			if (bBypassIOTCL) {
				frame_drop_cnt_for_4x++;
			}
			if(!bIsYushan_Init || bBypassIOTCL) {
				if (frame_drop_cnt_for_4x < 60) {
					break;
				}
			}
#else
			if(!bIsYushan_Init) break;
#endif
			//mutex_lock(&raw_dev->raw_ioctl_update_lock);
			CDBG("RAWCHIP_IOCTL_UPDATE_3A\n");
			rawchip_update_3a_params(raw_dev, argp);
			//mutex_unlock(&raw_dev->raw_ioctl_update_lock);
			break;

		case IOCTL_CHANGE_RAWCHIP_LOG_LEVEL:
			CDBG("IOCTL_CHANGE_RAWCHIP_LOG_LEVEL\n");
			rawchip_update_debug_level(raw_dev, argp);
			break;
			
		default:
			//CDBG("kuei, Unknown ioctl code\n");
			break;
	};
	mutex_unlock(&my_wait_lock);
	return rc;
}

static  const struct  file_operations msm_rawchip_fops = {
	.owner	  = THIS_MODULE,		
	.open	   = msm_rawchip_open,
	//.release	= msm_rawchip_release,
	.unlocked_ioctl = msm_rawchip_ioctl,
	.poll  = rawchip_fops_poll,
};

static struct class *msm_rawchip_class = NULL;
static dev_t msm_rawchip_devno;
static struct msm_rawchip_device *msm_rawchip_device_p;

static unsigned int rawchip_fops_poll(struct file *filp,
	struct poll_table_struct *pll_table)
{
	int rc = 0;
	unsigned long flags;
		
//	CDBG(" >  kuei, rawchip_fops_poll\n");
//HTC_START - prevent crash after enable rawchip
	if (!bIsYushan_Init)
		return 0;
//HTC_END - rawchip
	poll_wait(filp, &yushan_int.yushan_wait, pll_table);

	spin_lock_irqsave(&yushan_int.yushan_spin_lock, flags);
	if (atomic_read(&interrupt) || atomic_read(&interrupt2)) {
		atomic_set(&interrupt, 0);
		atomic_set(&interrupt2, 0);
		rc = POLLIN | POLLRDNORM;
	}
	spin_unlock_irqrestore(&yushan_int.yushan_spin_lock, flags);
	
//	CDBG("  kuei, rc = %d, rawchip_fops_poll, POLLIN = %d, POLLRDNORM = %d\n", rc, POLLIN, POLLRDNORM);

//	CDBG(" <  kuei, rawchip_fops_poll\n");
	return rc;
}

struct msm_rawchip_device *__msm_rawchip_init(struct platform_device *pdev)
{
	struct msm_rawchip_device *pgmn_dev;

	pgmn_dev = kzalloc(sizeof(struct msm_rawchip_device), GFP_ATOMIC);
	if (!pgmn_dev) {
		CDBG("%s:%d]no mem\n", __func__, __LINE__);
		return NULL;
	}
	

	mutex_init(&pgmn_dev->lock);

	pgmn_dev->pdev = pdev;
#if 0
	msm_rawchip_q_init("evt_q", &pgmn_dev->evt_q);
	msm_rawchip_q_init("output_rtn_q", &pgmn_dev->output_rtn_q);
	msm_rawchip_q_init("output_buf_q", &pgmn_dev->output_buf_q);
	msm_rawchip_q_init("input_rtn_q", &pgmn_dev->input_rtn_q);
	msm_rawchip_q_init("input_buf_q", &pgmn_dev->input_buf_q);
#endif

	return pgmn_dev;
}

static int msm_rawchip_init(struct platform_device *pdev)
{
	int rc = -1;
	struct device *dev;

	CDBG("> kuei, msm_rawchip_init\n");
	CDBG("%s:%d]\n", __func__, __LINE__);

	msm_rawchip_device_p = __msm_rawchip_init(pdev);
	if (msm_rawchip_device_p == NULL) {
		CDBG("> kuei, msm_rawchip_init, initialization failed\n");
		CDBG("%s: initialization failed\n", __func__);
		goto fail;
	}
	else
	{
		CDBG("> kuei, msm_rawchip_init, initialization success\n");
	}

	rc = alloc_chrdev_region(&msm_rawchip_devno, 0, 1, MSM_RAWCHIP_NAME);
	if (rc < 0) {
		CDBG("> kuei, msm_rawchip_init, failed to allocate chrdev\n");
		CDBG("%s: failed to allocate chrdev\n", __func__);
		goto fail_1;
	}
	else
	{
		CDBG("> kuei, msm_rawchip_init, allocate chrdev success\n");
	}

	if (!msm_rawchip_class) {
		msm_rawchip_class = class_create(THIS_MODULE, MSM_RAWCHIP_NAME);
		if (IS_ERR(msm_rawchip_class)) {
			CDBG("> kuei, msm_rawchip_init, create device class failed\n");
			rc = PTR_ERR(msm_rawchip_class);
			CDBG("%s: create device class failed\n",
				__func__);
			goto fail_2;
		}
		else
		{
			CDBG("> kuei, msm_rawchip_init, create device class success\n");
		}
	}

	dev = device_create(msm_rawchip_class, NULL,
		MKDEV(MAJOR(msm_rawchip_devno), MINOR(msm_rawchip_devno)), NULL,
		"%s%d", MSM_RAWCHIP_NAME, 0);

	if (IS_ERR(dev)) {
		CDBG("> kuei, msm_rawchip_init, error creating device\n");
		CDBG("%s: error creating device\n", __func__);
		rc = -ENODEV;
		goto fail_3;
	}
	else
	{
		CDBG("> kuei, msm_rawchip_init, creating device success\n");
	}

	cdev_init(&msm_rawchip_device_p->cdev, &msm_rawchip_fops);
	msm_rawchip_device_p->cdev.owner = THIS_MODULE;
	msm_rawchip_device_p->cdev.ops   =
		(const struct file_operations *) &msm_rawchip_fops;
	rc = cdev_add(&msm_rawchip_device_p->cdev, msm_rawchip_devno, 1);
	if (rc < 0) {
		CDBG("> kuei, msm_rawchip_init, error adding cdev\n");
		CDBG("%s: error adding cdev\n", __func__);
		rc = -ENODEV;
		goto fail_4;
	}
	else
	{
		CDBG("> kuei, msm_rawchip_init, adding cdev success\n");
	}

	CDBG("< kuei, %s %s: success\n", __func__, MSM_RAWCHIP_NAME);

	return rc;

fail_4:
	device_destroy(msm_rawchip_class, msm_rawchip_devno);

fail_3:
	class_destroy(msm_rawchip_class);

fail_2:
	unregister_chrdev_region(msm_rawchip_devno, 1);

fail_1:
	//__msm_rawchip_exit(msm_rawchip_device_p);

fail:
	return rc;
}

static void msm_rawchip_exit(void)
{
CDBG("> kuei, msm_rawchip_exit\n");
	cdev_del(&msm_rawchip_device_p->cdev);
	device_destroy(msm_rawchip_class, msm_rawchip_devno);
	class_destroy(msm_rawchip_class);
	unregister_chrdev_region(msm_rawchip_devno, 1);
}

static int __msm_rawchip_probe(struct platform_device *pdev)
{
	int rc;
CDBG("> kuei, __msm_rawchip_probe\n");

	rawchipCtrl = kzalloc(sizeof(struct rawchip_ctrl), GFP_ATOMIC);
	if (!rawchipCtrl) {
		pr_err("%s: could not allocate mem for rawchip_dev\n", __func__);
		return -ENOMEM;
	}
	
	//CDBG("%s\n", __func__);	
	yushan_pdev = pdev;
	rc = msm_rawchip_init(pdev);
	//rawchip_enable_spi2();
	mutex_init(&rawchipCtrl->raw_ioctl_get_lock);
	mutex_init(&rawchipCtrl->raw_ioctl_update_lock);
	mutex_init(&my_wait_lock);
	#if USE_NEW_YUSHAN_LAUNCH_WAY
//	mutex_init(&yushan_init_lock);
	#endif
	device_create_file(&pdev->dev, &dev_attr_rawchip_mode);
CDBG("< kuei, __msm_rawchip_probe\n");
	return rc;
}

static int __msm_rawchip_remove(struct platform_device *pdev)
{
	CDBG("> kuei, __msm_rawchip_remove\n");
	if(yushan_spi_write_addr != NULL) {
		/* 3 bytes one for Read(0x61), Write(0x60), two bytes for read/write address */
		kfree(yushan_spi_write_addr);
	}
	mutex_destroy(&rawchipCtrl->raw_ioctl_get_lock);
	mutex_destroy(&rawchipCtrl->raw_ioctl_update_lock);
	mutex_destroy(&my_wait_lock);
	#if USE_NEW_YUSHAN_LAUNCH_WAY
	//mutex_destroy(&yushan_init_lock);
	#endif
	msm_rawchip_exit();
	return 0;
}
	
static struct  platform_driver msm_rawchip_driver = {
	.probe  = __msm_rawchip_probe,
	.remove = __msm_rawchip_remove,
	
	.driver = {
		.name = "rawchip",
		.owner = THIS_MODULE,
	},
};

static int __init msm_rawchip_driver_init(void)
{
	int rc;

CDBG("\n> kuei, msm_rawchip_driver_init\n");
	//CDBG("%s\n", __func__);
	/*
	gpio_request(173, "rawchip");
	gpio_direction_input(173);
	gpio_free(173);
	*/
	printk("[RAW] open rawchip power\n");
  //mdelay(1);
  //EnablePwr();
  //mdelay(1);
  //EnableExtClk();
  //mdelay(1);
  //EnableRst();
  //mdelay(1);

	rc = platform_driver_register(&msm_rawchip_driver);
CDBG("< kuei, msm_rawchip_driver_init\n");
	
	return rc;
}

static void __exit msm_rawchip_driver_exit(void)
{
	CDBG("> kuei, msm_rawchip_driver_exit\n");
	platform_driver_unregister(&msm_rawchip_driver);
	CDBG("< kuei, msm_rawchip_driver_exit\n");
}

MODULE_DESCRIPTION("msm rawchip driver");
MODULE_VERSION("msm rawchip 0.1");

module_init(msm_rawchip_driver_init);
module_exit(msm_rawchip_driver_exit);

/*******************************************************************************
                           SPI related FUNCTIONS
*******************************************************************************/
int rawchip_spi_write_2B1B(uint16_t addr, unsigned char data)
{
	unsigned char buffer[4];
	int rc;

	if (!rawchip_dev)
		return 0;

	rawchip_dev->bits_per_word = 8;
	buffer[0] = 0x60;
	buffer[1] = addr >> 8;
	buffer[2] = addr & 0xff;
	buffer[3] = data;
	mutex_lock(&spi_rw_lock);
	rc = spi_write(rawchip_dev, buffer, 4);
	mutex_unlock(&spi_rw_lock);
	if(rc)
		return FAILURE;
	else
		return SUCCESS;
}

int rawchip_spi_read_2B1B(uint16_t addr, unsigned char* data)
{
	unsigned char buffer[3];
	int rc;

	if (!rawchip_dev)
		return FAILURE;

	rawchip_dev->bits_per_word = 8;
	buffer[0] = 0x60;
	buffer[1] = addr >> 8;
	buffer[2] = addr & 0xff;

	mutex_lock(&spi_rw_lock);
	rc = spi_write(rawchip_dev, buffer, 3);
	if(rc){
		mutex_unlock(&spi_rw_lock);
		return FAILURE; //in case error
	}

	buffer[0] = 0x61;
	rc = spi_write_then_read(rawchip_dev, buffer, 1, data, 1);
	mutex_unlock(&spi_rw_lock);
	if(rc)
		return FAILURE; //in case error
	else
		return SUCCESS;
}

int SPI_Read( uint16_t uwIndex , uint16_t uwCount , uint8_t * pData) {
	unsigned char buffer[3];
	int rc;

	if (!rawchip_dev)
		return FAILURE;

	if (gPllLocked) {
		rawchip_dev->bits_per_word = 8;
		buffer[0] = 0x60;
		buffer[1] = uwIndex >> 8;
		buffer[2] = uwIndex & 0xff;

		mutex_lock(&spi_rw_lock);
		rc = spi_write(rawchip_dev, buffer, 3);
		if(rc){
			mutex_unlock(&spi_rw_lock);
			return FAILURE; //in case error
		}

		buffer[0] = 0x61;
		rc = spi_write_then_read(rawchip_dev, buffer, 1, pData, uwCount);
		mutex_unlock(&spi_rw_lock);
		if(rc)
			return FAILURE; //in case error
		else
			return SUCCESS;
	}
	else{
		while(uwCount--){
			if(FAILURE == rawchip_spi_read_2B1B(uwIndex++, pData++))
				return FAILURE;
		}
		return SUCCESS;
	}
		
}

int SPI_Write( uint16_t uwIndex , uint16_t uwCount , uint8_t * pData) {
	unsigned char buffer[3];
	int rc;

	if (!rawchip_dev)
		return FAILURE;

	if (yushan_spi_write_addr && gPllLocked) {
		//could do bulk write
		rawchip_dev->bits_per_word = 8;
		yushan_spi_write_addr[0] = 0x60;
		yushan_spi_write_addr[1] = uwIndex >> 8;
		yushan_spi_write_addr[2] = uwIndex & 0xff;
		memcpy(&yushan_spi_write_addr[3], pData, uwCount);

		mutex_lock(&spi_rw_lock);
		rc = spi_write(rawchip_dev, yushan_spi_write_addr, uwCount + 3);
		mutex_unlock(&spi_rw_lock);
		if(rc)
			return FAILURE; //in case error
		else
			return SUCCESS;
	}
	else{
		while(uwCount--){
			if(FAILURE == rawchip_spi_write_2B1B(uwIndex++, *pData++))
				return FAILURE;
		}
		return SUCCESS;
	}
}

uint8_t yushan_spi_read(uint16_t reg) {
	uint8_t rval;

	rawchip_spi_read_2B1B(reg, &rval);

	return rval;
}

int yushan_spi_write(uint16_t reg, uint8_t val)
{
	return rawchip_spi_write_2B1B(reg, val);
}

int rawchip_spi_write(unsigned char addr, unsigned char data)
{
	return rawchip_spi_write_2B1B(addr, data);
}

int rawchip_spi_read_2B2B(uint16_t addr, uint16_t* data)
{
	return SPI_Read(addr, 2, data);
}

/*******************************************************************************
                           Yushan Testing FUNCTIONS
*******************************************************************************/
void Yushan_Function_Tester()
{
	uint32_t check_yushan_id;

	CDBG("> kuei, Yushan_Function_Tester\n");

	SPI_Read(0x5c04, 4, (uint8_t*)(&check_yushan_id));
	CDBG(KERN_ERR "> kuei, Yushan_Test_ChipStatus, Yushan ID = 0x%x\n", check_yushan_id);
	if (check_yushan_id == 0x02030200)
	{
		CDBG(KERN_ERR "> kuei, Yushan_Test_ChipStatus, Get correct raw chip ID\n");
	}	
	else
	{
		CDBG(KERN_ERR "> kuei, Yushan_Test_ChipStatus, Fail to get raw chip ID\n");
	}

	//Yushan_common_init();
	//Yushan_Color_Bar_Test();
	//Yushan_Interrupt_Tester();
	Yushan_sensor_open_init();

	CDBG("< kuei, Yushan_Function_Tester\n");
}

void Yushan_Interrupt_Tester(void)
{
	CDBG("> kuei, Yushan_Interrupt_Tester\n");

	CDBG("> kuei, Read previous ISR state\n");
	Yushan_ISR();
	CDBG("< kuei, Read previous ISR state\n");

	CDBG("> kuei, Yushan_Interrupt_Tester, verify interrupt 1\n");
	//Verify interrupt 1
	msleep(1000);
	CDBG("> kuei, Yushan_Interrupt_Tester, Enable CSI2RX interrupt\n");
	yushan_spi_write(0x0c14, 0x0f); // Enable CSI2RX interrupt
	msleep(1000);
	
	CDBG("> kuei, Yushan_Interrupt_Tester, Force to signal CSI2RX\n");
	yushan_spi_write(0x0c0c, 0x0f); // Force to signal CSI2RX

	Yushan_ISR();

	msleep(1000);
	yushan_spi_write(0x0c08, 0x0f);	// Clear interrupt
	CDBG("> kuei, Yushan_Interrupt_Tester, Clear interrupt\n");
	CDBG("> kuei, Yushan_Interrupt_Tester, verify interrupt 1 done\n");
	// Verify interrupt2
	yushan_spi_write(0x1030, 0x01); // Send CSI2RX interrupt group to INTR1
	msleep(1000);
	yushan_spi_write(0x0c14, 0x0f); // Enable CSI2RX interrupt
	msleep(1000);
	yushan_spi_write(0x0c0c, 0x0f); // Force the interrupt signal to 1
	msleep(1000);
	yushan_spi_write(0x0c08, 0x0f); // Clear interrupt
	
	CDBG("< kuei, Yushan_Interrupt_Tester\n");
}

int Yushan_sensor_open_init_testing(void)
{
	// It is supposed that from here SPI bus has been correctly 
	// Initialized. 
	//////////////////////////////////////// 
	// It is also supposed that from here 
	// BB RX is initialized and sensor is not streaming 
	//////////////////////////////////////// 
	///////// 
	// STEP 1: structures fillings 
	///////// 
	// Init structure 
	
	CDBG("> kuei, Yushan_sensor_open_init_testing\n");
	
	int RawFormat = RAW10;
	int bStatus;
	Yushan_Init_Struct_t	 m_sInitStruct;
	Yushan_SystemStatus_t	 sSystemStatus;
	uint32_t		udwIntrMask[] = {0x00130C30, 0x00000000, 0x00030000};
	
	m_sInitStruct.bNumberOfLanes = 2; 
	m_sInitStruct.uwBitRate = 844; 
	m_sInitStruct.fpExternalClock = 0x133333; 
	m_sInitStruct.fpSpiClock = 0x140000; 
	m_sInitStruct.bUseExternalLDO = 1; 
	m_sInitStruct.bDxoSettingCmdPerFrame = 1; 
	m_sInitStruct.uwLineBlankVf = 0; // No VF format defined 
	m_sInitStruct.uwActivePixels = 2608; 
	m_sInitStruct.uwLineBlankStill = 2738; 
	m_sInitStruct.uwLines = 1960; 
	m_sInitStruct.uwFrameBlank = 2082; 
	// Here is set only 1 frame format 
	memset(m_sInitStruct.sFrameFormat , 0 , sizeof (Yushan_Frame_Format_t)*1); 
	if (RawFormat == RAW10) { 
		// RAW 10 
		m_sInitStruct.uwPixelFormat = v_pixel_format; 
		m_sInitStruct.sFrameFormat[0].bDatatype = RAW10_DATA_TYPE; 
		m_sInitStruct.sFrameFormat[0].uwWordcount = (uint16_t) (m_sInitStruct.uwActivePixels*10/8);
/*	
		m_sInitStruct.sFrameFormat[1].uwWordcount 		= (uint16_t) (m_sInitStruct.uwActivePixels*10/8);
		m_sInitStruct.sFrameFormat[1].bDatatype	  		= 0x12;
*/	 
	} else if (RawFormat == RAW8) { 
		// RAW8 
		m_sInitStruct.uwPixelFormat = 0x0808; 
		m_sInitStruct.sFrameFormat[0].bDatatype = 0x2a; 
		m_sInitStruct.sFrameFormat[0].uwWordcount = m_sInitStruct.uwActivePixels; 
	} else if (RawFormat == RAW10_8) { 
		// RAW10_8 
		m_sInitStruct.uwPixelFormat = 0x0A08; 
		m_sInitStruct.sFrameFormat[0].bDatatype = 0x30; 
		m_sInitStruct.sFrameFormat[0].uwWordcount = m_sInitStruct.uwActivePixels; 
	} 
	m_sInitStruct.sFrameFormat[0].bActiveDatatype=1; 
	m_sInitStruct.sFrameFormat[0].bSelectStillVfMode=YUSHAN_FRAME_FORMAT_STILL_MODE; 
	m_sInitStruct.bValidWCEntries = 1;
	
	memset(&sSystemStatus, 0, sizeof(sSystemStatus)); 
	///////// 
	// End STEP 1: structures fillings 
	///////// 
	///////// 
	// STEP 2: Initialization of Yushan device 
	///////// 
	CDBG("> kuei, Yushan_Init_Clocks\n");
	bStatus=Yushan_Init_Clocks(&m_sInitStruct, &sSystemStatus, udwIntrMask) ; 
	CDBG("< kuei, Yushan_Init_Clocks\n");
	if ( bStatus == 0 ) { 
		// Clock Init Failed
		CDBG("kuei, Clock Init Failed\n"); 
		return 0; 
	} 
	// Setup interrupt PAD assignment 
	// Yushan_AssignInterruptGroupsToPad1(uwAssignITRGrpToPad1); 
	CDBG("> kuei, Yushan_Init\n");
	bStatus=Yushan_Init(&m_sInitStruct) ; 
	CDBG("< kuei, Yushan_Init\n");
	if ( bStatus == 0 ) { 
		CDBG("kuei, Yushan Init FAILED\n");
		//Yushan Init FAILED 
		return 0; 
	}
/*	 
	CDBG(KERN_ERR "> kuei, Yushan_Swap_Rx_Pins\n");
	Yushan_Swap_Rx_Pins(true, true, true, true, true);
	CDBG(KERN_ERR "< kuei, Yushan_Swap_Rx_Pins\n");
*/	
	// Initialize DCPX and CPX of Yushan if needed 
	if(RawFormat==RAW10_8) 
		Yushan_DCPX_CPX_Enable(); 
	///////// 
	// End STEP 2: Initialization of Yushan device 
	///////// 
	///////// 
	// STEP 3: Initialization of DxO pipe 
	///////// 
	// for DXO bypassing 
	Yushan_DXO_Lecci_Bypass(); 
	///////// 
	// End STEP 3: Initialization of DxO pipe 
	///////// 
	/// From now sensor can be runned 
  CDBG("< kuei, Yushan_sensor_open_init_testing\n");
	return 1;
}

void Yushan_Init_Test(void)
{
	CDBG("[ChenC]%s\n",__func__);
#if 0
	Yushan_Assert_Reset(0x001F0F10, RESET_MODULE);
	bSpiData =1;
	Yushan_DXO_Sync_Reset_Dereset(bSpiData);
	Yushan_Assert_Reset(0x001F0F10, DERESET_MODULE);
	bSpiData = 0;
	Yushan_DXO_Sync_Reset_Dereset(bSpiData);	
	Yushan_Init_Dxo(&sDxoStruct, 1);
	msleep(10);
#endif

CDBG("> kuei, msm_rawchip_ioctl, >> Yushan_sensor_open_init\n");
	Yushan_sensor_open_init();
CDBG("> kuei, msm_rawchip_ioctl, << Yushan_sensor_open_init\n");

CDBG("> kuei, msm_rawchip_ioctl, >> Yushan_ContextUpdate_Wrapper\n");
	Yushan_ContextUpdate_Wrapper(&sYushan4E1PreviewContextConfig);
CDBG("> kuei, msm_rawchip_ioctl, << Yushan_ContextUpdate_Wrapper\n");
}

/* Functions from PrimoU */
static int rawchip_get_interrupt(struct rawchip_ctrl *raw_dev, void __user *arg)
{
	int rc = 0;
	struct rawchip_stats_event_ctrl se;
	int timeout;
	uint8_t interrupt_type;

	CDBG("%s\n", __func__);

	if (copy_from_user(&se, arg,
			sizeof(struct rawchip_stats_event_ctrl))) {
		CDBG(" %s, ERR_COPY_FROM_USER\n", __func__);
		rc = -EFAULT;
		goto end;
	}

	timeout = (int)se.timeout_ms;

	interrupt_type = Yushan_parse_interrupt();
	se.type = 10;
	se.length = sizeof(interrupt_type);
	if (copy_to_user((void *)(se.data),
			&interrupt_type,
			se.length)) {
			CDBG(" %s, ERR_COPY_TO_USER 1\n", __func__);
		rc = -EFAULT;
		goto end;
	}

	if (copy_to_user((void *)arg, &se, sizeof(se))) {
		CDBG(" %s, ERR_COPY_TO_USER 2\n", __func__);
		rc = -EFAULT;
		goto end;
	}
end:
	return rc;
}

static int rawchip_get_af_status(struct rawchip_ctrl *raw_dev, void __user *arg)
{
	int rc = 0;
	struct rawchip_stats_event_ctrl se;
	int timeout;
	uint32_t pAfStatsGreen[20];

	CDBG("%s\n", __func__);

	if (copy_from_user(&se, arg,
			sizeof(struct rawchip_stats_event_ctrl))) {
		CDBG(" %s, ERR_COPY_FROM_USER\n", __func__);
		rc = -EFAULT;
		goto end;
	}

	timeout = (int)se.timeout_ms;

	rc = Yushan_get_AFSU(pAfStatsGreen);
	if (rc < 0) {
		CDBG(" %s, Yushan_get_AFSU failed\n", __func__);
		rc = -EFAULT;
		goto end;
	}
	se.type = 5;
	se.length = sizeof(pAfStatsGreen);

	if (copy_to_user((void *)(se.data),
			pAfStatsGreen,
			se.length)) {
			CDBG(" %s, ERR_COPY_TO_USER 1\n", __func__);
		rc = -EFAULT;
		goto end;
	}

	if (copy_to_user((void *)arg, &se, sizeof(se))) {
		CDBG(" %s, ERR_COPY_TO_USER 2\n", __func__);
		rc = -EFAULT;
		goto end;
	}

end:
	return rc;
}

static int rawchip_update_aec_awb_params(struct rawchip_ctrl *raw_dev, void __user *arg)
{
	struct rawchip_stats_event_ctrl se;
	rawchip_update_aec_awb_params_t *update_aec_awb_params;

	CDBG("%s\n", __func__);
	if (copy_from_user(&se, arg,
			sizeof(struct rawchip_stats_event_ctrl))) {
		CDBG(" %s, ERR_COPY_FROM_USER\n", __func__);
		return -EFAULT;
	}

	update_aec_awb_params = kmalloc(se.length, GFP_ATOMIC);
	if (!update_aec_awb_params) {
		CDBG(" %s %d: kmalloc failed\n", __func__,
			__LINE__);
		return -ENOMEM;
	}
	if (copy_from_user(update_aec_awb_params,
		(void __user *)(se.data),
		se.length)) {
		CDBG(" %s %d: copy_from_user failed\n", __func__,
			__LINE__);
		kfree(update_aec_awb_params);
		return -EFAULT;
	}

//	update_aec_awb_params->aec_params.gain = update_aec_awb_params->aec_params.gain / 2;
//	if (update_aec_awb_params->aec_params.gain >= 256)
//		update_aec_awb_params->aec_params.gain = 255;
		
//	CDBG("%s gain=%d line=%d\n", __func__,
//		update_aec_awb_params->aec_params.gain, update_aec_awb_params->aec_params.line);
	CDBG("%s rg_ratio=%d bg_ratio=%d\n", __func__,
		update_aec_awb_params->awb_params.rg_ratio, update_aec_awb_params->awb_params.bg_ratio);

	Yushan_Update_AEC_AWB_Params(update_aec_awb_params);

	kfree(update_aec_awb_params);
	return 0;
}

static int rawchip_update_af_params(struct rawchip_ctrl *raw_dev, void __user *arg)
{
	struct rawchip_stats_event_ctrl se;
	rawchip_update_af_params_t *update_af_params;

	CDBG("%s\n", __func__);
	if (copy_from_user(&se, arg,
			sizeof(struct rawchip_stats_event_ctrl))) {
		CDBG(" %s, ERR_COPY_FROM_USER\n", __func__);
		return -EFAULT;
	}

	update_af_params = kmalloc(se.length, GFP_ATOMIC);
	if (!update_af_params) {
		CDBG(" %s %d: kmalloc failed\n", __func__,
			__LINE__);
		return -ENOMEM;
	}
	if (copy_from_user(update_af_params,
		(void __user *)(se.data),
		se.length)) {
		CDBG(" %s %d: copy_from_user failed\n", __func__,
			__LINE__);
		kfree(update_af_params);
		return -EFAULT;
	}

	if (update_af_params->af_params.active_number > 0) {
	nROI_Number = update_af_params->af_params.active_number;
	
	CDBG("active_number=%d\n", update_af_params->af_params.active_number);
	CDBG("sYushanAfRoi[0] %d %d %d %d\n",
		update_af_params->af_params.sYushanAfRoi[0].bXStart,
		update_af_params->af_params.sYushanAfRoi[0].bXEnd,
		update_af_params->af_params.sYushanAfRoi[0].bYStart,
		update_af_params->af_params.sYushanAfRoi[0].bYEnd);

	Yushan_Update_AF_Params(update_af_params);
	}

	kfree(update_af_params);
	return 0;
}

static int rawchip_update_debug_level(struct rawchip_ctrl *raw_dev, void __user *arg)
{
	struct rawchip_stats_event_ctrl se;

	CDBG("%s\n", __func__);
	if (copy_from_user(&se, 
		                 arg,
			               sizeof(struct rawchip_stats_event_ctrl))) {
		CDBG(" %s, ERR_COPY_FROM_USER\n", __func__);
		return -EFAULT;
	}
	
	bShowHTCMsg = false;	
	// 32 bits to enable rawchip configuration
	// BIT 0: Enable rawchip's DTFilter bypass mode
	// BIT 1: Enable rawchip's Lecci bypass mode
	// BIT 2: Disable rawchip's PDP IP
	// BIT 3: Disable rawchip's DPP IP
	// BIT 4: Disable rawchip's DOP IP
	// BIT 8: Enable rawchip's debug message
	CDBG("se.logLevel = %d", se.logLevel);
	
	if (se.logLevel & 0x100) {
		bShowHTCMsg = true;
	}
	if (se.logLevel & 0x2) {
		bBypassIOTCL = false;
	}
	if (se.logLevel & 0x1) {
		bBypassIOTCL = true;
	}
	return 0;
}

static int rawchip_update_3a_params(struct rawchip_ctrl *raw_dev, void __user *arg)
{
	struct rawchip_stats_event_ctrl se;
	rawchip_newframe_ack_enable_t  *enable_newframe_ack;

	CDBG("%s\n", __func__);
	if (copy_from_user(&se, arg,
			sizeof(struct rawchip_stats_event_ctrl))) {
		CDBG(" %s, ERR_COPY_FROM_USER\n", __func__);
		return -EFAULT;
	}

	enable_newframe_ack = kmalloc(se.length, GFP_ATOMIC);
	if (!enable_newframe_ack) {
		CDBG(" %s %d: kmalloc failed\n", __func__,
			__LINE__);
		return -ENOMEM;
	}
	if (copy_from_user(enable_newframe_ack,
		(void __user *)(se.data),
		se.length)) {
		CDBG(" %s %d: copy_from_user failed\n", __func__,
			__LINE__);
		kfree(enable_newframe_ack);
		return -EFAULT;
	}

	CDBG("enable_newframe_ack=%d\n", *enable_newframe_ack);
	Yushan_Update_3A_Params(*enable_newframe_ack);
	CDBG("rawchip_update_3a_params done\n");

	kfree(enable_newframe_ack);
	return 0;
}

uint8_t Yushan_parse_interrupt(void)
{

	uint8_t		bCurrentInterruptID = 0;
	uint8_t		bAssertOrDeassert = 0, bInterruptWord = 0;
	/* The List consists of the Current asserted events. Have to be assignmed memory by malloc.
	*  and set every element to 0. */
	uint32_t	*udwListOfInterrupts;
	uint8_t	bSpiData;
	uint32_t udwSpiBaseIndex;
	uint8_t interrupt_type = 0;


	udwListOfInterrupts	= kmalloc(96, GFP_KERNEL);

	/* Call Yushan_Intr_Status_Read  for interrupts and add the same to the udwListOfInterrupts. */
	/* bInterruptID = Yushan_ReadIntr_Status();*/
	Yushan_Intr_Status_Read((uint8_t *)udwListOfInterrupts, INTERRUPT_PAD_0);

	/* Clear InterruptStatus */
	Yushan_Intr_Status_Clear((uint8_t *)udwListOfInterrupts);

	/* Adding the Current Interrupt asserted to the Proto List */
	while (bCurrentInterruptID < (TOTAL_INTERRUPT_COUNT + 1)) {
		bAssertOrDeassert = ((udwListOfInterrupts[bInterruptWord])>>(bCurrentInterruptID%32))&0x01;

		if (bAssertOrDeassert && interrupt_err_count < 10) {
			CDBG(" %s:bCurrentInterruptID:%d\n", __func__, bCurrentInterruptID+1);
			switch (bCurrentInterruptID+1) {
			case EVENT_PDP_EOF_EXECCMD :
				CDBG(" %s:[AF_INT]EVENT_PDP_EOF_EXECCMD\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_PDP_EOF_EXECCMD;
				break;

			case EVENT_DPP_EOF_EXECCMD :
				CDBG(" %s:[AF_INT]EVENT_DPP_EOF_EXECCMD\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_DPP_EOF_EXECCMD;
				break;

			case EVENT_DOP7_EOF_EXECCMD :
				CDBG(" %s:[AF_INT]EVENT_DOP7_EOF_EXECCMD\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_DOP_EOF_EXECCMD;
				break;

			case EVENT_DXODOP7_NEWFRAMEPROC_ACK :
				CDBG(" %s:[AF_INT]EVENT_DXODOP7_NEWFRAMEPROC_ACK\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_NEW_FRAME;
				break;

			case EVENT_CSI2RX_ECC_ERR :
				CDBGE(" %s:[ERR]EVENT_CSI2RX_ECC_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_CSI2RX_CHKSUM_ERR :
				CDBGE(" %s:[ERR]EVENT_CSI2RX_CHKSUM_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_CSI2RX_SYNCPULSE_MISSED :
				CDBGE(" %s:[ERR]EVENT_CSI2RX_SYNCPULSE_MISSED\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_DXOPDP_NEWFRAME_ERR :
				SPI_Read(DXO_PDP_BASE_ADDR + DxOPDP_error_code_7_0, 1, &bSpiData);
				CDBGE(" %s:[ERR]EVENT_DXOPDP_NEWFRAME_ERR, error code =%d\n", __func__, bSpiData);
				/* Reset_Yushan();*/
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_DXODPP_NEWFRAME_ERR :
				udwSpiBaseIndex = 0x010000;
				SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

				SPI_Read(DXO_DPP_BASE_ADDR + DxODPP_error_code_7_0-0x8000, 1, &bSpiData);
				CDBGE(" %s:[ERR]EVENT_DXODPP_NEWFRAME_ERR, error code =%d\n", __func__, bSpiData);

				udwSpiBaseIndex = 0x08000;
				SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
				/* Reset_Yushan();*/
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_DXODOP7_NEWFRAME_ERR :
				SPI_Read(DXO_DOP_BASE_ADDR + DxODOP_error_code_7_0, 1, &bSpiData);
				CDBGE(" %s:[ERR]EVENT_DXODOP7_NEWFRAME_ERR, error code =%d\n", __func__, bSpiData);
				/* Reset_Yushan();*/
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_CSI2TX_SP_ERR :
				CDBGE(" %s:[ERR]EVENT_CSI2TX_SP_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR_FATAL;
				break;

			case EVENT_CSI2TX_LP_ERR :
				CDBGE(" %s:[ERR]EVENT_CSI2TX_LP_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_CSI2TX_DATAINDEX_ERR :
				CDBGE(" %s:[ERR]EVENT_CSI2TX_DATAINDEX_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_SOFT_DL1 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_HARD_DL1 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_EOT_DL1 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_ESC_DL1 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_CTRL_DL1 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_SOFT_DL2 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_HARD_DL2 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_EOT_DL2 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_ESC_DL2 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_CTRL_DL2 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_SOFT_DL3 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_HARD_DL3 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_EOT_DL3 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_ESC_DL3 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_CTRL_DL3:
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_SOFT_DL4 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_SOFT_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_SOT_HARD_DL4 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_SOT_HARD_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_EOT_DL4:
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_EOT_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_ESC_DL4:
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_ESC_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_PHY_ERR_CTRL_DL4 :
				CDBGE(" %s:[ERR]EVENT_RX_PHY_ERR_CTRL_DL4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TXPHY_CTRL_ERR_D1 :
				CDBGE(" %s:[ERR]EVENT_TXPHY_CTRL_ERR_D1\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TXPHY_CTRL_ERR_D2 :
				CDBGE(" %s:[ERR]EVENT_TXPHY_CTRL_ERR_D2\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TXPHY_CTRL_ERR_D3 :
				CDBGE(" %s:[ERR]EVENT_TXPHY_CTRL_ERR_D3\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TXPHY_CTRL_ERR_D4 :
				CDBGE(" %s:[ERR]EVENT_TXPHY_CTRL_ERR_D4\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_UNMATCHED_IMAGE_SIZE_ERROR :
				CDBGE(" %s:[ERR]EVENT_UNMATCHED_IMAGE_SIZE_ERROR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case PRE_DXO_WRAPPER_PROTOCOL_ERR :
				CDBGE(" %s:[ERR]PRE_DXO_WRAPPER_PROTOCOL_ERR\n", __func__);
				/* Reset_Yushan();*/
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case PRE_DXO_WRAPPER_FIFO_OVERFLOW :
				CDBGE(" %s:[ERR]PRE_DXO_WRAPPER_FIFO_OVERFLOW\n", __func__);
				/* Reset_Yushan();*/
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_BAD_FRAME_DETECTION :
				CDBGE(" %s:[ERR]EVENT_BAD_FRAME_DETECTION\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TX_DATA_FIFO_OVERFLOW :
				CDBGE(" %s:[ERR]EVENT_TX_DATA_FIFO_OVERFLOW\n", __func__);
				/* Reset_Yushan();*/
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TX_INDEX_FIFO_OVERFLOW :
				CDBGE(" %s:[ERR]EVENT_TX_INDEX_FIFO_OVERFLOW\n", __func__);
				/* Reset_Yushan();*/
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_0_ERR :
				CDBGE(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_0_ERR\n", __func__);
				// interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_1_ERR :
				CDBGE(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_1_ERR\n", __func__);
				// interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_2_ERR :
				CDBGE(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_2_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_3_ERR :
				CDBGE(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_3_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_4_ERR :
				CDBGE(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_4_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_5_ERR :
				CDBGE(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_5_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_6_ERR :
				CDBGE(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_6_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_RX_CHAR_COLOR_BAR_7_ERR :
				CDBGE(" %s:[ERR]EVENT_RX_CHAR_COLOR_BAR_7_ERR\n", __func__);
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_POST_DXO_WRAPPER_PROTOCOL_ERR :
				CDBGE(" %s:[ERR]EVENT_POST_DXO_WRAPPER_PROTOCOL_ERR\n", __func__);
				/* Reset_Yushan();*/
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_POST_DXO_WRAPPER_FIFO_OVERFLOW :
				CDBGE(" %s:[ERR]EVENT_POST_DXO_WRAPPER_FIFO_OVERFLOW\n", __func__);
				/* Reset_Yushan();*/
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TX_DATA_UNDERFLOW :
				CDBGE(" %s:[ERR]EVENT_TX_DATA_UNDERFLOW\n", __func__);
				/* Reset_Yushan();*/
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			case EVENT_TX_INDEX_UNDERFLOW :
				CDBGE(" %s:[ERR]EVENT_TX_INDEX_UNDERFLOW\n", __func__);
				/* Reset_Yushan();*/
				interrupt_type |= RAWCHIP_INT_TYPE_ERROR;
				break;

			}
		}
		if(interrupt_err_count > 10){
			printk("[CAM] rawchip error > 10\n");
		}
		bCurrentInterruptID++;

		if (bCurrentInterruptID%32 == 0)
			bInterruptWord++;
	}

	kfree(udwListOfInterrupts);

	if (gpio_get_value(rawchip_intr0) == 1) {
		atomic_set(&interrupt, 1);
		wake_up(&yushan_int.yushan_wait);
	}
	if (gpio_get_value(rawchip_intr1) == 1) {
		atomic_set(&interrupt2, 1);
		wake_up(&yushan_int.yushan_wait);
	}

	if (interrupt_type & RAWCHIP_INT_TYPE_ERROR) {
		error_cnt++;
		if (bCanDoSnapshot) {
		    bCanDoSnapshot = false;
		    Yushan_Status_Snapshot();
	        }
	}

	if ((interrupt_type & RAWCHIP_INT_TYPE_ERROR) && interrupt_err_count <= 10) {
		interrupt_err_count++;
		printk("[CAM] rawchip err cnt ++,%d\n",interrupt_err_count);
	}

	return interrupt_type;
}

int Yushan_get_AFSU(uint32_t *pAfStatsGreen)
{

	uint8_t		bStatus = SUCCESS;
	uint16_t frameIdx = 0;

	CDBG(" nROI_Number = %d\n", nROI_Number);
	bStatus = Yushan_Read_AF_Statistics(pAfStatsGreen, nROI_Number, &frameIdx); // 20120618, This ROI number should change ~
	//bStatus = Yushan_Read_AF_Statistics(pAfStatsGreen, 1, &frameIdx); // 20120618, This ROI number should change ~
	if (bStatus == FAILURE) {
		CDBG(" Get AFSU statistic data fail\n");
		return -1;
	}
	CDBG(" GET_AFSU:frameIdx: %d G:%d, R:%d, B:%d, confi:%d\n",
		frameIdx, pAfStatsGreen[0], pAfStatsGreen[1], pAfStatsGreen[2], pAfStatsGreen[3]);
	return 0;
}
int cnt = 0;

int Yushan_Update_AEC_AWB_Params(rawchip_update_aec_awb_params_t *update_aec_awb_params)
{
	uint8_t		bStatus = SUCCESS;
	Yushan_GainsExpTime_t sGainsExpTime;

	CDBG("kuei, Yushan_Update_AEC_AWB_Params 0\n");

	// For STE Project, there 3A value 65536 means 1X gain
	// and 1X gain need to set 16 in rawchip's AE gain.
	// So if the 3A gain is 65536 * 2 it means 2X gain
	// and we need to set 32 to rawchip
	// Maximum gain is 16. So the maximum value in rawchip is 256
	sGainsExpTime.uwAnalogGainCodeGR = update_aec_awb_params->aec_params.a_gain;
	sGainsExpTime.uwAnalogGainCodeR = update_aec_awb_params->aec_params.a_gain;
	sGainsExpTime.uwAnalogGainCodeB = update_aec_awb_params->aec_params.a_gain;
	//sGainsExpTime.uwPreDigGainGR = 0x100;
	//sGainsExpTime.uwPreDigGainR = 0x100;
	//sGainsExpTime.uwPreDigGainB = 0x100;
	sGainsExpTime.uwPreDigGainGR = update_aec_awb_params->aec_params.d_gain;
	sGainsExpTime.uwPreDigGainR = update_aec_awb_params->aec_params.d_gain;
	sGainsExpTime.uwPreDigGainB = update_aec_awb_params->aec_params.d_gain;
	//sGainsExpTime.uwExposureTime = (uint16_t)((19*update_aec_awb_params->aec_params.line/1000));
	sGainsExpTime.uwExposureTime = update_aec_awb_params->aec_params.exp_time;
	sGainsExpTime.bRedGreenRatio = update_aec_awb_params->awb_params.rg_ratio;
	sGainsExpTime.bBlueGreenRatio = update_aec_awb_params->awb_params.bg_ratio;
		
	CDBG("kuei, Yushan_Update_AEC_AWB_Params update_aec_awb_params->aec_params.exp_time = %d\n", update_aec_awb_params->aec_params.exp_time);
	CDBG("kuei, Yushan_Update_AEC_AWB_Params 1\n");
#if 0
	if (sGainsExpTime.bRedGreenRatio == 0)
		sGainsExpTime.bRedGreenRatio = 0x40;
	if (sGainsExpTime.bBlueGreenRatio == 0)
		sGainsExpTime.bBlueGreenRatio = 0x40;
#endif

	CDBG(" uwExposureTime: %d\n", sGainsExpTime.uwExposureTime);
	bStatus = Yushan_Update_SensorParameters(&sGainsExpTime);

	return (bStatus == SUCCESS) ? 0 : -1;
}

int Yushan_Update_AF_Params(rawchip_update_af_params_t *update_af_params)
{
	uint8_t bStatus = SUCCESS;
	
	if (update_af_params->af_params.active_number > 0) {
		CDBG("Yushan_Update_AF_Paramsm active_number=%d\n", update_af_params->af_params.active_number);
	bStatus = Yushan_AF_ROI_Update(&update_af_params->af_params.sYushanAfRoi[0],
		update_af_params->af_params.active_number);
	nROI_Number = update_af_params->af_params.active_number;
	}
	return (bStatus == SUCCESS) ? 0 : -1;
}

int Yushan_Update_3A_Params(rawchip_newframe_ack_enable_t enable_newframe_ack)
{
	
	uint8_t bStatus = SUCCESS;
/*
	uint32_t		enableIntrMask[]  = {0x7DF38E3B, 0xA600007C, 0x000DBFFD};
	uint32_t		disableIntrMask[] = {0x7DE38E3B, 0xA600007C, 0x000DBFFD};

	if (enable_newframe_ack == RAWCHIP_NEWFRAME_ACK_ENABLE)
		Yushan_Intr_Enable((uint8_t *)enableIntrMask);
	else if (enable_newframe_ack == RAWCHIP_NEWFRAME_ACK_DISABLE)
		Yushan_Intr_Enable((uint8_t *)disableIntrMask);
*/
//	uint32_t		enableIntrMask[]  = {0x7DF38E3B,0xFC7C7C7C,0x001B7FFB};
//	uint32_t		disableIntrMask[] = {0x7DE38E3B,0xFC7C7C7C,0x001B7FFB};

	uint32_t		enableIntrMask[]  = {0x7DF38E3B,0xFC3C3C7C,0x001B7FFB};
	uint32_t		disableIntrMask[] = {0x7DE38E3B,0xFC3C3C7C,0x001B7FFB};

	if (enable_newframe_ack == RAWCHIP_NEWFRAME_ACK_ENABLE)
		Yushan_Intr_Enable((uint8_t *)enableIntrMask);
	else if (enable_newframe_ack == RAWCHIP_NEWFRAME_ACK_DISABLE)
		Yushan_Intr_Enable((uint8_t *)disableIntrMask);

	bStatus = Yushan_Update_Commit(PDPMode,DPPMode,DOPMode);
	if (bStatus == SUCCESS)
		CDBG("kuei, Yushan_Update_3A_Params, success\n");
	else
		CDBG("kuei, Yushan_Update_3A_Params, fail\n");
	return (bStatus == SUCCESS) ? 0 : -1;
	return 0;
}

/*debug file system*/
static int raw_get(void *data, u64 *val)
{
	int ret = -EINVAL;

	*val = 0x9999;

	return *val;
}

static int raw_set(void *data, u64 val)
{
	printk("get %x\n", val);
	if (val)
		return -EINVAL;
	else
		return val;
}

DEFINE_SIMPLE_ATTRIBUTE(rawchip_dbf_fops, raw_get,
			raw_set, "%llu\n");

static int raw_dbf_setup(void)
{
	struct dentry *d;
	d = debugfs_create_dir("rawchip", NULL);
	if (!(IS_ERR_OR_NULL(d))){
		(void) debugfs_create_file("op", S_IRUGO|S_IWUSR, d,
			NULL, &rawchip_dbf_fops);
		debugfs_create_u32("v_image_orientation", 0644, d,
			   &v_image_orientation);
		debugfs_create_u32("v_preview_sensor_width", 0644, d,
			   &v_preview_sensor_width);
		debugfs_create_u32("v_preview_sensor_height", 0644, d,
			   &v_preview_sensor_height);
		debugfs_create_u32("v_line_blank", 0644, d,
			   &v_line_blank);
		debugfs_create_u32("v_frame_blank", 0644, d,
			   &v_frame_blank);
		debugfs_create_u32("v_data_line", 0644, d,
			   &v_data_line);
		debugfs_create_u32("v_ext_clock", 0644, d,
			   &v_ext_clock);
		debugfs_create_u32("v_mipi_data_rate", 0644, d,
			   &v_mipi_data_rate);
		debugfs_create_u32("v_spi_clock", 0644, d,
			   &v_spi_clock);
		debugfs_create_u32("bypass_dxo", 0644, d,
			   &bypass_dxo);
		debugfs_create_u32("front_main", 0644, d,
			   &front_main);
	}
}
