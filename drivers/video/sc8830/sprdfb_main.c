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
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/fb.h>

#include "sprdfb.h"
#include "sprdfb_panel.h"
#include <mach/board.h>
#include <mach/board_htc.h>
#include <linux/regulator/consumer.h>


enum{
	SPRD_IN_DATA_TYPE_ABGR888 = 0,
	SPRD_IN_DATA_TYPE_BGR565,
/*
	SPRD_IN_DATA_TYPE_RGB666,
	SPRD_IN_DATA_TYPE_RGB555,
	SPRD_IN_DATA_TYPE_PACKET,
*/ /*not support*/
	SPRD_IN_DATA_TYPE_LIMIT
};

#define SPRDFB_IN_DATA_TYPE SPRD_IN_DATA_TYPE_ABGR888

#ifdef CONFIG_TRIPLE_FRAMEBUFFER
#define FRAMEBUFFER_NR		(3)
#else
#define FRAMEBUFFER_NR		(2)
#endif

#define SPRDFB_DEFAULT_FPS (60)

#define SPRDFB_ESD_TIME_OUT_CMD	(2000)

#define SPRDFB_ESD_TIME_OUT_VIDEO	(5000)

extern bool sprdfb_panel_get(struct sprdfb_device *dev);
extern int sprdfb_panel_probe(struct sprdfb_device *dev);

struct regulator *lcd_regulator_1v8 = NULL;
struct regulator *lcd_regulator_3v0 = NULL;

extern void sprdfb_panel_remove(struct sprdfb_device *dev);

extern struct display_ctrl sprdfb_dispc_ctrl ;

static unsigned PP[16];

static int sprdfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fb);
static int sprdfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *fb);
static int sprdfb_ioctl(struct fb_info *info, unsigned int cmd,
			unsigned long arg);
static int sprdfb_mmap(struct fb_info *info, struct vm_area_struct *vma);

#ifdef CONFIG_FB_CABC_LEVEL_CONTROL
unsigned long cabc_level_ctl_status = 0;
unsigned long cabc_level_ctl_status_old = 0;
#endif

static struct fb_ops sprdfb_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = sprdfb_check_var,
	.fb_pan_display = sprdfb_pan_display,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_ioctl = sprdfb_ioctl,
	.fb_mmap = sprdfb_mmap,
};

static int setup_fb_mem(struct sprdfb_device *dev, struct platform_device *pdev)
{
	uint32_t len, addr;

	len = dev->panel->width * dev->panel->height * (dev->bpp / 8) * FRAMEBUFFER_NR;
#ifndef  CONFIG_FB_LCD_RESERVE_MEM
	addr = __get_free_pages(GFP_ATOMIC | __GFP_ZERO, get_order(len));
	if (!addr) {
		printk(KERN_ERR "[DISP] sprdfb: Failed to allocate framebuffer memory\n");
		return -ENOMEM;
	}
	pr_debug(KERN_INFO "[DISP] sprdfb:  got %d bytes mem at 0x%lx\n", len, addr);

	dev->fb->fix.smem_start = __pa(addr);
	dev->fb->fix.smem_len = len;
	dev->fb->screen_base = (char*)addr;
#else
	dev->fb->fix.smem_start = FB_BASE_ADDR;
	printk("[DISP] sprdfb:setup_fb_mem--smem_start:%lx,len:%d\n",dev->fb->fix.smem_start,len);
	addr =	(uint32_t)ioremap(FB_BASE_ADDR, len);
	if (!addr) {
	        printk(KERN_ERR "[DISP] Unable to map framebuffer base: 0x%08x\n", addr);
			return -ENOMEM;
		}
		dev->fb->fix.smem_len = len;
		dev->fb->screen_base =	(char*)addr;
#endif
	return 0;
}

