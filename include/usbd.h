/*
 * USB Downloader for SLP
 *
 * Copyright (C) 2011 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Sanghee Kim <sh0130.kim@samsung.com>
 *
 */

#ifndef _USBD_H_
#define _USBD_H_

#define msleep(a)	udelay(a * 1000)

#ifdef CONFIG_ENV_UPDATE_WITH_DL
/*
 * updateflag is consist of below bit.
 * 7: RESERVED.
 * 6: RESERVED.
 * 5: RESERVED.
 * 4: RESERVED.
 * 3: SLP_ROOTFS_NEW
 * 2: SLP_KERNEL_NEW
 * 1: SLP_SBL_NEW
 * 0: SLP_PBL_NEW
 */
enum {
	SLP_PBL_NEW = 0,
	SLP_SBL_NEW,
	SLP_KERNEL_NEW,
	SLP_ROOTFS_NEW,
};
#endif

/* status definition */
enum {
	STATUS_DONE = 0,
	STATUS_RETRY,
	STATUS_ERROR,
};

/* download mode definition */
enum {
	MODE_NORMAL = 0,
	MODE_FORCE,
	MODE_UPLOAD,
};

/* end mode */
enum {
	END_BOOT = 0,
	END_RETRY,
	END_FAIL,
	END_NORMAL,
};
/*
 * USB Downloader Operations
 * All functions and valuable are mandatory
 *
 * usb_init	: initialize the USB Controller and check the connection
 * usb_stop	: stop and release USB
 * send_data	: send the data (BULK ONLY!!)
 * recv_data	: receive the data and returns received size (BULK ONLY!!)
 * recv_setup	: setup download address, length and DMA setting for receive
 * tx_data	: send data address
 * rx_data	: receive data address
 * tx_len	: size of send data
 * rx_len	: size of receive data
 * ram_addr	: address of will be stored data on RAM
 *
 * mmc_dev	: device number of mmc
 * mmc_max	: number of max blocks
 * mmc_blk	: mmc block size
 * mmc_total	: mmc total blocks
 */
struct usbd_ops {
	int (*usb_init)(void);
	void (*usb_stop)(void);
	void (*send_data)(char *, int);
	int (*recv_data)(void);
	void (*recv_setup)(char *, int);
#ifdef CONFIG_S5P_USB_DMA
	void (*prepare_dma)(void * , u32 , uchar);
	void (*dma_done)(int);
#endif
	char *tx_data;
	char *rx_data;
	ulong tx_len;
	ulong rx_len;
	ulong ram_addr;

	/* mmc device info */
	uint mmc_dev;
	uint mmc_max;
	uint mmc_blk;
	uint mmc_total;
	char *mmc_info;

	void (*set_logo)(char *, char *, int);
	void (*set_progress)(int);
	void (*cpu_reset)(void);
	void (*start)(void);
	void (*cancel)(int);
};

/* This function is interfaced between USB Device Controller and USB Downloader
 * Must Implementation this function at USB Controller!! */
struct usbd_ops *usbd_set_interface(struct usbd_ops *, int);

#endif /* _USBD_H_ */
