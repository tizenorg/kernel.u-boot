/*
 * Copyright (C) 2008-2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Hyuk Kang <hyuk78.kang@samsung.com>
 */

#ifndef _MUSB_H_
#define _MUSB_H_

#define CDC_ACM

/* Registers */

#define __arch_getb(a)		(*(volatile unsigned char *)(a))
#define __arch_getw(a)		(*(volatile unsigned short *)(a))
#define __arch_getl(a)		(*(volatile unsigned int *)(a))

#define __arch_putb(v, a)	(*(volatile unsigned char *)(a) = (v))
#define __arch_putw(v, a)	(*(volatile unsigned short *)(a) = (v))
#define __arch_putl(v, a)	(*(volatile unsigned int *)(a) = (v))

#define __raw_writeb(v, a)	__arch_putb(v, a)
#define __raw_writew(v, a)	__arch_putw(v, a)
#define __raw_writel(v, a)	__arch_putl(v, a)

#define __raw_readb(a)		__arch_getb(a)
#define __raw_readw(a)		__arch_getw(a)
#define __raw_readl(a)		__arch_getl(a)

#define OMAP_USB_BASE		(OMAP44XX_L4_CORE_BASE + 0xAB000)
#define OMAP_HSOTG(offset)	(*(unsigned int *)(OMAP_USB_BASE + offset + 0x400))

#define OTG_REG_REVISION	OMAP_HSOTG(0x0)
#define OTG_REG_SYSCONFIG	OMAP_HSOTG(0x4)
#define OTG_REG_SYSSTATUS	OMAP_HSOTG(0x8)
#define OTG_REG_INTERFSEL	OMAP_HSOTG(0xc)
#define OTG_REG_SIMENABLE	OMAP_HSOTG(0x10)
#define OTG_REG_FORCESTDBY	OMAP_HSOTG(0x14)

#define OMAP_MUSB(offset)	(OMAP_USB_BASE + offset)
#define MUSB_WRITEW(o, v)	(__raw_writew(v, OMAP_MUSB(o)))
#define MUSB_WRITEB(o, v)	(__raw_writeb(v, OMAP_MUSB(o)))
#define MUSB_WRITEL(o, v)	(__raw_writel(v, OMAP_MUSB(o)))
#define MUSB_READB(o)		(__raw_readb(OMAP_MUSB(o)))
#define MUSB_READW(o)		(__raw_readw(OMAP_MUSB(o)))
#define MUSB_READL(o)		(__raw_readl(OMAP_MUSB(o)))

#define MUSBREG_EP_OFFSET(_epnum, _offset) (0x100 + (0x10*(_epnum)) + (_offset))

/*
 * Common registers
 */
#define MUSBREG_FADDR		0x00	/* 8-bit */
#define MUSBREG_POWER		0x01	/* 8-bit */

#define MUSBREG_INTRTX		0x02	/* 16-bit */
#define MUSBREG_INTRRX		0x04
#define MUSBREG_INTRTXE		0x06
#define MUSBREG_INTRRXE		0x08
#define MUSBREG_INTRUSB		0x0A	/* 8 bit */
#define MUSBREG_INTRUSBE	0x0B	/* 8 bit */
#define MUSBREG_FRAME		0x0C
#define MUSBREG_INDEX		0x0E	/* 8 bit */
#define MUSBREG_TESTMODE	0x0F	/* 8 bit */

/* Get offset for a given FIFO from musb->mregs */
#define MUSBREG_FIFO_OFFSET(epnum)	(0x20 + ((epnum) * 4))

/*
 * Additional Control Registers
 */

#define MUSBREG_DEVCTL		0x60	/* 8 bit */

/* These are always controlled through the INDEX register */
#define MUSBREG_TXFIFOSZ	0x62	/* 8-bit (see masks) */
#define MUSBREG_RXFIFOSZ	0x63	/* 8-bit (see masks) */
#define MUSBREG_TXFIFOADD	0x64	/* 16-bit offset shifted right 3 */
#define MUSBREG_RXFIFOADD	0x66	/* 16-bit offset shifted right 3 */