static void setup_fb_info(struct sprdfb_device *dev)
{
	struct fb_info *fb = dev->fb;
	struct panel_spec *panel = dev->panel;
	int r;

	fb->fbops = &sprdfb_ops;
	fb->flags = FBINFO_DEFAULT;

	/* finish setting up the fb_info struct */
	strncpy(fb->fix.id, "sprdfb", 16);
	fb->fix.ypanstep = 1;
	fb->fix.type = FB_TYPE_PACKED_PIXELS;
	fb->fix.visual = FB_VISUAL_TRUECOLOR;
	fb->fix.line_length = panel->width * dev->bpp / 8;

	fb->var.xres = panel->width;
	fb->var.yres = panel->height;
	fb->var.width = panel->width;
	fb->var.height = panel->height;
	fb->var.xres_virtual = panel->width;
	fb->var.yres_virtual = panel->height * FRAMEBUFFER_NR;
	fb->var.bits_per_pixel = dev->bpp;
	if(0 != dev->panel->fps){
//		fb->var.pixclock = (1000 * 1000 * 1000) / (dev->panel->fps * panel->width * panel->height) * 1000;
		fb->var.pixclock = ((1000000000 /panel->width) * 1000) / (dev->panel->fps * panel->height);
	}else{
//		fb->var.pixclock = (1000 * 1000 * 1000) / (SPRDFB_DEFAULT_FPS * panel->width * panel->height) * 1000;
		fb->var.pixclock = ((1000000000 /panel->width) * 1000) / (SPRDFB_DEFAULT_FPS * panel->height);
	}
	fb->var.accel_flags = 0;
	fb->var.yoffset = 0;

	/* only support two pixel format */
	if (dev->bpp == 32) { /* ABGR */
		fb->var.red.offset     = 24;
		fb->var.red.length     = 8;
		fb->var.red.msb_right  = 0;
		fb->var.green.offset   = 16;
		fb->var.green.length   = 8;
		fb->var.green.msb_right = 0;
		fb->var.blue.offset    = 8;
		fb->var.blue.length    = 8;
		fb->var.blue.msb_right = 0;
	} else { /*BGR*/
		fb->var.red.offset     = 11;
		fb->var.red.length     = 5;
		fb->var.red.msb_right  = 0;
		fb->var.green.offset   = 5;
		fb->var.green.length   = 6;
		fb->var.green.msb_right = 0;
		fb->var.blue.offset    = 0;
		fb->var.blue.length    = 5;
		fb->var.blue.msb_right = 0;
	}
	r = fb_alloc_cmap(&fb->cmap, 16, 0);
	fb->pseudo_palette = PP;

	PP[0] = 0;
	for (r = 1; r < 16; r++){
		PP[r] = 0xffffffff;
	}
}

static void fb_free_resources(struct sprdfb_device *dev)
{
	if (dev == NULL)
		return;

	if (&dev->fb->cmap != NULL) {
		fb_dealloc_cmap(&dev->fb->cmap);
	}
	if (dev->fb->screen_base) {
		free_pages((unsigned long)dev->fb->screen_base,
				get_order(dev->fb->fix.smem_len));
	}
	unregister_framebuffer(dev->fb);
	framebuffer_release(dev->fb);
}

static int32_t first_time = 1;
static int sprdfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	int32_t ret;
	struct sprdfb_device *dev = fb->par;

	pr_debug("[DISP] sprdfb: [%s]\n", __FUNCTION__);

	if (first_time) {
		first_time = 0;
		memset(dev->fb->screen_base, 0, dev->fb->fix.smem_len);
		return 0;
	}

	if(0 == dev->enable){
		printk(KERN_ERR "[DISP] sprdfb:[%s]: Invalid Device status %d", __FUNCTION__, dev->enable);
		return -1;
	}

#if defined (CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
	ktime_t start_time, end_time;
	s64 delta_time;
	start_time = ktime_get();
#endif

	ret = dev->ctrl->refresh(dev);

