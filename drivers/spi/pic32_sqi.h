/*
 * Register definitions for the PIC32 SQI Controller
 *
 * Copyright (c) 2014, Microchip Technology Inc.
 *      Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __PIC32_SQI_H__
#define __PIC32_SQI_H__

/* SQI registers */
#define SQI_XIP_CTRL1_REG	0x00
#define SQI_XIP_CTRL2_REG	0x04
#define SQI_CONF_REG		0x08
#define SQI_CTRL_REG		0x0C
#define SQI_CLK_CTRL_REG	0x10
#define SQI_CMD_THRES_REG	0x14
#define SQI_INT_THRES_REG	0x18
#define SQI_INT_ENABLE_REG	0x1C
#define SQI_INT_STAT_REG	0x20
#define SQI_TX_DATA_REG		0x24
#define SQI_RX_DATA_REG		0x28
#define SQI_STAT1_REG		0x2C
#define SQI_STAT2_REG		0x30
#define SQI_BD_CTRL_REG		0x34
#define SQI_BD_CUR_ADDR_REG	0x38
#define SQI_BD_BASE_ADDR_REG	0x40
#define SQI_BD_STAT_REG		0x44
#define SQI_BD_POLL_CTRL_REG	0x48
#define SQI_BD_TX_DMA_STAT_REG	0x4C
#define SQI_BD_RX_DMA_STAT_REG	0x50
#define SQI_THRES_REG		0x54
#define SQI_INT_SIGEN_REG	0x58
#define SQI_XIP_CTRL3_REG	0x64
#define SQI_XIP_CTRL4_REG	0x68

/* XIP_CTRL1_REG fields */
#define SQI_XIP_CMD_TYPE	0x03
#define SQI_XIP_CMD_TYPE_SHIFT	0
#define SQI_XIP_ADDR_TYPE	0x03
#define SQI_XIP_ADDR_TYPE_SHIFT	2
#define SQI_XIP_MODE_TYPE	0x03
#define SQI_XIP_MODE_TYPE_SHIFT	4
#define SQI_XIP_DUMM_TYPE	0x03
#define SQI_XIP_DUMM_TYPE_SHIFT	6
#define SQI_XIP_DATA_TYPE	0x03
#define SQI_XIP_DATA_TYPE_SHIFT	8
#define SQI_XIP_OPCODE		0xff
#define SQI_XIP_OPCODE_SHIFT	10
#define SQI_XIP_ADDR_BYTE	0x07
#define SQI_XIP_ADDR_SHIFT	18
#define SQI_XIP_DUMMY_BYTE	0x07
#define SQI_XIP_DUMMY_SHIFT	21

/* XIP_CONF2_REG fields */
#define SQI_XIP_MODECODE	0xff
#define SQI_XIP_MODE_BYTE	0x03
#define SQI_XIP_MODE_BYTE_SHIFT	8
#define SQI_XIP_DEVSEL		0x03
#define SQI_XIP_DEVSEL_SHIFT	10

/* SQI_CONF_REG fields */
#define SQI_MODE_BOOT	0 /* Boot mode */
#define	SQI_MODE_PIO	1 /* CPU/PIO */
#define	SQI_MODE_DMA	2 /* descriptor-based DMA */
#define	SQI_MODE_XIP	3 /* XIP */
#define	SQI_MODE	0x7
#define SQI_MODE_SHIFT	0
#define SQI_CPHA	0x00000008
#define SQI_CPOL	0x00000010
#define SQI_LSBF	0x00000020
#define SQI_RXLATCH	0x00000080
#define SQI_SERMODE	0x00000100
#define SQI_WP_EN	0x00000200
#define SQI_HOLD_EN	0x00000400
#define SQI_BURST_EN	0x00001000
#define SQI_CS_CTRL_HW	0x00008000
#define SQI_SOFT_RESET	0x00010000
#define SQI_LANES_SHIFT	20
#define SQI_LANE_SINGLE	0   /* Single Lane */
#define SQI_LANE_DUAL	1   /* Dual Lane */
#define SQI_LANE_QUAD	2   /* Quad Lane */
#define SQI_EN		0x00800000
#define SQI_CSEN_SHIFT	24
#define SQI_EN_V1	0x80000000

