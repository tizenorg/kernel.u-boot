/*
 * (C) Copyright 2003
 * Kyle Harris, kharris@nexus-tech.net
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <common.h>
#include <command.h>
#include <mmc.h>

#ifndef CONFIG_GENERIC_MMC
static int curr_device = -1;

int do_mmc (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int dev;

	if (argc < 2)
		return cmd_usage(cmdtp);

	if (strcmp(argv[1], "init") == 0) {
		if (argc == 2) {
			if (curr_device < 0)
				dev = 1;
			else
				dev = curr_device;
		} else if (argc == 3) {
			dev = (int)simple_strtoul(argv[2], NULL, 10);
		} else {
			return cmd_usage(cmdtp);
		}

		if (mmc_legacy_init(dev) != 0) {
			puts("No MMC card found\n");
			return 1;
		}

		curr_device = dev;
		printf("mmc%d is available\n", curr_device);
	} else if (strcmp(argv[1], "device") == 0) {
		if (argc == 2) {
			if (curr_device < 0) {
				puts("No MMC device available\n");
				return 1;
			}
		} else if (argc == 3) {
			dev = (int)simple_strtoul(argv[2], NULL, 10);

#ifdef CONFIG_SYS_MMC_SET_DEV
			if (mmc_set_dev(dev) != 0)
				return 1;
#endif
			curr_device = dev;
		} else {
			return cmd_usage(cmdtp);
		}

		printf("mmc%d is current device\n", curr_device);
	} else {
		return cmd_usage(cmdtp);
	}

	return 0;
}

U_BOOT_CMD(
	mmc, 3, 1, do_mmc,
	"MMC sub-system",
	"init [dev] - init MMC sub system\n"
	"mmc device [dev] - show or set current device"
);
#else /* !CONFIG_GENERIC_MMC */

static void print_mmcinfo(struct mmc *mmc)
{
	printf("Device: %s\n", mmc->name);
	printf("Manufacturer ID: %x\n", mmc->cid[0] >> 24);
	printf("OEM: %x\n", (mmc->cid[0] >> 8) & 0xffff);
	printf("Name: %c%c%c%c%c \n", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);

	printf("Tran Speed: %d\n", mmc->tran_speed);
	printf("Rd Block Len: %d\n", mmc->read_bl_len);

	printf("%s version %d.%d%s\n", IS_SD(mmc) ? "SD" : "MMC",
			(mmc->version >> 4) & 0xf,
			(mmc->version & 0xf) == EXT_CSD_REV_1_5 ?
			(mmc->check_rev? 3 : 41) :
			((mmc->version & 0xf) > EXT_CSD_REV_1_5 ?
			 (mmc->version & 0xf) - 1 :
			 (mmc->version & 0xf)),
			mmc->check_rev? "+":"");

	printf("High Capacity: %s\n", mmc->high_capacity ? "Yes" : "No");
	puts("Capacity: ");
	print_size(mmc->capacity, "\n");

	printf("Bus Width: %d-bit\n", mmc->bus_width);
}

int do_mmcinfo (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct mmc *mmc;
	int dev_num;

	if (argc < 2)
		dev_num = 0;
	else
		dev_num = simple_strtoul(argv[1], NULL, 0);

	mmc = find_mmc_device(dev_num);

	if (mmc) {
		mmc_init(mmc);

		print_mmcinfo(mmc);
	}

	return 0;
}

U_BOOT_CMD(
	mmcinfo, 2, 0, do_mmcinfo,
	"display MMC info",
	"<dev num>\n"
	"    - device number of the device to dislay info of\n"
	""
);

