/*
 * Copyright (C) 2008-2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Hyuk Kang <hyuk78.kang@samsung.com>
 */

#include <common.h>
#include <asm/errno.h>
#include "musb.h"

#define MUSB_DEBUG
#undef MUSB_DEBUG
#ifdef MUSB_DEBUG
#define debug(fmt, args...) printf("[ %s :%d]\t " fmt, __func__, __LINE__, ##args)
#else
#define debug(fmt, args...) do { } while (0)
#endif

#ifdef MUSB_DEBUG
#define tags(t)	printf("[TAGS] =============== %s ===============\n", t)
#define tage(t)	printf("[TAGE] =============== %s ===============\n", t)

#define musb_csr0(offset, v)	\
	do { \
		printf(">>> write CSR0 : 0x%x \n ===> %s (%d)\n", v, __func__, __LINE__); \
		MUSB_WRITEW(offset, v); \
	} while (0)

#else
#define tags(t)
#define tage(t)
#define musb_csr0(offset, v)	MUSB_WRITEW(offset, v)
#endif

#define max(a, b)	((a > b) ? a : b)
#define min(a, b)	((a > b) ? b : a)

struct musb musb;

int musb_connected = 0;
int musb_recv_done = 0;

/* codes representing languages */
static const u8 string_desc0[] = {
	4, STRING_DESCRIPTOR, 0x09, 0x04,
};

#ifndef CDC_ACM
/* Manufacturer */
static const u8 string_desc1[] = {
	(0xE + 2), STRING_DESCRIPTOR,
	'S', 0x0, 'A', 0x0, 'M', 0x0, 'S', 0x0, 'U', 0x0,
	'N', 0x0, 'G', 0x0
};

/* Product */
static const u8 string_desc2[] = {
	(0x24 + 2), STRING_DESCRIPTOR,
	'S', 0x0, 'A', 0x0, 'M', 0x0, 'S', 0x0, 'U', 0x0,
	'N', 0x0, 'G', 0x0, ' ', 0x0, 'U', 0x0, 'S', 0x0,
	'B', 0x0, ' ', 0x0, 'D', 0x0, 'R', 0x0, 'I', 0x0,
	'V', 0x0, 'E', 0x0, 'R', 0x0
};
#else
static const u8 str_desc1[] = "SAMSUNG";
static const u8 str_desc2[] = "SAMSUNG USB DRIVER";
static const u8 str_desc3[] = {0x09};
static const u8 str_desc4[] = "SAMSUNG SERIAL BULK";
static const u8 str_desc5[] = "SAMSUNG SERIAL CDC ACM";
static const u8 str_desc6[] = "SAMSUNG SERIAL CONTROL";
static const u8 str_desc7[] = "SAMSUNG SERIAL DATA";

static string_desc_t usb_string_desc[8];

void musb_ep0_txstate(struct musb *musb);

static int usb_strncpy(string_desc_t *desc, const u8 *src, u32 len)
{
	u32 i = 0;

	for (i = 0; i < len; i++)
		desc->bString[i] = (u16)src[i];

	desc->bDescriptorType = STRING_DESCRIPTOR;
	desc->bLength = (len + 1) * 2;
}
#endif


/* Descriptor size */
enum DESCRIPTOR_SIZE {
	DEVICE_DESC_SIZE = sizeof(device_desc_t),
	CONFIG_DESC_SIZE = sizeof(config_desc_t),
	INTERFACE_DESC_SIZE = sizeof(intf_desc_t),
	ENDPOINT_DESC_SIZE = sizeof(ep_desc_t),
	OTHER_SPEED_CFG_SIZE = 9,
	STRING_DESC0_SIZE = sizeof(string_desc0),
#ifndef CDC_ACM
	STRING_DESC1_SIZE = sizeof(string_desc1),
	STRING_DESC2_SIZE = sizeof(string_desc2),
#else
	CDC_HEADER_DESC_SIZE		= sizeof(cdc_header_desc_t),
	CDC_CALL_MANAGEMENT_DESC_SIZE	= sizeof(cdc_call_management_desc_t),
	CDC_ACM_DESC_SIZE		= sizeof(cdc_acm_desc_t),
	CDC_UNION_DESC_SIZE		= sizeof(cdc_union_desc_t),
#endif
};

#ifdef MUSB_DEBUG
void musb_print_pkt(u8 *pt, u8 count)
{
	int i;

	debug("");

	printf("[ ");

	for (i = 0; i < count; i++)
		printf("%02x ", pt[i]);

	printf("]\n");
}

void musb_check_core(void)
{
	printf("Check core----------------\n");
	printf("REVISION:  %08x\n", OTG_REG_REVISION);
	printf("SYSCONFIG: %08x\n", OTG_REG_SYSCONFIG);
	printf("SYSSTATUS: %08x\n", OTG_REG_SYSSTATUS);
	printf("INTERSEL:  %08x\n", OTG_REG_INTERFSEL);
	printf("SIMENABLE: %08x\n", OTG_REG_SIMENABLE);
	printf("--------------------------\n");
}

void musb_check_reg(void)
{
	u8 reg;
	u16 val;

	/* check reg */
	printf("Check reg-----------------\n");
	val = MUSB_READW(MUSBREG_HWVERS);
	printf("HWVER:	%08x\n", val);
	val = MUSB_READW(0x68);
	printf("VSTAT:	%08x\n", val);
	reg = MUSB_READB(MUSBREG_FADDR);
	printf("FADDR:	%08x\n", reg);
	reg = MUSB_READB(MUSBREG_POWER);
	printf("POWER:	%08x\n", reg);
	reg = MUSB_READB(MUSBREG_DEVCTL);
	printf("DEVCTL:	%08x\n", reg);
	printf("--------------------------\n");
}

void musb_check_config(void)
{
	u8 reg;

	reg = MUSB_READB(MUSBREG_CONFIGDATA);

	printf("Check Config---------------\n");
	if (reg & CONFIGDATA_UTMIDW)
		printf("UTMI-16\n");
	else
		printf("UTMI-8\n");
	if (reg & CONFIGDATA_DYNFIFO)
		printf("Dynamic FIFO\n");
	if (reg & CONFIGDATA_SOFTCONE)
		printf("Soft Connect\n");
	printf("--------------------------\n");
}
#endif