/* SQI_CLK_CTRL_REG fields */
#define SQI_CLK_EN		0x01
#define SQI_CLK_STABLE		0x02
#define SQI_CLKDIV_SHIFT	8
#define SQI_CLKDIV		0xff

/* SQI_INT_THR/CMD_THR_REG */
#define TXTHR_MASK	0x1f
#define TXTHR_SHIFT	8
#define RXTHR_MASK	0x1f
#define RXTHR_SHIFT	0

/* SQI_INT_EN/INT_STAT/INT_SIG_EN_REG */
#define TXEMPTY		0x0001
#define TXFULL		0x0002
#define TXTHR		0x0004
#define RXEMPTY		0x0008
#define RXFULL		0x0010
#define RXTHR		0x0020
#define CONFULL		0x0040
#define CONEMPTY	0x0080
#define CONTHR		0x0100
#define BDDONE		0x0200 /* current BD processing complete */
#define PKTCOMP		0x0400 /* packet processing complete */
#define DMAERR		0x0800 /* error */

/* SQI_BD_CTRL_REG */
#define DMA_EN		0x01 /* enable dma engine */
#define POLL_EN		0x02 /* enable poll engine */
#define BDP_START	0x04 /* start BD processor */

/* XIP_CTRL3/CTRL4_REG */
#define SQI_XIP_INIT_CODE_SHIFT	0
#define SQI_XIP_INIT_TYPE_SHIFT	24
#define SQI_XIP_INIT_CODE_CNT_SHIFT	26

/* H/W BUFFER DESCRIPTOR */
struct hw_bd {
	u32 bd_ctrl;
	u32 bd_status;	/* reserved */
	u32 bd_addr;
	u32 bd_nextp;
};

/* BD_CTRL fields */
#define BD_BUFLEN	0x1ff
#define BD_CBD_INT_EN	0x00010000 /* current BD is processed */
#define BD_PKT_INT_EN	0x00020000 /* All BDs of PKT processed */
#define BD_LIFM		0x00040000 /* last data of pkt */
#define BD_LAST		0x00080000 /* end of list */
#define BD_DATA_RECV	0x00100000 /* receive data */
#define BD_DDR		0x00200000 /* DDR mode */
#define BD_DUAL		0x00400000 /* Dual SPI */
#define BD_QUAD		0x00800000 /* Quad SPI */
#define BD_LSBF		0x02000000 /* LSB First */
#define BD_STAT_CHECK	0x08000000 /* Status poll */
#define BD_DEVSEL_SHIFT	28	/* CS */
#define BD_CS_DEASSERT	0x40000000 /* de-assert CS after current BD */
#define BD_EN		0x80000000 /* BD owned by H/W */

/* Software buffer descriptor */
struct sw_desc {
	struct list_head list;
	struct hw_bd *bd;
	dma_addr_t bd_dma;
	void *buf;
	dma_addr_t buf_dma;
	unsigned long flags;
	void *xfer_buf;	/* dest buff for rx */
	u32 xfer_len;	/* len to copy for rx */
};

/* Global constants */
#define SQI_BD_BUF_SIZE		256 /* 256-byte */
#define SQI_BD_COUNT		((128 << 10) / SQI_BD_BUF_SIZE)
#define SQI_BD_COUNT_MASK	(SQI_BD_COUNT - 1)
#define SQI_MIN_LEN_DMA		CONFIG_SYS_CACHELINE_SIZE

#define SQI_MAX_CS		2

struct pic32_sqi {
	void __iomem		*regs;
	int			num_cs;
#define SQI_IP_V1	0x01 /* SQI IP version */
#define SQI_INIT	0x02
	u32			flags;

