#ifndef __LINUX_MTD_S3C_ONENAND_H
#define __LINUX_MTD_S3C_ONENAND_H

#include <linux/mtd/mtd.h>
//#include <s5pc1xx-onenand.h>

#define MAX_BUFFERRAM		2

/* Scan and identify a OneNAND device */
extern int onenand_scan(struct mtd_info *mtd, int max_chips);
/* Free resources held by the OneNAND device */
extern void onenand_release(struct mtd_info *mtd);

/**
 * struct onenand_bufferram - OneNAND BufferRAM Data
 * @block:		block address in BufferRAM
 * @page:		page address in BufferRAM
 * @valid:		valid flag
 */
struct onenand_bufferram {
	int block;
	int page;
	int valid;
};

/**
 * struct onenand_chip - OneNAND Private Flash Chip Data
 * @base:		[BOARDSPECIFIC] address to access OneNAND
 * @chipsize:		[INTERN] the size of one chip for multichip arrays
 * @device_id:		[INTERN] device ID
 * @density_mask:	chip density, used for DDP devices
 * @verstion_id:	[INTERN] version ID
 * @options:		[BOARDSPECIFIC] various chip options. They can
 *			partly be set to inform onenand_scan about
 * @erase_shift:	[INTERN] number of address bits in a block
 * @page_shift:		[INTERN] number of address bits in a page
 * @ppb_shift:		[INTERN] number of address bits in a pages per block
 * @page_mask:		[INTERN] a page per block mask
 * @bufferram_index:	[INTERN] BufferRAM index
 * @bufferram:		[INTERN] BufferRAM info
 * @readw:		[REPLACEABLE] hardware specific function for read short
 * @writew:		[REPLACEABLE] hardware specific function for write short
 * @command:		[REPLACEABLE] hardware specific function for writing
 *			commands to the chip
 * @wait:		[REPLACEABLE] hardware specific function for wait on ready
 * @read_bufferram:	[REPLACEABLE] hardware specific function for BufferRAM Area
 * @write_bufferram:	[REPLACEABLE] hardware specific function for BufferRAM Area
 * @read_word:		[REPLACEABLE] hardware specific function for read
 *			register of OneNAND
 * @write_word:		[REPLACEABLE] hardware specific function for write
 *			register of OneNAND
 * @mmcontrol:		sync burst read function
 * @block_markbad:	function to mark a block as bad
 * @scan_bbt:		[REPLACEALBE] hardware specific function for scanning
 *			Bad block Table
 * @chip_lock:		[INTERN] spinlock used to protect access to this
 *			structure and the chip
 * @wq:			[INTERN] wait queue to sleep on if a OneNAND
 *			operation is in progress
 * @state:		[INTERN] the current state of the OneNAND device
 * @page_buf:		data buffer
 * @subpagesize:	[INTERN] holds the subpagesize
 * @ecclayout:		[REPLACEABLE] the default ecc placement scheme
 * @bbm:		[REPLACEABLE] pointer to Bad Block Management
 * @priv:		[OPTIONAL] pointer to private chip date
 */
struct onenand_chip {
	void __iomem		*base;
	unsigned int		chipsize;
	unsigned int		device_id;
	unsigned int		version_id;
	unsigned int		density_mask;
	unsigned int		options;

	unsigned int		erase_shift;
	unsigned int		page_shift;
	unsigned int		ppb_shift;	/* Pages per block shift */
	unsigned int		page_mask;

	uint (*command)(struct mtd_info *mtd, int cmd, loff_t address);
	unsigned int (*read)(void __iomem *addr);
	void (*write)(unsigned int value, void __iomem *addr);
	int (*scan_bbt)(struct mtd_info *mtd);

	unsigned char		*page_buf;
	unsigned char		*oob_buf;

	int			subpagesize;
	struct nand_ecclayout	*ecclayout;

	void			*bbm;

	void			*priv;
};

/*
 * Options bits
 */
#define ONENAND_HAS_CONT_LOCK		(0x0001)
#define ONENAND_HAS_UNLOCK_ALL		(0x0002)
#define	ONENAND_CHECK_BAD		(0x0004)
#define	ONENAND_READ_POLLING		(0x0010)
#define ONENAND_READ_BURST		(0x0020)
#define ONENAND_READ_MASK		(0x00F0)
#define ONENAND_PIPELINE_AHEAD		(0x0100)
#define ONENAND_PAGEBUF_ALLOC		(0x1000)
#define ONENAND_OOBBUF_ALLOC		(0x2000)

/*
 * OneNAND Flash Manufacturer ID Codes
 */
