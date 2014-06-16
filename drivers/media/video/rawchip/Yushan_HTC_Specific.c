#include "Yushan_HTC_Specific.h"

/*******************************************************************************
File Name:	Yushan_DumpData.c
Author:		Kueihung Yeh
Description: 
********************************************************************************/

void Yushan_Dump_BB_IPP_Register()
{
	uint32_t index;
	uint32_t addr;
	//for (index = 0xa0254800; index < )
	
	addr = IPP_START_ADDR + 0x1A8;
	//printk("[CAM] Yushan_Dump_BB_IPP_Register, addr = 0x%x, data = 0x%x\n", addr, *(uint16_t *)addr);
}

void SPI_Tester()
{
	uint32_t SpiWriteData_32;
	uint32_t SpiReadData_32;
	
	uint8_t	bSpiWriteData;
	uint8_t bSpiReadDat;
	
	int index = 0;
	bool_t fStatus = SUCCESS;

	printk("> [CAM] SPI_Tester");
			
  for (index = 0; index < 255; index++)
  {
  	if (index == 0)  		
  		bSpiWriteData = 0x00;
  	else if (index > 64)
  		bSpiWriteData = 0x55;
  	//else
  	//	bSpiWriteData = 0xaa;
  		
		SPI_Write(0x808, 1, (uint8_t*)(&bSpiWriteData));
		SPI_Read(0x808, 1, &bSpiReadDat);	
		
		if (bSpiWriteData != bSpiReadDat)
		{
			printk("[CAM] SPI_Tester, Wrong value, index = %d, bSpiWriteData = 0x%x, bSpiReadDat = 0x%x", index, bSpiWriteData, bSpiReadDat);
		}
		else
		{
			printk("[CAM] SPI_Tester, index = %d, bSpiReadDat = 0x%x\n", index, bSpiReadDat);
		}
	}
/*	
	for (index = 0; index < 255; index++)
  {
  	SpiWriteData_32 = 0xaa55aa55;
		SPI_Write(0x808, 4, (uint8_t*)(&SpiWriteData_32));
		SPI_Read(0x808, 4, &SpiReadData_32);	
		
		if (SpiWriteData_32 != SpiReadData_32)
		{
			printk("[CAM] SPI_Tester, 4bytes Wrong value, index = %d, bSpiWriteData = 0x%x, bSpiReadDat = 0x%x", index, SpiWriteData_32, SpiReadData_32);
		}
		else
		{
			printk("[CAM] 4bytes SPI_Tester, index = %d, SpiReadData_32 = 0x%x\n", index, SpiReadData_32);
		}
	}
*/	
	printk("< [CAM] SPI_Tester");
}

