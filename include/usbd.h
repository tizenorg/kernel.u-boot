/*
 * USB Downloader for SAMSUNG Platform
 *
 * Copyright (C) 2007-2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 */

#define msleep(a) udelay(a * 1000)
#define WAIT_TIME 2

#undef	CMD_USBD_DEBUG
#ifdef	CMD_USBD_DEBUG
#define	PRINTF(fmt, args...)	printf(fmt, ##args)
#else
#define PRINTF(fmt, args...)
#endif

/* partition info for partition sync - typically not use */
enum {
	BOOT_PART_ID	= 0,
	PARAMS_PART_ID,
	KERNEL_PART_ID,
	RAMDISK_PART_ID,
	FILESYSTEM_PART_ID,
	FILESYSTEM2_PART_ID,
	FILESYSTEM3_PART_ID,
	MODEM_PART_ID,
	MMC_PART_ID,
	NUM_PARTITION,
};

/* image type definition */
enum {
	IMG_BOOT = 0,
	IMG_KERNEL,
	IMG_FILESYSTEM,
	IMG_MODEM,
	IMG_MMC,
};

/* Download command definition */
#define COMMAND_DOWNLOAD_IMAGE	200
#define COMMAND_WRITE_PART_0	201
#define COMMAND_WRITE_PART_1	202
#define COMMAND_WRITE_PART_2	203
#define COMMAND_WRITE_PART_3	204
#define COMMAND_WRITE_PART_4	205
#define COMMAND_WRITE_PART_5	206
#define COMMAND_WRITE_PART_6	207
#define COMMAND_WRITE_PART_7	208
#define COMMAND_WRITE_PART_8	209
#define COMMAND_WRITE_PART_9	210
#define COMMAND_WRITE_UBI_INFO	211
#define COMMAND_PARTITION_SYNC	212
#define COMMAND_ERASE_PARAMETER	213
#define COMMAND_RESET_PDA	214
#define COMMAND_RESET_USB	215
#define COMMAND_RAM_BOOT	216
#define COMMAND_RAMDISK_MODE	217
#ifdef CONFIG_DOWN_PHONE
#define COMMAND_DOWN_PHONE	220
#define COMMAND_CHANGE_USB	221
#endif
#define COMMAND_CSA_CLEAR	222
#define COMMAND_PROGRESS	230

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
	void (*usb_init)(void);
	void (*usb_stop)(void);
	void (*send_data)(char *, int);
	int (*recv_data)(void);
	void (*recv_setup)(char *, int);
	char *tx_data;
	char *rx_data;
	ulong tx_len;
	ulong rx_len;
	ulong ram_addr;

	/* mmc device info */
	uint mmc_dev;
	ulong mmc_max;
	ulong mmc_blk;
	ulong mmc_total;

	void (*set_progress)(int);
};

/* This function is interfaced between USB Device Controller and USB Downloader
 * Must Implementation this function at USB Controller!! */
struct usbd_ops *usbd_set_interface(struct usbd_ops *);