void musb_omap_init(struct musb *musb)
{
	u32 val;

	/*
	 * SYSCONFIG
	 * [2] ENABLEWAKEUP
	 */
	val = OTG_REG_SYSCONFIG;
	val &= ~(1 << 2);	/* disable wakeup */
	OTG_REG_SYSCONFIG = val;

	/*
	 * FORCESTDBY
	 * [0] ENABLEFORCE
	 */
	val = OTG_REG_FORCESTDBY;
	val &= ~(1 << 0);	/* disable MSTANDBY */
	OTG_REG_FORCESTDBY = val;

	/*
	 * SYSCONFIG
	 * [0] AUTOIDLE
	 * [2] ENABLEWAKEUP
	 * [3] NOIDLE
	 * [4] SMARTIDLE
	 * [12] NOSTDBY
	 * [13] SMARTSTDBY
	 */
	val = OTG_REG_SYSCONFIG;
	val &= ~(1 << 2);	/* disable wakeup */
	val &= ~(1 << 12);	/* remove possible nostdby */
	val |= (2 << 12);	/* enable smart standby */
	val &= ~(1 << 0);	/* disable auto idle */
	val &= ~(1 << 3);	/* remove possible noidle */
	val |= (2 << 3);	/* enable smart idle */
	val |= (1 << 0);	/* enable auto idle */
	OTG_REG_SYSCONFIG = val;

	/*
	 * INTERFSEL
	 * [0] ULPI_12PIN
	 */
	val = OTG_REG_INTERFSEL;
#if 0			/* ULPI */
	val |= (1 << 0);
#else			/* UMTI+ */
	val &= ~(1 << 0);
	val |= (0<<0);
#endif
	OTG_REG_INTERFSEL = val;
}

/*
 * configure a fifo; for non-shared endpoints, this may be called
 * once for a tx fifo and once for an rx fifo.
 */
int musb_fifo_setup(struct musb *musb, int ep, int style, int max, int offset)
{
	u16 c_off = offset >> 3;
	int size = 0;
	u8 c_size=0;

	/* use single buffer */
	size = ffs(max(max, (u16) 8)) - 1;
	max = 1 << size;
	c_size = size - 3;

	MUSB_WRITEB(MUSBREG_INDEX, ep);

	debug("ep %d, style %d,  size 0x%02x, add 0x%02x\n",
			ep, style, c_size, c_off);

	if (style & FIFO_TX) {
		MUSB_WRITEB(MUSBREG_TXFIFOSZ, c_size);
		MUSB_WRITEW(MUSBREG_TXFIFOADD, c_off);
	}

	if (style & FIFO_RX) {
		MUSB_WRITEB(MUSBREG_RXFIFOSZ, c_size);
		MUSB_WRITEW(MUSBREG_RXFIFOADD, c_off);
	}

	musb->epmask |= (1 << ep);

	offset += (max << ((c_size & FIFOSZ_DPB) ? 1 : 0));

	return offset;
}

/* Initialize MUSB (M)HDRC part of the USB hardware subsystem;
 * configure endpoints, or take their config from silicon
 */
void musb_core_init(struct musb *musb)
{
	u8 val;
	int offset=0;

	MUSB_WRITEB(MUSBREG_INDEX, 0);
	val = MUSB_READB(MUSBREG_CONFIGDATA);

#ifdef MUSB_DEBUG
	musb_check_config();
#endif

	/* discover endpoint configuration */
	musb->nr_endpoints	= 3/*1*/;
	musb->epmask		= 1;
	musb->ep_max_pkt[0]	= 64;
	musb->ep_max_pkt[1]	= 512;
	musb->ep_max_pkt[2]	= 512;
#ifdef CDC_ACM
	musb->ep_max_pkt[3]	= 16;
#endif

	if (val & CONFIGDATA_DYNFIFO) {
		offset = musb_fifo_setup(musb, 0,
				FIFO_RXTX, musb->ep_max_pkt[0], offset);
		debug("offset = %x\n", offset);
		offset = musb_fifo_setup(musb, 1,
				FIFO_TX, musb->ep_max_pkt[1], offset);
		debug("offset = %x\n", offset);
		offset = musb_fifo_setup(musb, 2,
				FIFO_RX, musb->ep_max_pkt[2], offset);
		debug("offset = %x\n", offset);
#ifdef CDC_ACM
		offset = musb_fifo_setup(musb, 3,
				FIFO_TX, musb->ep_max_pkt[3], offset);
		debug("offset = %x\n", offset);
#endif
	} else
		debug("Not support\n");
}

/*
 * Program the HDRC to start (enable interrupts, dma, etc.).
 */
static void musb_start(struct musb *musb)
{
	u8 devctl;

	/* enable ep */
	MUSB_WRITEW(MUSBREG_INTRTXE, musb->epmask);
	MUSB_WRITEW(MUSBREG_INTRRXE, musb->epmask & 0xfffe);
	MUSB_WRITEB(MUSBREG_INTRUSBE,0x7);

	MUSB_WRITEB(MUSBREG_TESTMODE, 0);

	/* put into basic highspeed mode and start session */
	MUSB_WRITEB(MUSBREG_POWER, POWER_ISOUPDATE
			| POWER_SOFTCONN | POWER_HSENAB);

	musb->is_active = 0;

	devctl = MUSB_READB(MUSBREG_DEVCTL);
	/* devctl &= ~DEVCTL_SESSION; */

	if ((devctl & DEVCTL_VBUS) == DEVCTL_VBUS)
		musb->is_active = 1;

	/* MUSB_WRITEB(MUSBREG_DEVCTL, devctl); */
	
#ifdef MUSB_DEBUG
	musb_check_core();
	musb_check_reg();
#endif
}

/* disable interrupt */
void musb_generic_disable(void)
{
	u16 val1,val2,val3;

	/* disable interrupts */
	MUSB_WRITEB(MUSBREG_INTRUSBE, 0);
	MUSB_WRITEW(MUSBREG_INTRTXE, 0);
	MUSB_WRITEW(MUSBREG_INTRRXE, 0);

	/* off */
	MUSB_WRITEB(MUSBREG_DEVCTL, 0);

	/*  flush pending interrupts */
	val1 = MUSB_READB(MUSBREG_INTRUSB);
	val2 = MUSB_READW(MUSBREG_INTRTX);
	val3 = MUSB_READW(MUSBREG_INTRRX);
}

