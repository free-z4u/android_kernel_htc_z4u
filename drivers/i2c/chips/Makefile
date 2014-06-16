#
# Makefile for miscellaneous I2C chip drivers.
#
# Do not add new drivers to this directory! It is DEPRECATED.
#
# Device drivers are better grouped according to the functionality they
# implement rather than to the bus they are connected to. In particular:
# * Hardware monitoring chip drivers go to drivers/hwmon
# * RTC chip drivers go to drivers/rtc
# * I/O expander drivers go to drivers/gpio
#


obj-$(CONFIG_SENSORS_NFC_PN544)                    += pn544.o
obj-$(CONFIG_TPS65200)          += tps65200.o
# obj-$(CONFIG_INPUT_CAPELLA_CM36282)         += cm36282.o
obj-$(CONFIG_INPUT_CAPELLA_CM36282)         += cm3629.o
obj-$(CONFIG_SENSORS_ONLY_BMA250E)      		+= bma250e_without_Ecompass.o
obj-$(CONFIG_BOSCH_BMA250)                  += bma250_bosch.o
obj-$(CONFIG_AMP_RT5501)                           += rt5501.o
obj-$(CONFIG_SND_AMP_TFA9887)                      += tfa9887.o
obj-$(CONFIG_SND_AMP_TFA9887L)                     += tfa9887l.o
obj-$(CONFIG_SENSORS_AKM8963)						+= akm8963.o
obj-$(CONFIG_INPUT_YAS_MAGNETOMETER) += yas53x_kernel.o
ifeq ($(CONFIG_I2C_DEBUG_CHIP),y)
EXTRA_CFLAGS += -DDEBUG
endif

