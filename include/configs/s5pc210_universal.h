/*
 * Copyright (C) 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * Configuation settings for the SAMSUNG Universal (EXYNOS4210) board.
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
#define CONFIG_EXYNOS4210	1	/* which is in a EXYNOS4210 */
#define CONFIG_UNIVERSAL	1	/* working with Universal */
#define CONFIG_SBOOT		1	/* use the s-boot */

#include <asm/arch/cpu.h>		/* get chip and board defs */

#define CONFIG_ARCH_CPU_INIT
#define CONFIG_DISPLAY_CPUINFO

#define CONFIG_USE_ARCH_MEMSET
#define CONFIG_USE_ARCH_MEMCPY

/* Keep L2 Cache Disabled */
#define CONFIG_L2_OFF			1

#define CONFIG_SYS_SDRAM_BASE		0x40000000
#define CONFIG_SYS_TEXT_BASE		0x44800000

/* input clock of PLL: Universal has 24MHz input clock at EXYNOS4210 */
#define CONFIG_SYS_CLK_FREQ_C210	24000000

#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG
#define CONFIG_REVISION_TAG

/*
 * Architecture magic and machine type
 */
#define MACH_TYPE	2989

#define CONFIG_DISPLAY_CPUINFO

#undef CONFIG_SKIP_RELOCATE_UBOOT

/*
 * select serial console configuration
 */
#define CONFIG_SERIAL_MULTI	1
#define CONFIG_SERIAL2          1	/* we use SERIAL 2 on S5PC210 */

/* INFORM0~3 registers are cleared by asserting XnRESET pin */
/* INFORM4~7 registers are cleared only by power-up reset */
#define CONFIG_INFO_ADDRESS		0x10020810	/* INFORM4 */

/* 
 * spi gpio 
 */
#define CONFIG_SPI_GPIO		1

/* MMC */
#if 1
#define CONFIG_GENERIC_MMC	1
#define CONFIG_MMC		1
#define CONFIG_S5P_MMC		1
#define CONFIG_MMC_ASYNC_WRITE	1
#endif

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
#define CONFIG_CMD_FAT
#define CONFIG_CMD_RAMOOPS
#define CONFIG_CMD_MBR
#define CONFIG_INFO_ACTION

/* disabled commands */
//#define CONFIG_CMD_GPIO

#define CONFIG_CMD_USB_MASS_STORAGE
/* #define CONFIG_USBDOWNLOAD_GADGET */

#undef CONFIG_CRC16
#undef CONFIG_XYZMODEM

#define CONFIG_SYS_64BIT_VSPRINTF	1

/* To enable UBI command */
#if 0
#define CONFIG_CMD_UBI
#define CONFIG_RBTREE
#define CONFIG_CMD_UBIFS
#define CONFIG_LZO
#endif

#define CONFIG_FAT_WRITE

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

#if defined (CONFIG_USBDOWNLOAD_GADGET) || defined (CONFIG_CMD_USB_MASS_STORAGE)
#define CONFIG_USB_GADGET		1
#define CONFIG_USB_GADGET_S3C_UDC_OTG		1
#define CONFIG_USB_GADGET_DUALSPEED	1
#define CONFIG_USB_GADGET_MASS_STORAGE
#endif

#define CONFIG_NETMASK		255.255.255.0
#define CONFIG_IPADDR		192.168.129.3
#define CONFIG_SERVERIP		192.168.129.1
#define CONFIG_GATEWAYIP	192.168.129.1
#define CONFIG_ETHADDR		8e:28:0f:fa:3c:39

#define CONFIG_CMDLINE_EDITING

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + (1 << 20))

/* select serial console configuration */
#define CONFIG_SERIAL_MULTI	1
#define CONFIG_SERIAL2		1	/* use SERIAL 2 */
#define CONFIG_BAUDRATE		115200

/* MMC */
#define CONFIG_GENERIC_MMC	1
#define CONFIG_MMC		1
#define CONFIG_S5P_MMC		1

/* PWM */
#define CONFIG_PWM			1

/* It should define before config_cmd_default.h */
#define CONFIG_SYS_NO_FLASH		1

/* Command definition */
#include <config_cmd_default.h>

#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_MISC
#undef CONFIG_CMD_NET
#undef CONFIG_CMD_NFS
#undef CONFIG_CMD_XIMG
#define CONFIG_CMD_CACHE
#define CONFIG_CMD_ONENAND
#define CONFIG_CMD_MTDPARTS
#define CONFIG_CMD_MMC
#define CONFIG_CMD_FAT

#define CONFIG_BOOTDELAY		0
#define CONFIG_ZERO_BOOTDELAY_CHECK

#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS

/* Actual modem binary size is 16MiB. Add 2MiB for bad block handling */
#define MTDIDS_DEFAULT		"onenand0=samsung-onenand"

