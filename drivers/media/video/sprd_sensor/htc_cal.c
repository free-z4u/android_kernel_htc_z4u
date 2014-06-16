/* Code to extract Camera AWB calibration information from ATAG
set up by the bootloader.

Copyright (C) 2008 Google, Inc.
Author: Dmitry Shmidt <dimitrysh@google.com>

This software is licensed under the terms of the GNU General Public
License version 2, as published by the Free Software Foundation, and
may be copied, distributed, and modified under those terms.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <asm/setup.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#define CAL_VERIFY "CAL_"
#define MAX_CAL_DATA_SIZE 1024
#define MAX_AWB_CAL_DATA_SIZE 32
#define MAX_LSC_CAL_DATA_SIZE_MAIN 5*1024//17408 //17*1024
#define MAX_LSC_CAL_DATA_SIZE_FRONT 1536//1024+512//3584 //3*1024+512
#define MAX_FLASHLIGHT_CAL_DATA_SIZE 32

struct htc_cal_header {
	char               cal_verify[4];
	unsigned long int  header_size;
	unsigned long int  cal_data_size;
	unsigned long int  cal_data_checksum;
};

struct cal_file_type {
	char               passfail[30];
	char               Tolerance[30];
	char               GoldenDataR_r[30];
	char               GoldenDataR_g[30];
	char               GoldenDataR_b[30];
	char               ComputeNvmDataC_r[50];
	char               ComputeNvmDataC_g[50];
	char               ComputeNvmDataC_b[50];
	char               VariationVRg[30];
	char               VariationVBg[30];
	char               Fuse_ID1[30];
	char               Fuse_ID2[30];
	char               Fuse_ID3[30];
	char               Fuse_ID4[30];
	char			   Delta[30];//HTC_CAM_ADD chuck
};

struct htc_cal_struct_main {
        struct htc_cal_header  awb_header;
        unsigned long int      awb_header_checksum;
        char                   awb_data[MAX_AWB_CAL_DATA_SIZE];

        struct htc_cal_header  lsc_header;
        unsigned long int      lsc_header_checksum;
        char                   lsc_data[MAX_LSC_CAL_DATA_SIZE_MAIN];

        struct htc_cal_header  flashlight_header;
        unsigned long int      flashlight_header_checksum;
        char                   flashlight_data[MAX_FLASHLIGHT_CAL_DATA_SIZE];

#if EMMC_GSAMPLE
        struct htc_cal_header  gsample_header;
        unsigned long int      gsample_header_checksum;
        char                   gsample_data[MAX_CAL_DATA_SIZE];
#endif

        struct cal_file_type   cal_file;
        unsigned long int      cal_file_size;
        unsigned long int      cal_file_checksum;
};

struct htc_cal_struct_front {
        struct htc_cal_header  awb_header;
        unsigned long int      awb_header_checksum;
        char                   awb_data[MAX_AWB_CAL_DATA_SIZE];

        struct htc_cal_header  lsc_header;
        unsigned long int      lsc_header_checksum;
        char                   lsc_data[MAX_LSC_CAL_DATA_SIZE_FRONT];

#if EMMC_GSAMPLE
        struct htc_cal_header  gsample_header;
        unsigned long int      gsample_header_checksum;
        char                   gsample_data[MAX_CAL_DATA_SIZE];
#endif

        struct cal_file_type   cal_file;
        unsigned long int      cal_file_size;
        unsigned long int      cal_file_checksum;
};

/* configuration tags specific*/
#define ATAG_HTC_CAL	0x59504550 

//main : 6k  0x10000 ~ 0x11800
//front: 3k  0x11800 ~ 0x12400
#define CAL_MAX_SIZE_MAIN	0x1800U     /* 0x1000 = 4096 bytes;0xA00=2560bytes */
#define CAL_MAX_SIZE_FRONT	0x0c00U     /* 0x1000 = 4096 bytes;0xA00=2560bytes */

