#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h> 
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h> 
#include <linux/io.h>     
#include <linux/version.h>
#include <linux/timer.h>         
#include <linux/input.h>      
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <mach/hardware.h>
#include <mach/pinmap.h>

#ifdef PRIMOU_REWORK
#undef PRIMOU_REWORK
#endif
#define DEBUG_ON 1

#if DEBUG_ON
#define GPSD(fmt, arg...) printk(KERN_DEBUG "[GPS].(DEBUG) "fmt"", ##arg)
#define GPSE(fmt, arg...) printk(KERN_ERR "[GPS].(ERROR) "fmt"", ##arg)
#else
#define GPSD(...) ((void *)0)
#define GPSE(fmt, arg...) printk(KERN_ERR "[GPS].(ERROR) "fmt"", ##arg)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
#define LINUX_3
#endif

#define GPS_ONOFF_VERSION "1.0.0"
#define DEVICE_NAME "gps_onoff"


#define GPIO_GPS_ONOFF		(168)
#define GPIO_GPS_TXD		(18)
#define GPIO_GPS_RTS		(21)


typedef enum{
	
	CLK_NONE,
	CLK_AUX0,
	CLK_AUX1
}CLK_TYPE;

CLK_TYPE clk_type;

int gps_onoff_major = -1;
static struct class  *gps_onoff_class = NULL;   /* GPS class during class_create */
static struct device *gps_onoff_dev   = NULL;   /* GPS dev during device_create */

//static struct regulator *cmmb_regulator_1v8 = NULL;

char status = '0';


typedef struct {
    uint32_t reg;
    uint32_t val;
} pinmap_t;

static pinmap_t gps_on_pinmap[] = {

	{REG_PIN_U3TXD,               BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_OE},
    {REG_PIN_U3RXD,               BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_NUL|BIT_PIN_SLP_WPU|BIT_PIN_SLP_IE},
    {REG_PIN_U3CTS,               BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_IE},
    {REG_PIN_U3RTS,               BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_OE},
	{REG_PIN_NFD13,               BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_OE},	//GPIO168 GPS_ONOFF
    {REG_PIN_CLK_AUX0,            BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},   //GPIO209 CLK_AUX0
    {REG_PIN_LCD_D23,             BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(2)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},   //GPIO131 CLK_AUX1
};

static pinmap_t gps_off_pinmap[] = {

	{REG_PIN_U3TXD,               BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_OE},
    {REG_PIN_U3RXD,               BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_WPU|BIT_PIN_SLP_IE},
    {REG_PIN_U3CTS,               BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_IE},
    {REG_PIN_U3RTS,               BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_OE},
	{REG_PIN_NFD13,               BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_OE},	//GPIO168 GPS_ONOFF
    {REG_PIN_CLK_AUX0,            BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},   //GPIO209 CLK_AUX0
    {REG_PIN_LCD_D23,             BIT_PIN_NULL|BITS_PIN_DS(1)|BITS_PIN_AF(2)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},   //GPIO131 CLK_AUX1
};

static int config_gps_pin(pinmap_t *pinmap)
{
	int i;

	for(i=0;i<5;i++){
		__raw_writel(pinmap[i].val,CTL_PIN_BASE + pinmap[i].reg);
	}	

	switch(clk_type){
		case CLK_NONE:
					break;
		case CLK_AUX0:
					__raw_writel(pinmap[5].val,CTL_PIN_BASE + pinmap[5].reg);
					break;
		case CLK_AUX1:
					__raw_writel(pinmap[6].val,CTL_PIN_BASE + pinmap[6].reg);
	}

	return 0;
}


static void gps_clk_exit(CLK_TYPE type)
{
	struct clk *gps_clk;

    switch(type)
    {
        case CLK_NONE:
                        break;
        case CLK_AUX0:
                        gps_clk = clk_get(NULL,"clk_aux0");
                        clk_disable(gps_clk);
                        clk_put(gps_clk);
                        break;
        case CLK_AUX1:
                        gps_clk = clk_get(NULL,"clk_aux1");
                        clk_disable(gps_clk);
                        clk_put(gps_clk);
                        break;
    }

}