/* lifecycle operations */
void musb_resume(struct musb *musb)
{
	musb->is_suspended = 0;

	switch (musb->xceiv_state) {
	case OTG_STATE_B_IDLE:
		break;
	case OTG_STATE_B_WAIT_ACON:
	case OTG_STATE_B_PERIPHERAL:
		musb->is_active = 1;
		break;
	default:
		break;
	}
}

/* called when SOF packets stop for 3+ msec */
void musb_suspend(struct musb *musb)
{
	u8 devctl;

	devctl = MUSB_READB(MUSBREG_DEVCTL);
	debug("musb suspend: devctl %02x\n", devctl);

	switch (musb->xceiv_state) {
	case OTG_STATE_B_IDLE:
		if ((devctl & DEVCTL_VBUS) == DEVCTL_VBUS)
			musb->xceiv_state = OTG_STATE_B_PERIPHERAL;
		break;
	case OTG_STATE_B_PERIPHERAL:
		musb->is_suspended = 1;
		break;
	default:
		break;
	}
}

void musb_stop(struct musb *musb)
{
	u8 devctl;

	devctl = MUSB_READB(MUSBREG_DEVCTL);
	debug("musb stop : devctl %02x\n", devctl);

	MUSB_WRITEB(MUSBREG_DEVCTL, devctl & DEVCTL_SESSION);

	switch (musb->xceiv_state) {
	case OTG_STATE_B_PERIPHERAL:
	case OTG_STATE_B_IDLE:
		musb->xceiv_state = OTG_STATE_B_IDLE;
	}

	musb->is_active = 0;
	musb_connected = 0;
}

void musb_reset(struct musb *musb)
{
	u8 devctl;

	devctl = MUSB_READB(MUSBREG_DEVCTL);
	debug("musb reset : devctl %02x\n", devctl);

	if (musb_connected)
		musb_stop(musb);
	else if (devctl & DEVCTL_HR)
		MUSB_WRITEB(MUSBREG_DEVCTL, DEVCTL_SESSION);

	/* start in USB_STATE_DEFAULT */
	musb->is_active = 1;
	musb->is_suspended = 0;
	musb->address = 0;
	musb->ep0_state = MUSB_EP0_STAGE_IDLE;
	musb->xceiv_state = OTG_STATE_B_PERIPHERAL;
}

/* enable endpoints
 * BULK only
 */
void musb_ep_enable(struct musb *musb, int ep, int type)
{
	u16 offset;
	u16 csr = 0;

	offset = MUSBREG_EP_OFFSET(ep, 0);

	if (type & USB_DIR_IN) {
		MUSB_WRITEW(offset + MUSBREG_TXMAXP, musb->ep_max_pkt[ep]);

		csr = MUSB_READW(offset + MUSBREG_CSR);

		if (csr & TXCSR_FIFONOTEMPTY)
			csr = TXCSR_FLUSHFIFO;

		csr |= (TXCSR_MODE | TXCSR_CLRDATATOG | TXCSR_DMAENAB);

		MUSB_WRITEW(offset + MUSBREG_CSR, csr);
	} else {
		MUSB_WRITEW(offset + MUSBREG_RXMAXP, musb->ep_max_pkt[ep]);

		csr = RXCSR_FLUSHFIFO | RXCSR_CLRDATATOG | RXCSR_AUTOCLEAR;
		csr &= ~RXCSR_DMAENAB;

		MUSB_WRITEW(offset + MUSBREG_RXCSR, csr);
	}
}

/*
 * Load an endpoint's FIFO
 */
 #if 1
static void musb_write_fifo(u8 *buf, u32 len, int ep)
{
	int i;
	int count = len / 4;
	int remain = len % 4;
	u32 *data = (u32*)buf;
	u8  *remainData = NULL;

	if (!buf) {
		debug("Error ep%d RX Buffer is NULL.\n", ep);
		return;
	}
	u16 fifo = MUSBREG_FIFO_OFFSET(ep);
	debug("ep%d : size %d\n", ep, len);

	for (i = 0; i < count; i++) {
		MUSB_WRITEL(fifo, *data++);
	}

	remainData = (u8*)data;
	for (i = 0; i < remain; i++) {
		MUSB_WRITEB(fifo, *remainData);
		remainData++;
	}
}
#else
static void musb_write_fifo(u8 *buf, int len, int ep)
{
	int i;

	u8  *remainData = NULL;
	u8 val = 0;

	if (!buf) {
		debug("Error ep%d RX Buffer is NULL.\n", ep);
		return;
	}

	u16 fifo = MUSBREG_FIFO_OFFSET(ep);
	debug("ep%d : size %d\n", ep, len);

	remainData = buf;
	for(i=0; i<len; i++)
	{
		MUSB_WRITEB(fifo, *remainData++);
	}

}
#endif
/*
 * Unload an endpoint's FIFO
 */
static void musb_read_fifo(u8 *buf, int len, int ep)
{
	int i=0;
	u32 *data = (u32 *)buf;
	u32 fifo = MUSBREG_FIFO_OFFSET(ep);

	for (i = 0; i < len; i+=4)
		*data++ = MUSB_READL(fifo);
}