static unsigned char cam_awb_ram[CAL_MAX_SIZE_MAIN];
static unsigned char cam_awb_ram_front[CAL_MAX_SIZE_FRONT];//HTC_CAM chuck

static int gCal_verify;
static int gCal_verify_front;//HTC_CAM chuck

static unsigned long int gCal_file_len;
static unsigned long int gCal_file_len_front;

static char gCal_file_buf[1024];
static char gCal_file_buf_front[1024];


static unsigned long int check_sum(void *pData, unsigned long int data_size) {

	unsigned long int i, sum = 0;
        unsigned char* p;

        if (pData == NULL) goto end;

        p = (unsigned char*) pData;
		printk("lqyuan-p=%d ,", p);
		printk("data_size=%d\n", data_size);
        for (i = 0, sum = 0; i < data_size; i++)
            sum += p[i];
end:
	return sum;
}

int cal_verify(uint8_t * pCal_str, uint32_t sensor_type) {

        int ret = -1;

		if(0 == sensor_type){
			struct htc_cal_struct_main *pCal_str_main = (struct htc_cal_struct_main *)pCal_str;

				//main awb
				printk("main cam\n");
				if (strncmp( pCal_str_main->awb_header.cal_verify, CAL_VERIFY, 4)) {
					goto end;
				}
				printk("main  awb: ");
				if (pCal_str_main->awb_header_checksum != check_sum( &(pCal_str_main->awb_header), pCal_str_main->awb_header.header_size)) {
					goto end;
				}
				if (pCal_str_main->awb_header.cal_data_checksum != check_sum( pCal_str_main->awb_data, pCal_str_main->awb_header.cal_data_size)) {
					goto end;
				}

				//main lsc
				if (strncmp( pCal_str_main->lsc_header.cal_verify, CAL_VERIFY, 4)) {
					goto end;
				}
				printk("main  lsc: ");
				if (pCal_str_main->lsc_header_checksum != check_sum( &(pCal_str_main->lsc_header), pCal_str_main->lsc_header.header_size)) {
					goto end;
				}
				if (pCal_str_main->lsc_header.cal_data_checksum != check_sum( pCal_str_main->lsc_data, pCal_str_main->lsc_header.cal_data_size)) {
					goto end;
				}

#ifndef CONFIG_MACH_Z4TD
				//main flashlight
				if (strncmp( pCal_str_main->flashlight_header.cal_verify, CAL_VERIFY, 4)) {
					goto end;
				}
				printk("main  flashlight: ");
				if (pCal_str_main->flashlight_header_checksum != check_sum( &(pCal_str_main->flashlight_header), pCal_str_main->flashlight_header.header_size)) {
					goto end;
				}
				if (pCal_str_main->flashlight_header.cal_data_checksum != check_sum( pCal_str_main->flashlight_data, pCal_str_main->flashlight_header.cal_data_size)) {
					goto end;
				}
#endif

#if EMMC_GSAMPLE
				if (strncmp( pCal_str_main->gsample_header.cal_verify, CAL_VERIFY, 4)) {
					goto end;
				}
				if (pCal_str_main->gsample_header_checksum != check_sum( &(pCal_str_main->gsample_header), pCal_str_main->gsample_header.header_size)) {
					goto end;
				}

				if (pCal_str_main->gsample_header.cal_data_checksum != check_sum( pCal_str_main->gsample_data, pCal_str_main->gsample_header.cal_data_size)) {
					goto end;
				}
#endif
				printk("main cal file: ");
				if (pCal_str_main->cal_file_checksum != check_sum( &(pCal_str_main->cal_file), pCal_str_main->cal_file_size)) {
					goto end;
				}

		}else{
			struct htc_cal_struct_front *pCal_str_front = (struct htc_cal_struct_front *)pCal_str;

				printk("front cam\n");
				//front awb
				if (strncmp( pCal_str_front->awb_header.cal_verify, CAL_VERIFY, 4)) {
					goto end;
				}
				printk("front  awb: ");
				if (pCal_str_front->awb_header_checksum != check_sum( &(pCal_str_front->awb_header), pCal_str_front->awb_header.header_size)) {
					goto end;
				}
				if (pCal_str_front->awb_header.cal_data_checksum != check_sum( pCal_str_front->awb_data, pCal_str_front->awb_header.cal_data_size)) {
					goto end;
				}

				//front lsc
				if (strncmp( pCal_str_front->lsc_header.cal_verify, CAL_VERIFY, 4)) {
					goto end;
				}
				printk("front  lsc: ");
				if (pCal_str_front->lsc_header_checksum != check_sum( &(pCal_str_front->lsc_header), pCal_str_front->lsc_header.header_size)) {
					goto end;
				}
				if (pCal_str_front->lsc_header.cal_data_checksum != check_sum( pCal_str_front->lsc_data, pCal_str_front->lsc_header.cal_data_size)) {
					printk("front  lsc data checksum fail %d %d\n",pCal_str_front->lsc_header.cal_data_checksum,check_sum( pCal_str_front->lsc_data, pCal_str_front->lsc_header.cal_data_size));
					goto end;
				}

#if EMMC_GSAMPLE
				if (strncmp( pCal_str_front->gsample_header.cal_verify, CAL_VERIFY, 4)) {
					goto end;
				}

				if (pCal_str_front->gsample_header_checksum != check_sum( &(pCal_str_front->gsample_header), pCal_str_front->gsample_header.header_size)) {
					goto end;
				}

				if (pCal_str_front->gsample_header.cal_data_checksum != check_sum( pCal_str_front->gsample_data, pCal_str_front->gsample_header.cal_data_size)) {
					goto end;
				}
#endif
				printk("front  cal file: ");
				if (pCal_str_front->cal_file_checksum != check_sum( &(pCal_str_front->cal_file), pCal_str_front->cal_file_size)) {
					goto end;
				}
		}

        ret = 0;
end:
	return ret;
}
#if 0
int cal_verify(struct htc_cal_struct* pCal_str) {

        int ret = -1;

        if (strncmp( pCal_str->nvm_header.cal_verify, CAL_VERIFY, 4)) {
            goto end;
        }

		if(pCal_str->nvm_header.header_size == 16){//HTC_CAM add header size protection
	        if (pCal_str->nvm_header_checksum != check_sum( &(pCal_str->nvm_header), pCal_str->nvm_header.header_size)) {
	            goto end;
	        }
		}
		//printk("chuck-pCal_str->nvm_header.header_size=%ld\n", pCal_str->nvm_header.header_size);

		if(pCal_str->nvm_header.cal_data_size == 704){
	        if (pCal_str->nvm_header.cal_data_checksum != check_sum( pCal_str->nvm_data, pCal_str->nvm_header.cal_data_size)) {
	            goto end;
	        }
		}
		//printk("chuck-nvm_header.cal_data_checksum=%ld\n", pCal_str->nvm_header.cal_data_checksum);

#if EMMC_GSAMPLE
        if (strncmp( pCal_str->gsample_header.cal_verify, CAL_VERIFY, 4)) {
            goto end;
        }

		if(pCal_str->gsample_header.header_size == 16){
	        if (pCal_str->gsample_header_checksum != check_sum( &(pCal_str->gsample_header), pCal_str->gsample_header.header_size)) {
	            goto end;
	        }
		}
		//printk("chuck-pCal_str->gsample_header.header_size=%ld\n", pCal_str->gsample_header.header_size);
		if(pCal_str->gsample_header.cal_data_size = 704){
	        if (pCal_str->gsample_header.cal_data_checksum != check_sum( pCal_str->gsample_data, pCal_str->gsample_header.cal_data_size)) {
	            goto end;
	        }
		}
		//printk("chuck-gsample_header.cal_data_checksum=%ld\n", pCal_str->gsample_header.cal_data_checksum);
#endif
		if(pCal_str->cal_file_size == 510){
	        if (pCal_str->cal_file_checksum != check_sum( &(pCal_str->cal_file), pCal_str->cal_file_size)) {
	            goto end;
	        }
		}
		//printk("chuck-pCal_str->cal_file_size=%ld\n", pCal_str->cal_file_size);


        ret = 0;

end:
	return ret;
}
#endif

