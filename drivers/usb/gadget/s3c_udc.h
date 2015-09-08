/*
 * drivers/usb/gadget/s3c_udc.h
 * Samsung S3C on-chip full/high speed USB device controllers
 * Copyright (C) 2005 for Samsung Electronics 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
#ifndef __S3C_USB_GADGET
#define __S3C_USB_GADGET


#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/list.h>


#define __iomem

/*-------------------------------------------------------------------------*/

#define DMA_BUFFER_SIZE		(4096*4)	/* DMA bounce buffer size, 16K is enough even for mass storage */

// Max packet size
/*
#if defined(CONFIG_USB_GADGET_S3C_FS)
#define EP0_FIFO_SIZE		8
#define EP_FIFO_SIZE		64
#define S3C_MAX_ENDPOINTS	5
#elif defined(CONFIG_USB_GADGET_S3C_HS) || defined(CONFIG_PLAT_S5P64XX) || defined(CONFIG_PLAT_S5PC1XX) \
	|| defined(CONFIG_CPU_S5P6442)
*/
#define EP0_FIFO_SIZE		64
#define EP_FIFO_SIZE		512
#define EP_FIFO_SIZE2		1024
#define S3C_MAX_ENDPOINTS	4 /* ep0-control, ep1in-bulk, ep2out-bulk, ep3in-int */
#define S3C_MAX_HW_ENDPOINTS	16
/*
#else
#define EP0_FIFO_SIZE		64
#define EP_FIFO_SIZE		512
#define EP_FIFO_SIZE2		1024
#define S3C_MAX_ENDPOINTS	16
#endif
*/
#define WAIT_FOR_SETUP          0
#define DATA_STATE_XMIT         1
#define DATA_STATE_NEED_ZLP     2
#define WAIT_FOR_OUT_STATUS     3
#define DATA_STATE_RECV         4
#define WAIT_FOR_COMPLETE	5
#define WAIT_FOR_OUT_COMPLETE	6
#define WAIT_FOR_IN_COMPLETE	7
#define WAIT_FOR_NULL_COMPLETE	8

#define TEST_J_SEL		0x1
#define TEST_K_SEL		0x2
#define TEST_SE0_NAK_SEL	0x3
#define TEST_PACKET_SEL		0x4
#define TEST_FORCE_ENABLE_SEL	0x5

/* ********************************************************************************************* */
/* IO
 */

typedef enum ep_type {
	ep_control, ep_bulk_in, ep_bulk_out, ep_interrupt
} ep_type_t;

struct s3c_ep {
	struct usb_ep ep;
	struct s3c_udc *dev;

	const struct usb_endpoint_descriptor *desc;
	struct list_head queue;
	unsigned long pio_irqs;
	int len;
	void *dma_buf;

	u8 stopped;
	u8 bEndpointAddress;
	u8 bmAttributes;

	ep_type_t ep_type;
	int fifo_num;
#ifdef CONFIG_USB_GADGET_S3C_FS
	u32 csr1;
	u32 csr2;
#endif
};

struct s3c_request {
	struct usb_request req;
	struct list_head queue;
};

struct s3c_udc {
	struct usb_gadget gadget;
	struct usb_gadget_driver *driver;
	//struct device *dev;
//	struct platform_device *dev;
	struct s3c_plat_otg_data *pdata;
	spinlock_t lock;
	void *dma_buf[S3C_MAX_ENDPOINTS+1];
	dma_addr_t dma_addr[S3C_MAX_ENDPOINTS+1];

	int ep0state;
	struct s3c_ep ep[S3C_MAX_ENDPOINTS];

	unsigned char usb_address;

//	struct regulator *regulator_a;
//	struct regulator *regulator_d;

	unsigned req_pending:1, req_std:1, req_config:1;
};

extern struct s3c_udc *the_controller;

#define ep_is_in(EP) 		(((EP)->bEndpointAddress&USB_DIR_IN)==USB_DIR_IN)
#define ep_index(EP) 		((EP)->bEndpointAddress&0xF)
#define ep_maxpacket(EP) 	((EP)->ep.maxpacket)



/*-------------------------------------------------------------------------*/

#define DEBUG
#ifdef DEBUG
#define DBG(stuff...)		printf("udc: " stuff)
#else
#define DBG(stuff...)		do{}while(0)
#endif

#ifdef VERBOSE
#define VDBG(x...)	printf(x)
//VDBG(x...) printf(__FUNCTION__ , x)
#else
#define VDBG(stuff...)	do{}while(0)
#endif

#ifdef PACKET_TRACE
#    define PACKET		VDBG
#else
#    define PACKET(stuff...)	do{}while(0)
#endif

#define ERR(stuff...)		printf("ERR udc: " stuff)
#define WARN(stuff...)		printf("WARNING udc: " stuff)
#define INFO(stuff...)		printf("INFO udc: " stuff)


#endif
