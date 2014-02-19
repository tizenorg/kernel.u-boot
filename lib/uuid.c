/*
 * Copyright 2011 Calxeda, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <linux/ctype.h>
#include <errno.h>
#include <common.h>
#include <part_efi.h>
#include <malloc.h>

#define UUID_STR_BYTE_LEN		37

#define UUID_VERSION_CLEAR_BITS		0x0fff
#define UUID_VERSION_SHIFT		12
#define UUID_VERSION			0x4

#define UUID_VARIANT_CLEAR_BITS		0x3f
#define UUID_VARIANT_SHIFT		7
#define UUID_VARIANT			0x1

struct uuid {
	unsigned int time_low;
	unsigned short time_mid;
	unsigned short time_hi_and_version;
	unsigned char clock_seq_hi_and_reserved;
	unsigned char clock_seq_low;
	unsigned char node[6];
};

/*
 * This is what a UUID string looks like.
 *
 * x is a hexadecimal character. fields are separated by '-'s. When converting
 * to a binary UUID, le means the field should be converted to little endian,
 * and be means it should be converted to big endian.
 *
 * 0        9    14   19   24
 * xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
 *    le     le   le   be       be
 */

int uuid_str_valid(const char *uuid)
{
	int i, valid;

	if (uuid == NULL)
		return 0;

	for (i = 0, valid = 1; uuid[i] && valid; i++) {
		switch (i) {
		case 8: case 13: case 18: case 23:
			valid = (uuid[i] == '-');
			break;
		default:
			valid = isxdigit(uuid[i]);
			break;
		}
	}

	if (i != 36 || !valid)
		return 0;

	return 1;
}

int uuid_str_to_bin(char *uuid, unsigned char *out)
{
	uint16_t tmp16;
	uint32_t tmp32;
	uint64_t tmp64;

	if (!uuid || !out)
		return -EINVAL;

	if (!uuid_str_valid(uuid))
		return -EINVAL;

	tmp32 = cpu_to_le32(simple_strtoul(uuid, NULL, 16));
	memcpy(out, &tmp32, 4);

	tmp16 = cpu_to_le16(simple_strtoul(uuid + 9, NULL, 16));
	memcpy(out + 4, &tmp16, 2);

	tmp16 = cpu_to_le16(simple_strtoul(uuid + 14, NULL, 16));
	memcpy(out + 6, &tmp16, 2);

	tmp16 = cpu_to_be16(simple_strtoul(uuid + 19, NULL, 16));
	memcpy(out + 8, &tmp16, 2);

	tmp64 = cpu_to_be64(simple_strtoull(uuid + 24, NULL, 16));
	memcpy(out + 10, (char *)&tmp64 + 2, 6);

	return 0;
}

int uuid_bin_to_str(unsigned char *uuid, char *str)
{
	static const u8 le[16] = {3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11,
				  12, 13, 14, 15};
	char *str_ptr = str;
	int i;

	for (i = 0; i < 16; i++) {
		sprintf(str, "%02x", uuid[le[i]]);
		str += 2;
		switch (i) {
		case 3:
		case 5:
		case 7:
		case 9:
			*str++ = '-';
			break;
		}
	}

	if (!uuid_str_valid(str_ptr))
		return -EINVAL;

	return 0;
}

/*
 * get_uuid_str() - this function returns pointer to 36 character hexadecimal
 * ASCII string representation of a 128-bit (16 octets) UUID (Universally
 * Unique Identifier) version 4 based on RFC4122.
 * source: https://www.ietf.org/rfc/rfc4122.txt
 *
 * Layout of UUID Version 4:
 * timestamp - 60-bit: time_low, time_mid, time_hi_and_version
 * version   - 4 bit (bit 4 through 7 of the time_hi_and_version)
 * clock seq - 14 bit: clock_seq_hi_and_reserved, clock_seq_low
 * variant:  - bit 6 and 7 of clock_seq_hi_and_reserved
 * node      - 48 bit
 * In this version all fields beside 4 bit version are randomly generated.
 *
 * @ret: pointer to 36 bytes len characters array
 */
char *get_uuid_str(void)
{
	struct uuid uuid;
	char *uuid_str = NULL;
	int *ptr = (int *)&uuid;
	int i;

	uuid_str = malloc(UUID_STR_BYTE_LEN);
	if (!uuid_str) {
		error("uuid_str pointer is null");
		return NULL;
	}

	memset(&uuid, 0x0, sizeof(uuid));

	/* Set all fields randomly */
	for (i=0; i < sizeof(uuid) / 4; i++)
		*(ptr + i) = rand();

	/* Set V4 format */
	uuid.time_hi_and_version &= UUID_VERSION_CLEAR_BITS;
	uuid.time_hi_and_version |= UUID_VERSION << UUID_VERSION_SHIFT;

	uuid.clock_seq_hi_and_reserved &= UUID_VARIANT_CLEAR_BITS;
	uuid.clock_seq_hi_and_reserved |= UUID_VARIANT << UUID_VARIANT_SHIFT;

	uuid_bin_to_str((unsigned char *)&uuid, uuid_str);

	if (!uuid_str_valid(uuid_str)) {
		error("Invalid UUID string");
		return NULL;
	}

	/* Put end of string */
	uuid_str[UUID_STR_BYTE_LEN - 1] = '\0';

	return uuid_str;
}
