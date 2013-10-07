/*
 * (C) Copyright 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#ifndef __ASM_ARCH_USB_HS_OTG_H_
#define __ASM_ARCH_USB_HS_OTG_H_

#include "cpu.h"

/* Core Global Registers */
#define OTG_GOTGCTL		0x000
#define OTG_GOTGINT		0x004
#define OTG_GAHBCFG		0x008
#define OTG_GUSBCFG		0x00C
#define OTG_GRSTCTL		0x010
#define OTG_GINTSTS		0x014
#define OTG_GINTMSK		0x018
#define OTG_GRXSTSR		0x01C
#define OTG_GRXSTSP		0x020
#define OTG_GRXFSIZ		0x024
#define OTG_GNPTXFSIZ		0x028
#define OTG_GNPTXSTS		0x02C

#define OTG_HPTXFSIZ		0x100
#define OTG_DPTXFSIZ1		0x104
#define OTG_DPTXFSIZ2		0x108
#define OTG_DPTXFSIZ3		0x10C
#define OTG_DPTXFSIZ4		0x110
#define OTG_DPTXFSIZ5		0x114
#define OTG_DPTXFSIZ6		0x118
#define OTG_DPTXFSIZ7		0x11C
#define OTG_DPTXFSIZ8		0x120
#define OTG_DPTXFSIZ9		0x124
#define OTG_DPTXFSIZ10		0x128
#define OTG_DPTXFSIZ11		0x12C
#define OTG_DPTXFSIZ12		0x130
#define OTG_DPTXFSIZ13		0x134
#define OTG_DPTXFSIZ14		0x138
#define OTG_DPTXFSIZ15		0x13C

/* Host Global Registers */
#define OTG_HCFG		0x400
#define OTG_HFIR		0x404
#define OTG_HFNUM		0x408
#define OTG_HPTXSTS		0x410
#define OTG_HAINT		0x414
#define OTG_HAINTMSK		0x418

/* Host Port Control & Status Registers */
#define OTG_HPRT		0x440

/* Host Channel-Specific Registers */
#define OTG_HCCHAR0		0x500
#define OTG_HCSPLT0		0x504
#define OTG_HCINT0		0x508
#define OTG_HCINTMSK0		0x50C
#define OTG_HCTSIZ0		0x510
#define OTG_HCDMA0		0x514

/* Device Global Registers */
#define OTG_DCFG		0x800
#define OTG_DCTL		0x804
#define OTG_DSTS		0x808
#define OTG_DIEPMSK		0x810
#define OTG_DOEPMSK		0x814
#define OTG_DAINT		0x818
#define OTG_DAINTMSK		0x81C
#define OTG_DTKNQR1		0x820
#define OTG_DTKNQR2		0x824
#define OTG_DVBUSDIS		0x828
#define OTG_DVBUSPULSE		0x82C
#define OTG_DTKNQR3		0x830
#define OTG_DTKNQR4		0x834

/* Device Logical IN Endpoint-Specific Registers */
#define OTG_DIEPCTL0		0x900
#define OTG_DIEPINT0		0x908
#define OTG_DIEPTSIZ0		0x910
#define OTG_DIEPDMA0		0x914

/* Device Logical OUT Endpoint-Specific Registers */
#define OTG_DOEPCTL0		0xB00
#define OTG_DOEPINT0		0xB08
#define OTG_DOEPTSIZ0		0xB10
#define OTG_DOEPDMA0		0xB14

/* Power & clock gating registers */
#define OTG_PCGCCTRL		0xE00

/* Endpoint FIFO address */
#define OTG_EP0_FIFO		0x1000

/* OTG PHY CORE REGISTERS */
#define OTG_PHYPWR		0x0
#define OTG_PHYCTRL		0x4
#define OTG_RSTCON		0x8
#define OTG_PHYTUN0		0x20
#define OTG_PHYTUN1		0x24

