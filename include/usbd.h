#ifndef _USBD_H_
#define _USBD_H_

#include <asm/arch-exynos/usb-hs-otg.h>

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

struct usbd_file_info {
	u32 download_total_size;
	u32 download_packet_size;
	u32 download_unit;
	u32 download_prog;
	u32 download_prog_check;

	int file_size;
	int file_type;
	char *file_name;

	int file_offset;
};

/* This function is interfaced between USB Device Controller and USB Downloader
 * Must Implementation this function at USB Controller!! */
struct usbd_ops *usbd_set_interface(struct usbd_ops *, int);

otg_dev_t *get_otg(void);
void set_s5p_receive_done(int value);
void set_usb_connected(int value);

#endif /* _USBD_H_ */