#if defined (CONFIG_MACH_CP5DTU) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_CP5DWG)
	end_time = ktime_get();
	delta_time = ktime_to_ms(ktime_sub(end_time, start_time));
	if (delta_time > 30)
		printk(KERN_DEBUG "[DISP] %s() start time = %lld, end time = %lld, delta time = %lld\n",
			__func__, ktime_to_us(start_time), ktime_to_us(end_time),delta_time);
#endif

	if (ret) {
		printk(KERN_ERR "[DISP] sprdfb: failed to refresh !!!!\n");
		return -1;
	}

#ifdef CONFIG_FB_CABC_LEVEL_CONTROL
	if (cabc_level_ctl_status_old != cabc_level_ctl_status) {
		if (dev->panel->ops->set_cabc != NULL) {
			printk(KERN_INFO "[DISP] sprdfb: [%s] cabc_level= %ld\n", __FUNCTION__, cabc_level_ctl_status);
			down(&dev->sem);
			dev->panel->ops->set_cabc(dev->panel, cabc_level_ctl_status);
			cabc_level_ctl_status_old = cabc_level_ctl_status;
			up(&dev->sem);
		}
	}
#endif

	return 0;
}

#include <video/sprd_fb.h>
static int sprdfb_ioctl(struct fb_info *info, unsigned int cmd,
			unsigned long arg)
{
	int result = 0;
	struct sprdfb_device *dev = NULL;

	if(NULL == info){
		printk(KERN_ERR "[DISP] sprdfb: sprdfb_ioctl error. (Invalid Parameter)");
		return -1;
	}

	dev = info->par;

	switch(cmd){
#ifdef CONFIG_FB_LCD_OVERLAY_SUPPORT
	case SPRD_FB_SET_OVERLAY:
		pr_debug(KERN_INFO "[DISP] sprdfb: [%s]: SPRD_FB_SET_OVERLAY\n", __FUNCTION__);
		if(NULL != dev->ctrl->enable_overlay){
			result = dev->ctrl->enable_overlay(dev, (overlay_info*)arg, 1);
		}
		break;
	case SPRD_FB_DISPLAY_OVERLAY:
		pr_debug(KERN_INFO "[DISP] sprdfb: [%s]: SPRD_FB_DISPLAY_OVERLAY\n", __FUNCTION__);
		if(NULL != dev->ctrl->display_overlay){
			result = dev->ctrl->display_overlay(dev, (overlay_display*)arg);
		}
		break;
#endif
#ifdef CONFIG_FB_VSYNC_SUPPORT
	case FBIO_WAITFORVSYNC:
		pr_debug(KERN_INFO "[DISP] sprdfb: [%s]: FBIO_WAITFORVSYNC\n", __FUNCTION__);
		if(NULL != dev->ctrl->wait_for_vsync){
			result = dev->ctrl->wait_for_vsync(dev);
		}
		break;
#endif
#ifdef CONFIG_FB_DYNAMIC_FPS_SUPPORT
    case SPRD_FB_CHANGE_FPS:
		printk(KERN_INFO "[DISP] sprdfb: [%s]: SPRD_FB_CHANGE_FPS\n", __FUNCTION__);
		if(NULL != dev->ctrl->change_fps){
			result = dev->ctrl->change_fps(dev, (int)arg);
		}
		break;
#endif
	default:
		printk(KERN_INFO "[DISP] sprdfb: [%s]: unknown cmd(%d)\n", __FUNCTION__, cmd);
		break;
	}

	pr_debug(KERN_INFO "[DISP] sprdfb: [%s]: return %d\n",__FUNCTION__, result);
	return result;
}


