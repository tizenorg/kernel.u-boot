#
#  Samsung Universal(S5PC100/S5PC110) board OneNAND IPL
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
# S5PC100 loads at 0x34000
# S5PC110 loads at 0x30000 (EVT0)
# S5PC110 loads at 0x20010 (EVT1)
# Now it uses the S5PC100 as default
# If you use the S5PC110, the load address is relocated at start.S
# XXX Never set it S5PC110 it don't relocate it
# FIXME BL1 header uses 16 bytes at S5PC110 EVT1
TEXT_BASE		= 0xD0034000
TEXT_BASE16K_EVT0	= 0xD0038000
TEXT_BASE16K_EVT1	= 0xD0037ff0
TEXT_BASE16K_EVT1_SEC	= 0xD0037000

LDSCRIPT= $(TOPDIR)/onenand_ipl/board/$(BOARDDIR)/u-boot-onenand.lds
LDFLAGS	= -Bstatic -T $(LDSCRIPT) -Ttext $(TEXT_BASE) $(PLATFORM_LDFLAGS)
AFLAGS	+= -DCONFIG_PRELOADER -DCONFIG_ONENAND_IPL -g -UTEXT_BASE -DTEXT_BASE=$(TEXT_BASE)
CFLAGS	+= -DCONFIG_PRELOADER -DCONFIG_ONENAND_IPL -g -D__HAVE_ARCH_MEMCPY32
OBJCFLAGS += --gap-fill=0x00

SOBJS	:= lowlevel_init.o mem_setup.o
SOBJS	+= start.o _memcpy32.o
SOBJS	+= cache.o
COBJS	+= onenand_read.o samsung_read.o
COBJS	+= onenand_boot.o

SRCS	:= $(addprefix $(obj),$(SOBJS:.o=.S) $(COBJS:.o=.c))
OBJS	:= $(addprefix $(obj),$(SOBJS) $(COBJS))
__OBJS	:= $(SOBJS) $(COBJS)
LNDIR	:= $(OBJTREE)/onenand_ipl/board/$(BOARDDIR)

onenandobj	:= $(OBJTREE)/onenand_ipl/

ALL	= $(onenandobj)onenand-ipl $(onenandobj)onenand-ipl.bin $(onenandobj)onenand-ipl-16k-evt0.bin $(onenandobj)onenand-ipl-16k-evt1.bin $(onenandobj)onenand-ipl-fused.bin

all:	$(obj).depend $(ALL)

$(onenandobj)onenand-ipl-16k-evt0.bin:	$(onenandobj)onenand-ipl checksum
	$(OBJCOPY) ${OBJCFLAGS} --pad-to=$(TEXT_BASE16K_EVT0) -O binary $< $@
	$(onenandobj)/board/$(BOARDDIR)/checksum $@ 0

$(onenandobj)onenand-ipl-16k-evt1.bin:	$(onenandobj)onenand-ipl checksum
	$(OBJCOPY) ${OBJCFLAGS} --pad-to=$(TEXT_BASE16K_EVT1) -O binary $< $@
	$(onenandobj)/board/$(BOARDDIR)/checksum $@ 1
	cat $(onenandobj)/board/$(BOARDDIR)/header.bin $@ > $(onenandobj)onenand-ipl-16k.bin

$(onenandobj)onenand-ipl-fused.bin:	$(onenandobj)onenand-ipl checksum
	$(OBJCOPY) ${OBJCFLAGS} --pad-to=$(TEXT_BASE16K_EVT1_SEC) -O binary $< $@
	cat $(onenandobj)/board/$(BOARDDIR)/bl1.img $@ > $(onenandobj)onenand-ipl-16k-fused.bin

$(onenandobj)onenand-ipl.bin:	$(onenandobj)onenand-ipl
	$(OBJCOPY) ${OBJCFLAGS} -O binary $< $@

$(onenandobj)onenand-ipl:	$(OBJS)
	cd $(LNDIR) && $(LD) $(LDFLAGS) $$UNDEF_SYM $(__OBJS) \
		-Map $@.map -o $@

checksum: checksum.c
	$(HOSTCC) -o $@ $<

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

# from onenand_ipl directory
$(obj)onenand_ipl.h:
	@rm -f $@
	ln -s $(SRCTREE)/onenand_ipl/onenand_ipl.h $@

$(obj)onenand_boot.c:	$(obj)onenand_ipl.h
	@rm -f $@
	ln -s $(SRCTREE)/onenand_ipl/onenand_boot.c $@

$(obj)onenand_read.c:	$(obj)onenand_ipl.h
	@rm -f $@
	ln -s $(SRCTREE)/onenand_ipl/onenand_read.c $@

$(obj)samsung_read.c:	$(obj)onenand_ipl.h
	@rm -f $@
	ln -s $(SRCTREE)/onenand_ipl/samsung_read.c $@

#########################################################################

$(obj)%.o:	$(obj)%.S
	$(CC) $(AFLAGS) -c -o $@ $<

$(obj)%.o:	$(obj)$.c
	$(CC) $(CFLAGS) -c -o $@ $<

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

clean:
	rm onenand_boot.c onenand_read.c samsung_read.c

#########################################################################