unsigned char *get_cam_awb_cal(void)
{
	return cam_awb_ram;
}

unsigned char *get_cam_awb_cal_front(void)
{
	return cam_awb_ram_front;
}

EXPORT_SYMBOL(get_cam_awb_cal);
EXPORT_SYMBOL(get_cam_awb_cal_front);

static int __init parse_tag_cam_awb_cal(const struct tag *tag)
{
	unsigned char *dptr = (unsigned char *)(&tag->u);
	unsigned long int size;
    unsigned long int len;
	unsigned long int len_front;//HTC_CAM_Add for flashlight calibration.
        char tempstr[40] = {0}; /* HTC_KER_MOD ken, modify size from 30 to 40 to fix PYD-TD-ICS Klocwork ANDROID #1521 */

	struct htc_cal_struct_main *pCal_str;
	struct htc_cal_struct_front *pCal_str_front;

	size = min((tag->hdr.size - 2) * sizeof(__u32), CAL_MAX_SIZE_MAIN);

	printk(KERN_INFO "lqyuan CAM_AWB_CAL Data size = %d , 0x%x, size = %d\n",
			tag->hdr.size, tag->hdr.tag, size);

	memcpy(cam_awb_ram, dptr, size);
	memcpy(cam_awb_ram_front, dptr + size, CAL_MAX_SIZE_FRONT);
		
	pCal_str = (struct htc_cal_struct_main *) cam_awb_ram;
	pCal_str_front = (struct htc_cal_struct_front *) cam_awb_ram_front;
		
	gCal_verify = cal_verify(pCal_str,0);
	gCal_verify_front = cal_verify(pCal_str_front,1);

	gCal_file_len = 0;
	gCal_file_len_front = 0;

	len = 0;
	len_front =0;
	printk("lqyuan verify main %d front %d\n",gCal_verify,gCal_verify_front);
        if (!gCal_verify) {

            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.passfail, len = strlen(pCal_str->cal_file.passfail));
            gCal_file_len += len;
            sprintf(tempstr, "awb_cal_size:%lu\n\0", pCal_str->awb_header.cal_data_size); /* HTC_KER_MOD ken, PYD-TD-ICS Klocwork ANDROID #1519 */
            memcpy(gCal_file_buf + gCal_file_len, tempstr, len = strlen(tempstr));
            gCal_file_len += len;

            sprintf(tempstr, "lsc_cal_size:%lu\n\0", pCal_str->lsc_header.cal_data_size); /* HTC_KER_MOD ken, PYD-TD-ICS Klocwork ANDROID #1519 */
            memcpy(gCal_file_buf + gCal_file_len, tempstr, len = strlen(tempstr));
            gCal_file_len += len;

#ifndef CONFIG_MACH_Z4TD
            sprintf(tempstr, "flash_cal_size:%lu\n\0", pCal_str->flashlight_header.cal_data_size); /* HTC_KER_MOD ken, PYD-TD-ICS Klocwork ANDROID #1519 */
            memcpy(gCal_file_buf + gCal_file_len, tempstr, len = strlen(tempstr));
            gCal_file_len += len;
#endif

#if EMMC_GSAMPLE
            sprintf(tempstr, "golden_sample_size:%lu\n\0", pCal_str->gsample_header.cal_data_size); /* HTC_KER_MOD ken, PYD-TD-ICS Klocwork ANDROID #1519 */
            memcpy(gCal_file_buf + gCal_file_len, tempstr, len = strlen(tempstr));
#endif
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.Tolerance, len = strlen(pCal_str->cal_file.Tolerance));
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.GoldenDataR_r, len = strlen(pCal_str->cal_file.GoldenDataR_r));
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.GoldenDataR_g, len = strlen(pCal_str->cal_file.GoldenDataR_g));
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.GoldenDataR_b, len = strlen(pCal_str->cal_file.GoldenDataR_b));
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.ComputeNvmDataC_r, len = strlen(pCal_str->cal_file.ComputeNvmDataC_r));
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.ComputeNvmDataC_g, len = strlen(pCal_str->cal_file.ComputeNvmDataC_g));
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.ComputeNvmDataC_b, len = strlen(pCal_str->cal_file.ComputeNvmDataC_b));
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.VariationVRg, len = strlen(pCal_str->cal_file.VariationVRg));
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.VariationVBg, len = strlen(pCal_str->cal_file.VariationVBg));
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.Fuse_ID1, len = strlen(pCal_str->cal_file.Fuse_ID1));
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.Fuse_ID2, len = strlen(pCal_str->cal_file.Fuse_ID2));
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.Fuse_ID3, len = strlen(pCal_str->cal_file.Fuse_ID3));
            gCal_file_len += len;
            memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.Fuse_ID4, len = strlen(pCal_str->cal_file.Fuse_ID4));
            gCal_file_len += len;
			memcpy(gCal_file_buf + gCal_file_len, pCal_str->cal_file.Delta, len = strlen(pCal_str->cal_file.Delta));
            gCal_file_len += len;
         }
		//HTC_CAM copy the calibration data from aother 4k ram allocated.

		
		if(!gCal_verify_front) {

            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.passfail, len_front = strlen(pCal_str_front->cal_file.passfail));
            gCal_file_len_front += len_front;

            sprintf(tempstr, "awb_cal_size:%lu\n\0", pCal_str_front->awb_header.cal_data_size); /* HTC_KER_MOD ken, PYD-TD-ICS Klocwork ANDROID #1519 */
            memcpy(gCal_file_buf_front + gCal_file_len_front, tempstr, len_front = strlen(tempstr));
            gCal_file_len_front += len_front;

            sprintf(tempstr, "lsc_cal_size:%lu\n\0", pCal_str_front->lsc_header.cal_data_size); /* HTC_KER_MOD ken, PYD-TD-ICS Klocwork ANDROID #1519 */
            memcpy(gCal_file_buf_front + gCal_file_len_front, tempstr, len_front = strlen(tempstr));
            gCal_file_len_front += len_front;