/* register for power control on PMU */
static inline unsigned int s5p_get_usb_power_reg(void)
{
	return EXYNOS4_USBPHY_CONTROL;
}

#define CONTROL_EP		0
#define BULK_IN_EP		1
#define BULK_OUT_EP		2
#define INTR_IN_EP		3

#define FS_CTRL_PKT_SIZE	8
#define FS_BULK_PKT_SIZE	64

#define HS_CTRL_PKT_SIZE	64
#define HS_BULK_PKT_SIZE	512

#define RX_FIFO_SIZE		512
#define NPTX_FIFO_START_ADDR	RX_FIFO_SIZE
#define NPTX_FIFO_SIZE		512
#define PTX_FIFO_SIZE		512

/* string descriptor */
#define LANGID_US_L		(0x09)
#define LANGID_US_H		(0x04)

/* Feature Selectors */
#define EP_STALL		0
#define DEVICE_REMOTE_WAKEUP	1
#define TEST_MODE		2

/* Test Mode Selector*/
#define TEST_J			1
#define TEST_K			2
#define TEST_SE0_NAK		3
#define TEST_PACKET		4
#define TEST_FORCE_ENABLE	5

#define OTG_DIEPCTL_IN		(OTG_DIEPCTL0 + 0x20 * BULK_IN_EP)
#define OTG_DIEPINT_IN		(OTG_DIEPINT0 + 0x20 * BULK_IN_EP)
#define OTG_DIEPTSIZ_IN		(OTG_DIEPTSIZ0 + 0x20 * BULK_IN_EP)
#define OTG_DIEPDMA_IN		(OTG_DIEPDMA0 + 0x20 * BULK_IN_EP)
#define OTG_DOEPCTL_OUT		(OTG_DOEPCTL0 + 0x20 * BULK_OUT_EP)
#define OTG_DOEPINT_OUT		(OTG_DOEPINT0 + 0x20 * BULK_OUT_EP)
#define OTG_DOEPTSIZ_OUT	(OTG_DOEPTSIZ0 + 0x20 * BULK_OUT_EP)
#define OTG_DOEPDMA_OUT		(OTG_DOEPDMA0 + 0x20 * BULK_OUT_EP)
#define OTG_IN_FIFO		(OTG_EP0_FIFO + 0x1000 * BULK_IN_EP)
#define OTG_OUT_FIFO		(OTG_EP0_FIFO + 0x1000 * BULK_OUT_EP)

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

#ifdef CONFIG_S5P_USB_DMA
/* set_dma type definition, ctrl_in/out == ep0,
 * bulk_in == ep1 , bulk_out == ep2 case
 */
enum {
	BULK_RESET_DATA_PID = 0,
	CTRL_IN_TYPE,
	CTRL_OUT_TYPE,
	BULK_IN_TYPE,
	BULK_OUT_TYPE,
};
#endif

enum CONFIG_ATTRIBUTES {
	CONF_ATTR_DEFAULT = 0x80,
	CONF_ATTR_REMOTE_WAKEUP = 0x20,
	CONF_ATTR_SELFPOWERED = 0x40
};

enum ENDPOINT_ATTRIBUTES {
	EP_ADDR_IN = 0x80,
	EP_ADDR_OUT = 0x00,
	EP_ATTR_CONTROL = 0x0,
	EP_ATTR_ISOCHRONOUS = 0x1,
	EP_ATTR_BULK = 0x2,
	EP_ATTR_INTERRUPT = 0x3
};

enum STANDARD_REQUEST_CODE {
	STANDARD_GET_STATUS = 0,
	STANDARD_CLEAR_FEATURE = 1,
	STANDARD_RESERVED_1 = 2,
	STANDARD_SET_FEATURE = 3,
	STANDARD_RESERVED_2 = 4,
	STANDARD_SET_ADDRESS = 5,
	STANDARD_GET_DESCRIPTOR = 6,
	STANDARD_SET_DESCRIPTOR = 7,
	STANDARD_GET_CONFIGURATION = 8,
	STANDARD_SET_CONFIGURATION = 9,
	STANDARD_GET_INTERFACE = 10,
	STANDARD_SET_INTERFACE = 11,
	STANDARD_SYNCH_FRAME = 12,
	STANDARD_RESERVED_3 = 32,
};

