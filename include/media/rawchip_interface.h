#ifndef __RAWCHIP_INTERFACE_H
#define __RAWCHIP_INTERFACE_H

void rawchip_poweron(void); //rawchip power up and send clk
void rawchip_powerdown(void); //rawchip power down
void rawchip_start(int width); //rawchip start work with size 'width'
void rawchip_stop(void); //rawchip stop work

#endif

