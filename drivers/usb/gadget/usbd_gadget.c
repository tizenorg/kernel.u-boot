/*
 * usbd_gadget.c -- USB Downloader Gadget
 * 
 * This works is based on the original "Gadget Zero" implementation
 *
 * Copyright (C) 2011 Lukasz Majewski
 * All rights reserved.
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
 */

#define USBD_DEBUG
#undef USBD_DEBUG

#include <common.h>
#include <malloc.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "usbd.h"
#include "gadget_chips.h"

/* #include "epautoconf.c" */
/* #include "config.c" */
/* #include "usbstring.c" */

#ifdef USBD_DEBUG
#define DBG(fmt, args...) printf(fmt , ## args)
#else
#define DBG(fmt, args...)
#endif

#define DMA_BUFFER_SIZE		(4096*4)	/* DMA bounce buffer size, 16K is NOT enough for USBDownloader */
#define DRIVER_VERSION		"usbd-gadget 1.0"

static const char shortname[] = "usbd";
static const char longname[] = "SAMSUNG SLP DRIVER";

static const char usbdown[] = "SLP usbdownloader";

/* Store the usb_gadget configuration */
static struct usb_gadget *usbd_gadget;

/* Global variables for control flow */
static int usbd_gadget_configuration_done = 0;

static int usbd_gadget_rxdata = 0;
static int usbd_gadget_txdata = 0;

static const char *EP_IN_NAME;
static const char *EP_OUT_NAME;

#define GFP_ATOMIC 0x20u

#define USB_BUFSIZ 256

struct usbd_dev {
	struct usb_gadget	*gadget;
	struct usb_request	*req;		/* for control responses */

	u8			config;
	/* IN/OUT EP's and correspoinding requests */
	struct usb_ep		*in_ep, *out_ep;
	struct usb_request	*in_req, *out_req;

};

const static unsigned buflen = 512;

/* Samsung's IDs */
#define DRIVER_VENDOR_NUM 0x04E8
#define DRIVER_PRODUCT_NUM 0x1204

#define STRING_MANUFACTURER 25
#define STRING_PRODUCT 2
#define STRING_USBDOWN 1
#define	CONFIG_USBDOWNLOADER 1

static struct usb_device_descriptor device_desc = {
	.bLength =		sizeof device_desc,
	.bDescriptorType =	USB_DT_DEVICE,

	.bcdUSB =		__constant_cpu_to_le16(0x0200),
	.bDeviceClass =		USB_CLASS_VENDOR_SPEC,

	.idVendor =		__constant_cpu_to_le16(DRIVER_VENDOR_NUM),
	.idProduct =		__constant_cpu_to_le16(DRIVER_PRODUCT_NUM),
	.iProduct =		STRING_PRODUCT,
	.bNumConfigurations =	1,
};

static struct usb_config_descriptor usb_downloader_config = {
	.bLength =		sizeof usb_downloader_config,
	.bDescriptorType =	USB_DT_CONFIG,

	/* compute wTotalLength on the fly */
	.bNumInterfaces =	1,
	.bConfigurationValue =	CONFIG_USBDOWNLOADER,
	.iConfiguration =	STRING_USBDOWN,
	.bmAttributes =		USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower =		1,	/* self-powered */
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,
	.bmAttributes =		USB_OTG_SRP,
};

/* one interface in each configuration */
static const struct usb_interface_descriptor usb_downloader_intf = {
	.bLength =		sizeof usb_downloader_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	.iInterface =		STRING_USBDOWN,
};

/* two full speed bulk endpoints; their use is config-dependent */
static struct usb_endpoint_descriptor fs_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static const struct usb_descriptor_header *fs_usb_downloader_function[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	(struct usb_descriptor_header *) &usb_downloader_intf,
	(struct usb_descriptor_header *) &fs_out_desc,
	(struct usb_descriptor_header *) &fs_in_desc,
	NULL,
};

