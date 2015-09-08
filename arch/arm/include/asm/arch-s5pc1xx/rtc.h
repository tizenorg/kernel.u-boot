/*
 * (C) Copyright 2009
 * Samsung Electronics, <www.samsung.com/sec>
 * MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#ifndef __ASM_ARM_ARCH_RTC_H_
#define __ASM_ARM_ARCH_RTC_H_

#ifndef __ASSEMBLY__
struct s5pc110_rtc {
	unsigned int _resv0[3*4]; /* Starts at 0xE2800000 */
	unsigned int intp; /* 0x0030 */
	unsigned int _resv1[3];
	unsigned int rtccon; /* 0x0040 */
	unsigned int ticcnt;
	unsigned int _resv2[2];
	unsigned int rtcalm; /* 0x0050 */
	unsigned int almsec;
	unsigned int almmin;
	unsigned int almhour;
	unsigned int almday; /* 0x0060 */
	unsigned int almmon;
	unsigned int almyear;
	unsigned int _resv3;
	unsigned int bcdsec; /* 0x0070 */
	unsigned int bcdmin;
	unsigned int bcdhour;
	unsigned int bcddayweek;
	unsigned int bcdday; /* 0x0080 */
	unsigned int bcdmon;
	unsigned int bcdyear;
	unsigned int _resv4;
	unsigned int curticcnt; /* 0x0090 */
};
#endif

#endif