void frame_counter_in_yushan(int dump_cnt)
{
	uint16_t fcnt_FPGA=0, tmp = 0;
	uint16_t fcnt_3H2=0;
	uint8_t i;
	int index = 0;
	int data = 0;
	uint8_t	bSpiData;

	printk("[CAM] ,> frame_counter_in_yushan \n");
/*
printk("[CAM], FPGA's out counting\n");
rawchip_spi_read_2B2B(0x4C30,&fcnt_FPGA);
printk("[CAM], [CAM] ASIC's out counting=%d (%d)\n",fcnt_FPGA, i);
*/
/*
		rawchip_spi_read_2B2B(0x10,&fcnt_FPGA);
		printk("[CAM], 0x10: PLL_CTRL_Main=0x%x\n",fcnt_FPGA, index);	
		mdelay(10);
*/

#if 0
		data = 0;
		SPI_Read(0x3404, 4, (uint8_t*)(&data));
		printk("[CAM], 0x3404: SMIA Version = 0x%x\n",data);	
		mdelay(10);

		data = 0;
		SPI_Read(0x10, 4, (uint8_t*)(&data));
		printk("[CAM], 0x10: PLL_CTRL_Main=0x%x\n",data);	
		mdelay(10);

		data = 0;
		SPI_Read(0x14, 4, (uint8_t*)(&data));
		printk("[CAM], 0x14: PLL_LOOP_OUT_DF=0x%x\n",data);	
		mdelay(10);	

		data = 0;
		SPI_Read(YUSHAN_IDP_GEN_WC_DI_0, 4, (uint8_t*)(&data));
		printk("[CAM], YUSHAN_IDP_GEN_WC_DI_0: YUSHAN_IDP_GEN_WC_DI_0=0x%x\n",data);	
		mdelay(10);

		data = 0;
		SPI_Read(0x0C, 2, (uint8_t*)(&data));
		printk("[CAM], 0x0c: Reset_CTRL=0x%x\n",data);	
		mdelay(10);

		data = 0;
		SPI_Read(0x1405, 1, (uint8_t*)(&data));
		printk("[CAM], 0x1405: Private_test_LDO_CTRL, data = 0x%x\n",data);	
		mdelay(10);

		bSpiData = 0;
		SPI_Read(0x1405,1,&bSpiData);
		printk("[CAM], 0x1405: Private_test_LDO_CTRL, bSpiData = 0x%x\n",bSpiData);	
		mdelay(10);

		bSpiData = 0;
		SPI_Read(0x4c18,4,&bSpiData);
		printk("[CAM], 0x4c18: CSI2_TX_STATUS_LINE_SIZE, bSpiData = 0x%x\n",bSpiData);	
		mdelay(10);

		bSpiData = 0;
		SPI_Read(0x4c1c,4,&bSpiData);
		printk("[CAM], 0x4c1c: CSI2_TX_STATUS_LINE_CTRL, bSpiData = 0x%x\n",bSpiData);	
		mdelay(10);
#endif
		index = 0;	
		for (index = 0; index < dump_cnt; index++)
		{
			mdelay(100);
			//printk("[CAM] \n");
			//printk("[CAM] index = %d\n", index);

			// TX status statistics
			rawchip_spi_read_2B2B(0x4C30,&fcnt_FPGA);
			printk("[CAM], 0x4c30: CSI2 TX Frame No=0x%x \n",fcnt_FPGA);	

			mdelay(20);
			rawchip_spi_read_2B1B(0x0C60,&fcnt_FPGA);
			printk("[CAM], 0x0C60: ITM_CSI2TX_STATUS =0x%x\n",fcnt_FPGA);		
			mdelay(20);	
/*			
			bSpiData = 0xF;
			SPI_Write(0x0C68, 1, &bSpiData);

			rawchip_spi_read_2B1B(0x4c4c,&fcnt_FPGA);
			printk("[CAM] 0x4c4c: CSI2_TX_PACKET_SIZE_0 = 0x%x\n",fcnt_FPGA);		
			mdelay(10);
			rawchip_spi_read_2B1B(0x4c50,&fcnt_FPGA);
			printk("[CAM] 0x4c50: CSI2_TX_PACKET_SIZE_1 = 0x%x\n",fcnt_FPGA);		
			mdelay(10);
*/
			SPI_Read(0x4c4c, 4, (uint8_t*)(&data));
			printk("[CAM], 0x4c4c: CSI2_TX_PACKET_SIZE_0 = 0x%x\n",data);		
			mdelay(20);

			SPI_Read(0x4c50, 4, (uint8_t*)(&data));
			printk("[CAM], 0x4c50 CSI2_TX_DI_INDEX_1 = 0x%x\n",data);		
			mdelay(20);
			
			rawchip_spi_read_2B1B(0x0D20,&fcnt_FPGA);
			printk("[CAM], 0x0D20: ITM_P2W_UFLOW_STATUS = 0x%x\n",fcnt_FPGA);		
			mdelay(20);

			// Rx status statistics
			/*
			rawchip_spi_read_2B1B(0x2438,&fcnt_FPGA);
			printk("[CAM] 0x2438: CSI2_RX_WORD_COUNT = 0x%x\n",fcnt_FPGA);		
			mdelay(10);

			rawchip_spi_read_2B1B(0x242C,&fcnt_FPGA);
			printk("[CAM] 0x242c: CSI2_RX_FRAME_NUMBER = 0x%x\n",fcnt_FPGA);		
			mdelay(10);

			rawchip_spi_read_2B1B(0x0C00,&fcnt_FPGA);
			printk("[CAM] 0x0C00: ITM_CSI2RX_STATUS = 0x%x\n",fcnt_FPGA);		
			mdelay(10);
			*/
			mdelay(20);
			SPI_Read(0x2438, 4, (uint8_t*)(&data));
			printk("[CAM], 0x2438 CSI2_RX_WORD_COUNT = 0x%x\n",data);		
			//mdelay(20);

			SPI_Read(0x242C, 4, (uint8_t*)(&data));
			printk("[CAM], 0x242C,CSI2_RX_FRAME_NUMBER = 0x%x\n",data);		
			mdelay(20);

			rawchip_spi_read_2B1B(0x0C00,&fcnt_FPGA);
			printk("[CAM], 0x0C00: ITM_CSI2RX_STATUS = 0x%x\n",fcnt_FPGA);		
			mdelay(20);

			bSpiData = 0;
			SPI_Read(0x4804,1,&bSpiData);
			printk("[CAM], 0x4804: P2W_FIFO_WR_STATUS, bSpiData = 0x%x\n",bSpiData);	
			mdelay(100);
		}
printk("[CAM],< frame_counter_in_yushan \n");
}

void DumpPDP_Registers()
{
	int index = 0;
	uint8_t  data;	
	uint16_t data1;

	SPI_Read(DXO_PDP_BASE_ADDR + DxOPDP_ucode_id_7_0 , 2, (uint8_t *)(&data1));
	printk("[CAM] DxOPDP_ucode_id_7_0  = 0x%x \n", data1);


	for (index = 0; index < 60; index++)
	{
			msleep(10);
			SPI_Read(DXO_PDP_BASE_ADDR + 0x200 + index, 1, (uint8_t *)(&data));
			printk("[CAM] DxO PDP Hardware address = 0x%x, data = 0x%x\n", DXO_PDP_BASE_ADDR + 0x200 + index, data);
	}
}

void DumpDPP_Registers()
{
	int index = 0;
	uint8_t  data;	
	uint16_t data1;
	uint32_t udwSpiBaseIndex;

	udwSpiBaseIndex = 0x10000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
	msleep(1);
	
	SPI_Read(DXO_DPP_BASE_ADDR + DxODPP_ucode_id_7_0 - 0x8000, 2, (uint8_t *)(&data1));
	printk("[CAM] DxODPP_ucode_id_7_0 = 0x%x \n", data1);

	SPI_Read(DXO_DPP_BASE_ADDR + DxODPP_hw_id_7_0 - 0x8000, 2, (uint8_t *)(&data1));
	printk("[CAM] DxODPP_hw_id_7_0 = 0x%x \n", data1);

	for (index = 0; index < 60; index++)
	{
			msleep(10);
			SPI_Read(DXO_DPP_BASE_ADDR - 0x8000 + 0x200 + index, 1, (uint8_t *)(&data));
			printk("[CAM] DxO DPP Hardware address = 0x%x, data = 0x%x\n", DXO_DPP_BASE_ADDR - 0x8000 + 0x200 + index, data);
	}
	udwSpiBaseIndex = 0x8000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
}

void DumpDOP_Registers()
{
	int index = 0;
	uint8_t  data;	
	uint16_t data1;
	
	SPI_Read(DXO_DOP_BASE_ADDR + DxODOP_ucode_id_7_0, 2, (uint8_t *)(&data1));
	printk("[CAM] DxODOP_ucode_id = 0x%x \n", data1);

	SPI_Read(DXO_DOP_BASE_ADDR + DxODOP_hw_id_7_0, 2, (uint8_t *)(&data1));
	printk("[CAM] DxODOP_hw_id = 0x%x \n", data1);

	for (index = 0; index < 165; index++)
	{
			msleep(10);
			SPI_Read(DXO_DOP_BASE_ADDR + 0x200 + index, 1, (uint8_t *)(&data));
			printk("[CAM] DxO DOP Hardware address = 0x%x, data = 0x%x\n", DXO_DOP_BASE_ADDR + 0x200 + index, data);
	}
}

