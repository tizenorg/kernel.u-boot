include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(SOC).a

SOBJS	= reset.o

COBJS-y		+= cpu_info.o
COBJS-y		+= timer.o
COBJS-$(CONFIG_CMD_USBDOWN)	+= usb-hs-otg.o usb_downloader.o

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS-y) $(SOBJS))

all:	 $(obj).depend $(LIB)

$(LIB):	$(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
