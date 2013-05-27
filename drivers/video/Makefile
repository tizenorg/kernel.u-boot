#
# (C) Copyright 2000-2007
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	:= $(obj)libvideo.o

COBJS-$(CONFIG_ATI_RADEON_FB) += ati_radeon_fb.o videomodes.o
COBJS-$(CONFIG_ATMEL_LCD) += atmel_lcdfb.o
COBJS-$(CONFIG_CFB_CONSOLE) += cfb_console.o
COBJS-$(CONFIG_FSL_DIU_FB) += fsl_diu_fb.o
COBJS-$(CONFIG_S6E63D6) += s6e63d6.o
COBJS-$(CONFIG_VIDEO_AMBA) += amba.o
COBJS-$(CONFIG_VIDEO_CT69000) += ct69000.o videomodes.o
COBJS-$(CONFIG_VIDEO_MB862xx) += mb862xx.o videomodes.o
COBJS-$(CONFIG_VIDEO_MB86R0xGDC) += mb86r0xgdc.o videomodes.o
COBJS-$(CONFIG_VIDEO_MX3) += mx3fb.o
COBJS-$(CONFIG_VIDEO_MX5) += mxc_ipuv3_fb.o ipu_common.o ipu_disp.o
COBJS-$(CONFIG_VIDEO_SED13806) += sed13806.o
COBJS-$(CONFIG_SED156X) += sed156x.o
COBJS-$(CONFIG_VIDEO_SM501) += sm501.o
COBJS-$(CONFIG_VIDEO_SMI_LYNXEM) += smiLynxEM.o videomodes.o
COBJS-$(CONFIG_VIDEO_VCXK) += bus_vcxk.o
#COBJS-$(CONFIG_S5PC1XXFB) += s5p-fb.o s5p-fimd.o s5p-spi.o
COBJS-$(CONFIG_S6E39A0X) += s6e39a0x.o
COBJS-$(CONFIG_S6E39A0X02) += s6e39a0x02.o
COBJS-$(CONFIG_S5PC1XXFB) += s5p-fb.o s5p-fimd.o
COBJS-$(CONFIG_DSIM) += s5p_mipi_dsi.o s5p_mipi_dsi_lowlevel.o s5p_mipi_dsi_common.o
COBJS-$(CONFIG_S6D6AA1) += s6d6aa1.o
COBJS-$(CONFIG_S6E63M0) += s6e63m0.o
COBJS-$(CONFIG_S6E8AX0) += s6e8ax0.o
COBJS-$(CONFIG_S6D16A0X) += s6d16a0x.o
COBJS-$(CONFIG_AMS701KA) += ams701ka.o
COBJS-$(CONFIG_LD9040) += ld9040.o
COBJS-$(CONFIG_OMAP_LD9040) += omap-ld9040.o
COBJS-$(CONFIG_NT39411) += nt39411.o
COBJS-$(CONFIG_CMC623) += cmc623.o
COBJS-$(CONFIG_PWM_BACKLIGHT) += pwm_backlight.o
COBJS-$(CONFIG_OPMA44XXFB) += omap-fb.o
COBJS-$(CONFIG_OPMA44XXFB) += omap-dss.o
COBJS-$(CONFIG_OPMA44XXFB) += font_8x16.o fbutils.o
#COBJS-$(CONFIG_S5PC1XXFB) += s5p-fb.o s5p_fimd_aquila.o tl2796_aquila.o
#COBJS-$(CONFIG_S5PC1XXFB) += s5p-dualfb.o s5p-dualfimd.o tl2796_dual.o
#COBJS-$(CONFIG_S5PC1XXFB) += s5p-dualfb.o s5p_dualfimd_aquila.o tl2796_dual_aquila.o
#COBJS-$(CONFIG_SUBLCD) += eltz_lcd.o
#COBJS-y += videomodes.o
COBJS-$(CONFIG_S5PC1XXFB) += font_8x16.o fbutils.o

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
