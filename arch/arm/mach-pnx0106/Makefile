#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).

USE_STANDARD_AS_RULE := true

O_TARGET := pnx0106.o

# Object files which are always linked
obj-y	:= arch.o		\
	   cpu.o		\
	   event_router.o	\
	   gpio.o		\
	   irq.o		\
	   kthread.o		\
	   mm.o			\
	   panic_handler.o	\
	   platform_info.o	\
	   sdma.o		\
	   sram.o		\
	   tsc.o		\
	   watchdog.o

obj-m	:=
obj-n	:=
obj-	:=

#obj-y	+= dbgmsg_uart1.o

# Object files which are conditionally linked
obj-$(CONFIG_LEDS) += leds.o

obj-$(CONFIG_PNX0106_ADC) += adc.o
obj-$(CONFIG_PNX0106_BLAS_SPI) += blas_spi.o
obj-$(CONFIG_PNX0106_BLAS_SPISD) += blas_spisd.o
obj-$(CONFIG_PNX0106_ECOSTANDBY) += eco_standby.o clocks.o
obj-$(CONFIG_PNX0106_EPICS) += epics.o
obj-$(CONFIG_PNX0106_EPICS_AAC) += aacbl.o aacblcodec.o
obj-$(CONFIG_PNX0106_EPICS_MP3) += mp3bl.o mp3blcodec.o
obj-$(CONFIG_PNX0106_EPICS_WMA) += wma9.o wma9codec.o
obj-$(CONFIG_PNX0106_EPICS_WMA) += wma9bl.o wma9blcodec.o
obj-$(CONFIG_PNX0106_LCDIF_WRAPPER) += lcdif_wrapper.o
obj-$(CONFIG_PNX0106_PCCARD) += pccard.o
obj-$(CONFIG_PNX0106_PCCARD_CPLD) += pccard_cpld.o

obj-$(CONFIG_BLK_DEV_IDE_PNX0106_SIMPLE) += ide_simple.o
obj-$(CONFIG_BLK_DEV_IDE_PNX0106_FULL) += ide_full.o

obj-$(CONFIG_PNX0106_W6) += w6_buttons.o w6_fiq.o w6_lcd.o w6_led.o w6_rtcdrv.o w6_uart1.o
obj-$(CONFIG_PNX0106_HASLI7) += w6_buttons.o w6_fiq.o w6_lcd.o w6_led.o w6_rtcdrv.o w6_uart1.o
obj-$(CONFIG_PNX0106_HACLI7) += w6_buttons.o w6_fiq.o w6_lcd.o w6_led.o w6_rtcdrv.o w6_uart1.o
obj-$(CONFIG_PNX0106_MCIH) += w6_buttons.o w6_fiq.o w6_lcd.o w6_led.o w6_rtcdrv.o w6_uart1.o
obj-$(CONFIG_PNX0106_MCI) += w6_buttons.o w6_fiq.o w6_lcd.o w6_led.o w6_rtcdrv.o w6_uart1.o


# Object files which are allowed to export symbols
export-objs :=	adc.o		\
		blas_spi.o	\
		blas_spisd.o	\
		cpu.o		\
		dbgmsg_uart1.o	\
		epics.o 	\
		event_router.o	\
		gpio.o		\
		irq.o		\
		lcdif_wrapper.o	\
		leds.o		\
		pccard.o	\
		pccard_cpld.o	\
		sdma.o		\
		tsc.o

include $(TOPDIR)/Rules.make
