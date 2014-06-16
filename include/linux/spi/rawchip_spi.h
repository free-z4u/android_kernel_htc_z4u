#ifndef __RAWCHIP_SPI_H__
#define __RAWCHIP_SPI_H__

#include <linux/types.h>


 typedef void (* SET_SPI_PIN_INPUT)(void);
 typedef void (* RES_SPI_PIN_CFG)(void);


struct rawchip_platform_data {
	SET_SPI_PIN_INPUT	set_spi_pin_input;
  	RES_SPI_PIN_CFG   restore_spi_pin_cfg;
};

#endif

