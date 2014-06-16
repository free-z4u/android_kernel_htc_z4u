#ifndef _YUSHAN_Dump_Data_H_
#define _YUSHAN_Dump_Data_H_

#include "Yushan_Platform_Specific.h"
#include "yushan_registermap.h"
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/io.h>

/* Help compiling in C++ */
#ifdef __cplusplus
extern "C"{
#endif   /*__cplusplus*/

void Yushan_Dump_BB_IPP_Register();
void SPI_Tester();
void DumpPDP_Registers();
void DumpDPP_Registers();
void DumpDOP_Registers();
void DumpSIARegs();
void Yushan_dump_all_register(void);
void ASIC_Test(void);

#define DUMP_FRAME_CNT 10
#define IPP_START_ADDR 0xA0254800
#define IPP_END_ADDR   0xA02549D8

/* help compiling in C++ */
#ifdef __cplusplus
}
#endif   /*__cplusplus*/

#endif /* _YUSHAN_DUMP_DATA_H_ */