static void gps_clk_init(CLK_TYPE type)
{
	struct clk *gps_clk;
	struct clk *clk_parent;

    switch(type)
    {   
        case CLK_NONE:
                        break;
        case CLK_AUX0:
                        gps_clk = clk_get(NULL,"clk_aux0");
                        clk_parent = clk_get(NULL,"ext_32k");
                        if(IS_ERR(gps_clk) || IS_ERR(clk_parent))
                        {   
                            GPSD("failed to get clk\n");
                            break;
                        }
                        clk_set_parent(gps_clk,clk_parent);
                        clk_set_rate(gps_clk, 32768);
                        clk_enable(gps_clk);
                        break;
        case CLK_AUX1:
                        gps_clk = clk_get(NULL,"clk_aux1");
                        clk_parent = clk_get(NULL,"ext_32k");
                        if(IS_ERR(gps_clk) || IS_ERR(clk_parent))
                        {
                            GPSD("failed to get clk\n");
                            break;
                        }
                        clk_set_parent(gps_clk,clk_parent);
                        clk_set_rate(gps_clk, 32768);
                        clk_enable(gps_clk);
                        break;
    }
}

static long gps_onoff_ioctl(
#ifndef LINUX_3
		struct inode *inode,
#endif
		struct file *filp , unsigned int cmd, unsigned long arg)
{
	char str[5];

	switch(cmd){
		case 1:
			gpio_direction_output(GPIO_GPS_ONOFF,1);
			status = '1';
			GPSD("set the gps_engine on\n");
			break;
		case 0:
			gpio_direction_output(GPIO_GPS_ONOFF,0);
			status = '0';
			GPSD("set the gps_engine off\n");
			break;
		case 2:
				gps_clk_exit(clk_type);
				switch(arg){
					case 0:
							clk_type = CLK_AUX0;
							memcpy(str,"aux0",4);
							break;
					case 1:
							clk_type = CLK_AUX1;
							memcpy(str,"aux1",4);
							break;
					default:
							clk_type = CLK_NONE;
							memcpy(str,"none",4);
							break;
				}	
				str[4] = 0;
				gps_clk_init(clk_type);
				config_gps_pin(gps_on_pinmap);
				GPSD("clk switch to %s!\n",str);
				break;
	}
	return 0;
}

static int gps_onoff_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_ops)
{
	char data[2];
	char str[5];
	
	if( copy_from_user(data, buf, count) ) {
		GPSE("write data error\n");
		return -1;
	}
	
	switch(data[0]){
		case '0':
				config_gps_pin(gps_off_pinmap);
				gpio_direction_output(GPIO_GPS_TXD,1);
				gpio_direction_output(GPIO_GPS_RTS,1);
				gpio_direction_output(GPIO_GPS_ONOFF,0);
				status = '0';
				GPSD("reset GPIO OK!\n");
				break;
		case '1':
				config_gps_pin(gps_on_pinmap);
				gpio_direction_output(GPIO_GPS_ONOFF,1);
				status = '1';
				GPSD("set GPIO OK!\n");
				break;
		case '2':
				gps_clk_exit(clk_type);
				switch(buf[1]){
					case '0':
							clk_type = CLK_AUX0;
							memcpy(str,"aux0",4);
							break;
					case '1':
							clk_type = CLK_AUX1;
							memcpy(str,"aux1",4);
							break;
					default:
							clk_type = CLK_NONE;
							memcpy(str,"none",4);
							break;
				}	
				str[4] = 0;
				gps_clk_init(clk_type);
				config_gps_pin(gps_on_pinmap);
				GPSD("clk changed to %s!\n",str);
				break;
	}
	
	return count;
}

static int gps_onoff_read(struct file *filp, char *buf, size_t count, loff_t *f_ops)
{
	char str[5];
	unsigned long ret;

	if(status)
		ret = copy_to_user(buf,"1",1);
	else
		ret = copy_to_user(buf,"0",1);
	
	switch(clk_type)
	{
		case CLK_NONE:
				memcpy(str,"none",4);
				break;	
		case CLK_AUX0:
				memcpy(str,"aux0",4);
				break;	
		case CLK_AUX1:
				memcpy(str,"axu1",4);
				break;	
	}

	GPSD("read:power[%d],clk[%s]!\n",status-'0',str);
	return 0;
}


struct file_operations gps_onoff_ops={
	.owner	= THIS_MODULE,
	.write	= gps_onoff_write,
	.read	= gps_onoff_read,
#ifdef LINUX_3
	.unlocked_ioctl = 
#else
	.ioctl	= 
#endif
	gps_onoff_ioctl,
};

//static unsigned gps_onoff_config = GPIO_CFG(GPS_ONOFF, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA);



