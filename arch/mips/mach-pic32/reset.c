/*
 * (c) 2015 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch-pic32/pic32.h>

void __attribute__ ((optimize("-O0"))) _machine_restart(void)
{
	int __attribute__((unused)) throwaway;

	writel(0, SYSKEY);
	writel(0xAA996655, SYSKEY);
	writel(0x556699AA, SYSKEY);
	writel(0x1, RSWRST);
	throwaway = readl(RSWRST);

	while (1)
		;
}
