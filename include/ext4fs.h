/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd. All rights reserved.
 * Przemyslaw Marczak <p.marczak@samsung.com>
 */

#ifndef __EXT4__
#define __EXT4__

void put_ext4(block_dev_desc_t *dev_desc,uint64_t off, void *buf, uint32_t size);
int ext4_register_device (block_dev_desc_t * dev_desc, int part_no);
int ext4fs_existing_filename_check(block_dev_desc_t *dev_desc,char *filename);
int ext4fs_delete_file(block_dev_desc_t *dev_desc, unsigned int inodeno);
int ext4fs_create_dir (block_dev_desc_t *dev_desc, char *dirname);
int ext4fs_create_symlink (block_dev_desc_t *dev_desc, int link_type,
		char *src_path, char *target_path);
int ext4fs_ls (const char *dirname);
int ext4fs_open(const char *filename);
int ext4fs_close(void);
int ext4fs_mount(unsigned part_length);
int ext4fs_read(char *buf, unsigned len);
int ext4fs_write(block_dev_desc_t *dev_desc, int part_no,char *fname,
				unsigned char* buffer,unsigned long sizebytes);


#endif