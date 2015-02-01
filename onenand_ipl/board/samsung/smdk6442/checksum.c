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

/*
 * For 2KiB page OneNAND
 * +------+------+------+------+
 * | 2KiB | 2KiB | 2KiB | 2KiB |
 * +------+------+------+------+
 */

/*
 * For 4KiB page OneNAND
 * +------+------+
 * | 4KiB | 4KiB |
 * +------+------+
 */
#define CHECKSUM_8K	(SZ_8K - 0x4)

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

	size = CHECKSUM_8K;

	for (i = 0; i < size; i++) {
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

	ret = write(fd, &sum, 4);

	if (ret < 0)
		printf("read err: %s\n", ret);

	close(fd);

	printf("checksum = %x\n", sum);

	return 0;
}