	/* Resources */
	void			*buffer;
	dma_addr_t		buffer_dma;
	struct list_head	bd_list;
	struct list_head	bd_list_used;

	/* Current SPI device specific */
	struct spi_slave	slave;
	u32			speed_hz; /* spi-clk rate */
	u8			spi_mode;
};

static inline struct pic32_sqi *to_pic32_sqi(struct spi_slave *spi)
{
	return container_of(spi, struct pic32_sqi, slave);
}

static inline void sqi_soft_reset(struct pic32_sqi *sqi)
{
	u32 v;
	unsigned long count = 5000;

	/* assert soft-reset */
	writel(SQI_SOFT_RESET, sqi->regs + SQI_CONF_REG);

	/* wait until clear */
	for (;;) {
		v = readl(sqi->regs + SQI_CONF_REG);
		if (!(v & SQI_SOFT_RESET))
			break;

		if (--count == 0) {
			printf("-- soft-reset timeout --\n");
			break;
		}
	}
}

static inline void sqi_enable_spi(struct pic32_sqi *sqi)
{
	u32 v = readl(sqi->regs + SQI_CONF_REG);

	v |= (sqi->flags & SQI_IP_V1) ? SQI_EN_V1 : SQI_EN;
	writel(v, sqi->regs + SQI_CONF_REG);
}

static inline void sqi_disable_spi(struct pic32_sqi *sqi)
{
	u32 v = readl(sqi->regs + SQI_CONF_REG);

	v &= (sqi->flags & SQI_IP_V1) ? ~SQI_EN_V1 : ~SQI_EN;
	writel(v, sqi->regs + SQI_CONF_REG);
}

static inline void sqi_set_mode(struct pic32_sqi *sqi, u8 sqi_mode)
{
	u32 v;

	v = readl(sqi->regs + SQI_CONF_REG);
	v &= ~(SQI_MODE << SQI_MODE_SHIFT);
	v |= (sqi_mode << SQI_MODE_SHIFT);
	writel(v, sqi->regs + SQI_CONF_REG);
}

static inline void sqi_set_spi_mode(struct pic32_sqi *sqi, int spi_mode)
{
	u32 v;

	v = readl(sqi->regs + SQI_CONF_REG);
	v &= ~(SQI_CPOL|SQI_CPHA|SQI_LSBF);

	/* active low ? */
	if (spi_mode & SPI_CPOL)
		v |= SQI_CPOL;

	/* rx at end of tx */
	v |= SQI_CPHA;

	/* LSB first ? */
	if (spi_mode & SPI_LSB_FIRST)
		v |= SQI_LSBF;

	writel(v, sqi->regs + SQI_CONF_REG);
}

static inline void sqi_enable_clk(struct pic32_sqi *sqi)
{
	u32 v;

	/* enable clock */
	v = readl(sqi->regs + SQI_CLK_CTRL_REG);
	writel(v|SQI_CLK_EN, sqi->regs + SQI_CLK_CTRL_REG);

	/* wait for stability */
	for (;;) {
		v = readl(sqi->regs + SQI_CLK_CTRL_REG);

		if (v & SQI_CLK_STABLE)
			break;
	}
}

static inline void sqi_disable_clk(struct pic32_sqi *sqi)
{
	u32 v;

	v = readl(sqi->regs + SQI_CLK_CTRL_REG);
	v &= ~SQI_CLK_EN;
	writel(v, sqi->regs + SQI_CLK_CTRL_REG);
}