#ifndef	CDC_ACM
/* SAMSUNG XO Driver */
void musb_set_desc(struct musb *musb)
{
	/* Standard device descriptor */
	musb->desc.dev.bLength		= DEVICE_DESC_SIZE;
	musb->desc.dev.bDescriptorType	= DEVICE_DESCRIPTOR;
	musb->desc.dev.bcdUSBL		= 0x00;
	musb->desc.dev.bcdUSBH		= 0x02;
	musb->desc.dev.bDeviceClass	= 0xFF;
	musb->desc.dev.bDeviceSubClass	= 0x0;
	musb->desc.dev.bDeviceProtocol	= 0x0;
	musb->desc.dev.bMaxPacketSize0	= musb->ep_max_pkt[0];
	musb->desc.dev.idVendorL	= 0xE8;
	musb->desc.dev.idVendorH	= 0x04;
	musb->desc.dev.idProductL	= 0x04;
	musb->desc.dev.idProductH	= 0x12;
	musb->desc.dev.iManufacturer	= 0x0;
	musb->desc.dev.iProduct		= 0x2;
	musb->desc.dev.iSerialNumber	= 0x0;
	musb->desc.dev.bNumConfigs	= 0x1;

	/* Standard configuration descriptor */
	musb->desc.config.bLength		= CONFIG_DESC_SIZE;
	musb->desc.config.bDescriptorType	= CONFIGURATION_DESCRIPTOR;
	musb->desc.config.wTotalLengthL		= CONFIG_DESC_TOTAL_SIZE;
	musb->desc.config.wTotalLengthH		= 0;
	musb->desc.config.bNumInterfaces	= 1;
	musb->desc.config.bConfigurationValue	= 1;
	musb->desc.config.iConfiguration	= 0;
	musb->desc.config.bmAttributes		= CONF_ATTR_DEFAULT | CONF_ATTR_SELFPOWERED;
	musb->desc.config.maxPower		= 50;

	/* Standard interface descriptor */
	musb->desc.intf.bLength			= INTERFACE_DESC_SIZE;
	musb->desc.intf.bDescriptorType		= INTERFACE_DESCRIPTOR;
	musb->desc.intf.bInterfaceNumber	= 0x0;
	musb->desc.intf.bAlternateSetting	= 0x0;
	musb->desc.intf.bNumEndpoints		= 2;	/* # of endpoints except EP0 */
	musb->desc.intf.bInterfaceClass		= 0xff;
	musb->desc.intf.bInterfaceSubClass	= 0xff;
	musb->desc.intf.bInterfaceProtocol	= 0xff;
	musb->desc.intf.iInterface		= 0x0;

	/* Standard endpoint1 descriptor */
	musb->desc.ep1.bLength			= ENDPOINT_DESC_SIZE;
	musb->desc.ep1.bDescriptorType		= ENDPOINT_DESCRIPTOR;
	musb->desc.ep1.bEndpointAddress		= BULK_IN_EP | EP_ADDR_IN;
	musb->desc.ep1.bmAttributes		= EP_ATTR_BULK;
	musb->desc.ep1.wMaxPacketSizeL		= (u8) musb->ep_max_pkt[1];
	musb->desc.ep1.wMaxPacketSizeH		= (u8) (musb->ep_max_pkt[1] >> 8);
	musb->desc.ep1.bInterval		= 0x0;

	/* Standard endpoint2 descriptor */
	musb->desc.ep2.bLength			= ENDPOINT_DESC_SIZE;
	musb->desc.ep2.bDescriptorType		= ENDPOINT_DESCRIPTOR;
	musb->desc.ep2.bEndpointAddress		= BULK_OUT_EP | EP_ADDR_OUT;
	musb->desc.ep2.bmAttributes		= EP_ATTR_BULK;
	musb->desc.ep2.wMaxPacketSizeL		= (u8) musb->ep_max_pkt[2];
	musb->desc.ep2.wMaxPacketSizeH		= (u8) (musb->ep_max_pkt[2] >> 8);
	musb->desc.ep2.bInterval		= 0x0;
}
#else
/* Swallowtail */
void musb_set_desc(struct musb *musb)
{
	/* Standard device descriptor */
	musb->desc.dev.bLength		= DEVICE_DESC_SIZE;
	musb->desc.dev.bDescriptorType	= DEVICE_DESCRIPTOR;
	musb->desc.dev.bcdUSBL		= 0x00;
	musb->desc.dev.bcdUSBH		= 0x02;
	musb->desc.dev.bDeviceClass	= 0x02;
	musb->desc.dev.bDeviceSubClass	= 0x02;
	musb->desc.dev.bDeviceProtocol	= 0x0;
	musb->desc.dev.bMaxPacketSize0	= musb->ep_max_pkt[0];
	musb->desc.dev.idVendorL	= 0xE8;
	musb->desc.dev.idVendorH	= 0x04;
#ifdef CONFIG_USB_DEVGURU
	musb->desc.dev.idProductL	= 0x60;
	musb->desc.dev.idProductH	= 0x68;
#else
	musb->desc.dev.idProductL	= 0x01;
	musb->desc.dev.idProductH	= 0x66;
#endif
	musb->desc.dev.iManufacturer	= 0x1;
	musb->desc.dev.iProduct		= 0x2;
	musb->desc.dev.iSerialNumber	= 0x3;
	musb->desc.dev.bNumConfigs	= 0x1;

	/* Standard configuration descriptor */
	musb->desc.config.bLength		= CONFIG_DESC_SIZE;
	musb->desc.config.bDescriptorType	= CONFIGURATION_DESCRIPTOR;
	musb->desc.config.wTotalLengthL		= CONFIG_DESC_TOTAL_SIZE;
	musb->desc.config.wTotalLengthH		= 0;
	musb->desc.config.bNumInterfaces	= 2;
	musb->desc.config.bConfigurationValue	= 2;
	musb->desc.config.iConfiguration	= 0x05;
	musb->desc.config.bmAttributes		= CONF_ATTR_DEFAULT | CONF_ATTR_SELFPOWERED;
	musb->desc.config.maxPower		= 0x01;

	musb->desc.intf.bLength			= INTERFACE_DESC_SIZE;
	musb->desc.intf.bDescriptorType		= INTERFACE_DESCRIPTOR;
	musb->desc.intf.bInterfaceNumber	= 0x0;
	musb->desc.intf.bAlternateSetting	= 0x0;
	musb->desc.intf.bNumEndpoints		= 0x01;	/* # of endpoints except EP0 */
	musb->desc.intf.bInterfaceClass		= 0x02;
	musb->desc.intf.bInterfaceSubClass	= 0x02;
	musb->desc.intf.bInterfaceProtocol	= 0x01;
	musb->desc.intf.iInterface		= 0x6;

	musb->desc.cdc_header.bLength			= CDC_HEADER_DESC_SIZE;
	musb->desc.cdc_header.bDescriptorType		= 0x24;
	musb->desc.cdc_header.bDescriptorSubType	= 0;
	musb->desc.cdc_header.bcdCDC			= 0x0110;

	musb->desc.cdc_call_management.bLength		= CDC_CALL_MANAGEMENT_DESC_SIZE;
	musb->desc.cdc_call_management.bDescriptorType		= 0x24;
	musb->desc.cdc_call_management.bDescriptorSubType	= 0x01;
	musb->desc.cdc_call_management.bmCapabilities		= 0x00;
	musb->desc.cdc_call_management.bDataInterface		= 0x01;

	musb->desc.cdc_acm.bLength			= CDC_ACM_DESC_SIZE;
	musb->desc.cdc_acm.bDescriptorType		= 0x24;
	musb->desc.cdc_acm.bDescriptorSubType		= 0x02;
	musb->desc.cdc_acm.bmCapabilities		= 0x0F;

	musb->desc.cdc_union.bLength			= CDC_UNION_DESC_SIZE;
	musb->desc.cdc_union.bDescriptorType		= 0x24;
	musb->desc.cdc_union.bDescriptorSubType		= 0x06;
	musb->desc.cdc_union.bMasterInterface0		= 0x0;
	musb->desc.cdc_union.bSlaveInterface0		= 0x1;

	musb->desc.ep3.bLength			= ENDPOINT_DESC_SIZE;
	musb->desc.ep3.bDescriptorType		= ENDPOINT_DESCRIPTOR;
	musb->desc.ep3.bEndpointAddress		= INTR_IN_EP | EP_ADDR_IN;
	musb->desc.ep3.bmAttributes		= EP_ATTR_INTERRUPT;
	musb->desc.ep3.wMaxPacketSizeL		= (u8) musb->ep_max_pkt[3];
	musb->desc.ep3.wMaxPacketSizeH		= 0;
	musb->desc.ep3.bInterval		= 0x09;

	/* Standard interface descriptor */
	musb->desc.intf2.bLength		= INTERFACE_DESC_SIZE;
	musb->desc.intf2.bDescriptorType	= INTERFACE_DESCRIPTOR;
	musb->desc.intf2.bInterfaceNumber	= 0x01;
	musb->desc.intf2.bAlternateSetting	= 0x0;
	musb->desc.intf2.bNumEndpoints		= 2;	/* # of endpoints except EP0 */
	musb->desc.intf2.bInterfaceClass	= 0x0A;
	musb->desc.intf2.bInterfaceSubClass	= 0x00;
	musb->desc.intf2.bInterfaceProtocol	= 0x00;
	musb->desc.intf2.iInterface		= 0x07;

	/* Standard endpoint1 descriptor */
	musb->desc.ep1.bLength			= ENDPOINT_DESC_SIZE;
	musb->desc.ep1.bDescriptorType		= ENDPOINT_DESCRIPTOR;
	musb->desc.ep1.bEndpointAddress		= BULK_IN_EP | EP_ADDR_IN;
	musb->desc.ep1.bmAttributes		= EP_ATTR_BULK;
	musb->desc.ep1.wMaxPacketSizeL		= (u8) musb->ep_max_pkt[1];
	musb->desc.ep1.wMaxPacketSizeH		= (u8) (musb->ep_max_pkt[1] >> 8);
	musb->desc.ep1.bInterval		= 0x0;

	/* Standard endpoint2 descriptor */
	musb->desc.ep2.bLength			= ENDPOINT_DESC_SIZE;
	musb->desc.ep2.bDescriptorType		= ENDPOINT_DESCRIPTOR;
	musb->desc.ep2.bEndpointAddress		= BULK_OUT_EP | EP_ADDR_OUT;
	musb->desc.ep2.bmAttributes		= EP_ATTR_BULK;
	musb->desc.ep2.wMaxPacketSizeL		= (u8) musb->ep_max_pkt[2];
	musb->desc.ep2.wMaxPacketSizeH		= (u8) (musb->ep_max_pkt[2] >> 8);
	musb->desc.ep2.bInterval		= 0x0;

	usb_strncpy(&usb_string_desc[1], str_desc1, strlen(str_desc1));
	usb_strncpy(&usb_string_desc[2], str_desc2, strlen(str_desc2));
	usb_strncpy(&usb_string_desc[3], str_desc3, strlen(str_desc3));
	usb_strncpy(&usb_string_desc[4], str_desc4, strlen(str_desc4));
	usb_strncpy(&usb_string_desc[5], str_desc5, strlen(str_desc5));
	usb_strncpy(&usb_string_desc[6], str_desc6, strlen(str_desc6));
	usb_strncpy(&usb_string_desc[7], str_desc7, strlen(str_desc7));
}
#endif
#define ALIGNUP(addr, bytes) ((((u32)addr)+(bytes)-1)&(~((bytes)-1)))
void musb_get_desc(struct musb *musb)
{
	musb->req_len = (u32) ((musb->dev_req.wLength_H << 8)
				| musb->dev_req.wLength_L);

	switch (musb->dev_req.wValue_H) {
	case DEVICE_DESCRIPTOR:
		debug("DEVICE_DESCRIPTOR = 0x%x\n", musb->req_len);
		debug("	size=%d, class=%d\n",
				DEVICE_DESC_SIZE, musb->desc.dev.bDeviceClass);

		musb->req_len = DEVICE_DESC_SIZE;
		musb->req_data = (u8 *)(&musb->desc.dev);	
		break;

	case CONFIGURATION_DESCRIPTOR:
		debug("CONFIGURATION_DESCRIPTOR = 0x%x\n", musb->req_len);
		debug("	size(all)=%d, size(config)=%d\n",
				CONFIG_DESC_TOTAL_SIZE, CONFIG_DESC_SIZE);

		if (musb->req_len > CONFIG_DESC_SIZE)
			musb->req_len = CONFIG_DESC_TOTAL_SIZE;
		else
			musb->req_len = CONFIG_DESC_SIZE;

		musb->req_data = (u8 *)(&musb->desc.config);
		break;

	case STRING_DESCRIPTOR:
		debug("STRING_DESCRIPTOR\n");

#ifdef CDC_ACM
		int i = musb->dev_req.wValue_L;

		if (i) {
			musb->req_len =  usb_string_desc[i].bLength;
			musb->req_data = (u8 *)&usb_string_desc[i];
		} else {
			musb->req_len = STRING_DESC0_SIZE;
			musb->req_data = (u8 *)string_desc0;
		}
		break;
#else
		switch (musb->dev_req.wValue_L) {
		case 0:
			musb->req_len = STRING_DESC0_SIZE;
			musb->req_data = (u8 *)string_desc0;
			break;
		case 1:
			musb->req_len = STRING_DESC1_SIZE;
			musb->req_data = (u8 *)string_desc1;
			break;
		case 2:
			musb->req_len = STRING_DESC2_SIZE;
			musb->req_data = (u8 *)string_desc2;
			break;
		default:
			break;
		}
		break;
#endif

	case ENDPOINT_DESCRIPTOR:
		debug("ENDPOINT_DESCRIPTOR\n");
		switch (musb->dev_req.wValue_L & 0xf) {
		case 0:
			musb->req_len = ENDPOINT_DESC_SIZE;
			musb->req_data = (u8 *)(&musb->desc.ep1);
			break;
		case 1:
			musb->req_len = ENDPOINT_DESC_SIZE;
			musb->req_data = (u8 *)(&musb->desc.ep2);
			break;
		default:
			break;
		}
		break;

	case DEVICE_QUALIFIER:
		debug("DEVICE_QUALIFIER = 0x%x\n", musb->req_len);
		/* not yet */
		break;

	case OTHER_SPEED_CONFIGURATION:
		debug("OTHER_SPEED_CONFIGURATION\n");
		/* not yet */
		break;
	}
	musb_ep0_txstate(musb);
}