#define MUSBREG_HWVERS		0x6C	/* 8 bit */

/* Offsets to endpoint registers */
#define MUSBREG_TXMAXP		0x00
#define MUSBREG_CSR		0x02	/* EPx(0x10) + 0x2 */
#define MUSBREG_RXMAXP		0x04
#define MUSBREG_RXCSR		0x06
#define MUSBREG_COUNT		0x08	/* EPx(0x10) + 0x8 */
#define MUSBREG_CONFIGDATA	0x1F

/*
 * MUSB Register bits
 */

/* POWER */
#define POWER_ISOUPDATE		0x80
#define POWER_SOFTCONN		0x40
#define POWER_HSENAB		0x20
#define POWER_HSMODE		0x10
#define POWER_RESET		0x08
#define POWER_RESUME		0x04
#define POWER_SUSPENDM		0x02
#define POWER_ENSUSPEND		0x01

/* INTRUSB */
#define INTRUSB_SUSPEND		0x01
#define INTRUSB_RESUME		0x02
#define INTRUSB_RESET		0x04
#define INTRUSB_BABBLE		0x04
#define INTRUSB_SOF		0x08
#define INTRUSB_CONNECT		0x10
#define INTRUSB_DISCONNECT	0x20
#define INTRUSB_SESSREQ		0x40
#define INTRUSB_VBUSERROR	0x80

/* DEVCTL */
#define DEVCTL_BDEVICE		0x80
#define DEVCTL_FSDEV		0x40
#define DEVCTL_LSDEV		0x20
#define DEVCTL_VBUS		0x18
#define DEVCTL_VBUS_SHIFT	3
#define DEVCTL_HM		0x04
#define DEVCTL_HR		0x02
#define DEVCTL_SESSION		0x01

/* Allocate for double-packet buffering (effectively doubles assigned _SIZE) */
#define FIFOSZ_DPB		0x10
/* Allocation size (8, 16, 32, ... 4096) */
#define FIFOSZ_SIZE		0x0f

/* CSR0 */
#define CSR0_FLUSHFIFO		0x0100
#define CSR0_TXPKTRDY		0x0002
#define CSR0_RXPKTRDY		0x0001

/* CSR0 in Peripheral mode */
#define CSR0_SVDSETUPEND	0x0080
#define CSR0_SVDRXPKTRDY	0x0040
#define CSR0_SENDSTALL		0x0020
#define CSR0_SETUPEND		0x0010
#define CSR0_DATAEND		0x0008
#define CSR0_SENTSTALL		0x0004

/* CONFIGDATA */
#define CONFIGDATA_MPRXE	0x80	/* Auto bulk pkt combining */
#define CONFIGDATA_MPTXE	0x40	/* Auto bulk pkt splitting */
#define CONFIGDATA_BIGENDIAN	0x20
#define CONFIGDATA_HBRXE	0x10	/* HB-ISO for RX */
#define CONFIGDATA_HBTXE	0x08	/* HB-ISO for TX */
#define CONFIGDATA_DYNFIFO	0x04	/* Dynamic FIFO sizing */
#define CONFIGDATA_SOFTCONE	0x02	/* SoftConnect */
#define CONFIGDATA_UTMIDW	0x01	/* Data width 0/1 => 8/16bits */

/* TXCSR in Peripheral */
#define TXCSR_AUTOSET		0x8000
#define TXCSR_MODE		0x2000
#define TXCSR_DMAENAB		0x1000
#define TXCSR_FRCDATATOG	0x0800
#define TXCSR_DMAMODE		0x0400
#define TXCSR_CLRDATATOG	0x0040
#define TXCSR_FLUSHFIFO		0x0008
#define TXCSR_FIFONOTEMPTY	0x0002
#define TXCSR_TXPKTRDY		0x0001
#define TXCSR_ISO		0x4000
#define TXCSR_INCOMPTX		0x0080
#define TXCSR_SENTSTALL		0x0020
#define TXCSR_SENDSTALL		0x0010
#define TXCSR_UNDERRUN		0x0004