#define ONENAND_MFR_SAMSUNG	0xec

/**
 * struct onenand_manufacturers - NAND Flash Manufacturer ID Structure
 * @name:	Manufacturer name
 * @id:		manufacturer ID code of device.
*/
struct onenand_manufacturers {
        int id;
        char *name;
};

#define ONENAND_REG_MEM_CFG             (0x000)
#define ONENAND_REG_BURST_LEN           (0x010)
#define ONENAND_REG_MEM_RESET           (0x020)
#define ONENAND_REG_INT_ERR_STAT        (0x030)
#define ONENAND_REG_INT_ERR_MASK        (0x040)
#define ONENAND_REG_INT_ERR_ACK         (0x050)
#define ONENAND_REG_ECC_ERR_STAT_1      (0x060)
#define ONENAND_REG_MANUFACT_ID         (0x070)
#define ONENAND_REG_DEVICE_ID           (0x080)
#define ONENAND_REG_DATA_BUF_SIZE       (0x090)
#define ONENAND_REG_BOOT_BUF_SIZE       (0x0A0)
#define ONENAND_REG_BUF_AMOUNT          (0x0B0)
#define ONENAND_REG_TECH                (0x0C0)
#define ONENAND_REG_FBA_WIDTH           (0x0D0)
#define ONENAND_REG_FPA_WIDTH           (0x0E0)
#define ONENAND_REG_FSA_WIDTH           (0x0F0)
#define ONENAND_REG_REVISION            (0x100)
#define ONENAND_REG_SYNC_MODE           (0x130)
#define ONENAND_REG_TRANS_SPARE         (0x140)
#define ONENAND_REG_PAGE_CNT            (0x170)
#define ONENAND_REG_ERR_PAGE_ADDR       (0x180)
#define ONENAND_REG_BURST_RD_LAT        (0x190)
#define ONENAND_REG_INT_PIN_ENABLE      (0x1A0)
#define ONENAND_REG_INT_MON_CYC         (0x1B0)
#define ONENAND_REG_ACC_CLOCK           (0x1C0)
#define ONENAND_REG_ERR_BLK_ADDR        (0x1E0)
#define ONENAND_REG_FLASH_VER_ID        (0x1F0)
#define ONENAND_REG_BANK_EN             (0x220)
#define ONENAND_REG_WTCHDG_RST_L        (0x260)
#define ONENAND_REG_WTCHDG_RST_H        (0x270)
#define ONENAND_REG_SYNC_WRITE          (0x280)
#define ONENAND_REG_CACHE_READ          (0x290)
#define ONENAND_REG_COLD_RST_DLY        (0x2A0)
#define ONENAND_REG_DDP_DEVICE          (0x2B0)
#define ONENAND_REG_MULTI_PLANE         (0x2C0)
#define ONENAND_REG_MEM_CNT             (0x2D0)
#define ONENAND_REG_TRANS_MODE          (0x2E0)
#define ONENAND_REG_DEV_STAT            (0x2F0)
#define ONENAND_REG_ECC_ERR_STAT_2      (0x300)
#define ONENAND_REG_ECC_ERR_STAT_3      (0x310)
#define ONENAND_REG_ECC_ERR_STAT_4      (0x320)
#define ONENAND_REG_EFCT_BUF_CNT        (0x330)
#define ONENAND_REG_DEV_PAGE_SIZE       (0x340)
#define ONENAND_REG_SUPERLOAD_EN        (0x350)
#define ONENAND_REG_CACHE_PRG_EN        (0x360)
#define ONENAND_REG_SINGLE_PAGE_BUF     (0x370)
#define ONENAND_REG_OFFSET_ADDR         (0x380)
#define ONENAND_REG_INT_MON_STATUS      (0x390)

