#
#  Samsung Universal(S5PC210) board OneNAND IPL
#
#  Copyright (C) 2010 Samsung Electronics
#  Kyungmin Park <kyungmin.park@samsung.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#

include $(TOPDIR)/config.mk
# S5PC210 loads at 0x02020000 (EVT0)
TEXT_BASE		= 0x02020000
TEXT_BASE14K_EVT0	= 0x02023800
TEXT_BASE16K_EVT0	= 0x02024000

LDSCRIPT= $(TOPDIR)/onenand_ipl/board/$(BOARDDIR)/u-boot-onenand.lds
LDFLAGS	= -Bstatic -T $(LDSCRIPT) -Ttext $(TEXT_BASE) $(PLATFORM_LDFLAGS)
AFLAGS	+= -DCONFIG_PRELOADER -DCONFIG_ONENAND_IPL -g -UTEXT_BASE -DTEXT_BASE=$(TEXT_BASE)
CFLAGS	+= -DCONFIG_PRELOADER -DCONFIG_ONENAND_IPL -g -D__HAVE_ARCH_MEMCPY32
OBJCFLAGS += --gap-fill=0x00

SOBJS	:= lowlevel_init.o mem_setup.o
SOBJS	+= start.o _memcpy32.o
SOBJS	+= cache.o
COBJS	+= onenand_read.o
COBJS	+= onenand_boot.o

SRCS	:= $(addprefix $(obj),$(SOBJS:.o=.S) $(COBJS:.o=.c))
OBJS	:= $(addprefix $(obj),$(SOBJS) $(COBJS))
__OBJS	:= $(SOBJS) $(COBJS)
LNDIR	:= $(OBJTREE)/onenand_ipl/board/$(BOARDDIR)

onenandobj	:= $(OBJTREE)/onenand_ipl/

ALL	= zerofile-16k $(onenandobj)onenand-ipl-32k.bin $(onenandobj)onenand-ipl-16k-evt0.bin

all:	$(obj).depend $(ALL)

$(onenandobj)onenand-ipl-32k.bin:	$(onenandobj)onenand-ipl-32k-evt0-fused.bin $(onenandobj)onenand-ipl-16k-evt0.bin
	cp $< $@
	cat $(onenandobj)onenand-ipl-16k-evt0.bin zerofile-16k > $(onenandobj)onenand-ipl-32k-evt0.bin

$(onenandobj)onenand-ipl-32k-evt0-fused.bin:
	cat c210.bl1-16k.bin.signed onenand-ipl-16k-evt0.bin.signed > $@

$(onenandobj)onenand-ipl-16k-evt0.bin:	$(onenandobj)onenand-ipl checksum
	$(OBJCOPY) ${OBJCFLAGS} --pad-to=$(TEXT_BASE14K_EVT0) -O binary $< $(onenandobj)onenand-ipl-14k-evt0.bin
	$(OBJCOPY) ${OBJCFLAGS} --pad-to=$(TEXT_BASE16K_EVT0) -O binary $< $@
	$(onenandobj)/board/$(BOARDDIR)/checksum $(onenandobj)onenand-ipl-14k-evt0.bin 0
	$(onenandobj)/board/$(BOARDDIR)/checksum $@ 0

$(onenandobj)onenand-ipl.bin:	$(onenandobj)onenand-ipl
	$(OBJCOPY) ${OBJCFLAGS} -O binary $< $@

$(onenandobj)onenand-ipl:	$(OBJS)
	cd $(LNDIR) && $(LD) $(LDFLAGS) $$UNDEF_SYM $(__OBJS) \
		-Map $@.map -o $@

zerofile-16k:
	dd if=/dev/zero of=zerofile-16k bs=1024 count=16

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

#########################################################################

$(obj)%.o:	$(obj)%.S
	$(CC) $(AFLAGS) -c -o $@ $<

$(obj)%.o:	$(obj)$.c
	$(CC) $(CFLAGS) -c -o $@ $<

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

clean:
	rm onenand_boot.c onenand_read.c checksum

#########################################################################