static int gps_onoff_init(struct platform_device *platdev)
{
	
	GPSD("In gps_onoff_init\n");

	//enable 1V8 power
	//cmmb_regulator_1v8 = regulator_get(NULL, "vddcmmb1p8");
       // regulator_set_voltage(cmmb_regulator_1v8, 1800000, 2200000);
       // regulator_disable(cmmb_regulator_1v8);
       // msleep(3);
       // regulator_enable(cmmb_regulator_1v8);
       // msleep(5);
	
#if 0	
	switch(z4dtg_get_board_revision()){
	
		case BOARD_EVM:
				clk_type = CLK_AUX0;break;
		default:
				clk_type = CLK_AUX1;
	}
#endif		

	clk_type = CLK_NONE;
	gps_clk_init(clk_type);

	gpio_request(GPIO_GPS_ONOFF,"GPS_ONOFF");
	gpio_request(GPIO_GPS_TXD,"GPS_TXD");
	gpio_request(GPIO_GPS_RTS,"GPS_RTS");

	config_gps_pin(gps_on_pinmap);
	gpio_direction_output(GPIO_GPS_ONOFF,0);
	
	gps_onoff_major = register_chrdev(0, DEVICE_NAME, &gps_onoff_ops);
	if(gps_onoff_major < 0){
		GPSE("register gps_onoff device err\n");
		goto err4;
	}
	GPSD(KERN_INFO "%s , major %d",DEVICE_NAME, gps_onoff_major);
	
	gps_onoff_class = class_create(THIS_MODULE, DEVICE_NAME);
	if(gps_onoff_class == NULL){
		GPSE("gps_onoff class create err\n");
		goto err5;
	}
	gps_onoff_dev = device_create(gps_onoff_class, NULL, MKDEV(gps_onoff_major, 0), NULL, DEVICE_NAME);
	if(gps_onoff_dev == NULL){
		GPSE("gps_onoff dev create err\n");
		goto err6;
	}

	GPSD("init OK\n");

	return 0;

//	device_destroy(gpsdrv_class, MKDEV(gpsdrv_major, 0));
err6:
	class_destroy(gps_onoff_class);
err5:
	unregister_chrdev(gps_onoff_major, DEVICE_NAME);
err4:	
	return -1;
}

static int  gps_onoff_exit(struct platform_device *platdev )
{
	GPSD("free up\n");
	config_gps_pin(gps_off_pinmap);
	gpio_direction_output(GPIO_GPS_ONOFF,0);

	gpio_free(GPIO_GPS_ONOFF);
	gpio_free(GPIO_GPS_TXD);
	gpio_free(GPIO_GPS_RTS);

	gps_clk_exit(clk_type);
	device_destroy(gps_onoff_class, MKDEV(gps_onoff_major, 0));
	class_destroy(gps_onoff_class);
	unregister_chrdev(gps_onoff_major, DEVICE_NAME);
	return 0;
}


static int gps_suspend(struct platform_device *platdev, pm_message_t state)
{
	GPSD("In gps_suspend function\n");
	
	config_gps_pin(gps_off_pinmap);
	gpio_direction_output(GPIO_GPS_TXD,1);
	gpio_direction_output(GPIO_GPS_RTS,1);
	
	return 0;
}

static int gps_resume(struct platform_device *platdev)
{
	GPSD("In gps_resume function\n");
	
	config_gps_pin(gps_on_pinmap);

	return 0;
}


static struct platform_driver gps_ctl_drv={
	.probe = gps_onoff_init,
	.remove = gps_onoff_exit,
	.suspend = gps_suspend,
	.resume = gps_resume,
	.driver = {
		.name = "gps_ctl",
		.owner = THIS_MODULE,
	},

};


static struct platform_device gps_ctl_dev={
	    .name="gps_ctl",
		.id=0,
};

/*
static int __init gps_ctl_device_init(void)
{
	return platform_device_register(&gps_ctl_dev);
}

static void __exit gps_ctl_device_exit(void)
{
	platform_device_unregister(&gps_ctl_dev);
}
*/

static int __init gps_ctl_init(void)
{
	int ret;
	ret = platform_device_register(&gps_ctl_dev);
	if(ret < 0){
		GPSE("platform_device register error!\n");
		return ret;
	}

	ret = platform_driver_register(&gps_ctl_drv);
	if(ret < 0)
	{
		GPSE("platform_driver register error!\n");
		platform_device_unregister(&gps_ctl_dev);
		return ret;
	}
	
	return 0;
}

static void __exit gps_ctl_exit(void)
{
	platform_device_unregister(&gps_ctl_dev);
	platform_driver_unregister(&gps_ctl_drv);
}

module_init(gps_ctl_init);
module_exit(gps_ctl_exit);

MODULE_AUTHOR("Derek Guo<Derek_Guo@htc.com>");
MODULE_DESCRIPTION("GPS power control");
MODULE_LICENSE("GPL");
MODULE_VERSION(GPS_ONOFF_VERSION);
