#include <common.h>
#include <sighdr.h>
#include <pit.h>

static int get_board_signature(phys_addr_t base_addr,
			       phys_size_t size, struct sig_header **hdr)
{
	*hdr = (struct sig_header *)(base_addr + size - HDR_SIZE);

	if ((*hdr)->magic != HDR_BOOT_MAGIC) {
		debug("can't found signature\n");
		return -1;
	}

	return 0;
}

int check_board_signature(char *fname, phys_addr_t dn_addr)
{
	struct sig_header *bh_target;
	struct sig_header *bh_addr;
	phys_size_t part_size;
	int pit_idx;
	int ret;

	/* u-boot-mmc.bin */
	if (strcmp(fname, PARTS_BOOTLOADER"-mmc.bin"))
		return 0;

	pit_idx = get_pitpart_id_by_filename(fname);

	if (!pit_idx)
		return 0;

	part_size = get_pitpart_size(pit_idx);

	ret = get_board_signature((phys_addr_t)CONFIG_SYS_TEXT_BASE,
				  part_size, &bh_target);

	/* can't found signature in target - download continue */
	if (ret)
		return 0;

	ret = get_board_signature(dn_addr, part_size, &bh_addr);

	/* can't found signature in address - download stop */
	if (ret)
		return -1;

	if (strncmp(bh_target->bd_name, bh_addr->bd_name,
		    ARRAY_SIZE(bh_target->bd_name))) {
		debug("board name Inconsistency\n");
		return -1;
	}

	debug("board signature check OK\n");

	return 0;
}