/*
 * usb 2.0 devices need to expose both high speed and full speed
 * descriptors, unless they only run at full speed.
 *
 */

static struct usb_endpoint_descriptor hs_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
};

static struct usb_qualifier_descriptor dev_qualifier = {
	.bLength =		sizeof dev_qualifier,
	.bDescriptorType =	USB_DT_DEVICE_QUALIFIER,

	.bcdUSB =		__constant_cpu_to_le16(0x0200),
	.bDeviceClass =		USB_CLASS_VENDOR_SPEC,

	.bNumConfigurations =	2,
};

static const struct usb_descriptor_header *hs_usb_downloader_function[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	(struct usb_descriptor_header *) &usb_downloader_intf,
	(struct usb_descriptor_header *) &hs_in_desc,
	(struct usb_descriptor_header *) &hs_out_desc,
	NULL,
};

/* maxpacket and other transfer characteristics vary by speed. */
static inline struct usb_endpoint_descriptor *
ep_desc(struct usb_gadget *g, struct usb_endpoint_descriptor *hs,
		struct usb_endpoint_descriptor *fs)
{
	if (gadget_is_dualspeed(g) && g->speed == USB_SPEED_HIGH)
		return hs;
	return fs;
}

static char manufacturer[50];

/* static strings, in UTF-8 */
static struct usb_string strings[] = {
	{ STRING_MANUFACTURER, manufacturer, },
	{ STRING_PRODUCT, longname, },
	{ STRING_USBDOWN, usbdown, },
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings,
};

static int config_buf(struct usb_gadget *gadget,
		u8 *buf, u8 type, unsigned index)
{
	int				len;
	const struct usb_descriptor_header **function;
	int				hs = 0;

	/* two configurations will always be index 0 and index 1 */
	if (index > 1)
		return -EINVAL;

	if (gadget_is_dualspeed(gadget)) {
		hs = (gadget->speed == USB_SPEED_HIGH);
		if (type == USB_DT_OTHER_SPEED_CONFIG)
			hs = !hs;
	}
	if (hs)
		function = hs_usb_downloader_function;
	else
		function = fs_usb_downloader_function;

	/* for now, don't advertise srp-only devices */
	if (!gadget_is_otg(gadget))
		function++;

	len = usb_gadget_config_buf(&usb_downloader_config,
	                            buf, USB_BUFSIZ, function);
	if (len < 0)
		return len;
	((struct usb_config_descriptor *) buf)->bDescriptorType = type;
	return len;
}

static struct usb_request *alloc_ep_req(struct usb_ep *ep, unsigned length)
{
	struct usb_request	*req;

	req = usb_ep_alloc_request(ep, GFP_ATOMIC);
	if (req) {
		req->length = length;
		req->buf = malloc(length);
		if (!req->buf) {
			usb_ep_free_request(ep, req);
			req = NULL;
		}
	}
	return req;
}

static void free_ep_req(struct usb_ep *ep, struct usb_request *req)
{
	free(req->buf);
	usb_ep_free_request(ep, req);
}

static int usbd_gadget_rx_data(void)
{
	int status;
	struct usbd_dev *dev = get_gadget_data(usbd_gadget);

	static int data_to_rx = 0, tmp;

	data_to_rx = dev->out_req->length;
	tmp = data_to_rx;

	do {

		if (data_to_rx > DMA_BUFFER_SIZE) {
			dev->out_req->length = DMA_BUFFER_SIZE;
		} else {
			dev->out_req->length = data_to_rx;
		}

		status = usb_ep_queue(dev->out_ep, dev->out_req, GFP_ATOMIC);
		if (status) {
			printf("kill %s:  resubmit %d bytes --> %d\n",
			       dev->out_ep->name, dev->out_req->length, status);
			usb_ep_set_halt(dev->out_ep);
			/* FIXME recover later ... somehow */
			return -1;
		}

		while(!usbd_gadget_rxdata) {
			usb_gadget_handle_interrupts();
		}

		usbd_gadget_rxdata = 0;

		if (data_to_rx > DMA_BUFFER_SIZE) {
			dev->out_req->buf += DMA_BUFFER_SIZE;
		}

		data_to_rx -= dev->out_req->actual;

	} while (data_to_rx);

	return tmp;
}

