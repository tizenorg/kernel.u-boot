/*
 * Copyright (C) 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * Configuation settings for the SAMSUNG Universal (s5pc100) board.
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

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * High Level Configuration Options
 * (easy to change)
 */
#define CONFIG_ARMV7		1	/* This is an ARM V7 CPU core */
#define CONFIG_SAMSUNG		1	/* in a SAMSUNG core */
#define CONFIG_S5P		1	/* which is in a S5P Family */
#define CONFIG_S5PC100		1	/* which is in a S5PC100 */
#define CONFIG_S5PC110		1	/* which is in a S5PC110 */
#define CONFIG_UNIVERSAL	1	/* working with Universal */
#define CONFIG_MACH_AQUILA	1	/* working with Aquila */
#define CONFIG_MACH_GONI	1	/* working with Goni */
#define CONFIG_SBOOT		1	/* use the s-boot */

#include <asm/arch/cpu.h>		/* get chip and board defs */

#define CONFIG_ARCH_CPU_INIT

#define CONFIG_SYS_SDRAM_BASE	0x30000000

/* input clock of PLL: Universal has 12MHz/24MHz input clock at S5PC100/C110 */
#define CONFIG_SYS_CLK_FREQ_C100	12000000
#define CONFIG_SYS_CLK_FREQ_C110	24000000

#define CONFIG_MEMORY_UPPER_CODE

#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG
#define CONFIG_REVISION_TAG

/* Clock Defines */
#define V_OSCK		26000000	/* Clock output from T2 */
#define V_SCLK		(V_OSCK >> 1)

/*
 * Architecture magic and machine type
 */
#define MACH_TYPE	3000

#define CONFIG_DISPLAY_CPUINFO

#undef CONFIG_SKIP_RELOCATE_UBOOT

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 1024 * 1024)
#define CONFIG_SYS_GBL_DATA_SIZE	128	/* size in bytes for initial data */

/*
 * select serial console configuration
 */
#define CONFIG_SERIAL_MULTI	1
#define CONFIG_SERIAL2          1	/* we use SERIAL 2 on S5PC100 */

/* 
 * spi gpio 
 */
#define CONFIG_SPI_GPIO		1

/* MMC */
#define CONFIG_GENERIC_MMC	1
#define CONFIG_MMC		1
#define CONFIG_S5P_MMC		1

#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/
#ifdef CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#endif

#define CONFIG_CMDLINE_EDITING

#define CONFIG_BAUDRATE		115200

/* It should define before config_cmd_default.h */
#define CONFIG_SYS_NO_FLASH		1
/***********************************************************
 * Command definition
 ***********************************************************/
#include <config_cmd_default.h>

#undef CONFIG_CMD_BOOTD
#undef CONFIG_CMD_CONSOLE
#undef CONFIG_CMD_ECHO
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_ITEST
#undef CONFIG_CMD_FLASH
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_LOADB
#undef CONFIG_CMD_LOADS
#undef CONFIG_CMD_NAND
#undef CONFIG_CMD_MISC
#undef CONFIG_CMD_NFS
#undef CONFIG_CMD_SETGETDCR
#undef CONFIG_CMD_SOURCE
#undef CONFIG_CMD_XIMG
#define CONFIG_CMD_CACHE
#define CONFIG_CMD_REGINFO
#define CONFIG_CMD_ONENAND
#define CONFIG_CMD_MTDPARTS
#define CONFIG_CMD_MTDPARTS_LITE
#define CONFIG_CMD_I2C
#define CONFIG_CMD_MMC
#define CONFIG_CMD_SLEEP
#define CONFIG_CMD_PMIC
#define CONFIG_CMD_DEVICE_POWER

/* disabled commands */
//#define CONFIG_CMD_GPIO

#undef CONFIG_CRC16
#undef CONFIG_XYZMODEM
#define CONFIG_SHOW_REGS_SILENT

#define CONFIG_SYS_64BIT_VSPRINTF	1

#define CONFIG_BOOTDELAY		1

#define CONFIG_ZERO_BOOTDELAY_CHECK

/* To enable UBI command */
#if 0
#define CONFIG_CMD_UBI
#define CONFIG_RBTREE
#define CONFIG_CMD_UBIFS
#define CONFIG_LZO
#endif

