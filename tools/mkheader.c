/*
 * Copyright (C) 2010 Samsung Electronics
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <compiler.h>
#include <mobile/header.h>

#ifdef CONFIG_SLP_NEW_HEADER
#define PADDING_ALIGN 16
#define HEADER_SIZE (sizeof(struct boot_header))
#endif

struct mkheader_params {
	char type;
	char *inputfile;
	char *imagefile;
#ifdef CONFIG_SLP_NEW_HEADER
	char *imagesize;
#endif
	char *version;
	char *date;
	char *boardname;
	char *cmdname;
};

static struct boot_header header;
static struct mkheader_params params;

static void usage(void);

static void set_header(void *ptr, struct stat *sbuf, int ifd,
				struct mkheader_params *params)
{
	struct boot_header *hdr = (struct boot_header *)ptr;

	if (params->type == 'B')
		hdr->magic = HDR_BOOT_MAGIC;
	else
		hdr->magic = HDR_KERNEL_MAGIC;

	hdr->size = sbuf->st_size;
	hdr->valid = 0;
	sprintf(hdr->date, "%s", params->date);
	sprintf(hdr->version, "%s", params->version);
	sprintf(hdr->boardname, "%s", params->boardname);
}

static void print_header(const void *ptr)
{
	const struct boot_header *hdr = (const struct boot_header *)ptr;
	const char *p = "";

	printf("%sMagic Code:    0x%8x\n", p, hdr->magic);
	printf("%sData size:     %d Bytes\n", p, hdr->size);
	printf("%sImage time:    %s\n", p, hdr->date);
	printf("%sImage version: %s\n", p, hdr->version);
	printf("%sTarget board:  %s\n", p, hdr->boardname);
}

static int set_type(struct mkheader_params *params)
{
	int ret = -1;
	char *s;

	params->type = 'U';

	s = strstr(params->inputfile, "boot");
	if (s != NULL) {
		params->type = 'B';
		ret = 0;
	}

	s = strstr(params->inputfile, "uImage");
	if (s != NULL) {
		params->type = 'K';
		ret = 0;
	}

	return ret;
}

static void copy_file(int ifd, const char *inputfile, int pad)
{
	int dfd;
	struct stat sbuf;
	unsigned char *ptr;
	int tail;
	int zero = 0;
	int offset = 0;
	int size;

	dfd = open(inputfile, O_RDONLY|O_BINARY);
	if (dfd < 0) {
		fprintf(stderr, "%s: Can't open %s: %s\n",
			params.cmdname, inputfile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (fstat(dfd, &sbuf) < 0) {
		fprintf(stderr, "%s: Can't stat %s: %s\n",
			params.cmdname, inputfile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	ptr = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, dfd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "%s: Can't read %s: %s\n",
			params.cmdname, inputfile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	size = sbuf.st_size - offset;
	if (write(ifd, ptr + offset, size) != size) {
		fprintf(stderr, "%s: Write error on %s: %s\n",
			params.cmdname, params.imagefile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	tail = size % 4;
	if (pad && (tail != 0)) {

		if (write(ifd, (char *)&zero, 4-tail) != 4-tail) {
			fprintf(stderr, "%s: Write error on %s: %s\n",
				params.cmdname, params.imagefile,
				strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	(void) munmap((void *)ptr, sbuf.st_size);
	(void) close(dfd);
}

#ifdef CONFIG_SLP_NEW_HEADER
static void padding_file(int ifd, int final_size)
{
	struct stat sbuf;
	int padding_size;
	int image_size;
	int remains;
	int zero = 0;
	char zero_pad[PADDING_ALIGN];

	if (fstat(ifd, &sbuf) < 0) {
		fprintf(stderr, "%s: Can't stat : %s\n",
			params.cmdname, strerror(errno));
		exit(EXIT_FAILURE);
	}

	image_size = sbuf.st_size;
	padding_size = final_size - image_size;

	printf("padding_size %d =  final_size %d - image_size %d\n", padding_size, final_size, image_size);

	if ((int)(padding_size - HEADER_SIZE) < 0) {
		fprintf(stderr, "%s: Exceed Image size %s\n",
			params.cmdname, strerror(errno));
		exit(EXIT_FAILURE);
	}

	memset(zero_pad, 0, PADDING_ALIGN);

	remains = padding_size;
	do {
		if (remains < 16) {
			if (write(ifd, (char *)&zero, remains) != remains) {
				fprintf(stderr, "%s: Write error on %s: %s\n",
					params.cmdname, params.imagefile,
					strerror(errno));
				exit(EXIT_FAILURE);
			}
			break;
		}

		if (write(ifd, zero_pad, 16) != 16) {
			fprintf(stderr, "%s: Write error on %s: %s\n",
				params.cmdname, params.imagefile,
				strerror(errno));
			exit(EXIT_FAILURE);
		}

		remains -= 16;
	} while (remains > 0);
}
#endif

int main(int argc, char **argv)
{
	int ifd = -1;
	struct stat sbuf;
	char *ptr;
#ifdef CONFIG_SLP_NEW_HEADER
	int image_size = 0;
#endif

	memset((void *)&params, 0, sizeof(struct mkheader_params));

	params.cmdname = *argv;

	while (--argc > 0 && **++argv == '-') {
		while (*++*argv) {
			switch (**argv) {
			case 'v':
			case 'V':
				if (--argc <= 0)
					usage();
				params.version = *++argv;
				goto NXTARG;
			case 't':
			case 'T':
				if (--argc <= 0)
					usage();
				params.date = *++argv;
				goto NXTARG;
			case 'n':
			case 'N':
				if (--argc <= 0)
					usage();
				params.boardname = *++argv;
				goto NXTARG;
			case 'i':
			case 'I':
				if (--argc <= 0)
					usage();
				params.inputfile = *++argv;
				goto NXTARG;
#ifdef CONFIG_SLP_NEW_HEADER
			case 's':
			case 'S':
				if (--argc <= 0)
					usage();
				params.imagesize = *++argv;
				goto NXTARG;
#endif
		default:
				usage();
			}
		}
NXTARG:		;
	}

	if (argc != 1)
		usage();

#ifdef CONFIG_SLP_NEW_HEADER
	if (params.imagesize == NULL) {
		fprintf(stderr, "Invalid imagesize..\n");
		usage();
		exit(EXIT_FAILURE);
	}
	image_size = atoi(params.imagesize);
	image_size *= 1024;
#endif

	params.imagefile = *argv;

	if (set_type(&params)) {
		fprintf(stderr, "%s: unsupported file %s\n",
			params.cmdname, params.inputfile);
		usage();
		exit(EXIT_FAILURE);
	}

	ifd = open(params.imagefile, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0666);

	if (ifd < 0) {
		fprintf(stderr, "%s: Can't open %s: %s\n",
			params.cmdname, params.imagefile, strerror(errno));
		exit(EXIT_FAILURE);
	}

#ifndef CONFIG_SLP_NEW_HEADER
	memset(&header, 0, sizeof(header));

	if (write(ifd, (void *)&header, sizeof(header))
					!= sizeof(header)) {
		fprintf(stderr, "%s: Write error on %s: %s\n",
			params.cmdname, params.imagefile, strerror(errno));
		exit(EXIT_FAILURE);
	}
#endif

	copy_file(ifd, params.inputfile, 0);

#ifdef CONFIG_SLP_NEW_HEADER
	/* Zero Padding to Input image size */
	padding_file(ifd, image_size);