#if EMMC_GSAMPLE
            sprintf(tempstr, "golden_sample_size:%lu\n\0", pCal_str_front->gsample_header.cal_data_size); /* HTC_KER_MOD ken, PYD-TD-ICS Klocwork ANDROID #1519 */
            memcpy(gCal_file_buf_front + gCal_file_len_front, tempstr, len_front = strlen(tempstr));
#endif
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.Tolerance, len_front = strlen(pCal_str_front->cal_file.Tolerance));
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.GoldenDataR_r, len_front = strlen(pCal_str_front->cal_file.GoldenDataR_r));
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.GoldenDataR_g, len_front = strlen(pCal_str_front->cal_file.GoldenDataR_g));
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.GoldenDataR_b, len_front = strlen(pCal_str_front->cal_file.GoldenDataR_b));
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.ComputeNvmDataC_r, len_front = strlen(pCal_str_front->cal_file.ComputeNvmDataC_r));
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.ComputeNvmDataC_g, len_front = strlen(pCal_str_front->cal_file.ComputeNvmDataC_g));
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.ComputeNvmDataC_b, len_front = strlen(pCal_str_front->cal_file.ComputeNvmDataC_b));
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.VariationVRg, len_front = strlen(pCal_str_front->cal_file.VariationVRg));
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.VariationVBg, len_front = strlen(pCal_str_front->cal_file.VariationVBg));
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.Fuse_ID1, len_front = strlen(pCal_str_front->cal_file.Fuse_ID1));
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.Fuse_ID2, len_front = strlen(pCal_str_front->cal_file.Fuse_ID2));
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.Fuse_ID3, len_front = strlen(pCal_str_front->cal_file.Fuse_ID3));
            gCal_file_len_front += len_front;
            memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.Fuse_ID4, len_front = strlen(pCal_str_front->cal_file.Fuse_ID4));
            gCal_file_len_front += len_front;
			memcpy(gCal_file_buf_front + gCal_file_len_front, pCal_str_front->cal_file.Delta, len_front = strlen(pCal_str_front->cal_file.Delta));
            gCal_file_len_front += len_front;
         }
