/*
 * Copyright (C) 2005-2009 Samsung Electronics
 * Kyungmin Park <kyungmin.park@samsung.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */
	.text

	.global	memcpy32
	.align	0
memcpy32:
	mov	ip, sp
	stmfd	sp!, {r0, r4 - r9, fp, ip, lr, pc}
	sub	fp, ip, #4
1:
	ldmia	r1!, {r3 - r9, ip}
	subges	r2, r2, #32
	stmgeia	r0!, {r3 - r9, ip}
	bge	1b
2:
	ldmea	fp, {r0, r4 - r9, fp, sp, pc}