#endif

	fsync(ifd);

	/* Make header */
#ifdef CONFIG_SLP_NEW_HEADER
	memset(&header, 0, sizeof(header));
#endif

	if (fstat(ifd, &sbuf) < 0) {
		fprintf(stderr, "%s: Can't stat %s: %s\n",
			params.cmdname, params.imagefile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	ptr = mmap(0, sbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, ifd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "%s: Can't map %s: %s\n",
			params.cmdname, params.imagefile, strerror(errno));
		exit(EXIT_FAILURE);
	}
#ifdef CONFIG_SLP_NEW_HEADER
	ptr += (sbuf.st_size - 512);
#endif

	/* Setup the image header as per input image type*/
	set_header(ptr, &sbuf, ifd, &params);

	/* Print the image information by processing image header */
	print_header(ptr);

	(void) munmap((void *)ptr, sbuf.st_size);

	close(ifd);

	return -1;
}

static void usage(void)
{
	fprintf(stderr, "Usage: mkheader -l image\n"
			 "          -l ==> list image header information\n");
	fprintf(stderr, "       mkheader [-x] -i input_file image\n"
			 "          -v ==> set image version\n"
			 "          -t ==> set creation time[YYMMDDHH]\n"
			 "          -n ==> set board name\n"
#ifdef CONFIG_SLP_NEW_HEADER
			 "          -s ==> total padding image size (KB)\n"
#endif
			 "          -i ==> use image data as inputfile "
			 "(u-boot or uImage)\n");

	exit(EXIT_FAILURE);
}

