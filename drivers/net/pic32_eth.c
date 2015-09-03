/*
 * (c) 2015 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <malloc.h>
#include <miiphy.h>
#include <phy.h>
#include <asm/io.h>

#include "pic32_eth.h"

/* local definitions */
#define MAX_RX_BUF_SIZE		1536
#define MAX_RX_DESCR		CONFIG_SYS_RX_ETH_BUFFER
#define MAX_TX_DESCR		2

struct pic32eth_device {
	struct eth_device netdev;
	struct eth_dma_desc rxd_ring[MAX_RX_DESCR];
	struct eth_dma_desc txd_ring[MAX_TX_DESCR];
	struct pic32_ectl_regs *ectl;
	struct pic32_emac_regs *emac;
	struct phy_device *phydev;
	phy_interface_t phyif;
	u32 phy_id; /* PHY addr */
	u32 rxd_idx; /* index of RX desc to read */
};

__attribute__ ((weak)) void board_netphy_reset(void)
{
}

/*
 * Initialize mii(MDIO) interface, discover which PHY is
 * attached to the device, and configure it properly.
 * If the PHY is not recognized, then return 0, else 1.
 */
static int mdiophy_init(struct pic32eth_device *pedev)
{
	struct mii_dev *mii;
	struct pic32_ectl_regs *eth = pedev->ectl;
	struct pic32_emac_regs *emac = pedev->emac;

	board_netphy_reset();

	/* disable RX, TX & all transactions */
	writel(ETHCON1_ON|ETHCON1_TXRTS|ETHCON1_RXEN, &eth->con1.clr);

	/* wait untill not BUSY */
	while (readl(&eth->stat.raw) & ETHSTAT_BUSY)
		;

	/* turn controller ON to access PHY over MII */
	writel(ETHCON1_ON, &eth->con1.set);

	udelay(DELAY_10MS);

	/* reset MAC */
	writel(EMAC_SOFTRESET, &emac->cfg1.set); /* reset assert */
	udelay(DELAY_10MS);
	writel(EMAC_SOFTRESET, &emac->cfg1.clr); /* reset deassert */

	/* initialize MDIO/MII */
	if (PHY_INTERFACE_MODE_RMII == pedev->phyif) {
		writel(EMAC_RMII_RESET, &emac->supp.set);
		udelay(DELAY_10MS);
		writel(EMAC_RMII_RESET, &emac->supp.clr);
	}

	pic32_mdio_init(PIC32_MDIO_NAME, (ulong)&emac->mii);

	mii = miiphy_get_dev_by_name(PIC32_MDIO_NAME);

	/* find & connect PHY */
	pedev->phydev = phy_connect(mii, pedev->phy_id,
				&pedev->netdev, pedev->phyif);
	if (pedev->phydev == NULL) {
		printf("%s: %s: Error, PHY connect\n", __FILE__, __func__);
		return 0;
	}

	/* Wait for phy to complete reset */
	udelay(DELAY_10MS);

	/* configure supported modes */
	pedev->phydev->supported = SUPPORTED_10baseT_Half|
			SUPPORTED_10baseT_Full|SUPPORTED_100baseT_Half|
			SUPPORTED_100baseT_Full|SUPPORTED_Autoneg;

	pedev->phydev->advertising = ADVERTISED_10baseT_Half|
		ADVERTISED_10baseT_Full|ADVERTISED_100baseT_Half|
		ADVERTISED_100baseT_Full|ADVERTISED_Autoneg;

	pedev->phydev->autoneg = AUTONEG_ENABLE;

	return 1;
}

/*
 * Configure MAC based on negotiated speed and duplex
 * reported by PHY.
 */
static int mac_adjust_link(struct eth_device *netdev)
{
	struct pic32eth_device *pedev = netdev->priv;
	struct phy_device *phydev = pedev->phydev;
	struct pic32_emac_regs *emac = pedev->emac;

	if (!phydev->link) {
		printf("%s: No link.\n", phydev->dev->name);
		return 0;
	}

	if (phydev->duplex) {
		writel(EMAC_FULLDPLEX, &emac->cfg2.set);
		writel(0x15, &emac->ipgt.raw);
	} else {
		writel(EMAC_FULLDPLEX, &emac->cfg2.clr);
		writel(0x12, &emac->ipgt.raw);
	}

	switch (phydev->speed) {
	case SPEED_100:
		writel(EMAC_RMII_SPD100, &emac->supp.set);
		break;
	case SPEED_10:
		writel(EMAC_RMII_SPD100, &emac->supp.clr);
		break;
	default:
		printf("%s: Speed was bad\n", phydev->dev->name);
		return 0;
	}

	printf("%s: PHY is %s with %dbase%s, %s\n",
	       netdev->name, phydev->drv->name,
	       phydev->speed, (phydev->port == PORT_TP) ? "T" : "X",
	       (phydev->duplex) ? "full" : "half");

	return 1;
}