/* To enable make ubifs and ubinized image*/
#define CONFIG_LZO_COMPRESSION
#define CONFIG_UBIFS_MK
#define CONFIG_UBINIZE

/* To use the TFTPBOOT over USB, Please enable the CONFIG_CMD_NET */
#undef CONFIG_CMD_NET

#ifdef CONFIG_CMD_NET
/* Ethernet */
#define CONFIG_NET_MULTI		1
#define CONFIG_NET_RETRY_COUNT		2
#define CONFIG_NET_DO_NOT_TRY_ANOTHER	1

/* NFS support in Ethernet over USB is broken */

/* Configure Ethernet over USB */
#define CONFIG_USB_ETH_RNDIS		1
#define CONFIG_USB_GADGET		1
#define CONFIG_USB_GADGET_S3C_UDC_OTG		1
#define CONFIG_USB_GADGET_DUALSPEED	1
#define CONFIG_USB_ETHER		1
#define CONFIG_USBNET_MANUFACTURER      "S5PC1xx U-Boot"
/* ethaddr settings can be overruled via environment settings */
#define CONFIG_USBNET_DEV_ADDR		"8e:28:0f:fa:3c:39"
#define CONFIG_USBNET_HOST_ADDR		"0a:fa:63:8b:e8:0a"
#define CONFIG_USB_CDC_VENDOR_ID        0x0525
#define CONFIG_USB_CDC_PRODUCT_ID       0xa4a1
#define CONFIG_USB_RNDIS_VENDOR_ID      0x0525
#define CONFIG_USB_RNDIS_PRODUCT_ID     0xa4a2

#endif

#define CONFIG_NETMASK		255.255.255.0
#define CONFIG_IPADDR		192.168.129.3
#define CONFIG_SERVERIP		192.168.129.1
#define CONFIG_GATEWAYIP	192.168.129.1
#define CONFIG_ETHADDR		8e:28:0f:fa:3c:39

#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS

/* Actual modem binary size is 16MiB. Add 2MiB for bad block handling */
#define MTDIDS_DEFAULT		"onenand0=samsung-onenand"
#define MTDPARTS_DEFAULT	"mtdparts=samsung-onenand:1m(bootloader)"\
				",128k(params)"\
				",2816k(config)"\
				",8m(csa)"\
				",7m(kernel)"\
				",1m(log)"\
				",12m(modem)"\
				",60m(qboot)"\
				",-(UBI)\0"

#ifdef CONFIG_SBOOT
#define MTDPARTS_DEFAULT_4KB	"mtdparts=samsung-onenand:"\
				"256k(s-boot)"\
				",512k(bootloader)"\
				",256k(params)"\
				",2816k(config)"\
				",8m(csa)"\
				",7m(kernel)"\
				",1m(log)"\
				",12m(modem)"\
				",60m(qboot)"\
				",-(UBI)\0"
#else
#define MTDPARTS_DEFAULT_4KB	"mtdparts=samsung-onenand:"\
				"768k(bootloader)"\
				",256k(params)"\
				",2816k(config)"\
				",8m(csa)"\
				",7m(kernel)"\
				",1m(log)"\
				",12m(modem)"\
				",60m(qboot)"\
				",-(UBI)\0"
#endif

#define NORMAL_MTDPARTS_DEFAULT MTDPARTS_DEFAULT

#define CONFIG_BOOTCOMMAND	"run ubifsboot"

#define CONFIG_DEFAULT_CONSOLE	"console=ttySAC2,115200n8\0"

#define CONFIG_RAMDISK_BOOT	"root=/dev/ram0 rw rootfstype=ext2" \
		" ${console} ${meminfo}"

#define CONFIG_COMMON_BOOT	"${console} ${meminfo} ${mtdparts}"

#define CONFIG_BOOTARGS	"root=/dev/mtdblock8 ubi.mtd=8 ubi.mtd=3 ubi.mtd=6" \
		" rootfstype=cramfs " CONFIG_COMMON_BOOT

#define CONFIG_UPDATEB	"updateb=onenand erase 0x0 0x100000;" \
			" onenand write 0x32008000 0x0 0x100000\0"

#define CONFIG_UBI_MTD	" ubi.mtd=${ubiblock} ubi.mtd=3 ubi.mtd=6"