static int sprdfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	/* Get frame buffer memory range. */
	unsigned long start = info->fix.smem_start;
	u32 len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);
	unsigned long off = vma->vm_pgoff << PAGE_SHIFT;

	if ((vma->vm_end <= vma->vm_start) || (off >= len) ||
			    ((vma->vm_end - vma->vm_start) > (len - off)))
		return -EINVAL;

	/* Set VM flags. */
	start &= PAGE_MASK;
	off += start;
	if (off < start)
		return -EINVAL;

	vma->vm_pgoff = off >> PAGE_SHIFT;
	/* This is an IO map - tell maydump to skip this VMA */
	vma->vm_flags |= VM_IO | VM_RESERVED;
	/* Set VM page protection */
	vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	/* Remap the frame buffer I/O range */
	if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
			     vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static int sprdfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	if ((var->xres != fb->var.xres) ||
		(var->yres != fb->var.yres) ||
		(var->xres_virtual != fb->var.xres_virtual) ||
		(var->yres_virtual != fb->var.yres_virtual) ||
		(var->xoffset != fb->var.xoffset) ||
#ifndef BIT_PER_PIXEL_SURPPORT
		(var->bits_per_pixel != fb->var.bits_per_pixel) ||
#endif
		(var->grayscale != fb->var.grayscale))
			return -EINVAL;
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void sprdfb_early_suspend (struct early_suspend* es)
{
	struct sprdfb_device *dev = container_of(es, struct sprdfb_device, early_suspend);
	struct fb_info *fb = dev->fb;
	printk("[DISP] sprdfb: [%s]\n",__FUNCTION__);
	
	fb_set_suspend(fb, FBINFO_STATE_SUSPENDED);

	if (!lock_fb_info(fb)) {
		return ;
	}
	dev->ctrl->suspend(dev);
#ifdef CONFIG_MACH_Z4TD
//	memset(dev->fb->screen_base, 0, dev->fb->fix.smem_len);
#endif
	unlock_fb_info(fb);
}

static void sprdfb_late_resume (struct early_suspend* es)
{
	struct sprdfb_device *dev = container_of(es, struct sprdfb_device, early_suspend);
	struct fb_info *fb = dev->fb;
	printk("[DISP] sprdfb: [%s]\n",__FUNCTION__);

	if (!lock_fb_info(fb)) {
		return ;
	}
	dev->ctrl->resume(dev);
	unlock_fb_info(fb);

	fb_set_suspend(fb, FBINFO_STATE_RUNNING);
}

/* HTC_KER_START, icey, implement on mode charging animation */
#ifdef CONFIG_HTC_ONMODE_CHARGING
static void onchg_suspend(struct early_suspend *es)
{
	struct sprdfb_device *dev = container_of(es, struct sprdfb_device, onchg_suspend);
	struct fb_info *fb = dev->fb;
	printk("[DISP] sprdfb: [%s]\n",__FUNCTION__);

	fb_set_suspend(fb, FBINFO_STATE_SUSPENDED);

	if (!lock_fb_info(fb)) {
		return ;
	}
	dev->ctrl->suspend(dev);
	unlock_fb_info(fb);
}

static void onchg_resume(struct early_suspend *es)
{
	struct sprdfb_device *dev = container_of(es, struct sprdfb_device, onchg_suspend);
	struct fb_info *fb = dev->fb;
	printk("[DISP] sprdfb: [%s]\n",__FUNCTION__);

	if (!lock_fb_info(fb)) {
		return ;
	}
	dev->ctrl->resume(dev);
	unlock_fb_info(fb);

	fb_set_suspend(fb, FBINFO_STATE_RUNNING);
}
#endif
/* HTC_KER_END, icey */
#else

static int sprdfb_suspend(struct platform_device *pdev,pm_message_t state)
{
	struct sprdfb_device *dev = platform_get_drvdata(pdev);
	printk("[DISP] sprdfb: [%s]\n",__FUNCTION__);

	dev->ctrl->suspend(dev);

	return 0;
}

static int sprdfb_resume(struct platform_device *pdev)
{
	struct sprdfb_device *dev = platform_get_drvdata(pdev);
	printk("[DISP] sprdfb: [%s]\n",__FUNCTION__);

	dev->ctrl->resume(dev);

	return 0;
}
#endif

