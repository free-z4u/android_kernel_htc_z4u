obj-y += irq.o arch_init.o globalregs.o arch_lock.o \
         reserve.o adi.o adie_irq.o adc.o efuse.o \
         watchdog.o common.o glb.o htc_debug.o

obj-y += emmc_partitions.o devices_htc.o

obj-$(CONFIG_HTC_BATTCHG) += htc_battery.o
obj-$(CONFIG_ARCH_SC8825) += dcdc_cal.o timer_sc8825.o
obj-$(CONFIG_ARCH_SC8825) += devices-sc8825.o io-sc8825.o clock-sc8825.o arch_misc.o
obj-$(CONFIG_ARCH_SCX35) += devices-sc8830.o io-sc8830.o clock-sc8830.o arch_misc.o pin_switch.o htc_util.o cpuidle-debug-htc.o
obj-$(CONFIG_ARCH_SCX35) += emc_earlysuspend_8830.o timer_sc8830.o

obj-$(CONFIG_HTC_DBG_LAST_IO) += htc-last-io.o
obj-$(CONFIG_HTC_DBG_UNCACHE_FTRACE) += htc-debug-ftrace.o

obj-$(CONFIG_PM) += pm.o emc_repower.o
obj-$(CONFIG_FIX_V7TAGRAM_BUG) += fix_v7_tag_ram_bug.o
obj-$(CONFIG_CPU_FREQ) += cpufreq.o
CFLAGS_emc_repower.o += -fPIC
ifeq ($(CONFIG_BUS_MONITOR),y)
obj-$(CONFIG_ARCH_SCX35) += bm_sc8830.o
endif

ifeq ($(CONFIG_CPU_IDLE),y)
	obj-$(CONFIG_ARCH_SCX35) += cpuidle-sc8830.o
endif

ifeq ($(CONFIG_PM),y)
	obj-$(CONFIG_ARCH_SC8825) += pm-sc8825.o standby-sc8825.o pm_debug.o
	obj-$(CONFIG_ARCH_SCX35) += pm-sc8830.o standby-sc8830.o pm_debug_sc8830.o cpuidle-sc8830.o
endif

obj-$(CONFIG_SPRD_DCDC_DEBUG) += dcdc_debug.o
obj-$(CONFIG_SMP) += platsmp.o headsmp.o
obj-$(CONFIG_SIPC) += sipc.o
obj-$(CONFIG_HOTPLUG_CPU) += hotplug.o
obj-$(CONFIG_SC_VIBRATOR) += vibrator.o
obj-$(CONFIG_ANDROID_RAM_CONSOLE) += ram_console.o

ifeq ($(CONFIG_RTL8723AS),m)
	obj-y  += rtl8723_adapter.o
endif

obj-$(CONFIG_MACH_SP8825EA) += board-sp8825ea.o pinmap-sp8825ea.o dma_r1p0.o
obj-$(CONFIG_MACH_SP8825EA_TIGER_EVM) += board-sp8825ea_tiger_evm.o pinmap-sp8825ea_tiger_evm.o htc_wifi_nvs.o board-sprd-wifi.o dma_r1p0.o
obj-$(CONFIG_MACH_SPX35FPGA) += board-sp8830fpga.o pinmap-sp8830fpga.o dma_r4p0.o
obj-$(CONFIG_MACH_SPX35EA) += board-sp8830ea.o dma_r4p0.o
obj-$(CONFIG_MACH_Z4DTG) += board-z4dtg.o pinmap-z4dtg.o htc_wifi_nvs.o board-sprd-wifi.o dma_r4p0.o board-z4dtg-bm.o htc_acoustic.o
obj-$(CONFIG_MACH_Z4TD) += board-z4td.o pinmap-z4td.o htc_wifi_nvs.o board-sprd-wifi.o dma_r4p0.o board-z4td-bm.o htc_acoustic.o
obj-$(CONFIG_MACH_CP5DTU) += board-cp5dtu.o pinmap-cp5dtu.o htc_wifi_nvs.o board-sprd-wifi.o dma_r4p0.o board-cp5dtu-bm.o htc_acoustic.o
obj-$(CONFIG_MACH_CP5DUG) += board-cp5dug.o pinmap-cp5dug.o htc_wifi_nvs.o board-sprd-wifi.o dma_r4p0.o board-cp5dug-bm.o htc_acoustic.o
obj-$(CONFIG_MACH_CP5DWG) += board-cp5dwg.o pinmap-cp5dwg.o htc_wifi_nvs.o board-sprd-wifi.o dma_r4p0.o board-cp5dwg-bm.o htc_acoustic.o
obj-$(CONFIG_MACH_CP5VEDTU) += board-cp5vedtu.o pinmap-cp5vedtu.o htc_wifi_nvs.o board-sprd-wifi.o dma_r4p0.o board-cp5vedtu-bm.o htc_acoustic.o
obj-$(CONFIG_MACH_CP5VEDWG) += board-cp5vedwg.o pinmap-cp5vedwg.o htc_wifi_nvs.o board-sprd-wifi.o dma_r4p0.o board-cp5vedwg-bm.o htc_acoustic.o

obj-$(CONFIG_SPRD_DEBUG) += sprd_debug.o

obj-$(CONFIG_SPRD_MODEM) += board-sprd-modem.o

ifeq ($(CONFIG_VIA_MODEM),y)
obj-$(CONFIG_MACH_SP8825EA_TIGER_EVM) += board-sp8825ea_tiger_evm-cbp.o
obj-$(CONFIG_MACH_CP5DWG) += board-cp5dwg-cbp.o
obj-$(CONFIG_MACH_CP5VEDWG) += board-cp5vedwg-cbp.o
endif
obj-$(CONFIG_MACH_SP8825EA_TIGER_EVM) += board-sp8825ea_tiger_evm-gps.o

ifeq ($(CONFIG_ARCH_SCX35),y)
obj-$(CONFIG_DEBUG_FS) += clock_debug_htc.o
obj-$(CONFIG_DEBUG_FS) += adc_debug_htc.o
obj-$(CONFIG_DEBUG_FS) += adc_htc.o
endif
obj-$(CONFIG_MACH_Z4DTG) += board-z4dtg-gps.o
obj-$(CONFIG_MACH_Z4TD) += board-z4td-gps.o
obj-$(CONFIG_MACH_CP5DTU) += board-cp5dtu-gps.o
obj-$(CONFIG_MACH_CP5DUG) += board-cp5dug-gps.o
obj-$(CONFIG_MACH_CP5DWG) += board-cp5dwg-gps.o
obj-$(CONFIG_MACH_CP5VEDTU) += board-cp5vedtu-gps.o
obj-$(CONFIG_MACH_CP5VEDWG) += board-cp5vedwg-gps.o
obj-$(CONFIG_BT) += htc_bdaddress.o

obj-$(CONFIG_HTC_HEADSET_MGR)		+= htc_headset_mgr.o
obj-$(CONFIG_HTC_HEADSET_GPIO)		+= htc_headset_gpio.o
obj-$(CONFIG_HTC_HEADSET_PMIC)		+= htc_headset_pmic.o
obj-$(CONFIG_HTC_HEADSET_ONE_WIRE)	+= htc_headset_one_wire.o

obj-$(CONFIG_PERFLOCK_HTC) += perflock.o
obj-$(CONFIG_PERFLOCK_HTC) += htc_set_perflock.o