enum DEV_REQUEST_RECIPIENT {
	DEVICE_RECIPIENT = 0,
	INTERFACE_RECIPIENT = 1,
	ENDPOINT_RECIPIENT = 2,
	OTHER_RECIPIENT = 3
};

struct XLLP_UDC_USB_CDC_HEADER_DESCRIPTOR_S {
	u8    length;
	u8    descriptor_type;
	u8    descriptor_subtype;
	u16   bcd_cdc;
} __packed;

#define XLLP_UDC_USB_CDC_HEADER_DESCRIPTOR_T \
		struct XLLP_UDC_USB_CDC_HEADER_DESCRIPTOR_S
#define P_XLLP_UDC_USB_CDC_HEADER_DESCRIPTOR_T \
		struct XLLP_UDC_USB_CDC_HEADER_DESCRIPTOR_S*

struct XLLP_UDC_USB_CDC_CALL_MANAGEMENT_DESCRIPTOR_S {
	u8  length;
	u8  descriptor_type;
	u8  descriptor_subtype;
	u8  bm_capabilities;
	u8  data_interface;
} __packed;

#define XLLP_UDC_USB_CDC_CALL_MANAGEMENT_DESCRIPTOR_T \
		struct XLLP_UDC_USB_CDC_CALL_MANAGEMENT_DESCRIPTOR_S
#define P_XLLP_UDC_USB_CDC_CALL_MANAGEMENT_DESCRIPTOR_T \
		ruct XLLP_UDC_USB_CDC_CALL_MANAGEMENT_DESCRIPTOR_S*

struct XLLP_UDC_USB_CDC_ACM_DESCRIPTOR_S {
	u8  length;
	u8  descriptor_type;
	u8  descriptor_subtype;
	u8  bm_capabilities;
} __packed;

#define XLLP_UDC_USB_CDC_ACM_DESCRIPTOR_T \
		struct XLLP_UDC_USB_CDC_ACM_DESCRIPTOR_S
#define P_XLLP_UDC_USB_CDC_ACM_DESCRIPTOR_T \
		struct XLLP_UDC_USB_CDC_ACM_DESCRIPTOR_S*

struct XLLP_UDC_USB_CDC_UNION_DESCRIPTOR_S {
	u8    length;
	u8    descriptor_type;
	u8    descriptor_subtype;
	u8    master_interface0;
	u8    slave_interface0;
} __packed;

#define XLLP_UDC_USB_CDC_UNION_DESCRIPTOR_T \
		struct XLLP_UDC_USB_CDC_UNION_DESCRIPTOR_S
#define P_XLLP_UDC_USB_CDC_UNION_DESCRIPTOR_T \
		struct XLLP_UDC_USB_CDC_UNION_DESCRIPTOR_S*

struct device_desc {
	u8 length;
	u8 descriptor_type;
	u8 bcd_usbl;
	u8 bcd_usbh;
	u8 device_class;
	u8 device_subclass;
	u8 device_protocol;
	u8 max_packetsize0;
	u8 id_vendorl;
	u8 id_vendorh;
	u8 id_productl;
	u8 id_producth;
	u8 bcd_devicel;
	u8 bcd_deviceh;
	u8 i_manufacturer;
	u8 i_product;
	u8 i_serialnumber;
	u8 num_configurations;
} __packed;

#define device_desc_t struct device_desc

struct config_desc {
	u8 length;
	u8 descriptor_type;
	u8 total_lengthl;
	u8 total_lengthh;
	u8 num_interfaces;
	u8 configuration_value;
	u8 configuration;
	u8 bm_attributes;
	u8 max_power;
} __packed;

#define config_desc_t struct config_desc

