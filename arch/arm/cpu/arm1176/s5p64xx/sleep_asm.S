/* linux/arch/arm/plat-s3c64xx/sleep.S
 *
 * Copyright (c) 2004 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C6410 Power Manager (Suspend-To-RAM) support
 *
 * Based on PXA/SA1100 sleep code by:
 *	Nicolas Pitre, (c) 2002 Monta Vista Software Inc
 *	Cliff Brake, (c) 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <asm/assembler.h>
#include <asm/ptrace.h>
#define ENTRY(name)				\
	.global name;				\
	.align 4,0x90;				\
name:

#include "sleep.h"
#define SWRESET_VALUE	0xE0102000

/* CONFIG_DEBUG_RESUME is dangerous if your bootloader does not
 * reset the UART configuration, only enable if you really need this!
*/
	.text

	/* s5pc110_cpu_save
	 *
	 * save enough of the CPU state to allow us to re-start
	 * pm.c code. as we store items like the sp/lr, we will
	 * end up returning from this function when the cpu resumes
	 * so the return value is set to mark this.
	 *
	 * This arangement means we avoid having to flush the cache
	 * from this code.
	 *
	 * entry:
	 *	r0 = pointer to save block
	 *
	 * exit:
	 *	r0 = 0 => we stored everything
	 *	     1 => resumed from sleep
	*/

ENTRY(s5pc110_cpu_save)

	stmfd	sp!, { r3 - r12, lr }

	mrc	p15, 0, r4, c13, c0, 0	@ FCSE/PID
	mrc	p15, 0, r5, c3, c0, 0	@ Domain ID
	mrc	p15, 0, r6, c2, c0, 0	@ Translation Table BASE0
	mrc	p15, 0, r7, c2, c0, 1	@ Translation Table BASE1
	mrc	p15, 0, r8, c2, c0, 2	@ Translation Table Control
	mrc	p15, 0, r9, c1, c0, 0	@ Control register
	mrc	p15, 0, r10, c1, c0, 1	@ Auxiliary control register
	mrc	p15, 0, r11, c1, c0, 2	@ Co-processor access controls
	mrc	p15, 0, r12, c10, c2, 0	@ Read PRRR
	mrc	p15, 0, r3, c10, c2, 1	@ READ NMRR	

	stmia	r0, { r3 - r13 }

	mov	r0, #0
	ldmfd	sp, { r3 - r12, pc }

	@@ return to the caller, after having the MMU
	@@ turned on, this restores the last bits from the
	@@ stack
	.align	4
resume_without_mmu:
	mov	r0, #1

	ldmfd	sp!, { r3 - r12, pc }

	.ltorg

	@@ the next bits sit in the .data segment, even though they
	@@ happen to be code... the s5pc110_sleep_save_phys needs to be
	@@ accessed by the resume code before it can restore the MMU.
	@@ This means that the variable has to be close enough for the
	@@ code to read it... since the .text segment needs to be RO,
	@@ the data segment can be the only place to put this code.

	.data



	.global	s5pc110_sleep_save_phys
s5pc110_sleep_save_phys:
	.word	0


	/* sleep magic, to allow the bootloader to check for an valid
	 * image to resume to. Must be the first word before the
	 * s5pc110_cpu_resume entry.
	*/

	.word	0x2bedf00d

	/* s5pc110_cpu_resume
	 *
	 * resume code entry for bootloader to call
	 *
	 * we must put this code here in the data segment as we have no
	 * other way of restoring the stack pointer after sleep, and we
	 * must not write to the code segment (code is read-only)
	*/

ENTRY(s5pc110_cpu_resume)
	mov	r0, #PSR_I_BIT | PSR_F_BIT | SVC_MODE
	msr	cpsr_c, r0

	@@ load UART to allow us to print the two characters for
	@@ resume debug

	mov	r1, #0
	mcr	p15, 0, r1, c8, c7, 0		@@ invalidate TLBs
	mcr	p15, 0, r1, c7, c5, 0		@@ invalidate I Cache

	ldr	r0, s5pc110_sleep_save_phys	@ address of restore block
	ldmia	r0, { r3 - r13 }

	mcr	p15, 0, r4, c13, c0, 0	@ FCSE/PID
	mcr	p15, 0, r5, c3, c0, 0	@ Domain ID
	
	mcr	p15, 0, r8, c2, c0, 2	@ Translation Table Control
	mcr	p15, 0, r7, c2, c0, 1	@ Translation Table BASE1
	mcr	p15, 0, r6, c2, c0, 0	@ Translation Table BASE0

	mcr	p15, 0, r10, c1, c0, 1	@ Auxiliary control register

	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 0	@ Invalidate I & D TLB

	mov	r0, #0			@ restore copro access controls
	mcr	p15, 0, r11, c1, c0, 2	@ Co-processor access controls
	mcr 	p15, 0, r0, c7, c5, 4	

	mcr	p15, 0, r12, c10, c2, 0	@ write PRRR
	mcr	p15, 0, r3, c10, c2, 1	@ write NMRR

	ldr	r2, =resume_without_mmu
	mov	pc, r2

	.ltorg