#define TXCSR_CLEAR \
	(TXCSR_INCOMPTX | TXCSR_SENTSTALL \
	| TXCSR_UNDERRUN | TXCSR_FIFONOTEMPTY)

/* RXCSR in Peripheral */
#define RXCSR_AUTOCLEAR		0x8000
#define RXCSR_DMAENAB		0x2000
#define RXCSR_DISNYET		0x1000
#define RXCSR_PID_ERR		0x1000
#define RXCSR_DMAMODE		0x0800
#define RXCSR_INCOMPRX		0x0100
#define RXCSR_CLRDATATOG	0x0080
#define RXCSR_FLUSHFIFO		0x0010
#define RXCSR_DATAERROR		0x0008
#define RXCSR_FIFOFULL		0x0002
#define RXCSR_RXPKTRDY		0x0001
#define RXCSR_ISO		0x4000
#define RXCSR_SENTSTALL		0x0040
#define RXCSR_SENDSTALL		0x0020
#define RXCSR_OVERRUN		0x0004

#define RXCSR_CLEAR \
	(RXCSR_SENTSTALL | RXCSR_OVERRUN | RXCSR_RXPKTRDY)

/* Structures */

typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u8 bcdUSBL;
	u8 bcdUSBH;
	u8 bDeviceClass;
	u8 bDeviceSubClass;
	u8 bDeviceProtocol;
	u8 bMaxPacketSize0;
	u8 idVendorL;
	u8 idVendorH;
	u8 idProductL;
	u8 idProductH;
	u8 bcdDeviceL;
	u8 bcdDeviceH;
	u8 iManufacturer;
	u8 iProduct;
	u8 iSerialNumber;
	u8 bNumConfigs;
} __attribute__ ((packed)) device_desc_t;

typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u8 wTotalLengthL;
	u8 wTotalLengthH;
	u8 bNumInterfaces;
	u8 bConfigurationValue;
	u8 iConfiguration;
	u8 bmAttributes;
	u8 maxPower;
} __attribute__ ((packed)) config_desc_t;

typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u8 bInterfaceNumber;
	u8 bAlternateSetting;
	u8 bNumEndpoints;
	u8 bInterfaceClass;
	u8 bInterfaceSubClass;
	u8 bInterfaceProtocol;
	u8 iInterface;
} __attribute__ ((packed)) intf_desc_t;

typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u8 bEndpointAddress;
	u8 bmAttributes;
	u8 wMaxPacketSizeL;
	u8 wMaxPacketSizeH;
	u8 bInterval;
} __attribute__ ((packed)) ep_desc_t;

typedef struct {
	u8 bLength;
	u8 bDescriptorType;
   u16 bString[30];
   	u8 reserved1;
	u8 reserved2;
} __attribute__ ((packed)) string_desc_t;

typedef struct {
	u8 bmRequestType;
	u8 bRequest;
	u8 wValue_L;
	u8 wValue_H;
	u8 wIndex_L;
	u8 wIndex_H;
	u8 wLength_L;
	u8 wLength_H;
} __attribute__ ((packed)) device_req_t;

#ifdef CDC_ACM
typedef struct  {
	u8	bLength;
	u8	bDescriptorType;
	u8	bDescriptorSubType;
	u16	bcdCDC;
} __attribute__ ((packed)) cdc_header_desc_t;

typedef struct  {
	u8  bLength;
	u8  bDescriptorType;
	u8  bDescriptorSubType;
	u8  bmCapabilities;
	u8  bDataInterface;
} __attribute__ ((packed)) cdc_call_management_desc_t;

typedef struct  {
	u8  bLength;
	u8  bDescriptorType;
	u8  bDescriptorSubType;
	u8  bmCapabilities;
} __attribute__ ((packed)) cdc_acm_desc_t;