static void mac_init(struct eth_device *netdev)
{
	struct pic32eth_device *pedev = netdev->priv;
	struct pic32_emac_regs *emac = pedev->emac;
	u32 stat = 0, v;
	u64 expire;

	v = EMAC_TXPAUSE|EMAC_RXPAUSE|EMAC_RXENABLE;
	writel(v, &emac->cfg1.raw);

	v = EMAC_EXCESS|EMAC_AUTOPAD|EMAC_PADENABLE|EMAC_CRCENABLE|
		EMAC_LENGTHCK|EMAC_FULLDPLEX;
	writel(v, &emac->cfg2.raw);

	/* recommended back-to-back inter-packet gap for 10 Mbps half duplex */
	writel(0x12, &emac->ipgt.raw);

	/* recommended non back-to-back intergap is 0xc12 */
	writel(0xc12, &emac->ipgr.raw);

	/* recommended collision window retry limit is 0x370F */
	writel(0x370f, &emac->clrt.raw);

	/* set maximum frame length: allow VLAN tagged frame */
	writel(0x600, &emac->maxf.raw);

	/* set the mac address */
	writel(netdev->enetaddr[0]|(netdev->enetaddr[1] << 8), &emac->sa2.raw);
	writel(netdev->enetaddr[2]|(netdev->enetaddr[3] << 8), &emac->sa1.raw);
	writel(netdev->enetaddr[4]|(netdev->enetaddr[5] << 8), &emac->sa0.raw);

	/* enable 10 Mbps operation */
	writel(EMAC_RMII_SPD100, &emac->supp.clr);

	/* wait until link status UP or deadline elapsed */
	expire = get_ticks() + get_tbclk() * 2;
	for (; get_ticks() < expire;) {
		stat = phy_read(pedev->phydev, pedev->phy_id, MII_BMSR);
		if (stat & BMSR_LSTATUS)
			break;
	}

	if (!(stat & BMSR_LSTATUS))
		printf("MAC: Link is DOWN!\n");

	/* delay to stabilize before any tx/rx */
	udelay(DELAY_10MS);
}

static void mac_reset(struct pic32eth_device *pedev)
{
	struct mii_dev *mii;
	struct pic32_emac_regs *emac = pedev->emac;

	/* Reset MAC */
	writel(EMAC_SOFTRESET, &emac->cfg1.raw);
	udelay(DELAY_10MS);

	/* clear reset */
	writel(0, &emac->cfg1.raw);

	/* Reset MII */
	mii = pedev->phydev->bus;
	if (mii && mii->reset)
		mii->reset(mii);
}

/* initializes the MAC and PHY, then establishes a link */
static void eth_ctrl_reset(struct pic32eth_device *pedev)
{
	u32 v;
	struct pic32_ectl_regs *eth = pedev->ectl;

	/* disable RX, TX & any other transactions */
	writel(ETHCON1_ON|ETHCON1_TXRTS|ETHCON1_RXEN, &eth->con1.clr);

	/* wait untill not BUSY */
	while (readl(&eth->stat.raw) & ETHSTAT_BUSY)
		;
	/* decrement received buffcnt to zero. */
	while (readl(&eth->stat.raw) & ETHSTAT_BUFCNT)
		writel(ETHCON1_BUFCDEC, &eth->con1.set);

	/* clear any existing interrupt event */
	writel(0xffffffff, &eth->irq.clr);

	/* clear RX/TX start address */
	writel(0xffffffff, &eth->txst.clr);
	writel(0xffffffff, &eth->rxst.clr);

	/* clear the receive filters */
	writel(0x00ff, &eth->rxfc.clr);

	/* set the receive filters
	 * ETH_FILT_CRC_ERR_REJECT
	 * ETH_FILT_RUNT_REJECT
	 * ETH_FILT_UCAST_ACCEPT
	 * ETH_FILT_MCAST_ACCEPT
	 * ETH_FILT_BCAST_ACCEPT
	 */
	v = ETHRXFC_BCEN|ETHRXFC_MCEN|ETHRXFC_UCEN|
		ETHRXFC_RUNTEN|ETHRXFC_CRCOKEN;
	writel(v, &eth->rxfc.set);

	/* turn controller ON to access PHY over MII */
	writel(ETHCON1_ON, &eth->con1.set);
}