#if 0
   {
         printk("cal_verify: %d\n", cal_verify(pCal_str));
	 printk("sizeof(struct htc_cal_struct): %d\n", sizeof(struct htc_cal_struct));
         printk("cal_str.nvm_header.cal_data_size: %d\n", pCal_str->nvm_header.cal_data_size);
         printk("cal_str.gsample_header.cal_data_size: %d\n", pCal_str->gsample_header.cal_data_size);
         printk("passfail: %s", pCal_str->cal_file.passfail);
         printk("Tolerance: %s", pCal_str->cal_file.Tolerance);
         printk("GoldenDataR_r: %s", pCal_str->cal_file.GoldenDataR_r);
         printk("GoldenDataR_g: %s", pCal_str->cal_file.GoldenDataR_g);
         printk("GoldenDataR_b: %s", pCal_str->cal_file.GoldenDataR_b);
         printk("ComputeNvmDataC_r: %s", pCal_str->cal_file.ComputeNvmDataC_r);
         printk("ComputeNvmDataC_g: %s", pCal_str->cal_file.ComputeNvmDataC_g);
         printk("ComputeNvmDataC_b: %s", pCal_str->cal_file.ComputeNvmDataC_b);
         printk("VariationVRg: %s", pCal_str->cal_file.VariationVRg);
         printk("VariationVBg: %s", pCal_str->cal_file.VariationVBg);
         printk("Fuse_ID1: %s", pCal_str->cal_file.Fuse_ID1);
         printk("Fuse_ID2: %s", pCal_str->cal_file.Fuse_ID2);
         printk("Fuse_ID3: %s", pCal_str->cal_file.Fuse_ID3);
         printk("Fuse_ID4: %s", pCal_str->cal_file.Fuse_ID4);
   }