static int lcd_regulator(void)
{
	int err = 0;

	printk("[DISP] sprdfb: [%s]\n",__FUNCTION__);

	lcd_regulator_1v8 = regulator_get(NULL, REGU_NAME_LCD_1V8);
	if (NULL == lcd_regulator_1v8) {
		printk("[DISP] could not get 1.8v regulator\n");
		return -1;
	}

	err = regulator_set_voltage(lcd_regulator_1v8,1800000,1800000);
	if (err)
		printk("[DISP] could not set to 1800mv.\n");


	err = regulator_enable(lcd_regulator_1v8);
	if (err) {
		pr_err("[DISP] LCD NT35512:could not enable lcd regulator1v8\n");
		return -1;
	}

	lcd_regulator_3v0 = regulator_get(NULL, REGU_NAME_LCD_3V0);
	if (NULL == lcd_regulator_3v0) {
		printk("[DISP] could not get 1.8v regulator\n");
		return -1;
	}

	err = regulator_set_voltage(lcd_regulator_3v0,3000000,3000000);
	if (err)
		printk("[DISP] could not set to 1800mv.\n");

	err = regulator_enable(lcd_regulator_3v0);
	if (err) {
		pr_err("[DISP] LCD NT35512:could not enable lcd regulator3v0\n");
		return -1;
	}

	return 0;
}

#ifdef CONFIG_FB_ESD_SUPPORT
static void ESD_work_func(struct work_struct *work)
{
	struct sprdfb_device *dev = container_of(work, struct sprdfb_device, ESD_work.work);

	pr_debug("[DISP] sprdfb: [%s] enter!\n", __FUNCTION__);

	//do real ESD check
	//mdelay(1000);
	if(NULL != dev->ctrl->ESD_check){
		dev->ctrl->ESD_check(dev);
	}

	if(0 != dev->enable){
		pr_debug("[DISP] sprdfb: reschedule ESD workqueue!\n");
		schedule_delayed_work(&dev->ESD_work, msecs_to_jiffies(dev->ESD_timeout_val));
		dev->ESD_work_start = true;
	}else{
		pr_debug("[DISP] sprdfb: DON't reschedule ESD workqueue since device not avialbe!!\n");
	}

	pr_debug("[DISP] sprdfb: [%s] leave!\n", __FUNCTION__);
}
#endif