#ifdef CONFIG_SBOOT
#define MTDPARTS_DEFAULT	"mtdparts=samsung-onenand:"\
				"128k(s-boot)"\
				",896k(bootloader)"\
				",256k(params)"\
				",2816k(config)"\
				",8m(csa)"\
				",7m(kernel)"\
				",1m(log)"\
				",12m(modem)"\
				",60m(qboot)"\
				",-(UBI)\0"
#else
#define MTDPARTS_DEFAULT	"mtdparts=samsung-onenand:"\
				"1m(bootloader)"\
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

#define MBRPARTS_DEFAULT	"20M(permanent)"\
				",20M(boot)"\
				",1G(system)"\
				",100M(swap)"\
				",-(UMS)\0"

#define CONFIG_BOOTLOADER_SECTOR 0x80

#define CONFIG_BOOTARGS		"Please use defined boot"
#define CONFIG_BOOTCOMMAND	"run mmcboot"
#define CONFIG_DEFAULT_CONSOLE	"console=ttySAC2,115200n8\0"
#ifdef CONFIG_SBOOT
#define CONFIG_ENV_UBI_MTD	" ubi.mtd=${ubiblock} ubi.mtd=4 ubi.mtd=7"
#define CONFIG_BOOTBLOCK	"10"
#define CONFIG_UBIBLOCK		"9"
#else
#define CONFIG_ENV_UBI_MTD	" ubi.mtd=${ubiblock} ubi.mtd=3 ubi.mtd=6"
#define CONFIG_BOOTBLOCK	"9"
#define CONFIG_UBIBLOCK		"8"
#endif

#define CONFIG_ENV_UBIFS_OPTION	" rootflags=bulk_read,no_chk_data_crc "
#define CONFIG_ENV_FLASHBOOT	CONFIG_ENV_UBI_MTD CONFIG_ENV_UBIFS_OPTION \
				"${mtdparts}"

#define CONFIG_ENV_COMMON_BOOT	"${console} ${meminfo}"

#define CONFIG_ENV_OVERWRITE
#define CONFIG_ENV_AUTOSAVE
#define CONFIG_SYS_CONSOLE_INFO_QUIET
#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#define CONFIG_EXTRA_ENV_SETTINGS					\
	"updateb=" \
		"onenand erase 0x0 0x100000;" \
		"onenand write 0x42008000 0x0 0x100000\0" \
	"updatek=" \
		"onenand erase 0xc00000 0x500000;" \
		"onenand write 0x41008000 0xc00000 0x500000\0" \
	"bootk=" \
		"run loaduimage; bootm 0x40007FC0\0" \
	"updatemmc=" \
		"mmc boot 0 1 1 1; mmc write 0 0x42008000 0 0x200;" \
		"mmc boot 0 1 1 0\0" \
	"updatebackup=" \
		"mmc boot 0 1 1 2; mmc write 0 0x42100000 0 0x200;" \
		"mmc boot 0 1 1 0\0" \
	"updatebootb=" \
		"mmc read 0 0x42100000 0x80 0x200; run updatebackup\0" \
	"lpj=lpj=3981312\0" \
	"ubifsboot=" \
		"set bootargs root=ubi0!rootfs rootfstype=ubifs ${lpj} " \
		CONFIG_ENV_FLASHBOOT " ${opts} ${lcdinfo} " \
		CONFIG_ENV_COMMON_BOOT "; run bootk\0" \
	"tftpboot=" \
		"set bootargs root=ubi0!rootfs rootfstype=ubifs " \
		CONFIG_ENV_FLASHBOOT " ${opts} ${lcdinfo} " \
		CONFIG_ENV_COMMON_BOOT \
		"; tftp 0x40007FC0 uImage; bootm 0x40007FC0\0" \
	"nfsboot=" \
		"set bootargs root=/dev/nfs rw " \
		"nfsroot=${nfsroot},nolock,tcp " \
		"ip=${ipaddr}:${serverip}:${gatewayip}:" \
		"${netmask}:generic:usb0:off " CONFIG_ENV_COMMON_BOOT \
		"; run bootk\0" \
	"ramfsboot=" \
		"set bootargs root=/dev/ram0 rw rootfstype=ext2 " \
		"${console} ${meminfo} " \
		"initrd=0x43000000,8M ramdisk=8192\0" \
	"mmcboot=" \
		"set bootargs root=/dev/mmcblk${mmcdev}p${mmcrootpart} " \
		"${lpj} rootwait ${console} ${meminfo} ${opts} ${lcdinfo}; " \
		"run loaduimage; bootm 0x40007FC0\0" \
	"bootchart=set opts init=/sbin/bootchartd; run bootcmd\0" \
	"boottrace=setenv opts initcall_debug; run bootcmd\0" \
	"mmcoops=mmc read 0 0x40000000 0x40 8; md 0x40000000 0x400\0" \
	"verify=n\0" \
	"rootfstype=ext4\0" \
	"console=" CONFIG_DEFAULT_CONSOLE \
	"mtdparts=" MTDPARTS_DEFAULT \
	"mbrparts=" MBRPARTS_DEFAULT \
	"meminfo=crashkernel=32M@0x50000000\0" \
	"nfsroot=/nfsroot/arm\0" \
	"bootblock=" CONFIG_BOOTBLOCK "\0" \
	"ubiblock=" CONFIG_UBIBLOCK" \0" \
	"ubi=enabled\0" \
	"loaduimage=fatload mmc ${mmcdev}:${mmcbootpart} 0x40007FC0 uImage\0" \
	"mmcdev=0\0" \
	"mmcbootpart=2\0" \
	"mmcrootpart=3\0" \
	"opts=always_resume=1"