static inline void sqi_set_clk_rate(struct pic32_sqi *sqi, u32 sck)
{
	u32 v, clk_in_rate;
	u16 div;

	v = readl(sqi->regs + SQI_CLK_CTRL_REG);
	v &= ~(SQI_CLK_STABLE|(SQI_CLKDIV << SQI_CLKDIV_SHIFT));

	/* sck = clk_in / [2 * div]
	 * ie. div = clk_in / (2 * sck)
	 */
	clk_in_rate = pic32_get_sqiclk();
	div = clk_in_rate / (2 * sck);
	div &= SQI_CLKDIV;

	/* apply divider */
	v |= (div << SQI_CLKDIV_SHIFT);
	writel(v, sqi->regs + SQI_CLK_CTRL_REG);

	/* wait for stability, if enabled */
	for (;;) {
		v = readl(sqi->regs + SQI_CLK_CTRL_REG);
		 /* clk enabled ? */
		if (!(v & SQI_CLK_EN))
			break;

		if (v & SQI_CLK_STABLE)
			break;
	}
}

static inline void sqi_set_rx_thr(struct pic32_sqi *sqi, int fifo_lvl)
{
	u32 v = readl(sqi->regs + SQI_CMD_THRES_REG);

	v &= ~(RXTHR_MASK << RXTHR_SHIFT);
	v |= ((fifo_lvl & RXTHR_MASK) << RXTHR_SHIFT);
	writel(v, sqi->regs + SQI_CMD_THRES_REG);
}

static inline void sqi_set_rx_intr(struct pic32_sqi *sqi, int fifo_lvl)
{
	u32 v = readl(sqi->regs + SQI_INT_THRES_REG);

	v &= ~(RXTHR_MASK << RXTHR_SHIFT);
	v |= (fifo_lvl << RXTHR_SHIFT);
	writel(v, sqi->regs + SQI_INT_THRES_REG);

	v = readl(sqi->regs + SQI_CMD_THRES_REG);
	v &= ~(RXTHR_MASK << RXTHR_SHIFT);
	v |= (fifo_lvl << RXTHR_SHIFT);
	writel(v, sqi->regs + SQI_CMD_THRES_REG);
}

static inline void sqi_set_tx_thr(struct pic32_sqi *sqi, int fifo_lvl)
{
	u32 v = readl(sqi->regs + SQI_CMD_THRES_REG);

	v &= ~(TXTHR_MASK << TXTHR_SHIFT);
	v |= ((fifo_lvl & TXTHR_MASK) << TXTHR_SHIFT);
	writel(v, sqi->regs + SQI_CMD_THRES_REG);
}

static inline void sqi_set_tx_intr(struct pic32_sqi *sqi, int fifo_lvl)
{
	u32 v = readl(sqi->regs + SQI_INT_THRES_REG);

	v &= ~(TXTHR_MASK << TXTHR_SHIFT);
	v |= (fifo_lvl << TXTHR_SHIFT);
	writel(v, sqi->regs + SQI_INT_THRES_REG);
}

static inline void sqi_enable_int(struct pic32_sqi *sqi)
{
	u32 mask = DMAERR;

	/* BD */
	mask |= BDDONE|PKTCOMP;
	writel(mask, sqi->regs + SQI_INT_ENABLE_REG);
	writel(mask, sqi->regs + SQI_INT_SIGEN_REG);
}

static inline void sqi_disable_int(struct pic32_sqi *sqi)
{
	writel(0, sqi->regs + SQI_INT_ENABLE_REG);
	writel(0, sqi->regs + SQI_INT_SIGEN_REG);
	writel(0, sqi->regs + SQI_INT_STAT_REG);
}

static inline void sqi_enable_dma(struct pic32_sqi *sqi)
{
	writel(DMA_EN|POLL_EN|BDP_START, sqi->regs + SQI_BD_CTRL_REG);
}

static inline void sqi_disable_dma(struct pic32_sqi *sqi)
{
	writel(0, sqi->regs + SQI_BD_CTRL_REG);
}

static inline int sqi_poll_for_completion(struct pic32_sqi *sqi, u32 timeout)
{
	u32 stat;

	/* Poll for packet completion */
	for (; timeout; --timeout) {
		stat = readl(sqi->regs + SQI_INT_STAT_REG);
		if (stat & BDDONE) {
			debug("bddone\n");
			break;
		} else if (stat & PKTCOMP) {
			debug("pktcomp\n");
			break;
		}

		if (ctrlc()) {
			debug("ctrl+c\n");
			timeout = 0;
		}
	}

	return timeout;
}