#define ONENAND_MEM_CFG_SYNC_READ       (1 << 15)
#define ONENAND_MEM_CFG_BRL_7           (7 << 12)
#define ONENAND_MEM_CFG_BRL_6           (6 << 12)
#define ONENAND_MEM_CFG_BRL_5           (5 << 12)
#define ONENAND_MEM_CFG_BRL_4           (4 << 12)
#define ONENAND_MEM_CFG_BRL_3           (3 << 12)
#define ONENAND_MEM_CFG_BRL_10          (2 << 12)
#define ONENAND_MEM_CFG_BRL_9           (1 << 12)
#define ONENAND_MEM_CFG_BRL_8           (0 << 12)
#define ONENAND_MEM_CFG_BRL_SHIFT       (12)
#define ONENAND_MEM_CFG_BL_1K           (5 << 9)
#define ONENAND_MEM_CFG_BL_32           (4 << 9)
#define ONENAND_MEM_CFG_BL_16           (3 << 9)
#define ONENAND_MEM_CFG_BL_8            (2 << 9)
#define ONENAND_MEM_CFG_BL_4            (1 << 9)
#define ONENAND_MEM_CFG_BL_CONT         (0 << 9)
#define ONENAND_MEM_CFG_BL_SHIFT        (9)
#define ONENAND_MEM_CFG_NO_ECC          (1 << 8)
#define ONENAND_MEM_CFG_RDY_HIGH        (1 << 7)
#define ONENAND_MEM_CFG_INT_HIGH        (1 << 6)
#define ONENAND_MEM_CFG_IOBE            (1 << 5)
#define ONENAND_MEM_CFG_RDY_CONF        (1 << 4)
#define ONENAND_MEM_CFG_HF              (1 << 2)
#define ONENAND_MEM_CFG_WM_SYNC         (1 << 1)
#define ONENAND_MEM_CFG_BWPS_UNLOCK     (1 << 0)

#define ONENAND_BURST_LEN_CONT          (0)
#define ONENAND_BURST_LEN_4             (4)
#define ONENAND_BURST_LEN_8             (8)
#define ONENAND_BURST_LEN_16            (16)

#define ONENAND_MEM_RESET_WARM          (0x1)
#define ONENAND_MEM_RESET_COLD          (0x2)
#define ONENAND_MEM_RESET_HOT           (0x3)

#define ONENAND_INT_ERR_CACHE_OP_ERR    (1 << 13)
#define ONENAND_INT_ERR_RST_CMP         (1 << 12)
#define ONENAND_INT_ERR_RDY_ACT         (1 << 11)
#define ONENAND_INT_ERR_INT_ACT         (1 << 10)
#define ONENAND_INT_ERR_UNSUP_CMD       (1 << 9)
#define ONENAND_INT_ERR_LOCKED_BLK      (1 << 8)
#define ONENAND_INT_ERR_BLK_RW_CMP      (1 << 7)
#define ONENAND_INT_ERR_ERS_CMP         (1 << 6)
#define ONENAND_INT_ERR_PGM_CMP         (1 << 5)
#define ONENAND_INT_ERR_LOAD_CMP        (1 << 4)
#define ONENAND_INT_ERR_ERS_FAIL        (1 << 3)
#define ONENAND_INT_ERR_PGM_FAIL        (1 << 2)
#define ONENAND_INT_ERR_INT_TO          (1 << 1)
#define ONENAND_INT_ERR_LD_FAIL_ECC_ERR (1 << 0)

#define ONENAND_DEVICE_DENSITY_SHIFT    (4)
#define ONENAND_DEVICE_IS_DDP           (1 << 3)
#define ONENAND_DEVICE_IS_DEMUX         (1 << 2)
#define ONENAND_DEVICE_VCC_MASK         (0x3)
#define ONENAND_DEVICE_DENSITY_128Mb    (0x000)
#define ONENAND_DEVICE_DENSITY_256Mb    (0x001)
#define ONENAND_DEVICE_DENSITY_512Mb    (0x002)
#define ONENAND_DEVICE_DENSITY_1Gb      (0x003)
#define ONENAND_DEVICE_DENSITY_2Gb      (0x004)
#define ONENAND_DEVICE_DENSITY_4Gb      (0x005)

#define ONENAND_SYNC_MODE_RM_SYNC       (1 << 1)
#define ONENAND_SYNC_MODE_WM_SYNC       (1 << 0)

#define ONENAND_TRANS_SPARE_TSRF_INC    (1 << 0)

#define ONENAND_INT_PIN_ENABLE          (1 << 0)

#define ONENAND_ACC_CLOCK_266_133       (0x5)
#define ONENAND_ACC_CLOCK_166_83        (0x3)
#define ONENAND_ACC_CLOCK_134_67        (0x3)
#define ONENAND_ACC_CLOCK_100_50        (0x2)
#define ONENAND_ACC_CLOCK_60_30         (0x2)

#define ONENAND_FLASH_AUX_WD_DISABLE    (1 << 0)

/*
 *  * Datain values for mapped commands
 *   */
