/*
 * PIC32 board - Memory sub-system initialization code
 *
 * (c) 2015 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
*/

#include <config.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>

	.text
	.set noreorder
	.set mips32

	.globl	lowlevel_init
lowlevel_init:

	/*
	 * Establish Status Register
	 * (set BEV, clear ERL, clear EXL, clear IE)
	 */
	li	t1, 0x00400000
	mtc0	t1, CP0_STATUS

	/*
	 * Establish Cause
	 * (set IV bit)
	 */
	li	t1, 0x00800000
	mtc0	t1, CP0_CAUSE

	/* Establish Wired (and Random) */
	mtc0	zero, CP0_WIRED
	nop

	/* Initialize 'boot_flags' to zero. */
	li	a0, 0x00000000

	jr	ra
	nop