static void usbd_gadget_tx_data(char *data, int len)
{
	int status;
	struct usbd_dev *dev = get_gadget_data(usbd_gadget);

	memset(dev->in_req->buf, '\0', len);
	strncpy(dev->in_req->buf, data, sizeof(data));

	dev->in_req->length = sizeof(dev->in_req->buf);

	status = usb_ep_queue(dev->in_ep, dev->in_req, GFP_ATOMIC);
	if (status) {
		printf("kill %s:  resubmit %d bytes --> %d\n",
		       dev->in_ep->name, dev->in_req->length, status);
		usb_ep_set_halt(dev->in_ep);
		/* FIXME recover later ... somehow */
	}
	
	/* Wait until tx interrupt received */
	while(!usbd_gadget_txdata) {
		usb_gadget_handle_interrupts();
	}

	usbd_gadget_txdata = 0;

}

static void usbd_rx_tx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct usbd_dev	*dev = ep->driver_data;
	int		status = req->status;

	DBG("%s: ep_ptr:%p, req_ptr:%p\n", __func__, ep, req);
	switch (status) {

	case 0:				/* normal completion? */
		if (ep == dev->out_ep) {
			usbd_gadget_rxdata = 1;
		} else {
			usbd_gadget_txdata = 1;
		}
		break;

	/* this endpoint is normally active while we're configured */
	case -ECONNABORTED:		/* hardware forced ep reset */
	case -ECONNRESET:		/* request dequeued */
	case -ESHUTDOWN:		/* disconnect from host */
		/* Exeptional situation - print error message */

	case -EOVERFLOW:
		printf("%s: ERROR:%d\n", __func__, status);
	default:
		DBG("%s complete --> %d, %d/%d\n", ep->name,
				status, req->actual, req->length);
	case -EREMOTEIO:		/* short read */
		break;
	}
}

static struct usb_request *usbd_gadget_start_ep(struct usb_ep *ep)
{
	struct usb_request	*req;

	req = alloc_ep_req(ep, buflen);
	DBG("usbd_gadget_start_ep ep:%p req:%p\n", ep, req);

	if (!req)
		return NULL;

	memset(req->buf, 0, req->length);
	req->complete = usbd_rx_tx_complete;

	memset(req->buf, 0x55, req->length);

	return req;
}

static int usbd_config(struct usbd_dev *dev)
{
	int			result = 0;
	struct usb_ep		*ep;
	struct usb_gadget	*gadget = dev->gadget;
	struct usb_request      *req;

	DBG("%s\n", __func__);

	gadget_for_each_ep(ep, gadget) {
		const struct usb_endpoint_descriptor	*d;

		if (strcmp(ep->name, EP_IN_NAME) == 0) {
			puts("EP_IN_NAME\n");
			d = ep_desc(gadget, &hs_in_desc, &fs_in_desc);
			result = usb_ep_enable(ep, d);
			if (result == 0) {
				ep->driver_data = dev;
				req = usbd_gadget_start_ep(ep);
				if (req != NULL) {
					dev->in_ep = ep;
					dev->in_req = req;
					continue;
				}
				usb_ep_disable(ep);
				result = -EIO;
			}

		} else if (strcmp(ep->name, EP_OUT_NAME) == 0) {
			puts("EP_OUT_NAME\n");
			d = ep_desc(gadget, &hs_out_desc, &fs_out_desc);
			result = usb_ep_enable(ep, d);
			if (result == 0) {
				ep->driver_data = dev;
				req = usbd_gadget_start_ep(ep);
				if (req != NULL) {
					dev->out_ep = ep;
					dev->out_req = req;
					continue;
				}
				usb_ep_disable(ep);
				result = -EIO;
			}

		/* ignore any other endpoints */
		} else
			continue;

		/* stop on error */
		printf("can't start %s, result %d\n", ep->name, result);
		break;
	}
	if (result == 0)
		DBG("buflen %d\n", buflen);

	/* caller is responsible for cleanup on error */
	return result;
}


