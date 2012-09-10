include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(SOC).o

SOBJS	= reset.o cache.o

COBJS-y		+= cpu_info.o
COBJS-y		+= timer.o

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
