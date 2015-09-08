#ifndef __UBINIZE_FUNC_H__
#define __UBINIZE_FUNC_H__

extern int ubinize(void *src_addr, unsigned long src_size,
		   void *dest_addr, unsigned long *dest_size,
		   int peb_size, int min_io_size,
		   int subpage_size, int vid_hdr_offs);
#endif