static int sprdfb_probe(struct platform_device *pdev)
{
	struct fb_info *fb = NULL;
	struct sprdfb_device *dev = NULL;
	int ret = 0;

	pr_debug(KERN_INFO "[DISP] sprdfb:[%s], id = %d\n", __FUNCTION__, pdev->id);

	fb = framebuffer_alloc(sizeof(struct sprdfb_device), &pdev->dev);
	if (!fb) {
		printk(KERN_ERR "[DISP] sprdfb: sprdfb_probe allocate buffer fail.\n");
		ret = -ENOMEM;
		goto err0;
	}

	dev = fb->par;
	dev->fb = fb;

	dev->dev_id = pdev->id;
	if((SPRDFB_MAINLCD_ID != dev->dev_id) &&(SPRDFB_SUBLCD_ID != dev->dev_id)){
		printk(KERN_ERR "[DISP] sprdfb: sprdfb_probe fail. (unsupported device id)\n");
		goto err0;
	}

	switch(SPRDFB_IN_DATA_TYPE){
	case SPRD_IN_DATA_TYPE_ABGR888:
		dev->bpp = 32;
		break;
	case SPRD_IN_DATA_TYPE_BGR565:
		dev->bpp = 16;
		break;
	default:
		dev->bpp = 32;
		break;
	}

	if(SPRDFB_MAINLCD_ID == dev->dev_id){
		dev->ctrl = &sprdfb_dispc_ctrl;
#ifdef CONFIG_FB_SC8825
	}else{
		dev->ctrl = &sprdfb_lcdc_ctrl;
#endif
	}

	lcd_regulator();

# if 1
	if(sprdfb_panel_get(dev)){
		dev->panel_ready = true;
	}else{
		dev->panel_ready = false;
	}
#endif
//	dev->panel_ready = false;
	dev->ctrl->early_init(dev);

	if(!dev->panel_ready){
		if (!sprdfb_panel_probe(dev)) {
			ret = -EIO;
			goto cleanup;
		}
	}

	ret = setup_fb_mem(dev, pdev);
	if (ret) {
		goto cleanup;
	}

	setup_fb_info(dev);

	dev->ctrl->init(dev);

	sema_init(&dev->sem, 1);

	/* register framebuffer device */
	ret = register_framebuffer(fb);
	if (ret) {
		printk(KERN_ERR "[DISP] sprdfb: sprdfb_probe register framebuffer fail.\n");
		goto cleanup;
	}
	platform_set_drvdata(pdev, dev);

#ifdef CONFIG_HAS_EARLYSUSPEND
	dev->early_suspend.suspend = sprdfb_early_suspend;
	dev->early_suspend.resume  = sprdfb_late_resume;
	dev->early_suspend.level   = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	register_early_suspend(&dev->early_suspend);
/* HTC_KER_START,icey, implement on mode charging animation */
#ifdef CONFIG_HTC_ONMODE_CHARGING
	dev->onchg_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	dev->onchg_suspend.suspend = onchg_suspend;
	dev->onchg_suspend.resume = onchg_resume;
	register_onchg_suspend(&dev->onchg_suspend);
#endif
/* HTC_KER_END, icey */
#endif

#ifdef CONFIG_FB_ESD_SUPPORT
	pr_debug("[DISP] sprdfb: Init ESD work queue!\n");
	INIT_DELAYED_WORK(&dev->ESD_work, ESD_work_func);
//	sema_init(&dev->ESD_lock, 1);

	if(SPRDFB_PANEL_IF_DPI == dev->panel_if_type){
		dev->ESD_timeout_val = SPRDFB_ESD_TIME_OUT_VIDEO;
	}else{
		dev->ESD_timeout_val = SPRDFB_ESD_TIME_OUT_CMD;
	}

	dev->ESD_work_start = false;
	dev->check_esd_time = 0;
	dev->reset_dsi_time = 0;
	dev->panel_reset_time = 0;
#endif

	return 0;

cleanup:
	sprdfb_panel_remove(dev);
	dev->ctrl->uninit(dev);
	fb_free_resources(dev);
err0:
	dev_err(&pdev->dev, "[DISP] failed to probe sprdfb\n");
	return ret;
}

static int __devexit sprdfb_remove(struct platform_device *pdev)
{
	struct sprdfb_device *dev = platform_get_drvdata(pdev);
	printk("[DISP] sprdfb: [%s]\n",__FUNCTION__);

	sprdfb_panel_remove(dev);
	dev->ctrl->uninit(dev);
	fb_free_resources(dev);
	return 0;
}

static void sprdfb_shutdown(struct platform_device *pdev)
{
    struct sprdfb_device *dev = platform_get_drvdata(pdev);
    struct fb_info *fb = dev->fb;
    printk("[DISP] sprdfb: [%s]\n",__FUNCTION__);

    if (!lock_fb_info(fb)) {
        return ;
    }

    dev->ctrl->suspend(dev);
    unlock_fb_info(fb);
}

static struct platform_driver sprdfb_driver = {
	.probe = sprdfb_probe,

#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = sprdfb_suspend,
	.resume = sprdfb_resume,
#endif
	.remove = __devexit_p(sprdfb_remove),
	.shutdown = sprdfb_shutdown,
	.driver = {
		.name = "sprd_fb",
		.owner = THIS_MODULE,
	},
};


static int __init sprdfb_init(void)
{
	return platform_driver_register(&sprdfb_driver);
}

static void __exit sprdfb_exit(void)
{
	return platform_driver_unregister(&sprdfb_driver);
}

module_init(sprdfb_init);
module_exit(sprdfb_exit);