#endif

	return 0;
}

__tagtable(ATAG_HTC_CAL, parse_tag_cam_awb_cal);

static ssize_t awb_cal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct htc_cal_struct_main *pCal_str;

	pCal_str = (struct htc_cal_struct_main *) get_cam_awb_cal();

	if (!gCal_verify) {
            ret = pCal_str->awb_header.cal_data_size;
	    memcpy(buf, pCal_str->awb_data, ret);
        }

	return ret;
}

static ssize_t lsc_cal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 2000;
	struct htc_cal_struct_main *pCal_str;

	pCal_str = (struct htc_cal_struct_main *) get_cam_awb_cal();

	if (!gCal_verify) {
//            ret = pCal_str->lsc_header.cal_data_size;
			printk("[CAM]-main-1-lsc size %d",ret);
			memcpy(buf, pCal_str->lsc_data, ret);
        }

	return ret;
}

static ssize_t lsc_cal_show_2(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct htc_cal_struct_main *pCal_str;

	pCal_str = (struct htc_cal_struct_main *) get_cam_awb_cal();

	if (!gCal_verify) {
            ret = pCal_str->lsc_header.cal_data_size - 2000;
			printk("[CAM]-main-2-lsc size %d",ret);
			memcpy(buf, pCal_str->lsc_data + 2000, ret);
        }

	return ret;
}
static ssize_t flashlight_cal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct htc_cal_struct_main *pCal_str;

	pCal_str = (struct htc_cal_struct_main *) get_cam_awb_cal();

	if (!gCal_verify) {
            ret = pCal_str->flashlight_header.cal_data_size;
	    memcpy(buf, pCal_str->flashlight_data, ret);
        }

	return ret;
}