static void usbd_reset_config(struct usbd_dev *dev)
{
	if (dev->config == 0)
		return;

	DBG("%s\n", __func__);

	if (dev->in_ep) {
		usb_ep_disable(dev->in_ep);
		dev->in_ep = NULL;
	}
	if (dev->out_ep) {
		usb_ep_disable(dev->out_ep);
		dev->out_ep = NULL;
	}
	dev->config = 0;
}

static int usbd_set_config(struct usbd_dev *dev, unsigned number)
{
	int			result = 0;
	struct usb_gadget	*gadget = dev->gadget;

	DBG("usbd_set_config num:%d\n", number);

	if (number == dev->config)
		return 0;

	usbd_reset_config(dev);

	switch (number) {
	case CONFIG_USBDOWNLOADER:
		result = usbd_config(dev);
		puts("USBDownloader\n");
		break;
	default:
		result = -EINVAL;
		/* FALL THROUGH */
	case 0:
		return result;
	}

	if (!result && (!dev->in_ep || !dev->out_ep))
		result = -ENODEV;
	if (result)
		usbd_reset_config(dev);
	else {
		char *speed;

		switch (gadget->speed) {
		case USB_SPEED_LOW:	speed = "low"; break;
		case USB_SPEED_FULL:	speed = "full"; break;
		case USB_SPEED_HIGH:	speed = "high"; break;
		default:		speed = "?"; break;
		}

		dev->config = number;
		DBG("%s speed config #%d: %s\n", speed, number,
					usbdown);
	}

	return result;
}

static void usbd_setup_complete(struct usb_ep *ep, struct usb_request *req)
{
	if (req->status || req->actual != req->length)
		DBG("setup complete --> %d, %d/%d\n",
		    req->status, req->actual, req->length);
}

static int
usbd_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	struct usbd_dev		*dev = get_gadget_data(gadget);
	struct usb_request	*req = dev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	DBG("%s\n", __func__);
	req->zero = 0;
	switch (ctrl->bRequest) {

	case USB_REQ_GET_DESCRIPTOR:
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;
		switch (w_value >> 8) {

		case USB_DT_DEVICE:
			value = min(w_length, (u16) sizeof device_desc);
			memcpy(req->buf, &device_desc, value);
			break;
		case USB_DT_DEVICE_QUALIFIER:
			if (!gadget_is_dualspeed(gadget))
				break;
			value = min(w_length, (u16) sizeof dev_qualifier);
			memcpy(req->buf, &dev_qualifier, value);
			break;

		case USB_DT_OTHER_SPEED_CONFIG:
			if (!gadget_is_dualspeed(gadget))
				break;
			// FALLTHROUGH
		case USB_DT_CONFIG:
			value = config_buf(gadget, req->buf,
					w_value >> 8,
					w_value & 0xff);
			if (value >= 0)
				value = min(w_length, (u16) value);
			break;

		case USB_DT_STRING:
			/* wIndex == language code.
			 * this driver only handles one language, you can
			 * add string tables for other languages, using
			 * any UTF-8 characters
			 */
			value = usb_gadget_get_string(&stringtab,
					w_value & 0xff, req->buf);
			if (value >= 0)
				value = min(w_length, (u16) value);
			break;
		}
		break;

	/* currently two configs, two speeds */
	case USB_REQ_SET_CONFIGURATION:
		if (ctrl->bRequestType != 0)
			goto unknown;
		if (gadget->a_hnp_support)
			DBG("HNP available\n");
		else if (gadget->a_alt_hnp_support)
			DBG("HNP needs a different root port\n");
		else
			DBG("HNP inactive\n");

		value = usbd_set_config(dev, w_value);

		/* Initial setup is done */
		usbd_gadget_configuration_done = 1;
		
		break;
	case USB_REQ_GET_CONFIGURATION:
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;
		*(u8 *)req->buf = dev->config;
		value = min(w_length, (u16) 1);
		break;

	case USB_REQ_SET_INTERFACE:
		if (ctrl->bRequestType != USB_RECIP_INTERFACE)
			goto unknown;

		if (dev->config && w_index == 0 && w_value == 0) {
			u8		config = dev->config;

			usbd_reset_config(dev);
			usbd_set_config(dev, config);
			value = 0;
		}

		break;
	case USB_REQ_GET_INTERFACE:
		if (ctrl->bRequestType != (USB_DIR_IN|USB_RECIP_INTERFACE))
			goto unknown;
		if (!dev->config)
			break;
		if (w_index != 0) {
			value = -EDOM;
			break;
		}
		*(u8 *)req->buf = 0;
		value = min(w_length, (u16) 1);
		break;

	default:
unknown:
		DBG("unknown control req%02x.%02x v%04x i%04x l%d\n",
		    ctrl->bRequestType, ctrl->bRequest,
		    w_value, w_index, w_length);
	}

	/* respond with data transfer before status phase? */
	if (value >= 0) {
		req->length = value;
		req->zero = value < w_length;
		value = usb_ep_queue(gadget->ep0, req, GFP_ATOMIC);
		if (value < 0) {
			DBG("ep_queue --> %d\n", value);
			req->status = 0;
			usbd_setup_complete(gadget->ep0, req);
		}
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}