/*
 * Read a SETUP packet from the hardware.
 * Fields are left in USB byte-order.
 */
void musb_read_setup(struct musb *musb)
{
	u32 buf[2] = {0x0000,};
	u16 offset=0;

	buf[0] = MUSB_READL(MUSBREG_FIFO_OFFSET(0));
	buf[1] = MUSB_READL(MUSBREG_FIFO_OFFSET(0));

	musb->dev_req.bmRequestType	= buf[0];
	musb->dev_req.bRequest		= buf[0] >> 8;
	musb->dev_req.wValue_L		= buf[0] >> 16;
	musb->dev_req.wValue_H		= buf[0] >> 24;
	musb->dev_req.wIndex_L		= buf[1];
	musb->dev_req.wIndex_H		= buf[1] >> 8;
	musb->dev_req.wLength_L		= buf[1] >> 16;
	musb->dev_req.wLength_H		= buf[1] >> 24;

#ifdef MUSB_DEBUG
	musb_print_pkt((u8 *) &musb->dev_req, 8);
#endif

	offset = MUSBREG_EP_OFFSET(0, 0);

	musb->set_address = 0;

	if (musb->dev_req.wLength_L == 0) {
		debug("ZERO_DATA\n");
	} else {
		if (musb->dev_req.bmRequestType & USB_DIR_IN)
			debug("USB_DIR_IN\n");
		musb_csr0(offset + MUSBREG_CSR, CSR0_SVDRXPKTRDY);
	}
}