static ssize_t calibration_txt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct htc_cal_struct_main *pCal_str;

	pCal_str = (struct htc_cal_struct_main *) get_cam_awb_cal();

	if (!gCal_verify) {
            ret = gCal_file_len;
	    memcpy(buf, gCal_file_buf, ret);
        }

	return ret;
}

static ssize_t awb_cal_show_front(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct htc_cal_struct_front *pCal_str_front;

	pCal_str_front = (struct htc_cal_struct_front *) get_cam_awb_cal_front();

	if (!gCal_verify_front) {
            ret = pCal_str_front->awb_header.cal_data_size;
			printk("lqyuan--------front--lsc size %d",ret);
	    memcpy(buf, pCal_str_front->awb_data, ret);
        }

	return ret;
}

static ssize_t lsc_cal_show_front(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct htc_cal_struct_front *pCal_str_front;

	pCal_str_front = (struct htc_cal_struct_front *) get_cam_awb_cal_front();

	if (!gCal_verify_front) {
            ret = pCal_str_front->lsc_header.cal_data_size;
	    memcpy(buf, pCal_str_front->lsc_data, ret);
        }

	return ret;
}

static ssize_t calibration_txt_show_front(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct htc_cal_struct_front *pCal_str_front;

	pCal_str_front = (struct htc_cal_struct_front *) get_cam_awb_cal_front();

	if (!gCal_verify_front) {
            ret = gCal_file_len_front;
	    memcpy(buf, gCal_file_buf_front, ret);
        }

	return ret;
}

#if EMMC_GSAMPLE
static ssize_t golden_sample_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct htc_cal_struct *pCal_str;

	pCal_str = (struct htc_cal_struct *) get_cam_awb_cal();

	if (!gCal_verify) {
            ret = pCal_str->gsample_header.cal_data_size;
	    memcpy(buf, pCal_str->gsample_data, ret);
        }

	return ret;
}

static ssize_t golden_sample_show_flash(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct htc_cal_struct *pCal_str_flash;

	pCal_str_flash = (struct htc_cal_struct *) get_cam_awb_cal_flash();

	if (!gCal_verify_flash) {
            ret = pCal_str_flash->gsample_header.cal_data_size;
	    memcpy(buf, pCal_str_flash->gsample_data, ret);
        }

	return ret;
}
#endif


#if 0
static ssize_t calibration_txt_show_flash(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct htc_cal_struct *pCal_str_flash;

	pCal_str_flash = (struct htc_cal_struct *) get_cam_awb_cal_flash();

	if (!gCal_verify_flash) {
            ret = gCal_file_len_flash;
	    memcpy(buf, gCal_file_buf_flash, ret);
        }

	return ret;
}
#endif

static DEVICE_ATTR(awb_cal, 0444, awb_cal_show, NULL);
static DEVICE_ATTR(lsc_cal, 0444, lsc_cal_show, NULL);
static DEVICE_ATTR(lsc_cal_2, 0444, lsc_cal_show_2, NULL);
static DEVICE_ATTR(flashlight_cal, 0444, flashlight_cal_show, NULL);
static DEVICE_ATTR(main_calibration_txt, 0444, calibration_txt_show, NULL);

#if EMMC_GSAMPLE
static DEVICE_ATTR(golden_sample, 0444, golden_sample_show, NULL);
static DEVICE_ATTR(golden_sample_flash, 0444, golden_sample_show_flash, NULL);
static DEVICE_ATTR(calibration_txt_flash, 0444, calibration_txt_show_flash, NULL);
#endif


static DEVICE_ATTR(awb_cal_front, 0444, awb_cal_show_front, NULL);
static DEVICE_ATTR(lsc_cal_front, 0444, lsc_cal_show_front, NULL);
static DEVICE_ATTR(front_calibration_txt, 0444, calibration_txt_show_front, NULL);

static struct kobject *cam_awb_cal;