void DumpSIARegs()
{
#if 1
                /*SIA*/
                void __iomem * pSIA = NULL;
        #define SIA_BASE_ADDR /*0x54000*/0xA0200000
                //#define SIA_BASE_ADDR    0xa0200000
        
                pSIA = ioremap(SIA_BASE_ADDR, 0xFF);
        
                if(pSIA == NULL)
                {
                        printk("[CAM] %s: ioremap Failed!\n", __FUNCTION__);
                }
                else
                {
                        printk("[CAM] %s: SIA physical addr:0x%x to virtual addr: 0x%x\n", __FUNCTION__, SIA_BASE_ADDR,pSIA);
                }
        
#endif       
        
#if 1
#define DUMP_REG(addr, offset, title) printk(#offset "[CAM] %s:0x%x\n", title, readl(pSIA+0x20));
//#define DUMP_REG(addr, offset, title) printk(#offset "%s:0x%x\n", title, readl(pSIA+ offset));

                printk(KERN_INFO "[CAM] %s\n", __FUNCTION__);
                
                DUMP_REG(pSIA, 0x00, "SIA_RESET");
                DUMP_REG(pSIA, 0x04, "SIA_SOFT_RESET");
                DUMP_REG(pSIA, 0x10, "SIA_CLK_ENABLE");
                DUMP_REG(pSIA, 0x20, "SIA_IDN_HRV");
                
                DUMP_REG(pSIA, 0x80, "SIA_PIPE_SELECT");
                DUMP_REG(pSIA, 0x90, "LICN_ITSN");
                DUMP_REG(pSIA, 0x94, "LICN_ITS_BCLR");
                DUMP_REG(pSIA, 0x98, "LICN_ITS_BSET");
                DUMP_REG(pSIA, 0x9C, "LICN_ITM");
                DUMP_REG(pSIA, 0xA0, "LICN_ITM_BCLR");
                DUMP_REG(pSIA, 0xA4, "LICN_ITM_BSET");
                DUMP_REG(pSIA, 0xC0, "SIA_HI_ITS");
                DUMP_REG(pSIA, 0xC4, "SIA_HI_ITS_BCLR");
                DUMP_REG(pSIA, 0xC8, "SIA_HI_ITS_BSET");
                DUMP_REG(pSIA, 0xCC, "SIA_HI_ITM");
                DUMP_REG(pSIA, 0xD0, "SIA_HI_ITM_BCLR");
        
                iounmap(pSIA);
                
#endif
}

void Yushan_dump_all_register(void)
{
	uint16_t read_data = 0;
	uint8_t i;
#if 0
	for (i = 0; i < 50; i++) {
		/* Yushan's in counting */
		rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_FRAME_NUMBER, &read_data);
		pr_info("[CAM] Yushan's in counting=%d\n", read_data);

		/* Yushan's out counting */
		rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_FRAME_NO_0, &read_data);
		pr_info("[CAM] Yushan's out counting=%d\n", read_data);

		mdelay(10);
	}