void musb_setup_class(struct musb *musb, u16 len)
{
	u16 offset = MUSBREG_EP_OFFSET(0, 0);

	switch (musb->dev_req.bRequest) {
	case 0x20:	/* SET_LINE_CODING */
		tags("set_line_coding");

		/* musb_read_fifo(&buf, len, 0); */
		udelay(10*1000);

		musb_csr0(offset + MUSBREG_CSR,
			CSR0_SVDRXPKTRDY | CSR0_DATAEND);
		break;

	case 0x22:	/* SET_CONTROL_LINE_STATE */
		tags("set_control_line_state");
		musb_csr0(offset + MUSBREG_CSR,
			CSR0_SVDRXPKTRDY | CSR0_DATAEND);
		break;
	}
}

void musb_setup_req(struct musb *musb)
{
	u16 offset = MUSBREG_EP_OFFSET(0, 0);

	switch (musb->dev_req.bRequest) {
	case STANDARD_SET_ADDRESS:
		/* Set Address Update bit */
		musb->set_address = 1;
		musb->address = (musb->dev_req.wValue_L);

		debug("STANDARD_SET_ADDRESS : %d\n", musb->address);

		musb_csr0(offset + MUSBREG_CSR,
				CSR0_SVDRXPKTRDY | CSR0_DATAEND);
		break;

	case STANDARD_SET_DESCRIPTOR:
		debug("STANDARD_SET_DESCRIPTOR\n");
		break;

	case STANDARD_SET_CONFIGURATION:
		tags("set_configuration");
		debug("STANDARD_SET_CONFIGURATION\n");
		/* Configuration value in configuration descriptor */
		musb->config = musb->dev_req.wValue_L;

		/* enumeration is completed */
		musb_ep_enable(musb, 1, USB_DIR_IN);
		musb_ep_enable(musb, 2, USB_DIR_OUT);

		musb_connected = 1;
		musb_csr0(offset + MUSBREG_CSR,
				CSR0_SVDRXPKTRDY | CSR0_DATAEND);
		break;

	case STANDARD_GET_CONFIGURATION:
		debug("STANDARD_GET_CONFIGURATION\n");
		break;

	case STANDARD_GET_DESCRIPTOR:
		debug("STANDARD_GET_DESCRIPTOR\n");
		musb_get_desc(musb);
		break;

	case STANDARD_CLEAR_FEATURE:
		debug("STANDARD_CLEAR_FEATURE\n");

		musb_csr0(offset + MUSBREG_CSR,
				CSR0_SVDRXPKTRDY | CSR0_DATAEND);
		/* not yet */
		break;

	case STANDARD_SET_FEATURE:
		debug("STANDARD_SET_FEATURE\n");
		/* not yet */
		break;

	case STANDARD_GET_STATUS:
		debug("STANDARD_GET_STATUS\n");
		/* not yet */
		break;

	case STANDARD_GET_INTERFACE:
		debug("STANDARD_GET_INTERFACE\n");
		/* not yet */
		break;

	case STANDARD_SET_INTERFACE:
		debug("STANDARD_SET_INTERFACE\n");
		musb_csr0(offset + MUSBREG_CSR,
				CSR0_SVDRXPKTRDY | CSR0_DATAEND);
		/* not yet */
		break;

	case STANDARD_SYNCH_FRAME:
		debug("STANDARD_SYNCH_FRAME\n");
		/* not yet */
		break;

	default:
		break;
	}
}