/* Miscellaneous configurable options */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_PROMPT	"Universal # "
#define CONFIG_SYS_CBSIZE	256	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE	384	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS	16	/* max number of command args */
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
/* memtest works on */
#define CONFIG_SYS_MEMTEST_START	CONFIG_SYS_SDRAM_BASE
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_SDRAM_BASE + 0x5000000)
#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE + 0x4800000)

#define CONFIG_SYS_HZ			1000

/* valid baudrates */
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/* Stack sizes */
#define CONFIG_STACKSIZE	(256 << 10)	/* regular stack 256KB */

/* Universal has 2 banks of DRAM */
#define CONFIG_NR_DRAM_BANKS	2
#define PHYS_SDRAM_1		CONFIG_SYS_SDRAM_BASE	/* LDDDR2 DMC 0 */
#define PHYS_SDRAM_1_SIZE	(256 << 20)		/* 256 MB in CS 0 */
#define PHYS_SDRAM_2		0x50000000		/* LPDDR2 DMC 1 */
#define PHYS_SDRAM_2_SIZE	(256 << 20)		/* 256 MB in CS 0 */

#define CONFIG_SYS_MEM_TOP_HIDE		(1 << 20)	/* ram console */

#define CONFIG_SYS_MONITOR_BASE		0x00000000
#define CONFIG_SYS_MONITOR_LEN		(256 << 10)	/* Reserve 2 sectors */

/* OneNAND IPL uses 32KiB = 2KiB * 16 */
#define CONFIG_ONENAND_START_PAGE	16
#define CONFIG_USE_ONENAND_BOARD_INIT
#define CONFIG_SYS_ONENAND_BASE		0x0C000000

#define CONFIG_ENV_IS_IN_MMC		1
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_ENV_SIZE			4096
#define CONFIG_ENV_OFFSET		((32 - 4) << 10)/* 32KiB - 4KiB */

#define CONFIG_MISC_INIT_R


#define CONFIG_DOS_PARTITION		1

#define CONFIG_SYS_INIT_SP_ADDR	(CONFIG_SYS_LOAD_ADDR - GENERATED_GBL_DATA_SIZE)

#define CONFIG_SYS_CACHELINE_SIZE       32

#include <asm/arch/gpio.h>
/*
 * I2C Settings
 */
#define CONFIG_SOFT_I2C_GPIO_SCL exynos4_gpio_part1_get_nr(b, 7)
#define CONFIG_SOFT_I2C_GPIO_SDA exynos4_gpio_part1_get_nr(b, 6)

#define CONFIG_SOFT_I2C
#define CONFIG_SOFT_I2C_READ_REPEATED_START
#define CONFIG_SYS_I2C_INIT_BOARD
#define CONFIG_SYS_I2C_SPEED	50000
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_MAX_I2C_BUS	7

/* USB Downloader */
#define CONFIG_CMD_USBDOWN
#define CONFIG_SAMSUNG_USB
/* select USB operation mode: CONFIG_S5P_USB_DMA or CONFIG_S5P_USB_CPU */
#define CONFIG_S5P_USB_DMA
#define CONFIG_OTG_CLK_OSCC
#define CONFIG_SYS_DOWN_ADDR	CONFIG_SYS_SDRAM_BASE

/* LCD */
#if 1
#define CONFIG_LCD		1
#define CONFIG_FB_ADDR		0x52504000
#define CONFIG_S5PC1XXFB	1
#define CONFIG_LD9040		1
#define CONFIG_DSIM		1
/* Insert bmp animation compressed */
#define CONFIG_VIDEO_BMP_GZIP
#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE	(250*250*4)
#endif

#define CONFIG_PMIC
#define CONFIG_PMIC_I2C
#define CONFIG_PMIC_MAX8998



#endif	/* __CONFIG_H */