static void usbd_connect(struct usb_gadget *gadget)
{
	struct usbd_dev		*dev = get_gadget_data(gadget);

	usbd_reset_config(dev);
}


static void usbd_unbind(struct usb_gadget *gadget)
{
	struct usbd_dev		*dev = get_gadget_data(gadget);

	DBG("%s\n", __func__);

	if (dev->req) {
		dev->req->length = USB_BUFSIZ;
		free_ep_req(gadget->ep0, dev->req);
	}

	free(dev);
	set_gadget_data(gadget, NULL);
}

static int usbd_bind(struct usb_gadget *gadget)
{
	struct usbd_dev		*dev;
	struct usb_ep		*ep;
	int			gcnum;

	usb_ep_autoconfig_reset(gadget);
	
	ep = usb_ep_autoconfig(gadget, &fs_in_desc);
	if (!ep) {
autoconf_fail:
		printf("%s: can't autoconfigure on %s\n",
			shortname, gadget->name);
		return -ENODEV;
	}
	EP_IN_NAME = ep->name;
	ep->driver_data = ep;	/* claim */

	ep = usb_ep_autoconfig(gadget, &fs_out_desc);
	if (!ep)
		goto autoconf_fail;
	EP_OUT_NAME = ep->name;
	ep->driver_data = ep;	/* claim */

	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum);
	else {
		printf("%s: controller '%s' not recognized\n",
			shortname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}

	dev = calloc(sizeof(*dev), 1);
	if (!dev)
		return -ENOMEM;

	dev->gadget = gadget;
	set_gadget_data(gadget, dev);

	/* preallocate control response and buffer */
	dev->req = usb_ep_alloc_request(gadget->ep0, 0);
	if (!dev->req)
		goto enomem;
	dev->req->buf = malloc(USB_BUFSIZ);
	if (!dev->req->buf)
		goto enomem;

	dev->req->complete = usbd_setup_complete;
	device_desc.bMaxPacketSize0 = gadget->ep0->maxpacket;

	if (gadget_is_dualspeed(gadget)) {
		/* assume ep0 uses the same value for both speeds ... */
		dev_qualifier.bMaxPacketSize0 = device_desc.bMaxPacketSize0;

		/* and that all endpoints are dual-speed */
		hs_in_desc.bEndpointAddress =
				fs_in_desc.bEndpointAddress;
		hs_out_desc.bEndpointAddress =
				fs_out_desc.bEndpointAddress;
	}

	if (gadget_is_otg(gadget)) {
		otg_descriptor.bmAttributes |= USB_OTG_HNP,
		usb_downloader_config.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

	usb_gadget_set_selfpowered(gadget);

	gadget->ep0->driver_data = dev;

	DBG("%s, version: " DRIVER_VERSION "\n", longname);
	DBG("using %s, OUT %s IN %s\n", gadget->name,
		EP_OUT_NAME, EP_IN_NAME);

	usbd_gadget = gadget;
	
	return 0;

enomem:
	usbd_unbind(gadget);
	return -ENOMEM;
}

static struct usb_gadget_driver usbdown_driver = {
#ifdef CONFIG_USB_GADGET_DUALSPEED
	.speed		= USB_SPEED_HIGH,
#else
	.speed		= USB_SPEED_FULL,
#endif
	.bind		= usbd_bind,
	.unbind		= usbd_unbind,

	.setup		= usbd_setup,
	.disconnect	= usbd_connect,
};

extern void usbd_gadget_udc_probe(void);

int init_usbd_gadget(char *ch)
{
	int ret;

	usbd_gadget_udc_probe();
	
	ret = usb_gadget_register_driver(&usbdown_driver);
	if (ret) {
		printf("%s: failed!, error:%d ", __func__, ret);
		return 0; /* by convention ERRNO (minus) shall be returned */
	}
	/* Wait for a device enumeration and configuration settings */
	DBG("USBD enumeration/configuration setting....\n");

	while (!usbd_gadget_configuration_done) {
		usb_gadget_handle_interrupts();
	}

	return 1; /* Idicates that everything is OK
	           * by convention 0 shall be returned
	           */
}

void usbd_cleanup(void)
{	
	DBG("USBD unregister....\n");
	usb_gadget_unregister_driver(&usbdown_driver);
}

static void usbd_gadget_set_dma(char *addr, int len) {
	struct usbd_dev *dev = get_gadget_data(usbd_gadget);

	DBG("in_req:%p, out_req:%p\n", dev->in_req, dev->out_req);
	DBG("addr:%p, len:%d", addr, len);

	dev->out_req->buf = addr;
	dev->out_req->length = len;

}

static void usbd_gadget_cancel(int mode) {

	switch (mode) {
	case END_BOOT:
		run_command("run bootcmd", 0);
		break;
	case END_RETRY:
		run_command("usbdown", 0);
		break;
	default:
		break;
	}
}

static void usbd_gadget_dummy(void) {

}

/* USB Downloader's specific definitions - required for this framework */

#define TX_DATA_LEN 64
#define RX_DATA_LEN 64

static char usbd_tx_data[TX_DATA_LEN] = "MPL";
static char usbd_rx_data[RX_DATA_LEN];
/*
 * This function is a interface between
 * USB Downloader and USB Gadget Framework
 */
struct usbd_ops *usbd_set_interface_gadget(struct usbd_ops *usbd)
{
	usbd->usb_init = init_usbd_gadget;
	usbd->usb_stop = usbd_gadget_dummy;
	usbd->send_data = usbd_gadget_tx_data;
	usbd->recv_data = usbd_gadget_rx_data;
	usbd->recv_setup = usbd_gadget_set_dma;
	usbd->tx_data = usbd_tx_data;
	usbd->rx_data = usbd_rx_data;
	usbd->tx_len = TX_DATA_LEN;
	usbd->rx_len = RX_DATA_LEN;
	usbd->ram_addr = CONFIG_SYS_DOWN_ADDR;
	usbd->down_cancel = usbd_gadget_cancel;
	usbd->down_start = usbd_gadget_dummy;
#ifdef CONFIG_S5PC1XXFB
	if (!s5p_no_lcd_support())
		usbd->set_progress = usbd_gadget_dummy;
#endif

	return usbd;
}

