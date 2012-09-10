/*
 * Copyright (C) 2010 - 2011 Samsung Electrnoics
 * Kyungmin Park <kyungmin.park@samsung.com>
 * Lukasz Majewski <l.majewski@samsung.com>
 *
 */

#include <errno.h>
#include <common.h>
#include <command.h>
#include <usb_mass_storage.h>


#undef UMS_DBG
#define UMS_DBG(fmt,args...)	printf (fmt ,##args)
//#define UMS_DBG(...)

#define PARTITION_START_OFFSET 0x800 /* According to the documentation */
#define EBR_PART_ALIGNMENT 0x10 /* In blocks of 512B */

/**
 * Parses a string into a number.  The number stored at ptr is
 * potentially suffixed with K (for kilobytes, or 1024 bytes),
 * M (for megabytes, or 1048576 bytes), or G (for gigabytes, or
 * 1073741824).  If the number is suffixed with K, M, or G, then
 * the return value is the number multiplied by one kilobyte, one
 * megabyte, or one gigabyte, respectively.
 *
 * @param ptr where parse begins
 * @param retptr output pointer to next char after parse completes (output)
 * @return resulting unsigned int
 */
static unsigned long memsize_parse (const char *const ptr, const char **retptr)
{
	unsigned long ret = simple_strtoul(ptr, (char **)retptr, 0);

	switch (**retptr) {
		case 'G':
		case 'g':
			ret <<= 10;
		case 'M':
		case 'm':
			ret <<= 10;
		case 'K':
		case 'k':
			ret <<= 10;
			(*retptr)++;
		default:
			break;
	}

	return ret;
}

static int extract_partition_data(char *addr, unsigned int len, int part,
                                   unsigned int *ptr_offset, unsigned int *ptr_part_size)
{
	int i = 0, k = 0;
	char part_str[256];
	char save[16][16];
	char *p;
	char *tok;
	unsigned int size[16];
	int n;

	*ptr_offset = PARTITION_START_OFFSET;

	strncpy(part_str, addr, len);
	p = part_str;

	for (i = 0; ; i++, p = NULL) {
		tok = strtok(p, ",");
		if (tok == NULL)
			break;
		strcpy(save[i], tok);
		UMS_DBG("part%d: %s\n", i, save[i]);
	}

	UMS_DBG("found: %d partitions, part:%d\n", i, part);

	if (i < (part - 1)) {
		printf("%s: Error with parsing mbrparts !!!\n", __func__);
		return -1;
	}


	n = min(part,3);
	/* For partitions 1,2,3 */
	for (i = 0; i < n ; i++) {
		p = save[i];
		size[i] = memsize_parse(p, (const char **)&p) / 512;

		if (i < (n - 1)) {
			*ptr_offset += size[i];
			UMS_DBG("size[%d]:%d %x offset:%d %x\n", i, size[i], size[i],
			        *ptr_offset, *ptr_offset);
		}
	}
	i--;
	*ptr_part_size = size[i];

	/* For partitions 4 ... */
	if (part > 3) {
		UMS_DBG("size[%d]: %d %x\n",i, size[i], size[i]);
		*ptr_offset += size[i];

		if (part == 4) {/* Extended partition - itself*/
			*ptr_part_size = 0;
		} else {
			/* SWAP */
			i = 3;
			*ptr_offset += EBR_PART_ALIGNMENT;
			p = save[i];
			size[i] = memsize_parse(p, (const char **)&p) / 512;
			*ptr_part_size = size[i];
			UMS_DBG("size[%d]: %d %x\n",i, size[i], size[i]);

			/* UMS */
			if (part == 6) {
				*ptr_offset += EBR_PART_ALIGNMENT;
				*ptr_part_size = 0;
				*ptr_offset += size[i];
				UMS_DBG("size[%d]: %d %x\n",i, size[i], size[i]);
			}
		}
	}

	UMS_DBG("\n%d/%d: part_size:%d %x offset:%d %x\n", i, k, *ptr_part_size,
	        *ptr_part_size, *ptr_offset, *ptr_offset);

	return 0;
}

static int do_usb_mass_storage(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int part = 0;
	char *ep, *ptr_parts;
	unsigned int dev_num = 0, offset = 0, part_size = 0;

	struct ums_board_info* ums_info;

	if (argc < 2) {
		printf("usage: ums <dev:[part]> - e.g. <2 SD, 0 eMMC> [6: UMS]\n");
		printf("\t no [part] - the whole disk\n");
		return 0;
	}
	
	dev_num = (int)simple_strtoul(argv[1], &ep, 16);

	if (*ep) {
		if (*ep != ':') {
			puts("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	ptr_parts = getenv("mbrparts");
	if (part) {
		if (extract_partition_data(ptr_parts, strlen(ptr_parts),
		                           part, &offset, &part_size) < 0)
			goto fail;
	}

	UMS_DBG("dev_num:%d, part:%d, offset:%d %x, part_size:%d %x\n", dev_num, part,
	       offset, offset, part_size, part_size);

	ums_info = board_ums_init(dev_num, offset, part_size);

	if (!ums_info) {
		printf("MMC: %d -> NOT available\n", dev_num);
		goto fail;
	}

	fsg_init(ums_info);
	while (1)
	{
		int irq_res;
		/* Handle control-c and timeouts */
		if (ctrlc()) {
			printf("The remote end did not respond in time.\n");
			goto fail;
		}

		irq_res = usb_gadget_handle_interrupts();

		/* Check if USB cable has been detached */
		if (fsg_main_thread(NULL) == EIO)
			goto fail;
	}
fail:
	return -1;
}

U_BOOT_CMD(ums, CONFIG_SYS_MAXARGS, 1, do_usb_mass_storage,
	"Use the UMS [User Mass Storage]",
        "ums - User Mass Storage"
);

