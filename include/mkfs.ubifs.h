#ifndef __MKFS_UBIFS_FUNC_H__
#define __MKFS_UBIFS_FUNC_H__

extern int mkfs(void *src_addr, unsigned long src_size,
		void *dest_addr, unsigned long *dest_size,
		int min_io_size, int leb_size, int max_leb_cnt);
#endif
