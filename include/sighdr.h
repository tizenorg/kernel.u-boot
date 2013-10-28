#ifndef __HEADER_H__
#define __HEADER_H__

#define HDR_BOOT_MAGIC		0x744f6f42	/* BoOt */

#define HDR_SIZE		sizeof(struct sig_header)

/* HDR_SIZE - 512 */
struct sig_header {
	uint32_t magic;		/* image magic number */
	uint32_t size;		/* image data size */
	uint32_t valid;		/* valid flag */
	char date[12];		/* image creation timestamp - YYMMDDHH */
	char version[24];	/* image version */
	char bd_name[16];	/* target board name */
	char reserved[448];	/* reserved */
};

int check_board_signature(char *fname, phys_addr_t dn_addr);
#endif