static inline void show_busy_list(struct pic32_sqi *sqi)
{
#ifdef DEBUG
	struct hw_bd *bd;
	struct sw_desc *desc;

	list_for_each_entry(desc, &sqi->bd_list_used, list) {
		bd = desc->bd;
		printf("bd %p: .ctrl %08x .addr %08x .stat %08x .next %08x\n",
		       (void *)bd, bd->bd_ctrl, bd->bd_addr,
		       bd->bd_status, bd->bd_nextp);
	}
#endif
}

static inline void dump_regs(struct pic32_sqi *sqi)
{
#ifdef DEBUG
	printf("SQICFG: %08x\n", readl(sqi->regs + SQI_CONF_REG));
	printf("SQICTL: %08x\n", readl(sqi->regs + SQI_CTRL_REG));
	printf("SQICLK: %08x\n", readl(sqi->regs + SQI_CLK_CTRL_REG));
	printf("SQICMDTH: %08x\n", readl(sqi->regs + SQI_CMD_THRES_REG));
	printf("SQIINTTH: %08x\n", readl(sqi->regs + SQI_INT_THRES_REG));
	printf("SQIINTEN: %08x\n", readl(sqi->regs + SQI_INT_ENABLE_REG));
	printf("SQIINTSIGEN: %08x\n", readl(sqi->regs + SQI_INT_SIGEN_REG));
	printf("SQIINTST: %08x\n", readl(sqi->regs + SQI_INT_STAT_REG));
	printf("SQIBDCON: %08x\n", readl(sqi->regs + SQI_BD_CTRL_REG));
	printf("SQIBDCUR: %08x\n", readl(sqi->regs + SQI_BD_CUR_ADDR_REG));
	printf("SQIBDBAS: %08x\n", readl(sqi->regs + SQI_BD_BASE_ADDR_REG));
#endif
}

static inline void pic32_debug_sqi(struct pic32_sqi *sqi, const char *fmt)
{
#ifdef DEBUG
	u32 bd_current, bd_status, tx_status, rx_status;
	u32 int_status, int_enable, int_sig_enable;

	bd_current	= readl(sqi->regs + SQI_BD_CUR_ADDR_REG);
	bd_status	= readl(sqi->regs + SQI_BD_STAT_REG);
	tx_status	= readl(sqi->regs + SQI_BD_TX_DMA_STAT_REG);
	rx_status	= readl(sqi->regs + SQI_BD_RX_DMA_STAT_REG);
	int_status	= readl(sqi->regs + SQI_INT_STAT_REG);
	int_enable	= readl(sqi->regs + SQI_INT_ENABLE_REG);
	int_sig_enable	= readl(sqi->regs + SQI_INT_SIGEN_REG);

	printf("bd_cur %x: %s/ bd_status 0x%x/ tx_stat 0x%x / rx_stat 0x%x\n",
	       bd_current, fmt, bd_status, tx_status, rx_status);

	printf("intstatus %08x, inten %08x intsig %08x\n",
	       int_status, int_enable, int_sig_enable);

	show_busy_list(sqi);
#endif
}

static inline void print_buffer8(const char *pfx, const void *buf, int len)
{
#ifdef DEBUG
	char printb[32 + 32];
	u8 *b8 = (u8 *)buf;
	int i = 0, off = 0;

	if (buf == NULL)
		return;

	for (i = 0; i < len; i++) {
		off += sprintf(printb + off, "%02x ", b8[i]);
		if ((i & 0xf) == 0xf) {
			printf("<%s%p> %s\n", pfx, buf + i - 0xf, printb);
			off = 0;
		}
	}

	if (off)
		printf("<%s%p> %s\n", pfx, buf + len - (len % 0xf), printb);
#endif
}

#endif /*__PIC32_SQI_H__*/