static int cam_get_awb_cal(void)
{
	int ret ;
        printk("chuck-cam_get_awb_cal + \n");
	cam_awb_cal = kobject_create_and_add("android_camera_awb_cal", NULL);
	if (cam_awb_cal == NULL) {
		pr_info("[CAM]cam_get_awb_cal: subsystem_register failed\n");
		ret = -ENOMEM;
		return ret ;
	}

	ret = sysfs_create_file(cam_awb_cal, &dev_attr_awb_cal.attr);
	if (ret) {
		pr_info("[CAM]cam_get_awb_cal:: sysfs_create_file nvm failed\n");
		kobject_del(cam_awb_cal);
                goto done;
	}

	ret = sysfs_create_file(cam_awb_cal, &dev_attr_lsc_cal.attr);
	if (ret) {
		pr_info("[CAM]cam_get_awb_cal:: sysfs_create_file nvm failed\n");
		kobject_del(cam_awb_cal);
                goto done;
	}

	ret = sysfs_create_file(cam_awb_cal, &dev_attr_lsc_cal_2.attr);
	if (ret) {
		pr_info("[CAM]cam_get_awb_cal:: sysfs_create_file nvm failed\n");
		kobject_del(cam_awb_cal);
                goto done;
	}

#ifndef CONFIG_MACH_Z4TD
	ret = sysfs_create_file(cam_awb_cal, &dev_attr_flashlight_cal.attr);
	if (ret) {
		pr_info("[CAM]cam_get_awb_cal:: sysfs_create_file nvm failed\n");
		kobject_del(cam_awb_cal);
                goto done;
	}
#endif

#if EMMC_GSAMPLE
	ret = sysfs_create_file(cam_awb_cal, &dev_attr_golden_sample.attr);
	if (ret) {
		pr_info("[CAM]cam_get_awb_cal:: sysfs_create_file golden_sample failed\n");
		kobject_del(cam_awb_cal);
                goto done;
	}

#endif

	ret = sysfs_create_file(cam_awb_cal, &dev_attr_main_calibration_txt.attr);
	if (ret) {
		pr_info("[CAM]cam_get_awb_cal:: sysfs_create_file calibration failed\n");
		kobject_del(cam_awb_cal);
                goto done;
	}

    ret = sysfs_create_file(cam_awb_cal, &dev_attr_awb_cal_front.attr);
	if (ret) {
		pr_info("[CAM]cam_get_awb_cal:: sysfs_create_file nvm_flash failed\n");
		kobject_del(cam_awb_cal);
                goto done;
	}

    ret = sysfs_create_file(cam_awb_cal, &dev_attr_lsc_cal_front.attr);
	if (ret) {
		pr_info("[CAM]cam_get_awb_cal:: sysfs_create_file nvm_flash failed\n");
		kobject_del(cam_awb_cal);
                goto done;
	}

#if EMMC_GSAMPLE
	ret = sysfs_create_file(cam_awb_cal, &dev_attr_golden_sample_flash.attr);
	if (ret) {
		pr_info("[CAM]cam_get_awb_cal:: sysfs_create_file golden_sample_flash failed\n");
		kobject_del(cam_awb_cal);
                goto done;
	}

	ret = sysfs_create_file(cam_awb_cal, &dev_attr_calibration_txt_flash.attr);
	if (ret) {
		pr_info("[CAM]cam_get_awb_cal:: sysfs_create_file calibration_flash failed\n");
		kobject_del(cam_awb_cal);
                goto done;
	}
#endif

	ret = sysfs_create_file(cam_awb_cal, &dev_attr_front_calibration_txt.attr);
	if (ret) {
		pr_info("[CAM]cam_get_awb_cal:: sysfs_create_file calibration_front failed\n");
		kobject_del(cam_awb_cal);
                goto done;
	}
	//HTC_CAM_END
done:
        printk("chuck-cam_get_awb_cal - \n");
	return 0 ;
}

late_initcall(cam_get_awb_cal);