#endif
	rawchip_spi_read_2B2B(YUSHAN_CLK_DIV_FACTOR, &read_data);
	pr_info("[CAM]YUSHAN_CLK_DIV_FACTOR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CLK_DIV_FACTOR_2, &read_data);
	pr_info("[CAM]YUSHAN_CLK_DIV_FACTOR_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CLK_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_CLK_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_RESET_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_RESET_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PLL_CTRL_MAIN, &read_data);
	pr_info("[CAM]YUSHAN_PLL_CTRL_MAIN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PLL_LOOP_OUT_DF, &read_data);
	pr_info("[CAM]YUSHAN_PLL_LOOP_OUT_DF=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PLL_SSCG_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_PLL_SSCG_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_HOST_IF_SPI_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_HOST_IF_SPI_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_HOST_IF_SPI_DEVADDR, &read_data);
	pr_info("[CAM]YUSHAN_HOST_IF_SPI_DEVADDR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, &read_data);
	pr_info("[CAM]YUSHAN_HOST_IF_SPI_BASE_ADDRESS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_PDP_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_PDP_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_PDP_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_PDP_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_PDP_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_PDP_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_PDP_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_PDP_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_PDP_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_PDP_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_PDP_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_PDP_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DPP_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DPP_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DPP_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DPP_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DPP_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DPP_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DPP_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DPP_STATUS_BSET=%x\n", read_data);
  rawchip_spi_read_2B2B(YUSHAN_ITM_DPP_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DPP_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DPP_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DPP_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DOP7_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DOP7_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DOP7_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DOP7_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DOP7_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DOP7_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DOP7_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DOP7_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DOP7_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DOP7_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DOP7_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DOP7_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_PHY_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_PHY_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_PHY_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_PHY_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_PHY_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_PHY_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_PHY_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_PHY_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_PHY_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_PHY_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_PHY_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_PHY_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_TX_PHY_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_TX_PHY_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_TX_PHY_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_TX_PHY_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_TX_PHY_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_TX_PHY_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_TX_PHY_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_TX_PHY_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_TX_PHY_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_TX_PHY_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_TX_PHY_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_TX_PHY_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_CHAR_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_CHAR_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_CHAR_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_CHAR_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_CHAR_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_CHAR_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_CHAR_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_CHAR_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_CHAR_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_CHAR_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_CHAR_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_CHAR_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_LBE_POST_DXO_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_LBE_POST_DXO_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_LBE_POST_DXO_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_LBE_POST_DXO_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_LBE_POST_DXO_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_LBE_POST_DXO_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_LBE_POST_DXO_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_LBE_POST_DXO_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_LBE_POST_DXO_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_LBE_POST_DXO_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_LBE_POST_DXO_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_LBE_POST_DXO_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_SYS_DOMAIN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_SYS_DOMAIN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_SYS_DOMAIN_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_SYS_DOMAIN_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_SYS_DOMAIN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_SYS_DOMAIN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_SYS_DOMAIN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_SYS_DOMAIN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_SYS_DOMAIN_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_SYS_DOMAIN_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_SYS_DOMAIN_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_SYS_DOMAIN_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_ITPOINT_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_ITPOINT_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_ITPOINT_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_ITPOINT_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_ITPOINT_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_ITPOINT_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_ITPOINT_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_ITPOINT_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_ITPOINT_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_ITPOINT_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_ITPOINT_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_ITPOINT_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_CTRL, &read_data);
  pr_info("[CAM]YUSHAN_IOR_NVM_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_DATA_WORD_0, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_DATA_WORD_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_DATA_WORD_1, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_DATA_WORD_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_DATA_WORD_2, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_DATA_WORD_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_DATA_WORD_3, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_DATA_WORD_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_HYST, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_HYST=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_PDN, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_PDN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_PUN, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_PUN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_LOWEMI, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_LOWEMI=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_PAD_IN, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_PAD_IN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_RATIO_PAD, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_RATIO_PAD=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_SEND_ITR_PAD1, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_SEND_ITR_PAD1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_INTR_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_INTR_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_LDO_STS_REG, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_LDO_STS_REG=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_REG_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_REG_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_REG_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_REG_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_REG_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_REG_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_REG_REFILL_ELT_NB, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_REG_REFILL_ELT_NB=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_REG_REFILL_ERROR, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_REG_REFILL_ERROR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_REG_DFV_CONTROL, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_REG_DFV_CONTROL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_MEM_PAGE, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_MEM_PAGE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_MEM_LOWER_ELT, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_MEM_LOWER_ELT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_MEM_UPPER_ELT, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_MEM_UPPER_ELT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_UIX4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_UIX4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_SWAP_PINS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_SWAP_PINS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_INVERT_HS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_INVERT_HS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_STOP_STATE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_STOP_STATE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_ULP_STATE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_ULP_STATE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_CLK_ACTIVE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_CLK_ACTIVE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_FORCE_RX_MODE_DL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_FORCE_RX_MODE_DL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_TEST_RESERVED, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_TEST_RESERVED=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_ESC_DL_STS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_ESC_DL_STS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_EOT_BYPASS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_EOT_BYPASS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_HSRX_SHIFT_CL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_HSRX_SHIFT_CL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_HS_RX_SHIFT_DL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_HS_RX_SHIFT_DL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_VIL_CL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_VIL_CL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_VIL_DL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_VIL_DL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_OVERSAMPLE_BYPASS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_OVERSAMPLE_BYPASS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_OVERSAMPLE_FLAG1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_OVERSAMPLE_FLAG1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_SKEW_OFFSET_1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_SKEW_OFFSET_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_SKEW_OFFSET_2, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_SKEW_OFFSET_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_SKEW_OFFSET_3, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_SKEW_OFFSET_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_SKEW_OFFSET_4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_SKEW_OFFSET_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_OFFSET_CL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_OFFSET_CL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_CALIBRATE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_CALIBRATE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_SPECS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_SPECS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_COMP, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_COMP=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_MIPI_IN_SHORT, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_MIPI_IN_SHORT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_LANE_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_LANE_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_VER_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_VER_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_NB_DATA_LANES, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_NB_DATA_LANES=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_IMG_UNPACKING_FORMAT, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_IMG_UNPACKING_FORMAT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_WAIT_AFTER_PACKET_END, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_WAIT_AFTER_PACKET_END=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_MULTIPLE_OF_5_HSYNC_EXTENSION_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_MULTIPLE_OF_5_HSYNC_EXTENSION_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_MULTIPLE_OF_5_HSYNC_EXTENSION_PADDING_DATA, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_MULTIPLE_OF_5_HSYNC_EXTENSION_PADDING_DATA=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_CHARACTERIZATION_MODE, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_CHARACTERIZATION_MODE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_BYTE2PIXEL_READ_TH, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_BYTE2PIXEL_READ_TH=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_VIRTUAL_CHANNEL, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_VIRTUAL_CHANNEL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_DATA_TYPE, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_DATA_TYPE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_FRAME_NUMBER, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_FRAME_NUMBER=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_LINE_NUMBER, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_LINE_NUMBER=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_DATA_FIELD, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_DATA_FIELD=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_WORD_COUNT, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_WORD_COUNT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_ECC_ERROR_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_ECC_ERROR_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_DFV, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_PIX_POS, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_PIX_POS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_LINE_POS, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_LINE_POS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_PIX_CNT, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_PIX_CNT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_LINE_CNT, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_LINE_CNT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_FRAME_CNT, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_FRAME_CNT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_DFV, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_AUTO_RUN, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_AUTO_RUN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_CONTROL, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_CONTROL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_LINE_LENGTH, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_LINE_LENGTH=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_FRAME_LENGTH, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_FRAME_LENGTH=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_ERROR_LINES_EOF_GAP, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_ERROR_LINES_EOF_GAP=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_0, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_1, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_2, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_3, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_4, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_5, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_6, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_7, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_8, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_8=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_9, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_9=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_10, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_10=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_11, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_11=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_12, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_12=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_13, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_13=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_14, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_14=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_DFV, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_VERSION_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_VERSION_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLORBAR_WIDTH_BY4_M1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLORBAR_WIDTH_BY4_M1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_0, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_2, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_3, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_5, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_6, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_7, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_0, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_2, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_3, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_5, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_6, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_7, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_0, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_2, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_3, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_5, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_6, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_7, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_0, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_2, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_3, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_5, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_6, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_7, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_DFV, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_PATTERN_TYPE_REQ, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_PATTERN_TYPE_REQ=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_TPAT_DATA_RG, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_TPAT_DATA_RG=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_TPAT_DATA_BG, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_TPAT_DATA_BG=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_TPAT_HCUR_WP, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_TPAT_HCUR_WP=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_TPAT_VCUR_WP, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_TPAT_VCUR_WP=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_PATTERN_TYPE_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_PATTERN_TYPE_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_DCPX_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_DCPX_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_DCPX_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_DCPX_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_DCPX_ENABLE_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_DCPX_ENABLE_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_DCPX_MODE_REQ, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_DCPX_MODE_REQ=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_DCPX_MODE_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_DCPX_MODE_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_CPX_CTRL_REQ, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_CPX_CTRL_REQ=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_CPX_MODE_REQ, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_CPX_MODE_REQ=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_CPX_CTRL_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_CPX_CTRL_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_CPX_MODE_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_CPX_MODE_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_FM_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_FM_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_FM_PIX_WIDTH, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_FM_PIX_WIDTH=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_FM_GROUPED_PARAMETER_HOLD, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_FM_GROUPED_PARAMETER_HOLD=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_FM_EOF_INT_EN, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_FM_EOF_INT_EN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_FM_EOF_INT_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_FM_EOF_INT_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_P2W_FIFO_WR_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_P2W_FIFO_WR_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_P2W_FIFO_WR_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_P2W_FIFO_WR_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_P2W_FIFO_RD_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_P2W_FIFO_RD_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_P2W_FIFO_RD_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_P2W_FIFO_RD_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_WRAPPER_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_WRAPPER_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_WRAPPER_THRESH, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_WRAPPER_THRESH=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_WRAPPER_CHAR_EN, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_WRAPPER_CHAR_EN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_VERSION_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_VERSION_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_NUMBER_OF_LANES, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_NUMBER_OF_LANES=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LANE_MAPPING, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LANE_MAPPING=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_CONTROL, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_CONTROL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_INTERPACKET_DELAY, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_INTERPACKET_DELAY=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_STATUS_LINE_SIZE, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_STATUS_LINE_SIZE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_STATUS_LINE_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_STATUS_LINE_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_VC_CTRL_0, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_VC_CTRL_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_VC_CTRL_1, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_VC_CTRL_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_VC_CTRL_2, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_VC_CTRL_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_VC_CTRL_3, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_VC_CTRL_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_FRAME_NO_0, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_FRAME_NO_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_FRAME_NO_1, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_FRAME_NO_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_FRAME_NO_2, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_FRAME_NO_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_FRAME_NO_3, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_FRAME_NO_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_BYTE_COUNT, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_BYTE_COUNT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_CURRENT_DATA_IDENTIFIER, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_CURRENT_DATA_IDENTIFIER=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DFV, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_0, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_0, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_0, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_1, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_1, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_1, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_2, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_2, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_2, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_3, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_3, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_3, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_4, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_4, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_4, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_5, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_5, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_5, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_6, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_6, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_6, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_7, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_7, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_7, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_8, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_8=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_8, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_8=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_8, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_8=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_9, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_9=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_9, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_9=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_9, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_9=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_10, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_10=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_10, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_10=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_10, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_10=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_11, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_11=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_11, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_11=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_11, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_11=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_12, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_12=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_12, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_12=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_12, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_12=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_13, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_13=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_13, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_13=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_13, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_13=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_14, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_14=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_14, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_14=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_14, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_14=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_15, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_15=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_15, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_15=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_15, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_15=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_UIX4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_UIX4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_SWAP_PINS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_SWAP_PINS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_INVERT_HS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_INVERT_HS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_STOP_STATE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_STOP_STATE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_FORCE_TX_MODE_DL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_FORCE_TX_MODE_DL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_ULP_STATE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_ULP_STATE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_ULP_EXIT, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_ULP_EXIT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_ESC_DL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_ESC_DL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_HSTX_SLEW, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_HSTX_SLEW=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_SKEW, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_SKEW=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_GPIO_CL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_GPIO_CL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_GPIO_DL1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_GPIO_DL1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_GPIO_DL2, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_GPIO_DL2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_GPIO_DL3, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_GPIO_DL3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_GPIO_DL4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_GPIO_DL4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_SPECS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_SPECS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_SLEW_RATE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_SLEW_RATE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_TEST_RESERVED, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_TEST_RESERVED=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_TCLK_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_TCLK_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_TCLK_POST_DELAY, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_TCLK_POST_DELAY=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_BYPASS_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_BYPASS_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_BYPASS_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_BYPASS_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_BYPASS_LSTART_LEVEL, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_BYPASS_LSTART_LEVEL=%x\n", read_data);

	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_BYPASS_LSTOP_LEVEL, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_BYPASS_LSTOP_LEVEL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_BYPASS_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_BYPASS_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_BYPASS_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_BYPASS_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_BYPASS_MATCH0, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_BYPASS_MATCH0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_BYPASS_MATCH1, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_BYPASS_MATCH1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_BYPASS_MATCH2, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_BYPASS_MATCH2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_BYPASS_MATCH3, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_BYPASS_MATCH3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_DXO_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_DXO_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_DXO_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_DXO_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_DXO_LSTART_LEVEL, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_DXO_LSTART_LEVEL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_DXO_LSTOP_LEVEL, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_DXO_LSTOP_LEVEL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_DXO_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_DXO_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_DXO_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_DXO_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_DXO_MATCH0, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_DXO_MATCH0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_DXO_MATCH1, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_DXO_MATCH1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_DXO_MATCH2, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_DXO_MATCH2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_DXO_MATCH3, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_DXO_MATCH3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_PRE_DXO_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_PRE_DXO_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_PRE_DXO_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_PRE_DXO_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_PRE_DXO_AUTOMATIC_CONTROL, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_PRE_DXO_AUTOMATIC_CONTROL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_PRE_DXO_H_SIZE, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_PRE_DXO_H_SIZE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_PRE_DXO_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_LBE_PRE_DXO_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_PRE_DXO_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_LBE_PRE_DXO_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_PRE_DXO_DFV, &read_data);
	pr_info("[CAM]YUSHAN_LBE_PRE_DXO_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_PRE_DXO_H_SIZE, &read_data);
	pr_info("[CAM]YUSHAN_LBE_PRE_DXO_H_SIZE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_PRE_DXO_READ_START, &read_data);
	pr_info("[CAM]YUSHAN_LBE_PRE_DXO_READ_START=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_POST_DXO_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_POST_DXO_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_POST_DXO_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_POST_DXO_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_POST_DXO_AUTOMATIC_CONTROL, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_POST_DXO_AUTOMATIC_CONTROL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_POST_DXO_H_SIZE, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_POST_DXO_H_SIZE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LECCI_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_LECCI_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LECCI_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_LECCI_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LECCI_MIN_INTERLINE, &read_data);
	pr_info("[CAM]YUSHAN_LECCI_MIN_INTERLINE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LECCI_OUT_BURST_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_LECCI_OUT_BURST_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LECCI_LINE_SIZE, &read_data);
	pr_info("[CAM]YUSHAN_LECCI_LINE_SIZE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LECCI_BYPASS_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_LECCI_BYPASS_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_POST_DXO_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_LBE_POST_DXO_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_POST_DXO_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_LBE_POST_DXO_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_POST_DXO_DFV, &read_data);
	pr_info("[CAM]YUSHAN_LBE_POST_DXO_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_POST_DXO_H_SIZE, &read_data);
	pr_info("[CAM]YUSHAN_LBE_POST_DXO_H_SIZE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_POST_DXO_READ_START, &read_data);
	pr_info("[CAM]YUSHAN_LBE_POST_DXO_READ_START=%x\n", read_data);

}

void ASIC_Test(void)
{
	printk("> [CAM] ASIC_Test");
	mdelay(10);
	//rawchip_spi_write_2B1B(0x0008, 0x6f); // CLKMGR - CLK_CTRL
	mdelay(10);
	rawchip_spi_write_2B1B(0x000c, 0x00); // CLKMGR - RESET_CTRL
	mdelay(10);
	rawchip_spi_write_2B1B(0x000d, 0x00); // CLKMGR - RESET_CTRL
	mdelay(10);
	rawchip_spi_write_2B1B(0x000c, 0x3f); // CLKMGR - RESET_CTRL
	mdelay(10);
	rawchip_spi_write_2B1B(0x000d, 0x07); // CLKMGR - RESET_CTRL
	mdelay(10);
	rawchip_spi_write_2B1B(0x000f, 0x00); // Unreferenced register - Warning
	mdelay(10);

	// LDO enable sequence
	rawchip_spi_write_2B1B(0x1405, 0x03); // 
	mdelay(10);
	rawchip_spi_write_2B1B(0x1405, 0x02); // 
	mdelay(10);
	rawchip_spi_write_2B1B(0x1405, 0x00); // 

	//HD
	//rawchip_spi_write_2B1B(0x0015, 0x14); // Unreferenced register - Warning
	rawchip_spi_write_2B1B(0x0015, 0x19); // CLKMGR - PLL_LOOP_OUT_DF
	rawchip_spi_write_2B1B(0x0014, 0x03); // CLKMGR - PLL_LOOP_OUT_DF

	mdelay(10);
	rawchip_spi_write_2B1B(0x0000, 0x0a); // CLKMGR - CLK_DIV_FACTOR
	mdelay(10);
	rawchip_spi_write_2B1B(0x0001, 0x0a); // Unreferenced register - Warning
	mdelay(10);
	rawchip_spi_write_2B1B(0x0002, 0x14); // Unreferenced register - Warning
	mdelay(10);
	rawchip_spi_write_2B1B(0x0010, 0x18); // CLKMGR - PLL_CTRL_MAIN
	mdelay(10);
	rawchip_spi_write_2B1B(0x0009, 0x01); // Unreferenced register - Warning
	mdelay(10);
	rawchip_spi_write_2B1B(0x1000, 0x01); // NVM - IOR_NVM_CTRL
	mdelay(10);
	rawchip_spi_write_2B1B(0x2000, 0xff); // MIPIRX - DPHY_SC_4SF_ENABLE
	mdelay(10);
	rawchip_spi_write_2B1B(0x2004, 0x06); // MIPIRX - DPHY_SC_4SF_UIX4
	mdelay(10);
	//rawchip_spi_write_2B1B(0x5000, 0xff); // MIPITX - DPHY_MC_4MF_ENABLE
	mdelay(10);
	//rawchip_spi_write_2B1B(0x5004, 0x06); // MIPITX - DPHY_MC_4MF_UIX4
	rawchip_spi_write_2B1B(0x5004, 0x14); // MIPITX - DPHY_MC_4MF_UIX4
	mdelay(10);
	rawchip_spi_write_2B1B(0x2408, 0x04); // CSI2RX - CSI2_RX_NB_DATA_LANES
	mdelay(10);
	rawchip_spi_write_2B1B(0x240c, 0x0a); // CSI2RX - CSI2_RX_IMG_UNPACKING_FORMAT
	mdelay(10);
	rawchip_spi_write_2B1B(0x2420, 0x01); // CSI2RX - CSI2_RX_BYTE2PIXEL_READ_TH
	mdelay(10);
	rawchip_spi_write_2B1B(0x2428, 0x2b); // CSI2RX - CSI2_RX_DATA_TYPE
	mdelay(10);
	rawchip_spi_write_2B1B(0x4400, 0x01); // SMIAF - SMIAF_CTRL
	mdelay(10);
	rawchip_spi_write_2B1B(0x4404, 0x0a); // SMIAF - SMIAF_PIX_WIDTH
	mdelay(10);

	//HD 
	rawchip_spi_write_2B1B(0x4a05, 0x04); // PW2_FIFO THRESHOLD

	//rawchip_spi_write_2B1B(0x2c09, 0x08); // Unreferenced register - Warning
	rawchip_spi_write_2B1B(0x2c09, 0x10); // Unreferenced register - Warning

	mdelay(10);
	rawchip_spi_write_2B1B(0x2c0c, 0xd0); // IDP_GEN - IDP_GEN_LINE_LENGTH
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c0d, 0x0c); // IDP_GEN - IDP_GEN_LINE_LENGTH
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c0e, 0xa0); // IDP_GEN - IDP_GEN_LINE_LENGTH
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c0f, 0x0f); // IDP_GEN - IDP_GEN_LINE_LENGTH
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c10, 0xa0); // IDP_GEN - IDP_GEN_FRAME_LENGTH
	mdelay(10);


	rawchip_spi_write_2B1B(0x2c11, 0x09); // IDP_GEN - IDP_GEN_FRAME_LENGTH
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c12, 0xf0); // IDP_GEN - IDP_GEN_FRAME_LENGTH
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c13, 0xff); // IDP_GEN - IDP_GEN_FRAME_LENGTH
	mdelay(10);
	rawchip_spi_write_2B1B(0x3400, 0x01); // PAT_GEN - PATTERN_GEN_ENABLE
	mdelay(10);
	rawchip_spi_write_2B1B(0x3401, 0x00); // PAT_GEN - PATTERN_GEN_ENABLE
	mdelay(10);
	rawchip_spi_write_2B1B(0x3402, 0x00); // PAT_GEN - PATTERN_GEN_ENABLE
	mdelay(10);
	rawchip_spi_write_2B1B(0x3403, 0x00); // PAT_GEN - PATTERN_GEN_ENABLE
	mdelay(10);
	rawchip_spi_write_2B1B(0x3408, 0x02); // PAT_GEN - PATTERN_GEN_PATTERN_TYPE_REQ
	mdelay(10);
	rawchip_spi_write_2B1B(0x3409, 0x00); // PAT_GEN - PATTERN_GEN_PATTERN_TYPE_REQ
	mdelay(10);
	rawchip_spi_write_2B1B(0x340a, 0x00); // PAT_GEN - PATTERN_GEN_PATTERN_TYPE_REQ
	mdelay(10);
	rawchip_spi_write_2B1B(0x340b, 0x00); // PAT_GEN - PATTERN_GEN_PATTERN_TYPE_REQ
	mdelay(10);
	rawchip_spi_write_2B1B(0x5880, 0x01); // EOFREPRE - EOFRESIZE_ENABLE
	mdelay(10);
	rawchip_spi_write_2B1B(0x5888, 0x01); // EOFREPRE - EOFRESIZE_AUTOMATIC_CONTROL
	mdelay(10);
	rawchip_spi_write_2B1B(0x4400, 0x11); // SMIAF - SMIAF_CTRL
	mdelay(10);
	rawchip_spi_write_2B1B(0x4408, 0x01); // SMIAF - SMIAF_GROUPED_PARAMETER_HOLD
	mdelay(10);
	rawchip_spi_write_2B1B(0x440c, 0x03); // SMIAF - SMIAF_EOF_INT_EN
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c00, 0x01); // CSI2TX - CSI2_TX_ENABLE
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c08, 0x01); // CSI2TX - CSI2_TX_NUMBER_OF_LANES
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c10, 0x01); // CSI2TX - CSI2_TX_PACKET_CONTROL
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c4c, 0x14); // CSI2TX - CSI2_TX_PACKET_SIZE_0
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c4d, 0x00); // CSI2TX - CSI2_TX_PACKET_SIZE_0
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c50, 0x2b); // CSI2TX - CSI2_TX_DI_INDEX_CTRL_0
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c51, 0x00); // CSI2TX - CSI2_TX_DI_INDEX_CTRL_0
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c5c, 0x2b); // CSI2TX - CSI2_TX_DI_INDEX_CTRL_1
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c5d, 0x00); // CSI2TX - CSI2_TX_DI_INDEX_CTRL_1
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c58, 0x04); // CSI2TX - CSI2_TX_PACKET_SIZE_1
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c59, 0x10); // CSI2TX - CSI2_TX_PACKET_SIZE_1
	mdelay(10);
	rawchip_spi_write_2B1B(0x5828, 0x01); // DTFILTER0 - DTFILTER_MATCH0
	mdelay(10);
	rawchip_spi_write_2B1B(0x582c, 0x02); // DTFILTER0 - DTFILTER_MATCH1
	mdelay(10);
	rawchip_spi_write_2B1B(0x5830, 0x0d); // DTFILTER0 - DTFILTER_MATCH2
	mdelay(10);
	rawchip_spi_write_2B1B(0x5834, 0x03); // DTFILTER0 - DTFILTER_MATCH3
	mdelay(10);
	rawchip_spi_write_2B1B(0x5820, 0x01); // DTFILTER0 - DTFILTER_ENABLE
	mdelay(10);
	rawchip_spi_write_2B1B(0x5868, 0xff); // DTFILTER1 - DTFILTER_MATCH0
	mdelay(10);
	rawchip_spi_write_2B1B(0x586c, 0xff); // DTFILTER1 - DTFILTER_MATCH1
	mdelay(10);
	rawchip_spi_write_2B1B(0x5870, 0xff); // DTFILTER1 - DTFILTER_MATCH2
	mdelay(10);
	rawchip_spi_write_2B1B(0x5874, 0xff); // DTFILTER1 - DTFILTER_MATCH3
	mdelay(10);
	rawchip_spi_write_2B1B(0x5860, 0x01); // DTFILTER1 - DTFILTER_ENABLE
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c08, 0x94); // LECCI - LECCI_MIN_INTERLINE
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c09, 0x02); // LECCI - LECCI_MIN_INTERLINE
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c0c, 0xfc); // LECCI - LECCI_OUT_BURST_CTRL
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c10, 0x90); // LECCI - LECCI_LINE_SIZE
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c11, 0x01); // LECCI - LECCI_LINE_SIZE
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c14, 0x01); // LECCI - LECCI_BYPASS_CTRL
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c00, 0x01); // LECCI - LECCI_ENABLE
	mdelay(10);

	//HD
	rawchip_spi_write_2B1B(0x5000, 0x33); // LECCI - LECCI_ENABLE
	mdelay(100);


	rawchip_spi_write_2B1B(0x2c00, 0x01); // IDP_GEN - IDP_GEN_AUTO_RUN
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c01, 0x00); // IDP_GEN - IDP_GEN_AUTO_RUN
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c02, 0x00); // IDP_GEN - IDP_GEN_AUTO_RUN
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c03, 0x00); // IDP_GEN - IDP_GEN_AUTO_RUN
	msleep(2000);
	printk("< [CAM] ASIC_Test");
}