static void eth_desc_init(struct pic32eth_device *pedev)
{
	u32 idx, bufsz;
	struct eth_dma_desc *rxd;
	struct pic32_ectl_regs *eth = pedev->ectl;

	pedev->rxd_idx = 0;
	for (idx = 0; idx < MAX_RX_DESCR; idx++) {
		rxd = &pedev->rxd_ring[idx];

		/* hw owned */
		rxd->hdr = EDH_NPV|EDH_EOWN|EDH_STICKY;

		/* packet buffer address */
		rxd->data_buff = __virt_to_phys(net_rx_packets[idx]);

		/* link to next desc */
		rxd->next_ed = __virt_to_phys(rxd + 1);

		/* reset status */
		rxd->stat1 = 0;
		rxd->stat2 = 0;

		/* decrement bufcnt */
		writel(ETHCON1_BUFCDEC, &eth->con1.set);
	}

	/* link last descr to beginning of list */
	rxd->next_ed = __virt_to_phys(&pedev->rxd_ring[0]);

	/* flush rx ring */
	__dcache_flush(pedev->rxd_ring, sizeof(pedev->rxd_ring));

	/* set rx desc-ring start address */
	writel((ulong)__virt_to_phys(&pedev->rxd_ring[0]), &eth->rxst.raw);

	/* RX Buffer size */
	bufsz = readl(&eth->con2.raw);
	bufsz &= ~(ETHCON2_RXBUFSZ << ETHCON2_RXBUFSZ_SHFT);
	bufsz |= ((MAX_RX_BUF_SIZE / 16) << ETHCON2_RXBUFSZ_SHFT);
	writel(bufsz, &eth->con2.raw);

	/*
	 * enable the receiver in hardware which allows hardware
	 * to DMA received pkts to the descriptor pointer address.
	 */
	writel(ETHCON1_RXEN, &eth->con1.set);
}

static void pic32eth_halt(struct eth_device *netdev)
{
	struct pic32eth_device *pedev = netdev->priv;
	struct pic32_ectl_regs *eth = pedev->ectl;
	struct pic32_emac_regs *emac = pedev->emac;

	/* Reset the phy if the controller is enabled */
	if (readl(&eth->con1.raw) & ETHCON1_ON)
		phy_reset(pedev->phydev);

	/* Shut down the PHY */
	phy_shutdown(pedev->phydev);

	/* Stop rx/tx */
	writel(ETHCON1_TXRTS|ETHCON1_RXEN, &eth->con1.clr);
	udelay(DELAY_10MS);

	/* reset MAC */
	writel(EMAC_SOFTRESET, &emac->cfg1.raw);

	/* clear reset */
	writel(0, &emac->cfg1.raw);
	udelay(DELAY_10MS);

	/* disable eth controller */
	writel(ETHCON1_ON, &eth->con1.clr);
	udelay(DELAY_10MS);

	/* wait until everything is down */
	while (readl(&eth->stat.raw) & ETHSTAT_BUSY)
		;

	/* clear any existing interrupt event */
	writel(0xffffffff, &eth->irq.clr);

	return;
}


static int pic32eth_init(struct eth_device *netdev, bd_t *bis)
{
	struct pic32eth_device *pedev = netdev->priv;

	/* configure controller */
	eth_ctrl_reset(pedev);

	/* reset emac */
	mac_reset(pedev);

	/* configure the PHY */
	phy_config(pedev->phydev);

	/* initialize MAC */
	mac_init(netdev);

	/* init RX descriptor; TX descriptor is taken care in xmit */
	eth_desc_init(pedev);

	/* Start up & update link status of PHY */
	phy_startup(pedev->phydev);

	/* adjust mac with phy link status */
	if (!mac_adjust_link(netdev)) {
		pic32eth_halt(netdev);
		return -1;
	}

	/* If there's no link, fail */
	return pedev->phydev->link ? 0 : -1;
}

static int pic32eth_xmit(struct eth_device *netdev, void *packet, int length)
{
	u64 deadline;
	struct eth_dma_desc *txd;
	struct pic32eth_device *pedev = netdev->priv;
	struct pic32_ectl_regs *eth = pedev->ectl;

	txd = &pedev->txd_ring[0];

	/* set proper flags & length in descriptor header */
	txd->hdr = EDH_SOP|EDH_EOP|EDH_EOWN|EDH_BCOUNT(length);

	/* pass buffer address to hardware */
	txd->data_buff = __virt_to_phys(packet);

	cond_debug("%s: %d / .hdr %x, .data_buff %x, .stat %x, .nexted %x\n",
		   __func__, __LINE__, txd->hdr, txd->data_buff, txd->stat2,
		   txd->next_ed);

	/* cache flush (packet) */
	__dcache_flush(packet, length);

	/* cache flush (txd) */
	__dcache_flush(txd, sizeof(*txd));

	/* pass descriptor table base to h/w */
	writel(__virt_to_phys(txd), &eth->txst.raw);

	/* ready to send enabled, hardware can now send the packet(s) */
	writel(ETHCON1_TXRTS|ETHCON1_ON, &eth->con1.set);

	/* wait untill tx has completed and h/w has released ownership
	 * of the tx descriptor or timeout elapsed.
	 */
	deadline = get_ticks() + get_tbclk();
	for (;;) {
		/* check timeout */
		if (get_ticks() > deadline)
			break;

		/* tx completed? */
		if (readl(&eth->con1.raw) & ETHCON1_TXRTS)
			continue;

		/* h/w not released ownership yet? */
		__dcache_invalidate(txd, sizeof(*txd));
		if (!(txd->hdr & EDH_EOWN))
			break;

		if (ctrlc())
			break;
	}

	return 0;
}

