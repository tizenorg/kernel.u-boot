#
#  Samsung SMDK6442(S5P6442) board OneNAND IPL
#
#  Copyright (C) 2009 Samsung Electronics
#  Minkyu Kang <mk7.kang@samsung.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#

include $(TOPDIR)/config.mk
TEXT_BASE = 0xD0021800
TEXT_BASE8K = 0xD0023800

LDSCRIPT= $(TOPDIR)/onenand_ipl/board/$(BOARDDIR)/u-boot-onenand.lds
LDFLAGS	= -Bstatic -T $(LDSCRIPT) -Ttext $(TEXT_BASE) $(PLATFORM_LDFLAGS)
AFLAGS	+= -DCONFIG_PRELOADER -DCONFIG_ONENAND_IPL -g
CFLAGS	+= -DCONFIG_PRELOADER -DCONFIG_ONENAND_IPL -g -D__HAVE_ARCH_MEMCPY32
OBJCFLAGS += --gap-fill=0x00

SOBJS	:= lowlevel_init.o mem_setup.o
SOBJS	+= start.o _memcpy32.o
COBJS	+= onenand_read.o
COBJS	+= onenand_boot.o

SRCS	:= $(addprefix $(obj),$(SOBJS:.o=.S) $(COBJS:.o=.c))
OBJS	:= $(addprefix $(obj),$(SOBJS) $(COBJS))
__OBJS	:= $(SOBJS) $(COBJS)
LNDIR	:= $(OBJTREE)/onenand_ipl/board/$(BOARDDIR)

onenandobj	:= $(OBJTREE)/onenand_ipl/

ALL	= $(onenandobj)onenand-ipl $(onenandobj)onenand-ipl.bin $(onenandobj)onenand-ipl-8k.bin

all:	$(obj).depend $(ALL)

$(onenandobj)onenand-ipl-8k.bin:	$(onenandobj)onenand-ipl checksum
	$(OBJCOPY) ${OBJCFLAGS} --pad-to=$(TEXT_BASE8K) -O binary $< $@
	$(onenandobj)/board/$(BOARDDIR)/checksum $@

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
	rm onenand_boot.c onenand_read.c

#########################################################################
