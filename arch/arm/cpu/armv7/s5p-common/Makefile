#
# Copyright (C) 2009 Samsung Electronics
# Minkyu Kang <mk7.kang@samsung.com>
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y		+= cpu_info.o
ifndef CONFIG_SPL_BUILD
obj-y		+= timer.o
obj-y		+= sromc.o
obj-$(CONFIG_PWM)	+= pwm.o
COBJS-$(CONFIG_CMD_USBDOWN)   += usb_downloader.o
COBJS-$(CONFIG_S5P_USB_DMA)   += usb-hs-otg-dma.o
endif