void Dump_DxO_Frame_Count()
{
	uint16_t data;
	uint32_t udwSpiBaseIndex;

	//Yushan_WaitForInterruptEvent(EVENT_DXODPP_NEWFRAMEPROC_ACK, TIME_20MS);

	msleep(1);
	SPI_Read(DXO_DOP_BASE_ADDR + DxODOP_frame_number_7_0, 2, (uint8_t *)(&data));
	printk("[CAM] DxO IP frame count, DOP = 0x%x\n", data);

	msleep(1);
	SPI_Read(DXO_DOP_BASE_ADDR + DxODOP_visible_line_size_7_0, 2, (uint8_t *)(&data));
	printk("[CAM] DxO IP, DOP Visible Line = 0x%x\n", data);

	udwSpiBaseIndex = 0x10000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
	msleep(1);
	SPI_Read(DXO_DPP_BASE_ADDR + DxODPP_frame_number_7_0 - 0x8000, 2, (uint8_t *)(&data));
	printk("[CAM] DxO IP frame count, DxODPP_frame_number_7_0, DPP = 0x%x\n", data);

	msleep(1);
	SPI_Read(DXO_DPP_BASE_ADDR + DxODPP_visible_line_size_7_0 - 0x8000, 2, (uint8_t *)(&data));
	printk("[CAM] DxO IP, DPP Visible Line = 0x%x\n", data);

	udwSpiBaseIndex = 0x8000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	msleep(1);
	SPI_Read(DXO_PDP_BASE_ADDR + DxOPDP_frame_number_7_0, 2, (uint8_t *)(&data));
	printk("[CAM] DxO IP frame count, PDP = 0x%x\n", data);

	msleep(1);
	SPI_Read(DXO_PDP_BASE_ADDR + DxOPDP_visible_line_size_7_0, 2, (uint8_t *)(&data));
	printk("[CAM] DxO IP, PDP Visible Line = 0x%x\n", data);
}