void musb_ep0_rxstate(struct musb *musb)
{
	u16 tmp = 0;
	u16 offset;

	offset = MUSBREG_EP_OFFSET(0, 0);

	musb->ep0_state = MUSB_EP0_STAGE_TX1;
	tmp |= CSR0_DATAEND;

	musb_csr0(offset + MUSBREG_CSR, tmp);
}

void musb_ep0_txstate(struct musb *musb)
{
	u16 csr = CSR0_TXPKTRDY;
	u16 offset = MUSBREG_EP_OFFSET(0, 0);
	u32 len = min(musb->req_len, 64);

	/* load the data */
	musb_write_fifo(musb->req_data, len, 0);

	musb->req_len -= len;
	musb->req_data += len;

	if (!musb->req_len) {
		musb->ep0_state = MUSB_EP0_STAGE_IDLE;
		csr |= CSR0_DATAEND;
	} else
		musb->ep0_state = MUSB_EP0_STAGE_TX2;

	musb_csr0(offset + MUSBREG_CSR, csr);
}

void musb_rx(struct musb *musb, int ep)
{
	u16 offset;
	u16 fifo;
	u16 csr;
	u16 len;

	offset = MUSBREG_EP_OFFSET(ep, 0);
	fifo = MUSBREG_FIFO_OFFSET(ep);

	csr = MUSB_READW(offset + MUSBREG_RXCSR);

	if (csr & RXCSR_SENTSTALL) {
		debug("RX: SENT STALL\n");
		csr |= RXCSR_CLEAR;
		csr &= ~RXCSR_SENTSTALL;
		MUSB_WRITEW(offset + MUSBREG_RXCSR, csr);
		return;
	}

	if (csr & RXCSR_OVERRUN) {
		debug("RX: OVERRUN\n");
		csr &= ~RXCSR_OVERRUN;
		MUSB_WRITEW(offset + MUSBREG_RXCSR, csr);
	}

	if (csr & RXCSR_INCOMPRX)
		debug("RX: INCOMPRX\n");

	/* rx state */
	if (csr & RXCSR_RXPKTRDY) {
		debug("RX: RXCSR_RXPKTRDY\n");
		len = MUSB_READW(offset + MUSBREG_COUNT);
		musb_read_fifo(musb->recv_ptr, len, ep);
		musb->recv_ptr += len;

		if ((u32)musb->recv_ptr - musb->recv_addr >= musb->recv_len) {
			debug("RX Complete: addr = 0x%08x, size = 0x%08x\n",
					musb->recv_addr, musb->recv_len);
			musb_recv_done = 1;
		}

		csr |= RXCSR_CLEAR;
		csr &= ~RXCSR_RXPKTRDY;

		MUSB_WRITEW(offset + MUSBREG_RXCSR, csr);
	}
}

int musb_tx(struct musb *musb, int ep)
{
	u16 offset;
	u16 fifo;
	u16 csr;
	int ret=0;

	offset = MUSBREG_EP_OFFSET(ep, 0);
	fifo = MUSBREG_FIFO_OFFSET(ep);

	csr = MUSB_READW(offset + MUSBREG_CSR);

	if (csr & TXCSR_SENTSTALL) {
		csr |= TXCSR_CLEAR;
		csr &= ~TXCSR_SENTSTALL;
		MUSB_WRITEW(offset + MUSBREG_CSR, csr);
		return -1;
	}

	if (csr & TXCSR_UNDERRUN) {
		csr |= TXCSR_CLEAR;
		csr &= ~(TXCSR_UNDERRUN	| TXCSR_TXPKTRDY);
		MUSB_WRITEW(offset + MUSBREG_CSR, csr);
	}

	if (musb->send_flag) {
		debug("TX: send flag txcsr 0x%x\n", csr);
		musb->send_flag = 0;
		return;
	}

	if (csr & TXCSR_FIFONOTEMPTY) {
		csr = MUSB_READW(offset + MUSBREG_CSR);
	}

	if (csr & TXCSR_TXPKTRDY) {
		debug("TX: old packet still ready , txcsr 0x%x\n", csr);
		return -1;
	}

	if (csr & TXCSR_SENDSTALL) {
		debug("TX: stalling, txcsr %03x\n", csr);
		return -1;
	}
	musb_write_fifo(musb->send_data, musb->send_len, ep);

	csr |= TXCSR_TXPKTRDY;
	csr &= ~TXCSR_UNDERRUN;
	MUSB_WRITEW(offset + MUSBREG_CSR, csr);

	debug("TX: len = %d\n", musb->send_len);

	ret = musb->send_len;

	musb->send_flag = 1;
	musb->send_data = NULL;
	musb->send_len = 0;

	return ret;

}

#define STAGE0_MASK (INTRUSB_RESUME | INTRUSB_SESSREQ \
		| INTRUSB_VBUSERROR | INTRUSB_CONNECT \
		| INTRUSB_RESET)

void musb_stage0_irq(struct musb *musb, u8 int_usb, u8 devctl)
{
	if (int_usb & INTRUSB_RESUME) {
		debug("RESUME\n");

		switch (musb->xceiv_state) {
		case OTG_STATE_B_WAIT_ACON:
		case OTG_STATE_B_PERIPHERAL:
			if ((devctl & DEVCTL_VBUS) != (3 << DEVCTL_VBUS_SHIFT)) {
				musb->int_usb |= INTRUSB_DISCONNECT;
				musb->int_usb &= ~INTRUSB_SUSPEND;
				break;
			}
			musb_resume(musb);
			break;
		case OTG_STATE_B_IDLE:
			musb->int_usb &= ~INTRUSB_SUSPEND;
			break;
		default:
			break;
		}
	}

	if (int_usb & INTRUSB_RESET) {
		debug("RESET\n");

		switch (musb->xceiv_state) {
		case OTG_STATE_B_IDLE:
			musb->xceiv_state = OTG_STATE_B_PERIPHERAL;
			/* FALLTHROUGH */
		case OTG_STATE_B_PERIPHERAL:
			musb_reset(musb);
			break;
		default:
			break;
		}
	}
}