typedef struct  {
	u8	bLength;
	u8	bDescriptorType;
	u8	bDescriptorSubType;
	u8	bMasterInterface0;
	u8	bSlaveInterface0;
} __attribute__ ((packed)) cdc_union_desc_t;
#endif


#ifndef CDC_ACM
typedef struct {
	device_desc_t dev;
	config_desc_t config;
	intf_desc_t intf;
	ep_desc_t ep1;
	ep_desc_t ep2;
	ep_desc_t ep3;
	ep_desc_t ep4;
} __attribute__ ((packed)) descriptors_t;
#else
typedef struct {
	device_desc_t			dev;
	u8 reserved1;
	u8 reserved2;
	config_desc_t			config;
	intf_desc_t			intf;
	cdc_header_desc_t		cdc_header;
	cdc_call_management_desc_t	cdc_call_management;
	cdc_acm_desc_t			cdc_acm;
	cdc_union_desc_t		cdc_union;
	ep_desc_t			ep3;
	intf_desc_t			intf2;
	ep_desc_t			ep1;
	ep_desc_t			ep2;
	u8 					reserved3;
} __attribute__ ((packed)) descriptors_t;
#endif


typedef struct {
	u8 Device;
	u8 Interface;
	u8 ep_ctrl;
	u8 ep_in;
	u8 ep_out;
} __attribute__ ((packed)) get_status_t;

/* Standard bmRequestType (direction) */
enum DEV_REQUEST_DIRECTION {
	USB_DIR_OUT	= 0x00,
	USB_DIR_IN	= 0x80
};

/* Standard bmRequestType (Type) */
enum DEV_REQUEST_TYPE {
	STANDARD_TYPE = 0x00,
	CLASS_TYPE = 0x20,
	VENDOR_TYPE = 0x40,
	RESERVED_TYPE = 0x60
};

/* Standard bmRequestType (Recipient) */
enum DEV_REQUEST_RECIPIENT {
	DEVICE_RECIPIENT = 0,
	INTERFACE_RECIPIENT = 1,
	ENDPOINT_RECIPIENT = 2,
	OTHER_RECIPIENT = 3
};

/* Descriptor types */
enum DESCRIPTOR_TYPE {
	DEVICE_DESCRIPTOR = 1,
	CONFIGURATION_DESCRIPTOR = 2,
	STRING_DESCRIPTOR = 3,
	INTERFACE_DESCRIPTOR = 4,
	ENDPOINT_DESCRIPTOR = 5,
	DEVICE_QUALIFIER = 6,
	OTHER_SPEED_CONFIGURATION = 7,
	INTERFACE_POWER = 8
};

/* configuration descriptor: bmAttributes */
enum CONFIG_ATTRIBUTES {
	CONF_ATTR_DEFAULT = 0x80,
	CONF_ATTR_REMOTE_WAKEUP = 0x20,
	CONF_ATTR_SELFPOWERED = 0x40
};

/* endpoint descriptor */
enum ENDPOINT_ATTRIBUTES {
	EP_ADDR_IN = 0x80,
	EP_ADDR_OUT = 0x00,
	EP_ATTR_CONTROL = 0x0,
	EP_ATTR_ISOCHRONOUS = 0x1,
	EP_ATTR_BULK = 0x2,
	EP_ATTR_INTERRUPT = 0x3
};

enum FIFO_STYLE {
	FIFO_TX = 1,
	FIFO_RX,
	FIFO_RXTX
};

#ifndef CDC_ACM
	#define CONFIG_DESC_TOTAL_SIZE	\
		(CONFIG_DESC_SIZE+INTERFACE_DESC_SIZE+ENDPOINT_DESC_SIZE*2)
#else
	#define CONFIG_DESC_TOTAL_SIZE	\
		(CONFIG_DESC_SIZE + \
		 INTERFACE_DESC_SIZE*2 + \
		 ENDPOINT_DESC_SIZE*3 + \
		 CDC_HEADER_DESC_SIZE + \
		 CDC_CALL_MANAGEMENT_DESC_SIZE + \
		 CDC_ACM_DESC_SIZE + \
		 CDC_UNION_DESC_SIZE)