void Dump_DxO_Reqiured_Register()
{
	uint8_t i;
	int index = 0;
	int data = 0;
	uint16_t addr = 0;

	printk("[CAM] >>> Dump_DxO_Reqiured_Register\n");

	data = 0;
	SPI_Read(0x10, 4, (uint8_t*)(&data));
	printk("[CAM] 0x10: PLL_CTRL_Main=0x%x\n",data);
	mdelay(10);

	data = 0;
	SPI_Read(0x14, 4, (uint8_t*)(&data));
	printk("[CAM] 0x14: PLL_LOOP_OUT_DF=0x%x\n",data);
	mdelay(10);

	data = 0;
	SPI_Read(0x1a08, 4, (uint8_t*)(&data));
	printk("[CAM] 0x1a08: Data = 0x%x\n",data);
	mdelay(10);

	data = 0;
	SPI_Read(0x1c04, 4, (uint8_t*)(&data));
	printk("[CAM] 0x1c04: Data = 0x%x\n",data);
	mdelay(10);

	data = 0;
	SPI_Read(0x1c08, 4, (uint8_t*)(&data));
	printk("[CAM] 0x1c08: Data = 0x%x\n",data);
	mdelay(10);

	data = 0;
	SPI_Read(0x1c0c, 4, (uint8_t*)(&data));
	printk("[CAM] 0x1c0c: Data = 0x%x\n",data);
	mdelay(10);

	data = 0;
	SPI_Read(0x1c10, 4, (uint8_t*)(&data));
	printk("[CAM] 0x1c10: Data = 0x%x\n",data);
	mdelay(10);

	data = 0;
	SPI_Read(0x1c14, 4, (uint8_t*)(&data));
	printk("[CAM] 0x1c14: Data = 0x%x\n",data);
	mdelay(10);

	data = 0;
	SPI_Read(0x1c18, 4, (uint8_t*)(&data));
	printk("[CAM] 0x1c18: Data = 0x%x\n",data);
	mdelay(10);

	data = 0;
	SPI_Read(0x1c0c, 4, (uint8_t*)(&data));
	printk("[CAM] 0x1c0c: Data = 0x%x\n",data);

	data = 0;
	SPI_Read(0x2c08, 4, (uint8_t*)(&data));
	printk("[CAM] 0x2c08: Data = 0x%x\n",data);

	addr = 0x1c04;
	for (index = 0; index < 256; index++) {
		mdelay(10);
		data = 0;
		SPI_Read(addr, 4, (uint8_t*)(&data));
		printk("[CAM] addr: 0x%x,  Data = 0x%x\n",addr, data);
		addr += 0x4;
	}

	addr = 0x2c18;
	for (index = 0; index < 16; index++) {
		mdelay(10);
		data = 0;
		SPI_Read(addr, 4, (uint8_t*)(&data));
		printk("[CAM] addr: 0x%x,  Data = 0x%x\n",addr, data);
		addr += 0x4;
	}

printk("[CAM] <<< Dump_DxO_Reqiured_Register\n");
}