void musb_stage2_irq(struct musb *musb, u8 int_usb, u8 devctl)
{
	if (int_usb & INTRUSB_DISCONNECT) {
		debug("DISCONNECT\n");

		switch (musb->xceiv_state) {
		case OTG_STATE_B_PERIPHERAL:
		case OTG_STATE_B_IDLE:
			musb->xceiv_state = OTG_STATE_B_IDLE;
			break;
		default:
			break;
		}
	}

	if (int_usb & INTRUSB_SUSPEND) {
		debug("SUSPEND devctl %02x\n", devctl);

		switch (musb->xceiv_state) {
		case OTG_STATE_B_PERIPHERAL:
		case OTG_STATE_B_IDLE:
			musb_suspend(musb);
			musb->is_active = 0;
			break;
		default:
			/* "should not happen" */
			musb->is_active = 0;
			break;
		}
	}
}

/*
 * Handle peripheral ep0 interrupt
 */
void musb_ep0_irq(struct musb *musb)
{
	u16	csr=0;
	u16	len=0;
	u16	offset=0;
	u16	addr=0;

	tags("ep0_irq");

	MUSB_WRITEB(MUSBREG_INDEX, 0);
	offset = MUSBREG_EP_OFFSET(0, 0);

	csr = MUSB_READW(offset + MUSBREG_CSR);
	len = MUSB_READB(offset + MUSBREG_COUNT);
	addr = MUSB_READB(MUSBREG_FADDR);

	debug("csr %04x, count %d, myaddr %d, state %d\n",
			csr, len, addr, musb->ep0_state);

	/* I sent a stall.. need to acknowledge it now.. */
	if (csr & CSR0_SENTSTALL) {
		debug("CSR0: SENT_STALL\n");
		musb_csr0(offset + MUSBREG_CSR,	csr & ~CSR0_SENTSTALL);
		musb->ep0_state = MUSB_EP0_STAGE_IDLE;
		csr = MUSB_READW(offset + MUSBREG_CSR);
	}

	/* request ended "early" */
	if (csr & CSR0_SETUPEND) {
		debug("CSR0: SETUP_END\n");
		musb_csr0(offset + MUSBREG_CSR, csr | CSR0_SVDSETUPEND);
		musb->ep0_state = MUSB_EP0_STAGE_IDLE;
		csr = MUSB_READW(offset + MUSBREG_CSR);
		/* NOTE:  request may need completion */
	}

	if (musb->set_address) {
		musb->set_address = 0;
		MUSB_WRITEB(MUSBREG_FADDR, musb->address);
		return;
	}

	if (musb->ep0_state == MUSB_EP0_STAGE_TX2) {
		if ((csr & CSR0_TXPKTRDY) == 0)
			musb_ep0_txstate(musb);
		return;
	}

	if (csr & CSR0_RXPKTRDY) {
		u8 req_type = 0;

		if (len != 8) {
			debug("SETUP packet len %d != 8 ?\n", len);
			return;
		}

		musb_read_setup(musb);

		/* ch9 9.3 type */
		req_type = (musb->dev_req.bmRequestType >> 5) & 3;
		debug("request type : %d\n", req_type);

		if (req_type == 0)	/* Standard request */
			musb_setup_req(musb);
		else if (req_type == 1)	/* class request */
			musb_setup_class(musb, len);
		else
			debug("Unknown Request\n");
	}
}

void musb_interrupt(struct musb *musb)
{
	u8 devctl, power;
	int ep_num;
	u32 reg;

	musb->int_usb	= MUSB_READB(MUSBREG_INTRUSB);
	musb->int_tx	= MUSB_READW(MUSBREG_INTRTX);
	musb->int_rx	= MUSB_READW(MUSBREG_INTRRX);

	if (musb->int_usb || musb->int_tx || musb->int_rx) {
		if (musb->int_usb != 0x8 && musb->int_tx != 0 && musb->int_rx != 0)
			debug("intr - usb %04x, tx %04x, rx %04x\n",
				musb->int_usb, musb->int_tx, musb->int_rx);

		devctl = MUSB_READB(MUSBREG_DEVCTL);
		/* power = MUSB_READB(MUSBREG_POWER); */
		/* the core can interrupt us for multiple reasons; docs have
		 * a generic interrupt flowchart to follow
		 */
		if (musb->int_usb & STAGE0_MASK)
			musb_stage0_irq(musb, musb->int_usb, devctl);

		/* "stage 1" is handling endpoint irqs */

		/* handle endpoint 0 first */
		if (musb->int_tx & 1)
			musb_ep0_irq(musb);

		/* RX on endpoints 1-15 */
		reg = musb->int_rx >> 1;
		ep_num = 1;
		while (reg) {
			if (reg & 1)
				musb_rx(musb, ep_num);

			reg >>= 1;
			ep_num++;
		}

		/* TX on endpoints 1-15 */
		reg = musb->int_tx >> 1;
		ep_num = 1;
		while (reg) {
			if (reg & 1)
				musb_tx(musb, ep_num);

			reg >>= 1;
			ep_num++;
		}

		/* finish handling "global" interrupts after handling fifos */
		if (musb->int_usb)
			musb_stage2_irq(musb, musb->int_usb, devctl);
	}

}

/* OMAP3 USB entry point
 * init twl4030, OMAP3 core, MUSBHDRC
 */
void musb_init(void)
{
	printf("musb_init : OMAP4 :  High-Speed USB OTG Controller\n");
	musb.board_mode = MUSB_PERIPHERAL;
	musb.is_host = 0;
	musb.set_address = 0;
	musb.xceiv_state = OTG_STATE_B_IDLE;

	/* init OMAP Controller */
	musb_omap_init(&musb);
	debug("musb_init : OMAP4 :  musb_omap_init - Done!\n");

	/* be sure interrupts are disabled before connecting ISR */
	musb_generic_disable();
	debug("musb_init : OMAP4 :  musb_generic_disable - Done!\n");

	/* setup musb parts of the core (especially endpoints) */
	musb_core_init(&musb);
	debug("musb_init : OMAP4 :  musb_core_init - Done!\n");

	/* setup descriptors */
	musb_set_desc(&musb);
	debug("musb_init : OMAP4 :  musb_set_desc - Done!\n");

	/* start musb */
	musb_start(&musb);
	debug("musb_init : OMAP4 :  musb_start - Done!\n");
}