#define ONENAND_DATAIN_ERASE_STATUS     (0x00)
#define ONENAND_DATAIN_ERASE_MULTI      (0x01)
#define ONENAND_DATAIN_ERASE_SINGLE     (0x03)
#define ONENAND_DATAIN_ERASE_VERIFY     (0x15)
#define ONENAND_DATAIN_UNLOCK_START     (0x08)
#define ONENAND_DATAIN_UNLOCK_END       (0x09)
#define ONENAND_DATAIN_LOCK_START       (0x0A)
#define ONENAND_DATAIN_LOCK_END         (0x0B)
#define ONENAND_DATAIN_LOCKTIGHT_START  (0x0C)
#define ONENAND_DATAIN_LOCKTIGHT_END    (0x0D)
#define ONENAND_DATAIN_UNLOCK_ALL       (0x0E)
#define ONENAND_DATAIN_COPYBACK_SRC     (0x1000)
#define ONENAND_DATAIN_COPYBACK_DST     (0x2000)
#define ONENAND_DATAIN_ACCESS_OTP       (0x12)
#define ONENAND_DATAIN_ACCESS_MAIN      (0x14)
#define ONENAND_DATAIN_ACCESS_SPARE     (0x13)
#define ONENAND_DATAIN_ACCESS_MAIN_AND_SPARE    (0x16)
#define ONENAND_DATAIN_PIPELINE_READ    (0x4000)
#define ONENAND_DATAIN_PIPELINE_WRITE   (0x4100)
#define ONENAND_DATAIN_RMW_LOAD         (0x10)
#define ONENAND_DATAIN_RMW_MODIFY       (0x11)

/*
 *  * Device ID Register F001h (R)
 *   */
#define ONENAND_DEVICE_DENSITY_SHIFT    (4)
#define ONENAND_DEVICE_IS_DDP           (1 << 3)
#define ONENAND_DEVICE_IS_DEMUX         (1 << 2)
#define ONENAND_DEVICE_VCC_MASK         (0x3)
/*
 *  * Version ID Register F002h (R)
 *   */
#define ONENAND_VERSION_PROCESS_SHIFT   (8)

/*
 *  * Start Address 1 F100h (R/W)
 *   */
#define ONENAND_DDP_SHIFT               (15)
#define ONENAND_DDP_CHIP0               (0)
#define ONENAND_DDP_CHIP1               (1 << ONENAND_DDP_SHIFT)

/*
 *  * Start Buffer Register F200h (R/W)
 *   */
#define ONENAND_BSA_MASK                (0x03)
#define ONENAND_BSA_SHIFT               (8)
#define ONENAND_BSA_BOOTRAM             (0 << 2)
#define ONENAND_BSA_DATARAM0            (2 << 2)
#define ONENAND_BSA_DATARAM1            (3 << 2)
#define ONENAND_BSC_MASK                (0x03)

/*
 *  * Command Register F220h (R/W)
 *   */
#define ONENAND_CMD_READ                (0x00)
#define ONENAND_CMD_READOOB             (0x13)
#define ONENAND_CMD_PROG                (0x80)
#define ONENAND_CMD_PROGOOB             (0x1A)
#define ONENAND_CMD_UNLOCK              (0x23)
#define ONENAND_CMD_LOCK                (0x2A)
#define ONENAND_CMD_LOCK_TIGHT          (0x2C)
#define ONENAND_CMD_UNLOCK_ALL          (0x27)
#define ONENAND_CMD_ERASE               (0x94)
#define ONENAND_CMD_RESET               (0xF0)
#define ONENAND_CMD_OTP_ACCESS          (0x65)
#define ONENAND_CMD_READID              (0x90)
#define ONENAND_CMD_STARTADDR1          (0xE0)
#define ONENAND_CMD_WP_STATUS           (0xE1)
#define ONENAND_CMD_PIPELINE_READ       (0x01)
#define ONENAND_CMD_PIPELINE_WRITE      (0x81)
/*
 *  * Command Mapping for S5PC100 OneNAND Controller
 *   */
#define ONENAND_AHB_ADDR                (0xB0000000)
#define ONENAND_DUMMY_ADDR              (0xB0400000)
#define ONENAND_CMD_SHIFT               (26)
#define ONENAND_CMD_MAP_00              (0x0)
#define ONENAND_CMD_MAP_01              (0x1)
#define ONENAND_CMD_MAP_10              (0x2)
#define ONENAND_CMD_MAP_11              (0x3)
#define ONENAND_CMD_MAP_FF              (0xF)

/*
 *  * Mask for Mapping table
 *   */
