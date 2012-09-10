#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>

#ifndef SZ_8K
#define SZ_8K		0x2000
#endif
#ifndef SZ_14K
#define SZ_14K		0x3800
#endif

#define PAGE_SIZE	0x800
/*
 * For 2KiB page OneNAND
 * +------+------+------+------+
 * | 2KiB | 2KiB | 2KiB | 2KiB |
 * +------+------+------+------+
 */
#define CHECKSUM_8K	(SZ_8K - 0x4)
/*
 * For 4KiB page OneNAND
 * +----------------+----------------+----------------+----------------+
 * | 2KiB, reserved | 2KiB, reserved | 2KiB, reserved | 2KiB, reserved |
 * +----------------+----------------+----------------+----------------+
 */
#define CHECKSUM_16K	(SZ_14K - 0x4)

int main(int argc, char *argv[])
{
	int ret;
	int i, j;
	char buf;
	int fd;
	int fd0;
	unsigned int sum = 0;
	struct stat stat;
	off_t size;
	unsigned int header[4];
	int evt;

	if (argc != 3)
		evt = 0;
	else
		evt = atoi(argv[2]);

	fd = open(argv[1], O_RDWR);

	if (fd < 0) {
		printf("open err: %s\n", argv[1]);
		return 1;
	}

	ret = fstat(fd, &stat);
	if (ret < 0) {
		perror("fstat");
		return 1;
	}

	if (stat.st_size > SZ_8K)
		size = CHECKSUM_16K;
	else
		size = CHECKSUM_8K;

	for (i = 0; i < size; i++) {
		/* evt1 doesn't have reserved area */
		if (evt == 0) {
			/* if reserved area, skip the checksum */
			if (stat.st_size > SZ_8K) {
				if (i % PAGE_SIZE == 0) {
					int res;
					char tbuf[PAGE_SIZE];

					res = i / PAGE_SIZE;
					if (res % 2) {
							i += PAGE_SIZE;
						/*
						 * if 1st page's reverved area,
						 * copy the data to 2nd page
						 */
						if (res == 1) {
							ret = read(fd, &tbuf,
								PAGE_SIZE);
							ret = write(fd, tbuf,
								PAGE_SIZE);
						}
						lseek(fd, i, SEEK_SET);
					}
				}
			}
		}

		ret = read(fd, &buf, 1);

		if (ret < 0) {
			perror("read");
			return 1;
		}

		sum += (buf & 0xFF);

#if 0
		if (i % 16 == 0)
			printf("%04x: ", i);
		printf("%02x ", buf & 0xFF);
		if (i % 16 == 15)
			printf("\n");
#endif
	}

	if (evt == 0)
		ret = write(fd, &sum, 4);

	if (ret < 0)
		printf("read err: %s\n", ret);

	close(fd);

	printf("checksum = %x\n", sum);

	if (evt == 1) {
		fd0 = open("header.bin", O_RDWR | O_CREAT | O_TRUNC,
					S_IRUSR | S_IWUSR);

		if (fd0 < 0) {
			printf("open err: header.bin\n");
			return 1;
		}

		if (size == CHECKSUM_16K)
			header[0] = 0x4000;
		else
			header[0] = 0x2000;
		header[1] = 0;
		header[2] = sum;
		header[3] = 0;

		ret = write(fd0, header, 16);
		close(fd0);
	}

	return 0;
}