struct intf_desc {
	u8 length;
	u8 descriptor_type;
	u8 interface_number;
	u8 alternate_setting;
	u8 num_endpoints;
	u8 interface_class;
	u8 interface_subclass;
	u8 interface_protocol;
	u8 interface;
} __packed;

#define intf_desc_t struct intf_desc

struct av_desc {
	u8 length;
	u8 descriptor_type;
	u8 descriptor_subtype;
	u16 dau_type;
	u16 dau_length;
	u8 dau_value;
} __packed;

#define av_desc_t struct av_desc

struct ep_desc {
	u8 length;
	u8 descriptor_type;
	u8 endpoint_address;
	u8 bm_attributes;
	u8 max_packetsizel;
	u8 max_packetsizeh;
	u8 interval;
} __packed;

#define ep_desc_t struct ep_desc

struct device_req {
	u8 bm_request_type;
	u8 request;
	u8 value_l;
	u8 value_h;
	u8 index_l;
	u8 index_h;
	u8 length_l;
	u8 length_h;
} __packed;

#define device_req_t struct device_req

struct descriptors {
	device_desc_t dev;
	config_desc_t config;
	intf_desc_t intf;
	XLLP_UDC_USB_CDC_HEADER_DESCRIPTOR_T cdc_header;
	XLLP_UDC_USB_CDC_CALL_MANAGEMENT_DESCRIPTOR_T cdc_call;
	XLLP_UDC_USB_CDC_ACM_DESCRIPTOR_T cdc_abstract;
	XLLP_UDC_USB_CDC_UNION_DESCRIPTOR_T cdc_union;
	ep_desc_t ep3;
	intf_desc_t intf2;
#ifdef CONFIG_S5P_USB_NON_ZLP
	av_desc_t avd;
#endif
	ep_desc_t ep1;
	ep_desc_t ep2;
} __packed;

#define descriptors_t struct descriptors

struct get_status {
	u8 device;
	u8 interface;
	u8 ep_ctrl;
	u8 ep_in;
	u8 ep_out;
} __packed;

#define get_status_t struct get_status

struct get_intf {
	u8 alternate_setting;
} __packed;

#define get_intf_t struct get_intf

enum e_usb_opmode {
	USB_CPU, USB_DMA
};

#define	USB_OPMODE enum e_usb_opmode

enum e_usb_speed {
	USB_HIGH, USB_FULL, USB_LOW
};

#define USB_SPEED enum e_usb_speed

enum e_ep_type {
	EP_TYPE_CONTROL, EP_TYPE_ISOCHRONOUS, EP_TYPE_BULK, EP_TYPE_INTERRUPT
};

#define EP_TYPE enum e_ep_type

struct otg_dev {
	descriptors_t desc;	/* unaligned */
	device_req_t dev_req;

	u32 ep0_state;
	u32 ep0_substate;
	USB_OPMODE op_mode;
	USB_SPEED speed;
	u32 ctrl_max_pktsize;
	u32 bulkin_max_pktsize;
	u32 bulkout_max_pktsize;
	u32 dn_addr;
	u32 dn_filesize;
	u32 up_addr;
	u32 up_size;
	u8 *dn_ptr;
	u8 *up_ptr;
	u32 set_config;
	u32 req_length;
	u8 ep0_zlp;
};

#define otg_dev_t struct otg_dev

int s5p_usbctl_init(void);
void s5p_udc_int_hndlr(void);
void s5p_usb_tx(char *tx_data, int tx_size);
int s5p_usb_detect_irq(void);
void s5p_usb_clear_irq(void);
void s5p_usb_set_dn_addr(unsigned long addr, unsigned long size);
#ifdef CONFIG_S5P_USB_DMA
void s5p_usb_setdma(void *addr , u32 bytes , uchar type);
void s5p_usb_dma_done(int direction);
#endif

#ifdef CONFIG_CMD_USBDOWN
int usb_board_init(void);
#endif

#endif