#define ONENAND_MEM_ADDR_MASK           (0x3ffffff)
#define ONENAND_DDP_SHIFT_1Gb           (22)
#define ONENAND_DDP_SHIFT_2Gb           (23)
#define ONENAND_DDP_SHIFT_4Gb           (24)
#define ONENAND_FBA_SHIFT               (13)
#define ONENAND_FPA_SHIFT               (7)
#define ONENAND_FSA_SHIFT               (5)
#define ONENAND_FBA_MASK_128Mb          (0xff)
#define ONENAND_FBA_MASK_256Mb          (0x1ff)
#define ONENAND_FBA_MASK_512Mb          (0x1ff)
#define ONENAND_FBA_MASK_1Gb_DDP        (0x1ff)
#define ONENAND_FBA_MASK_1Gb            (0x3ff)
#define ONENAND_FBA_MASK_2Gb_DDP        (0x3ff)
#define ONENAND_FBA_MASK_2Gb            (0x7ff)
#define ONENAND_FBA_MASK_4Gb_DDP        (0x7ff)
#define ONENAND_FBA_MASK_4Gb            (0xfff)
#define ONENAND_FPA_MASK                (0x3f)
#define ONENAND_FSA_MASK                (0x3)

/*
 *  * System Configuration 1 Register F221h (R, R/W)
 *   */
#define ONENAND_SYS_CFG1_SYNC_READ      (1 << 15)
#define ONENAND_SYS_CFG1_BRL_7          (7 << 12)
#define ONENAND_SYS_CFG1_BRL_6          (6 << 12)
#define ONENAND_SYS_CFG1_BRL_5          (5 << 12)
#define ONENAND_SYS_CFG1_BRL_4          (4 << 12)
#define ONENAND_SYS_CFG1_BRL_3          (3 << 12)
#define ONENAND_SYS_CFG1_BRL_10         (2 << 12)
#define ONENAND_SYS_CFG1_BRL_9          (1 << 12)
#define ONENAND_SYS_CFG1_BRL_8          (0 << 12)
#define ONENAND_SYS_CFG1_BRL_SHIFT      (12)
#define ONENAND_SYS_CFG1_BL_32          (4 << 9)
#define ONENAND_SYS_CFG1_BL_16          (3 << 9)
#define ONENAND_SYS_CFG1_BL_8           (2 << 9)
#define ONENAND_SYS_CFG1_BL_4           (1 << 9)
#define ONENAND_SYS_CFG1_BL_CONT        (0 << 9)
#define ONENAND_SYS_CFG1_BL_SHIFT       (9)
#define ONENAND_SYS_CFG1_NO_ECC         (1 << 8)
#define ONENAND_SYS_CFG1_RDY            (1 << 7)
#define ONENAND_SYS_CFG1_INT            (1 << 6)
#define ONENAND_SYS_CFG1_IOBE           (1 << 5)
#define ONENAND_SYS_CFG1_RDY_CONF       (1 << 4)

/*
 *  * Controller Status Register F240h (R)
 *   */
#define ONENAND_CTRL_ONGO               (1 << 15)
#define ONENAND_CTRL_LOCK               (1 << 14)
#define ONENAND_CTRL_LOAD               (1 << 13)
#define ONENAND_CTRL_PROGRAM            (1 << 12)
#define ONENAND_CTRL_ERASE              (1 << 11)
#define ONENAND_CTRL_ERROR              (1 << 10)
#define ONENAND_CTRL_RSTB               (1 << 7)
#define ONENAND_CTRL_OTP_L              (1 << 6)
#define ONENAND_CTRL_OTP_BL             (1 << 5)

/*
 *  * Interrupt Status Register F241h (R)
 *   */
#define ONENAND_INT_MASTER              (1 << 15)
#define ONENAND_INT_READ                (1 << 7)
#define ONENAND_INT_WRITE               (1 << 6)
#define ONENAND_INT_ERASE               (1 << 5)
#define ONENAND_INT_RESET               (1 << 4)
#define ONENAND_INT_CLEAR               (0 << 0)

/*
 *  * NAND Flash Write Protection Status Register F24Eh (R)
 *   */
#define ONENAND_WP_US                   (1 << 2)
#define ONENAND_WP_LS                   (1 << 1)
#define ONENAND_WP_LTS                  (1 << 0)

/*
 *  * ECC Status Register FF00h (R)
 *   */
#define ONENAND_ECC_1BIT                (1 << 0)
#define ONENAND_ECC_1BIT_ALL            (0x5555)
#define ONENAND_ECC_2BIT                (1 << 1)
#define ONENAND_ECC_2BIT_ALL            (0xAAAA)

/*
 *  * One-Time Programmable (OTP)
 *   */
#define ONENAND_OTP_LOCK_OFFSET         (14)


#endif	/* __LINUX_MTD_ONENAND_H */

