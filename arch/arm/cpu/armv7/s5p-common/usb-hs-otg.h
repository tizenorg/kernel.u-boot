/*
 * (C) Copyright 2007
 * Byungjae Lee, Samsung Erectronics, bjlee@samsung.com.
 *	- only support for S3C6400
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
 */

#ifndef __USBD_HS_OTG_H__
#define __USBD_HS_OTG_H__

#include <asm/io.h>
#include <asm/byteorder.h>
#include <asm/arch/usb-hs-otg.h>

#define make_word_c(w) __constant_cpu_to_le16(w)
#define make_word(w)   __cpu_to_le16(w)

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

#define OTG_DIEPCTL_INTR		(OTG_DIEPCTL0 + 0x20 * INTR_IN_EP)
#define OTG_DIEPINT_INTR		(OTG_DIEPINT0 + 0x20 * INTR_IN_EP)
#define OTG_DIEPTSIZ_INTR		(OTG_DIEPTSIZ0 + 0x20 * INTR_IN_EP)

typedef struct XLLP_UDC_USB_CDC_HEADER_DESCRIPTOR_S {
	u8    bLength;
	u8    bDescriptorType;
	u8    bDescriptorSubType;
	u16   bcdCDC;
} __attribute__ ((packed)) XLLP_UDC_USB_CDC_HEADER_DESCRIPTOR_T, *P_XLLP_UDC_USB_CDC_HEADER_DESCRIPTOR_T;

typedef struct XLLP_UDC_USB_CDC_CALL_MANAGEMENT_DESCRIPTOR_S {
	u8  bLength;
	u8  bDescriptorType;
	u8  bDescriptorSubType;
	u8  bmCapabilities;
	u8  bDataInterface;
} __attribute__ ((packed)) XLLP_UDC_USB_CDC_CALL_MANAGEMENT_DESCRIPTOR_T, *P_XLLP_UDC_USB_CDC_CALL_MANAGEMENT_DESCRIPTOR_T;

typedef struct XLLP_UDC_USB_CDC_ACM_DESCRIPTOR_S {
	u8  bLength;
	u8  bDescriptorType;
	u8  bDescriptorSubType;
	u8  bmCapabilities;
} __attribute__ ((packed)) XLLP_UDC_USB_CDC_ACM_DESCRIPTOR_T, *P_XLLP_UDC_USB_CDC_ACM_DESCRIPTOR_T;

typedef struct XLLP_UDC_USB_CDC_UNION_DESCRIPTOR_S {
	u8    bLength;
	u8    bDescriptorType;
	u8    bDescriptorSubType;
	u8    bMasterInterface0;
	u8    bSlaveInterface0;
} __attribute__ ((packed)) XLLP_UDC_USB_CDC_UNION_DESCRIPTOR_T, *P_XLLP_UDC_USB_CDC_UNION_DESCRIPTOR_T;

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
	u8 bNumConfigurations;
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
	u8 bDescriptorSubType;
	u16 DAUType;
	u16 DAULength;
	u8 DAUValue;
} __attribute__ ((packed)) av_desc_t;

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

typedef struct {
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
} __attribute__ ((packed)) descriptors_t;

typedef struct {
	u8 Device;
	u8 Interface;
	u8 ep_ctrl;
	u8 ep_in;
	u8 ep_out;
	u8 ep_int;
} __attribute__ ((packed)) get_status_t;

typedef struct {
	u8 AlternateSetting;
} __attribute__ ((packed)) get_intf_t;

typedef enum {
	USB_CPU, USB_DMA
} USB_OPMODE;

typedef enum {
	USB_HIGH, USB_FULL, USB_LOW
} USB_SPEED;

typedef enum {
	EP_TYPE_CONTROL, EP_TYPE_ISOCHRONOUS, EP_TYPE_BULK, EP_TYPE_INTERRUPT
} EP_TYPE;

typedef struct {
	descriptors_t desc;
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
} __attribute__ ((packed)) otg_dev_t;

enum DEV_REQUEST_DIRECTION {
	HOST_TO_DEVICE = 0x00,
	DEVICE_TO_HOST = 0x80
};

enum DEV_REQUEST_TYPE {
	STANDARD_TYPE = 0x00,
	CLASS_TYPE = 0x20,
	VENDOR_TYPE = 0x40,
	RESERVED_TYPE = 0x60
};

enum DEV_REQUEST_RECIPIENT {
	DEVICE_RECIPIENT = 0,
	INTERFACE_RECIPIENT = 1,
	ENDPOINT_RECIPIENT = 2,
	OTHER_RECIPIENT = 3
};

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

#ifdef CONFIG_S5P_USB_DMA
/* set_dma type definition , ctrl_in/out == ep0 , bulk_in == ep1 , bulk_out == ep2 case*/
enum {
	BULK_RESET_DATA_PID = 0,
	CTRL_IN_TYPE,
	CTRL_OUT_TYPE,
	BULK_IN_TYPE,
	BULK_OUT_TYPE,
};
#endif

int s5p_usb_status_check(void);
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
#endif