#endif

#define CONTROL_EP		0
#define BULK_IN_EP		1
#define BULK_OUT_EP		2
#define INTR_IN_EP      3

enum musb_mode {
	MUSB_UNDEFINED = 0,
	MUSB_HOST,			/* A or Mini-A connector */
	MUSB_PERIPHERAL,	/* B or Mini-B connector */
	MUSB_OTG			/* Mini-AB connector */
};

/* peripheral side ep0 states */
enum musb_ep0_state {
	MUSB_EP0_STAGE_IDLE,		/* idle, waiting for setup */
	MUSB_EP0_STAGE_TX1,			/* IN data */
	MUSB_EP0_STAGE_TX2,			/* (after IN data) */
	MUSB_EP0_STAGE_RX1,			/* OUT data */
	MUSB_EP0_STAGE_RX2,			/* (after OUT data) */
	MUSB_EP0_STAGE_ZERO,		/* after zlp, before statusin */
};// __attribute__ ((packed));

/* OTG defines lots of enumeration states before device reset */
enum usb_otg_state {
	OTG_STATE_UNDEFINED = 0,

	/* single-role peripheral, and dual-role default-b */
	OTG_STATE_B_IDLE,
	OTG_STATE_B_SRP_INIT,
	OTG_STATE_B_PERIPHERAL,

	/* extra dual-role default-b states */
	OTG_STATE_B_WAIT_ACON,
	OTG_STATE_B_HOST,

	/* dual-role default-a */
	OTG_STATE_A_IDLE,
	OTG_STATE_A_WAIT_VRISE,
	OTG_STATE_A_WAIT_BCON,
	OTG_STATE_A_HOST,
	OTG_STATE_A_SUSPEND,
	OTG_STATE_A_PERIPHERAL,
	OTG_STATE_A_WAIT_VFALL,
	OTG_STATE_A_VBUS_ERR,
};

/* Standard bRequest codes */
enum STANDARD_REQUEST_CODE {
	STANDARD_GET_STATUS = 0,
	STANDARD_CLEAR_FEATURE,
	STANDARD_RESERVED_1,
	STANDARD_SET_FEATURE,
	STANDARD_RESERVED_2,
	STANDARD_SET_ADDRESS,
	STANDARD_GET_DESCRIPTOR,
	STANDARD_SET_DESCRIPTOR,
	STANDARD_GET_CONFIGURATION,
	STANDARD_SET_CONFIGURATION,
	STANDARD_GET_INTERFACE,
	STANDARD_SET_INTERFACE,
	STANDARD_SYNCH_FRAME
};

/*
 * struct musb - Driver instance data.
 */
struct musb {
	/* passed down from chip/board specific irq handlers */
	u8		int_usb;
	u16		int_rx;
	u16		int_tx;

	int		xceiv_state;

	unsigned long	ep_max_pkt[16];

	u16		epmask;
	u8		nr_endpoints;
	u8		board_mode;	/* enum musb_mode */

	int		is_host;

	/* active means connected and not suspended */
	unsigned	is_active:1;
	unsigned	is_multipoint:1;

	/* is_suspended means USB B_PERIPHERAL suspend */
	unsigned	is_suspended:1;
	unsigned	set_address:1;
	unsigned	test_mode:1;
	unsigned	softconnect:1;

	u8		address;
	u8		test_mode_nr;
	u16		ackpend;		/* ep0 */
	u16		config;
	enum musb_ep0_state	ep0_state;

	descriptors_t	desc;
	device_req_t	dev_req;
	u8		*req_data;
	u32		req_len;
	u32		recv_len;
	u32		recv_addr;
	u8		*recv_ptr;
	u32		send_len;
	u8		*send_data;
	u32		send_flag;
};

void musb_rx(struct musb *musb, int ep);
int musb_tx(struct musb *musb, int ep);
void musb_interrupt(struct musb *musb);
void musb_init(void);

#endif