static int pic32eth_rx_poll(struct eth_device *netdev)
{
	int idx, top;
	u32 rx_stat;
	int rx_count, bytes_rcvd = 0;
	struct eth_dma_desc *rxd;
	struct pic32eth_device *pedev = netdev->priv;
	struct pic32_ectl_regs *eth = pedev->ectl;

	top = (pedev->rxd_idx + MAX_RX_DESCR - 1) % MAX_RX_DESCR;

	/* non-blocking receive loop - receive until nothing left to receive */
	for (idx = pedev->rxd_idx; idx != top; idx = (idx + 1) % MAX_RX_DESCR) {
		rxd = &pedev->rxd_ring[idx];

		/* check ownership */
		__dcache_invalidate(rxd, sizeof(*rxd));
		if (rxd->hdr & EDH_EOWN)
			break;

		if (rxd->hdr & EDH_SOP) {
			if (!(rxd->hdr & EDH_EOP)) {
				printf("%s: %s, rx pkt across multiple descr\n",
				       __FILE__, __func__);
				goto refill_one;
			}

			rx_stat = rxd->stat2;
			rx_count = RSV_RX_COUNT(rx_stat);

			cond_debug("%s: %d /rx-idx %i, .hdr=%x, .data_buff %x, .stat %x, .nexted %x\n",
				   __func__, __LINE__, idx, rxd->hdr,
				   rxd->data_buff, rxd->stat2, rxd->next_ed);

			/* we can do some basic checks */
			if ((!RSV_RX_OK(rx_stat)) || RSV_CRC_ERR(rx_stat)) {
				printf("%s: %s: Error, rx problem detected\n",
				       __FILE__, __func__);
				goto refill_one;
			}

			/* invalidate dcache */
			__dcache_invalidate(net_rx_packets[idx], rx_count);

			/* Pass the packet to protocol layer */
			net_process_received_packet(net_rx_packets[idx],
						    rx_count - 4);

			/* increment number of bytes rcvd (ignore CRC) */
			bytes_rcvd += (rx_count - 4);
refill_one:
			/* prepare for new receive */
			rxd->hdr = EDH_STICKY|EDH_NPV|EDH_EOWN;

			__dcache_flush(rxd, sizeof(*rxd));

			/* decrement rx pkt count */
			writel(ETHCON1_BUFCDEC, &eth->con1.set);

			cond_debug("%s: %d /fill-idx %i, .hdr=%x, .data_buff %x, .stat %x, .nexted %x / rx-idx %i\n",
				   __func__, __LINE__, idx, rxd->hdr,
				   rxd->data_buff, rxd->stat2, rxd->next_ed,
				   pedev->rxd_idx);

			pedev->rxd_idx = (pedev->rxd_idx + 1) % MAX_RX_DESCR;
		}

		if (ctrlc())
			break;
	}

	return bytes_rcvd;
}

/*
 * driver initialization function (called from board/<.c>)
 */
int pic32eth_initialize(bd_t *bis, ulong ioaddr, int phy_id, int phyif)
{
	struct eth_device *netdev;
	struct pic32eth_device *pedev;

	if (!ioaddr || (phyif == PHY_INTERFACE_MODE_NONE)) {
		printf("%s: %s: Error, invalid parameter, not initialized!\n",
		       __FILE__, __func__);
		return -1;
	}

	/* Setup private data */
	pedev = (struct pic32eth_device *)malloc(sizeof(*pedev));
	if (pedev == NULL) {
		printf("%s: %s: Error, malloc failed, data not initialized!\n",
		       __FILE__, __func__);
		return -1;
	}

	memset(pedev, 0, sizeof(*pedev));
	netdev = &pedev->netdev;

	/* initialize */
	pedev->phy_id	= phy_id;
	pedev->phyif	= phyif;
	pedev->ectl	= (struct pic32_ectl_regs *)(ioaddr);
	pedev->emac	= (struct pic32_emac_regs *)(ioaddr + PIC32_EMAC1CFG1);

	sprintf(netdev->name, "pic32_eth");
	netdev->iobase	= (ulong) ioaddr;
	netdev->priv	= pedev;
	netdev->init	= pic32eth_init;
	netdev->send	= pic32eth_xmit;
	netdev->recv	= pic32eth_rx_poll;
	netdev->halt	= pic32eth_halt;

	eth_register(netdev);

	return mdiophy_init(pedev);
}