#define CONFIG_UBIFS_OPTION	"rootflags=bulk_read,no_chk_data_crc"

#ifdef CONFIG_SBOOT
#define CONFIG_BOOTBLOCK	"10"
#define CONFIG_UBIBLOCK		"9"
#else
#define CONFIG_BOOTBLOCK	"9"
#define CONFIG_UBIBLOCK		"8"
#endif

#define CONFIG_ENV_OVERWRITE
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_EXTRA_ENV_SETTINGS					\
	CONFIG_UPDATEB \
	"updatek=onenand erase 0xc00000 0x600000;" \
	" onenand write 0x31008000 0xc00000 0x600000\0" \
	"updateu=onenand erase 0x01560000 0x1eaa0000;" \
	" onenand write 0x32000000 0x1260000 0x8C0000\0" \
	"bootk=onenand read 0x30007FC0 0xbc0000 0x600000;" \
	" bootm 0x30007FC0\0" \
	"flashboot=set bootargs root=/dev/mtdblock${bootblock}" \
	 " rootfstype=${rootfstype}" \
	 CONFIG_UBI_MTD " ${opts} ${lcdinfo} " CONFIG_COMMON_BOOT "; run bootk\0" \
	"ubifsboot=set bootargs root=ubi0!rootfs rootfstype=ubifs " \
	 CONFIG_UBIFS_OPTION CONFIG_UBI_MTD " ${opts} ${lcdinfo} " \
	 CONFIG_COMMON_BOOT "; run bootk\0" \
	"tftpboot=set bootargs root=ubi0!rootfs rootfstype=ubifs " \
	 CONFIG_UBIFS_OPTION CONFIG_UBI_MTD " ${opts} ${lcdinfo} " \
	 CONFIG_COMMON_BOOT "; tftp 0x30007FC0 uImage; bootm 0x30007FC0\0" \
	"boottrace=setenv opts initcall_debug; run bootcmd\0" \
	"android=set bootargs root=ubi0!ramdisk " CONFIG_UBI_MTD \
	 " rootfstype=ubifs init=/init.sh " CONFIG_COMMON_BOOT "; run bootk\0" \
	"nfsboot=set bootargs root=/dev/nfs rw " CONFIG_UBI_MTD \
	 " nfsroot=${nfsroot},nolock,tcp ip=${ipaddr}:${serverip}:${gatewayip}:" \
	 "${netmask}:generic:usb0:off " CONFIG_COMMON_BOOT "; run bootk\0" \
	"ramboot=set bootargs " CONFIG_RAMDISK_BOOT \
	 " initrd=0x33000000,8M ramdisk=8192\0" \
	"mmcboot=set bootargs root=${mmcblk} rootfstype=${rootfstype}" \
	 CONFIG_UBI_MTD " ${opts} ${lcdinfo} " CONFIG_COMMON_BOOT "; run bootk\0" \
	"bootchart=set opts init=/sbin/bootchartd; run bootcmd\0" \
	"verify=n\0" \
	"rootfstype=cramfs\0" \
	"console=" CONFIG_DEFAULT_CONSOLE \
	"mtdparts=" MTDPARTS_DEFAULT \
	"meminfo=mem=80M mem=128M@0x40000000\0" \
	"nfsroot=/nfsroot/arm\0" \
	"mmcblk=/dev/mmcblk1p1\0" \
	"bootblock=" CONFIG_BOOTBLOCK "\0" \
	"ubiblock=" CONFIG_UBIBLOCK" \0" \
	"ubi=enabled\0" \
	"opts=always_resume=1"

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP	/* undef to save memory */
#define CONFIG_SYS_PROMPT	"Universal # "	/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE	256	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE	384	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS	16	/* max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE	/* Boot Argument Buffer Size */

#define CONFIG_SYS_MEMTEST_START	CONFIG_SYS_SDRAM_BASE	/* memtest works on	      */
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_SDRAM_BASE + 0x5000000)

#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE + 0x4800000)

#define CONFIG_SYS_HZ			1000

/* valid baudrates */
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(256 << 10)	/* regular stack 256KB */

/*******************************
 Support Clock Settings(APLL)
 *******************************
 ARMCLK		HCLKD0		PCLKD0
 -------------------------------
 667		166			83
 600		150			75
 533		133			66
 500		166			66
 467		117			59
 400		100			50
 *******************************/

