/*
 * Support of SDHCI devices for Microchip PIC32 SoC.
 *
 * Copyright (C) 2015 Microchip
 * Andrei Pistirica
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/errno.h>
#include <asm/arch-pic32/pic32.h>
#include <malloc.h>
#include <sdhci.h>

#define DRIVER_NAME "pic32-sdhci"

/* SDHC capabilities bits */
#define SDHC_CAPS_SLOT_TYPE_MASK		0xC0000000
#define  SLOT_TYPE_REMOVABLE			0x0
#define  SLOT_TYPE_EMBEDDED			0x1
#define  SLOT_TYPE_SHARED_BUS			0x2

/* SDHC SHARED BUS bits */
#define SDHC_SHARED_BUS_NR_CLK_PINS_MASK	0x7
#define SDHC_SHARED_BUS_NR_IRQ_PINS_MASK	0x30
#define SDHC_SHARED_BUS_CLK_PINS		0x10
#define SDHC_SHARED_BUS_IRQ_PINS		0x14

static int pic32_sdhci_set_shared(struct sdhci_host *host, u32 clk, u32 irq)
{
	unsigned int caps;
	u32 caps_slot_type;
	u32 bus;
	u32 clk_pins, irq_pins;

	/* Card slot connected on shared bus? */
	caps = sdhci_readl(host, SDHCI_CAPABILITIES);

	caps_slot_type = ((caps & SDHC_CAPS_SLOT_TYPE_MASK) >> 30);
	if (caps_slot_type != SLOT_TYPE_SHARED_BUS)
		return 0;

	bus = readl(host->ioaddr + SDHC_SHARED_BUS_CTRL);
	clk_pins = (bus & SDHC_SHARED_BUS_NR_CLK_PINS_MASK) >> 0;
	irq_pins = (bus & SDHC_SHARED_BUS_NR_IRQ_PINS_MASK) >> 4;

	/* select first clock */
	if (clk_pins & clk)
		bus |= (clk << SDHC_SHARED_BUS_CLK_PINS);

	/* select first interrupt */
	if (irq_pins & irq)
		bus |= (irq << SDHC_SHARED_BUS_IRQ_PINS);

	writel(bus, host->ioaddr + SDHC_SHARED_BUS_CTRL);
	return 0;
}

int pic32_sdhci_init(u32 regbase, u32 max_clk, u32 min_clk)
{
	struct sdhci_host *host = NULL;
	int ret = 0;

	host = (struct sdhci_host *)malloc(sizeof(*host));
	if (!host) {
		printf("sdhci host malloc fail!\n");
		ret = -ENOMEM;
		goto _out;
	}

	host->name	= DRIVER_NAME;
	host->ioaddr	= (void *)regbase;

	ret = pic32_sdhci_set_shared(host, 0x1, 0x1);
	if (ret)
		goto _out;

	return add_sdhci(host, max_clk, min_clk);

_out:
	return ret;
}
