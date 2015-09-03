/*
 * pic32_mdio.c: PIC32 MDIO/MII driver, part of pic32_eth.c.
 *
 * Copyright 2015 Microchip Inc.
 *	Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <miiphy.h>
#include <phy.h>
#include <asm/io.h>

#include "pic32_eth.h"

#if !defined(CONFIG_MII)
# error "MIC32-PEMAC requires MII -- missing CONFIG_MII"
#endif

#if !defined(CONFIG_PHYLIB)
# error "PIC32-PEMAC requires PHYLIB -- missing CONFIG_PHYLIB"
#endif

static inline int wait_till_busy(struct pic32_mii_regs *mii, u32 mask, u32 exp)
{
	u64 expire = get_ticks() + get_tbclk() * 2;
	for (;;) {
		if ((readl(&mii->mind) & mask) == exp)
			return 0;
		if (get_ticks() > expire)
			break;
	}

	printf("%s, %s: wait timedout\n", __FILE__, __func__);
	return -1;
}

static int pic32_mdio_write(struct mii_dev *bus,
	int addr, int dev_addr, int reg, u16 value)
{
	u32 v;
	struct pic32_mii_regs *mii = bus->priv;

	/* Wait for the previous operation to finish */
	wait_till_busy(mii, MIIMIND_BUSY, 0);

	/* Put phyaddr and regaddr into MIIMADD */
	v = (addr << MIIMADD_PHYADDR_SHIFT)|(reg & MIIMADD_REGADDR);
	writel(v, &mii->madr);

	/* Initiate a write command */
	writel(value, &mii->mwtd);

	/* Wait 30 clock cycles for busy flag to be set */
	udelay(1);

	/* Wait for write to complete */
	wait_till_busy(mii, MIIMIND_BUSY, 0);

	return 0;
}

static int pic32_mdio_read(struct mii_dev *bus, int addr, int devaddr, int reg)
{
	u32 v;
	struct pic32_mii_regs *mii = bus->priv;

	/* Wait for the previous operation to finish */
	wait_till_busy(mii, MIIMIND_BUSY, 0);

	/* Put phyaddr and regaddr into MIIMADD */
	v = (addr << MIIMADD_PHYADDR_SHIFT)|(reg & MIIMADD_REGADDR);
	writel(v, &mii->madr);

	/* Initiate a read command */
	writel(MIIMCMD_READ, &mii->mcmd);

	/* Wait 30 clock cycles for busy flag to be set */
	udelay(1);

	/* Wait for read to complete */
	wait_till_busy(mii, MIIMIND_NOTVALID|MIIMIND_BUSY, 0);

	/* Clear the command register */
	writel(0, &mii->mcmd);

	/* Grab the value read from the PHY */
	v = readl(&mii->mrdd);
	return v;
}

static int pic32_mdio_reset(struct mii_dev *bus)
{
	struct pic32_mii_regs *mii = bus->priv;

	/* Reset MII (due to new addresses) */
	writel(MIIMCFG_RSTMGMT, &mii->mcfg);

	/* Wait for the operation to finish */
	while (readl(&mii->mind) & MIIMIND_BUSY)
		;

	/* Clear reset bit */
	writel(0, &mii->mcfg);

	/* Wait for the operation to finish */
	while (readl(&mii->mind) & MIIMIND_BUSY)
		;

	/* Set the MII Management Clock (MDC) - no faster than 2.5 MHz */
	writel(MIIMCFG_CLKSEL_DIV40, &mii->mcfg);

	/* Wait for the operation to finish */
	while (readl(&mii->mind) & MIIMIND_BUSY)
		;

	return 0;
}

int pic32_mdio_init(const char *name, ulong ioaddr)
{
	struct mii_dev *bus;

	bus = mdio_alloc();
	if (!bus) {
		printf("Failed to allocate PIC32-MDIO bus\n");
		return -1;
	}

	bus->read = pic32_mdio_read;
	bus->write = pic32_mdio_write;
	bus->reset = pic32_mdio_reset;
	sprintf(bus->name, name);

	bus->priv = (void *)ioaddr;

	return mdio_register(bus);
}