int do_mmcops(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int rc = 0;

	switch (argc) {
	case 3:
		if (strcmp(argv[1], "rescan") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			struct mmc *mmc = find_mmc_device(dev);

			if (!mmc)
				return 1;

			mmc_init(mmc);

			return 0;
		} else if (strncmp(argv[1], "part", 4) == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			block_dev_desc_t *mmc_dev;
			struct mmc *mmc = find_mmc_device(dev);

			if (!mmc) {
				puts("no mmc devices available\n");
				return 1;
			}
			mmc_init(mmc);
			mmc_dev = mmc_get_dev(dev);
			if (mmc_dev != NULL &&
			    mmc_dev->type != DEV_TYPE_UNKNOWN) {
				print_part(mmc_dev);
				return 0;
			}

			puts("get mmc type error!\n");
			return 1;
		}

	case 0:
	case 1:
	case 4:
		return cmd_usage(cmdtp);

	case 2:
		if (!strcmp(argv[1], "list")) {
			print_mmc_devices('\n');
			return 0;
		}
		return 1;
	default: /* at least 5 args */
		if (strcmp(argv[1], "read") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			void *addr = (void *)simple_strtoul(argv[3], NULL, 16);
			u32 cnt = simple_strtoul(argv[5], NULL, 16);
			u32 n;
			u32 blk = simple_strtoul(argv[4], NULL, 16);
			struct mmc *mmc = find_mmc_device(dev);

			if (!mmc)
				return 1;

			printf("\nMMC read: dev # %d, block # %d, count %d ... ",
				dev, blk, cnt);

			mmc_init(mmc);

			n = mmc->block_dev.block_read(dev, blk, cnt, addr);

			/* flush cache after read */
			flush_cache((ulong)addr, cnt * 512); /* FIXME */

			printf("%d blocks read: %s\n",
				n, (n==cnt) ? "OK" : "ERROR");
			return (n == cnt) ? 0 : 1;
		} else if (strcmp(argv[1], "write") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			void *addr = (void *)simple_strtoul(argv[3], NULL, 16);
			u32 cnt = simple_strtoul(argv[5], NULL, 16);
			u32 n;
			struct mmc *mmc = find_mmc_device(dev);

			int blk = simple_strtoul(argv[4], NULL, 16);

			if (!mmc)
				return 1;

			printf("MMC write: dev # %d, block # %d, count %d ... ",
				dev, blk, cnt);

			/* Not initialize mmc in boot mode */
			if (!(mmc->boot_config & 0x7))
				mmc_init(mmc);

			n = mmc->block_dev.block_write(dev, blk, cnt, addr);

			printf("%d blocks written: %s\n",
				n, (n == cnt) ? "OK" : "ERROR");
			return (n == cnt) ? 0 : 1;
		} else if (strcmp(argv[1], "boot") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			u8 ack = simple_strtoul(argv[3], NULL, 10) & 0x1;
			u8 enable = simple_strtoul(argv[4], NULL, 10) & 0x7;
			u8 access = simple_strtoul(argv[5], NULL, 10) & 0x7;
			struct mmc *mmc = find_mmc_device(dev);

			if (!mmc)
				return 1;

			/*
			 * BOOT_CONFIG[179]
			 * BOOT_ACK[6]
			 * 	0x0: No boot acknowledge sent
			 * 	0x1: Boot acknowledge sent
			 * BOOT_PARTITION_ENABLE[5:3]
			 * 	0x0: Device not boot enabled
			 * 	0x1: Boot partition 1 enabled for boot
			 * 	0x2: Boot partition 2 enabled for boot
			 * 	0x7: User area enabled for boot
			 * PARTITION_ACCESS[2:0]
			 * 	0x0: No access to boot partition
			 * 	0x1: R/W boot partition 1
			 * 	0x2: R/W boot partition 2
			 * 	0x3: R/W Replay Protected Memory Block
			 * 	0x4: Access to General Purpose partition 1
			 * 	0x5: Access to General Purpose partition 2
			 * 	0x6: Access to General Purpose partition 3
			 * 	0x7: Access to General Purpose partition 4
			 */
			mmc->boot_config = (ack << 6) | (enable << 3) | access;

			if (mmc_init(mmc))
				return 1;

			/*
			 * Note : S5PC210 EVT0 only can boot from 4-bit
			 * buswidth. Set 4-bit boot buswidth
			 */
			if (!access) {
				mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BOOT_BUS_WIDTH, MMC_BOOT_4BIT);

			}
		} else if (strcmp(argv[1], "erase") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			u32 blk = simple_strtoul(argv[3], NULL, 16);
			u32 cnt = simple_strtoul(argv[4], NULL, 16);
			u32 n, i, max_cnt = 10;
			struct mmc *mmc = find_mmc_device(dev);
			void *addr = (void *)CONFIG_SYS_SDRAM_BASE + 0x02200000;

			/* clear random address */
			memset((void *)addr, 0, (mmc->write_bl_len * max_cnt));

			if (!mmc)
				return 1;

			printf("\nMMC erase: dev # %d, block # %d, count %d ... ",
				dev, blk, cnt);

			for (i = 0; i < cnt; i += max_cnt) {
				if ((cnt - i) < max_cnt)
					max_cnt = cnt - i;

				n = mmc->block_dev.block_write(dev, blk, max_cnt, addr);
				blk += n;
			}

			printf("%d blocks erased: %s\n",
				i, (i == cnt) ? "OK" : "ERROR");

		} else
			rc = cmd_usage(cmdtp);

		return rc;
	}
}

U_BOOT_CMD(
	mmc, 6, 1, do_mmcops,
	"MMC sub system",
	"read <device num> addr blk# cnt\n"
	"mmc write <device num> addr blk# cnt\n"
	"mmc rescan <device num>\n"
	"mmc part <device num> - lists available partition on mmc\n"
	"mmc boot <device num> ack en access\n"
	"mmc list - lists available devices\n"
	"mmc erase <device num> blk# cnt");
#endif
