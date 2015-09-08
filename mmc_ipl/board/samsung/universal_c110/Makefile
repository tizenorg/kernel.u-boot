#
#  Samsung Universal(S5PC100/S5PC110) board MMC IPL
#
#  Copyright (C) 2009 Samsung Electronics
#  Minkyu Kang <mk7.kang@samsung.com>
#  Kyungmin Park <kyungmin.park@samsung.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#

include $(TOPDIR)/config.mk
# S5PC110 loads at 0x20010 (EVT1)
# BL2 loads at 0x21000 (EVT1)
TEXT_BASE		= 0xD0021000
TEXT_BASE8K_EVT1_SEC	= 0xD0023000

LDSCRIPT= $(TOPDIR)/mmc_ipl/board/$(BOARDDIR)/u-boot-mmc.lds
LDFLAGS	= -Bstatic -T $(LDSCRIPT) -Ttext $(TEXT_BASE) $(PLATFORM_LDFLAGS)
AFLAGS	+= -DCONFIG_PRELOADER -DCONFIG_MMC_IPL -g -UTEXT_BASE -DTEXT_BASE=$(TEXT_BASE)
CFLAGS	+= -DCONFIG_PRELOADER -DCONFIG_MMC_IPL -g -D__HAVE_ARCH_MEMCPY32
OBJCFLAGS += --gap-fill=0x00

SOBJS	:= lowlevel_init.o mem_setup.o
SOBJS	+= start.o _memcpy32.o
SOBJS	+= cache.o
COBJS	+= mmc.o
COBJS	+= mmc_boot.o
COBJS	+= universal.o

SRCS	:= $(addprefix $(obj),$(SOBJS:.o=.S) $(COBJS:.o=.c))
OBJS	:= $(addprefix $(obj),$(SOBJS) $(COBJS))
__OBJS	:= $(SOBJS) $(COBJS)
LNDIR	:= $(OBJTREE)/mmc_ipl/board/$(BOARDDIR)

mmcobj	:= $(OBJTREE)/mmc_ipl/

ALL	= $(mmcobj)mmc-ipl $(mmcobj)mmc-ipl.bin $(mmcobj)mmc-ipl-fused.bin

all:	$(obj).depend $(ALL)

$(mmcobj)mmc-ipl-fused.bin:	$(mmcobj)mmc-ipl
	$(OBJCOPY) ${OBJCFLAGS} --pad-to=$(TEXT_BASE8K_EVT1_SEC) -O binary $< $@
	cat $(mmcobj)/board/$(BOARDDIR)/bl1.img $@ > $(mmcobj)mmc-ipl-12k-fused.bin

$(mmcobj)mmc-ipl.bin:	$(mmcobj)mmc-ipl
	$(OBJCOPY) ${OBJCFLAGS} -O binary $< $@

$(mmcobj)mmc-ipl:	$(OBJS)
	cd $(LNDIR) && $(LD) $(LDFLAGS) $$UNDEF_SYM $(__OBJS) \
		-Map $@.map -o $@

# create symbolic links from common files

# from cpu directory
$(obj)start.S:
	@rm -f $@
	ln -s $(SRCTREE)/$(CPUDIR)/start.S $@

# from SoC directory
$(obj)cache.S:
	@rm -f $@
	ln -s $(SRCTREE)/$(CPUDIR)/$(SOC)/cache.S $@

$(obj)mem_setup.S:
	ln -sf $(SRCTREE)/board/$(BOARDDIR)/mem_setup.S $@

$(obj)lowlevel_init.S:
	ln -sf $(SRCTREE)/board/$(BOARDDIR)/lowlevel_init.S $@

# from lib_arm
$(obj)_memcpy32.S:
	ln -sf $(SRCTREE)/arch/$(ARCH)/lib/_memcpy32.S $@

# from mmc_ipl directory
$(obj)mmc_ipl.h:
	@rm -f $@
	ln -s $(SRCTREE)/mmc_ipl/mmc_ipl.h $@

$(obj)mmc_boot.c:	$(obj)mmc_ipl.h
	@rm -f $@
	ln -s $(SRCTREE)/mmc_ipl/mmc_boot.c $@

$(obj)mmc.c:	$(obj)mmc_ipl.h
	@rm -f $@
	ln -s $(SRCTREE)/mmc_ipl/mmc.c $@

#########################################################################

$(obj)%.o:	$(obj)%.S
	$(CC) $(AFLAGS) -c -o $@ $<

$(obj)%.o:	$(obj)$.c
	$(CC) $(CFLAGS) -c -o $@ $<

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

clean:
	rm mmc_boot.c mmc.c

#########################################################################