#define CONFIG_CLK_667_166_83
/*#define CONFIG_CLK_666_166_66*/
/*#define CONFIG_CLK_600_150_75*/
/*#define CONFIG_CLK_533_133_66*/
/*#define CONFIG_CLK_500_166_66*/
/*#define CONFIG_CLK_467_117_59*/
/*#define CONFIG_CLK_400_100_50*/

/* Universal has 2 banks of DRAM, but swap the bank */
#define CONFIG_NR_DRAM_BANKS	2
#define PHYS_SDRAM_1		CONFIG_SYS_SDRAM_BASE	/* OneDRAM Bank #0 */
#define PHYS_SDRAM_1_SIZE	(80 << 20)		/* 80 MB in Bank #0 */
#define S5PC100_PHYS_SDRAM_2	0x38000000		/* mDDR DMC0 Bank #1 */
#define S5PC110_PHYS_SDRAM_2	0x40000000		/* mDDR DMC1 Bank #0 */
#define PHYS_SDRAM_2_SIZE	(128 << 20)		/* 128 MB in Bank #1 */


#define CONFIG_SYS_MONITOR_BASE	0x00000000

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */

#define CONFIG_SYS_MONITOR_LEN		(256 << 10)	/* Reserve 2 sectors */

/* OneNAND IPL uses 8KiB */
#define CONFIG_ONENAND_START_PAGE	4

#define CONFIG_ENV_IS_IN_ONENAND	1
#define CONFIG_ENV_SIZE			4096
#define CONFIG_ENV_ADDR			(768 << 10)	/* 768KB, 0xc00000 */

#define CONFIG_USE_ONENAND_BOARD_INIT
#define CONFIG_SAMSUNG_ONENAND		1
#define CONFIG_SYS_ONENAND_BASE		0xB0000000

#define CONFIG_DOS_PARTITION	1

#define CONFIG_MISC_INIT_R
#define CONFIG_BOARD_EARLY_INIT_F

/* I2C */
#if 0
#define CONFIG_DRIVER_S5PC1XX_I2C
#define CONFIG_HARD_I2C		1
#define CONFIG_SYS_I2C_SPEED	50000
#define CONFIG_SYS_I2C_SLAVE	0xFE
#define CONFIG_SYS_I2C_0	1
#else
#include <i2c-gpio.h>
#define CONFIG_S5P_GPIO_I2C	1
#define CONFIG_SOFT_I2C		1
#define CONFIG_SYS_I2C_INIT_BOARD
#define CONFIG_SYS_I2C_SPEED	50000
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_MAX_I2C_BUS	7
#endif

/* USB Downloader */
#define CONFIG_CMD_USBDOWN
#define CONFIG_SAMSUNG_USB
#define CONFIG_OTG_CLK_OSCC
#define CONFIG_SYS_DOWN_ADDR	CONFIG_SYS_SDRAM_BASE
#define CONFIG_RAMDISK_ADDR	(CONFIG_SYS_SDRAM_BASE + 0x03000000)

/* LCD */
#if 1		/* For LCD test */
#define CONFIG_LCD		1
#define CONFIG_FB_ADDR		0x42504000
#define CONFIG_S5PC1XXFB	1
#define CONFIG_DSIM		1
#define CONFIG_S6E63M0		1
#define CONFIG_S6E39A0X		1
#define CONFIG_S6D16A0X		1
#define CONFIG_LD9040		1
#define CONFIG_CMD_BMP
#endif

#if 0
#define CONFIG_CMD_EXT2			1
#define CONFIG_CMD_ONENAND_EXT2		1
#endif

/* Insert bmp animation compressed */
#define CONFIG_VIDEO_BMP_GZIP
#ifndef CONFIG_SYS_VIDEO_LOGO_MAX_SIZE
#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE	(250*250*4)
#endif

#define CONFIG_SYS_SP_ADDR	(CONFIG_SYS_LOAD_ADDR - 0x1000000)
#define CONFIG_SYS_INIT_SP_ADDR	(CONFIG_SYS_SP_ADDR - CONFIG_SYS_GBL_DATA_SIZE)

#endif	/* __CONFIG_H */
