#include <common.h>
#include <samsung/sighdr.h>

static struct sig_header *
get_image_signature(phys_addr_t base_addr, phys_size_t size)
{
	struct sig_header *hdr;

	hdr = (struct sig_header *)(base_addr + size - HDR_SIZE);
	if (hdr->magic != HDR_BOOT_MAGIC) {
		debug("can't found signature\n");
		return NULL;
	}

	return hdr;
}

int check_board_signature(char *fname, phys_addr_t dn_addr, phys_size_t size)
{
	struct sig_header *bh_target;
	struct sig_header *bh_addr;

	/* u-boot-mmc.bin */
	if (strcmp(fname, "u-boot-mmc.bin"))
		return 0;

	/* can't found signature in target - download continue */
	bh_target = get_image_signature((phys_addr_t)CONFIG_SYS_TEXT_BASE,
					size);
	if (!bh_target)
		return 0;

	/* can't found signature in address - download stop */
	bh_addr = get_image_signature(dn_addr, size);
	if (!bh_addr)
		return -1;

	if (strncmp(bh_target->bd_name, bh_addr->bd_name,
		    ARRAY_SIZE(bh_target->bd_name))) {
		debug("board name Inconsistency\n");
		return -1;
	}

	debug("board signature check OK\n");

	return 0;
}
