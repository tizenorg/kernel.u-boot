#ifndef __USB_MASS_STORAGE_H__
#define __USB_MASS_STORAGE_H__

#define SECTOR_SIZE		0x200

#include <mmc.h>

struct ums_device {
	struct mmc *mmc;
	int dev_num;
	int offset;
	int part_size;
};

struct ums_board_info {
	int (*read_sector)(struct ums_device *ums_dev, unsigned int n, void *buf);
	int (*write_sector)(struct ums_device *ums_dev, unsigned int n, void *buf);
	void (*get_capacity)(struct ums_device *ums_dev, long long int *capacity);
	const char* name;
	struct ums_device ums_dev;
};

extern int fsg_init(struct ums_board_info*);
extern struct ums_board_info* board_ums_init(unsigned int, unsigned int, unsigned int);
extern int usb_gadget_handle_interrupts(void);
extern int fsg_main_thread(void*);

/* USB Attach/Detach */
extern int micro_usb_attached(void);
extern int micro_usb_detach(void);

#endif
