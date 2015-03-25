/*
 * PIC32 SPI driver (refer linux spi-pic32.c)
 *
 * Copyright (c) 2014, Microchip Technology Inc.
 *      Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
/*#define DEBUG */
#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <linux/compat.h>
#include <asm/types.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch-pic32/pic32.h>
#include <asm/arch-pic32/ap.h>

/* SPI Register offsets */
#define SPI_CON		0x00
#define SPI_CON_CLR	0x04
#define SPI_CON_SET	0x08
#define SPI_STAT	0x10
#define SPI_STAT_CLR	0x14
#define SPI_BUF		0x20
#define SPI_BRG		0x30
#define SPI_CON2	0x40
#define SPI_CON2_CLR	0x44
#define SPI_CON2_SET	0x48

/* Bit fields in SPI_CON Register */
#define SPI_CON_RXI_SHIFT	0  /* Rx interrupt generation condition */
#define SPI_CON_TXI_SHIFT	2  /* TX interrupt generation condition */
#define SPI_CON_MSTEN		(1 << 5) /* Enable SPI Master */
#define SPI_CON_CKP		(1 << 6) /* active low */
#define SPI_CON_CKE		(1 << 8) /* Tx on falling edge */
#define SPI_CON_SMP		(1 << 9) /* Rx at middle or end of tx */
#define SPI_CON_BPW		0x03	 /* Bits per word/audio-sample */
#define SPI_CON_BPW_SHIFT	10
#define SPI_CON_SIDL		(1 << 13) /* STOP on idle */
#define SPI_CON_ON		(1 << 15) /* Macro enable */
#define SPI_CON_ENHBUF		(1 << 16) /* Enable enhanced buffering */
#define SPI_CON_MCLKSEL		(1 << 23) /* Select SPI Clock src */
#define SPI_CON_MSSEN		(1 << 28) /* SPI macro will drive SS */
#define SPI_CON_FRMEN		(1 << 31) /* Enable framing mode */

/* Bit fields in SPI_STAT Register */
#define STAT_RF_FULL		(1 << 0) /* RX fifo full */
#define STAT_TF_FULL		(1 << 1) /* TX fifo full */
#define STAT_TX_EMPTY		(1 << 3) /* standard buffer mode */
#define STAT_RF_EMPTY		(1 << 5) /* RX Fifo empty */
#define STAT_RX_OV		(1 << 6) /* err, s/w needs to clear */
#define STAT_SHIFT_REG_EMPTY	(1 << 7) /* Internal shift-reg empty */
#define STAT_TX_UR		(1 << 8) /* UR in Framed SPI modes */
#define STAT_BUSY		(1 << 11) /* Macro is processing tx or rx */
#define STAT_FRM_ERR		(1 << 12) /* Multiple Frame Sync pulse */
#define STAT_TF_LVL_MASK	0x1F
#define STAT_TF_LVL_SHIFT	16
#define STAT_RF_LVL_MASK	0x1F
#define STAT_RF_LVL_SHIFT	24

/* Bit fields in SPI_BRG Register */
#define SPI_BRG_MASK		0x1ff
#define SPI_BRG_SHIFT		0x0

/* Bit fields in SPI_CON2 Register */
#define SPI_INT_TX_UR_EN	0x0400 /* Enable int on Tx under-run */
#define SPI_INT_RX_OV_EN	0x0800 /* Enable int on Rx over-run */
#define SPI_INT_FRM_ERR_EN	0x1000 /* Enable frame err int */

/* Rx-fifo state for RX interrupt generation */
#define SPI_RX_FIFO_EMTPY	0x0
#define SPI_RX_FIFO_NOT_EMPTY	0x1 /* not empty */
#define SPI_RX_FIFO_HALF_FULL	0x2 /* full by one-half or more */
#define SPI_RX_FIFO_FULL	0x3 /* completely full */

/* TX-fifo state for TX interrupt generation */
#define SPI_TX_FIFO_ALL_EMPTY	0x0 /* completely empty */
#define SPI_TX_FIFO_EMTPY	0x1 /* empty */
#define SPI_TX_FIFO_HALF_EMPTY	0x2 /* empty by half or more */
#define SPI_TX_FIFO_NOT_FULL	0x3 /* atleast one empty */

/* Transfer bits-per-word */
#define SPI_BPW_8		0x0
#define SPI_BPW_16		0x1
#define SPI_BPW_32		0x2

/* SPI clock sources */
#define SPI_CLKSRC_PBCLK	0x0
#define SPI_CLKSRC_MCLK		0x1

struct pic32_spi {
	void __iomem		*regs;
	u32			fifo_n_byte; /* FIFO depth in bytes */
	u32			fifo_n_elm;

	/* Current SPI device specific */
	struct spi_slave	slave;
	u32			speed_hz; /* spi-clk rate */
	int			mode;

	/* Current message/transfer state */
	const void		*tx;
	const void		*tx_end;
	const void		*rx;
	const void		*rx_end;
	u32			len;

	/* SPI FiFo accessor */
	void (*rx_fifo)(struct pic32_spi *);
	void (*tx_fifo)(struct pic32_spi *);
};

static inline struct pic32_spi *to_pic32_spi(struct spi_slave *slave)
{
	return container_of(slave, struct pic32_spi, slave);
}

static inline void spi_enable_fifo(struct pic32_spi *pic32s)
{
	/* In enhanced buffer mode fifo-depth is fixed to 128bit(= 16B) */
	writel(SPI_CON_ENHBUF, pic32s->regs + SPI_CON_SET);
	pic32s->fifo_n_byte = 16;
}

static inline u32 spi_rx_fifo_level(struct pic32_spi *pic32s)
{
	u32 sr = readl(pic32s->regs + SPI_STAT);
	return (sr >> STAT_RF_LVL_SHIFT) & STAT_RF_LVL_MASK;
}

static inline u32 spi_tx_fifo_level(struct pic32_spi *pic32s)
{
	u32 sr = readl(pic32s->regs + SPI_STAT);
	return (sr >> STAT_TF_LVL_SHIFT) & STAT_TF_LVL_MASK;
}

static inline void spi_enable_master(struct pic32_spi *pic32s)
{
	writel(SPI_CON_MSTEN, pic32s->regs + SPI_CON_SET);
}

static inline void spi_enable_chip(struct pic32_spi *pic32s)
{
	writel(SPI_CON_ON, pic32s->regs + SPI_CON_SET);
}

static inline void spi_disable_chip(struct pic32_spi *pic32s)
{
	writel(SPI_CON_ON, pic32s->regs + SPI_CON_CLR);
}

static inline void spi_set_clk_mode(struct pic32_spi *pic32s, int mode)
{
	u32 v;

	v = readl(pic32s->regs);
	if (mode & SPI_CPOL)  /* idle HIGH */
		v |= SPI_CON_CKP;
	else
		v &= ~SPI_CON_CKP;

	if (mode & SPI_CPHA) /* tx at idle->active transition */
		v &= ~SPI_CON_CKE;
	else
		v |= SPI_CON_CKE;

	/* RX at end of tx */
	v |= SPI_CON_SMP;

	writel(v, pic32s->regs);
}

static inline void spi_set_ws(struct pic32_spi *pic32s, int ws)
{
	writel(SPI_CON_BPW << SPI_CON_BPW_SHIFT, pic32s->regs + SPI_CON_CLR);
	writel(ws << SPI_CON_BPW_SHIFT, pic32s->regs + SPI_CON_SET);
}

static inline void spi_drain_rx_buf(struct pic32_spi *pic32s)
{
	u32 sr;

	/* drain rx bytes until empty */
	for (;;) {
		sr = readl(pic32s->regs + SPI_STAT);
		if (sr & STAT_RF_EMPTY)
			break;

		(void)readl(pic32s->regs + SPI_BUF);
	}

	/* clear rx overflow */
	writel(STAT_RX_OV, pic32s->regs + SPI_STAT_CLR);
}

static inline void spi_set_clk_rate(struct pic32_spi *pic32s, u32 sck)
{
	u16 clk_div = 0;
	/* sck = clk_in / [2 * (clk_div + 1)]
	 * ie. clk_div = [clk_in / (2 * sck)] - 1
	 */
	clk_div = (pic32_get_pbclk(2) / (2 * sck)) - 1;
	clk_div &= SPI_BRG_MASK;

	/* apply baud */
	writel(clk_div, pic32s->regs + SPI_BRG);
}

static inline void spi_set_clk(struct pic32_spi *pic32s, int clk_id)
{
	switch (clk_id) {
	case SPI_CLKSRC_PBCLK:
		writel(SPI_CON_MCLKSEL, pic32s->regs + SPI_CON_CLR);
		break;
	case SPI_CLKSRC_MCLK:
		writel(SPI_CON_MCLKSEL, pic32s->regs + SPI_CON_SET);
		break;
	}
}

static inline void spi_clear_rx_fifo_overflow(struct pic32_spi *pic32s)
{
	writel(STAT_RX_OV, pic32s->regs + SPI_STAT_CLR);
}

static inline void spi_set_ss_auto(struct pic32_spi *pic32s, u16 mst_driven)
{
	/* spi controller can drive CS/SS during transfer depending on fifo
	 * fill-level. SS will stay asserted as long as TX fifo has something
	 * to transfer, else will be deasserted confirming completion of
	 * the ongoing transfer.
	 */
	if (mst_driven)
		writel(SPI_CON_MSSEN, pic32s->regs + SPI_CON_SET);
	else
		writel(SPI_CON_MSSEN, pic32s->regs + SPI_CON_CLR);
}

static inline void spi_disable_frame_mode(struct pic32_spi *pic32s)
{
	writel(SPI_CON_FRMEN, pic32s->regs + SPI_CON_CLR);
}

/* Return the max entries we can fill into tx fifo */
static inline u32 pic32_tx_max(struct pic32_spi *pic32s, int n_bytes)
{
	u32 tx_left, tx_room, rxtx_gap;

	tx_left = (pic32s->tx_end - pic32s->tx) / n_bytes;
	tx_room = pic32s->fifo_n_elm - spi_tx_fifo_level(pic32s);

	/*
	 * Another concern is about the tx/rx mismatch, we
	 * though to use (pic32s->fifo_n_byte - rxfl - txfl) as
	 * one maximum value for tx, but it doesn't cover the
	 * data which is out of tx/rx fifo and inside the
	 * shift registers. So a control from sw point of
	 * view is taken.
	 */
	rxtx_gap = ((pic32s->rx_end - pic32s->rx) -
		    (pic32s->tx_end - pic32s->tx)) / n_bytes;
	return min3(tx_left, tx_room, (u32) (pic32s->fifo_n_elm - rxtx_gap));
}

/* Return the max entries we should read out of rx fifo */
static inline u32 pic32_rx_max(struct pic32_spi *pic32s, int n_bytes)
{
	u32 rx_left = (pic32s->rx_end - pic32s->rx) / n_bytes;

	return min_t(u32, rx_left, spi_rx_fifo_level(pic32s));
}

#define BUILD_SPI_FIFO_RW(__name, __type, __bwl)		\
static void pic32_spi_rx_##__name(struct pic32_spi *pic32s)	\
{								\
	__type v;						\
	u32 mx = pic32_rx_max(pic32s, sizeof(__type));		\
	for (; mx; mx--) {					\
		v = read##__bwl(pic32s->regs + SPI_BUF);	\
		if (pic32s->rx_end - pic32s->len)		\
			*(__type *)(pic32s->rx) = v;		\
		pic32s->rx += sizeof(__type);			\
								\
	}							\
}								\
								\
static void pic32_spi_tx_##__name(struct pic32_spi *pic32s)	\
{								\
	__type v;						\
	u32 mx = pic32_tx_max(pic32s, sizeof(__type));		\
	for (; mx ; mx--) {					\
		v = (__type) ~0U;				\
		if (pic32s->tx_end - pic32s->len)		\
			v =  *(__type *)(pic32s->tx);		\
		write##__bwl(v, pic32s->regs + SPI_BUF);	\
		pic32s->tx += sizeof(__type);			\
	}							\
}
BUILD_SPI_FIFO_RW(byte, u8, b);
BUILD_SPI_FIFO_RW(word, u16, w);
BUILD_SPI_FIFO_RW(dword, u32, l);

static void pic32_spi_set_word_size(struct pic32_spi *pic32s, u8 wordlen)
{
	u8 spi_bpw;

	switch (wordlen) {
	default:
	case 8:
		pic32s->rx_fifo = pic32_spi_rx_byte;
		pic32s->tx_fifo = pic32_spi_tx_byte;
		spi_bpw = SPI_BPW_8;
		break;
	case 16:
		pic32s->rx_fifo = pic32_spi_rx_word;
		pic32s->tx_fifo = pic32_spi_tx_word;
		spi_bpw = SPI_BPW_16;
		break;
	case 32:
		pic32s->rx_fifo = pic32_spi_rx_dword;
		pic32s->tx_fifo = pic32_spi_tx_dword;
		spi_bpw = SPI_BPW_32;
		break;
	}

	/* calculate maximum elements fifo can hold */
	pic32s->fifo_n_elm = DIV_ROUND_UP(pic32s->fifo_n_byte, (wordlen >> 3));

	/* set bits per word */
	spi_set_ws(pic32s, spi_bpw);
}

int spi_claim_bus(struct spi_slave *slave)
{
	struct pic32_spi *pic32s = to_pic32_spi(slave);

	/* disable chip */
	spi_disable_chip(pic32s);
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
	struct pic32_spi *pic32s = to_pic32_spi(slave);

	/* disable chip */
	spi_disable_chip(pic32s);
}

static void print_spi_regs(void __iomem *regs)
{
#ifdef DEBUG
	printf("SPI_CON : 0x%08x\n", readl(regs + SPI_CON));
	printf("SPI_CON2: 0x%08x\n", readl(regs + SPI_CON2));
	printf("SPI_STAT: 0x%08x\n", readl(regs + SPI_STAT));
	printf("SPI_BRG : 0x%08x\n", readl(regs + SPI_BRG));
#endif
}

static void print_buffer8(const char *pfx, const void *buf, int len)
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
			debug("<%s%p> %s\n", pfx, buf + i - 0xf, printb);
			off = 0;
		}
	}

	if (off)
		debug("<%s%p> %s\n", pfx, buf + len - (len % 0xf), printb);
#endif
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen,
	     const void *tx_buf, void *rx_buf, unsigned long flags)
{
	unsigned long expire;
	int ret = 0, len = bitlen / 8;
	int timeout = 5 * get_tbclk();
	struct pic32_spi *pic32s = to_pic32_spi(slave);

	debug("msg tx %p, rx %p submitted of %d byte(s)\n",
	      tx_buf, rx_buf, len);
	print_buffer8("tx_buf", tx_buf, len);

	/* assert cs */
	if (flags & SPI_XFER_BEGIN) {
		debug("xfer begin\n");
		print_spi_regs(pic32s->regs);
		spi_cs_activate(slave);
	}

	/* set current transfer information */
	pic32s->tx = (const void *)tx_buf;
	pic32s->rx = (const void *)rx_buf;
	pic32s->tx_end = pic32s->tx + len;
	pic32s->rx_end = pic32s->rx + len;
	pic32s->len = len;

	/* enable chip */
	spi_enable_chip(pic32s);

	/* polling mode */
	expire = get_ticks() + timeout;
	for (;;) {
		pic32s->tx_fifo(pic32s);
		pic32s->rx_fifo(pic32s);

		/* received sufficient data */
		if (pic32s->rx >= pic32s->rx_end) {
			ret = 0;
			break;
		}

		if (get_ticks() > expire) {
			printf("xfer-timedout: Abort transfer.\n");
			flags |= SPI_XFER_END;
			ret = -1;
			break;
		}
	}


	if (flags & SPI_XFER_END) {
		/* deassert cs */
		spi_cs_deactivate(slave);
		debug("xfer end\n");
		print_spi_regs(pic32s->regs);
	}

	/* disable chip */
	spi_disable_chip(pic32s);

	print_buffer8("rx_buf", rx_buf, len);
	return ret;
}

static void pic32_spi_hw_init(struct pic32_spi *pic32s)
{
	/* disable module */
	spi_disable_chip(pic32s);

	/* drain rx buf */
	spi_drain_rx_buf(pic32s);

	/* enable enhanced buffer mode */
	spi_enable_fifo(pic32s);

	/* clear rx overflow indicator */
	spi_clear_rx_fifo_overflow(pic32s);

	/* disable frame synchronization mode */
	spi_disable_frame_mode(pic32s);

	/* enable master mode while disabled */
	spi_enable_master(pic32s);
}

/* This may be called twice for each spi dev */
struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
				  unsigned int max_hz, unsigned int mode)
{
	struct pic32_spi *pic32s;
	struct spi_slave *spi;

	debug("%s. spi-%u.%u, speed %u, mode %x\n", __func__,
	      bus, cs, max_hz, mode);

	pic32s = spi_alloc_slave(struct pic32_spi, bus, cs);
	if (pic32s == NULL)
		return NULL;

	pic32s->regs = (void __iomem *)(PIC32_BASE_SPI1 + (bus * 0x0200));
	spi = &pic32s->slave;

	/* Basic HW init */
	pic32_spi_hw_init(pic32s);

	/* disable chip */
	spi_disable_chip(pic32s);

	/* check maximum SPI clk rate */
	if (!max_hz) {
		debug("No max speed HZ parameter\n");
		return NULL;
	}

	/* set word size */
	pic32_spi_set_word_size(pic32s, spi->wordlen);

	/* set spi-clk rate */
	pic32s->speed_hz = max_hz;
	spi_set_clk(pic32s, SPI_CLKSRC_PBCLK);
	spi_set_clk_rate(pic32s, max_hz);

	/* set spi-clk mode */
	pic32s->mode = mode;
	spi_set_clk_mode(pic32s, mode);

	/* set gpio controlled CS */
	spi_set_ss_auto(pic32s, 0);
	return spi;
}

void spi_free_slave(struct spi_slave *slave)
{
	struct pic32_spi *pic32s;

	debug("%s\n", __func__);
	pic32s = to_pic32_spi(slave);

	/* diasable chip */
	spi_disable_chip(pic32s);

	/* reset reference */
	pic32s->speed_hz = 0;
	free(pic32s);
}

void spi_set_speed(struct spi_slave *slave, uint hz)
{
	struct pic32_spi *pic32s = to_pic32_spi(slave);

	debug("%s. speed %u\n", __func__, hz);
	/* set spi-clk rate */
	if (pic32s->speed_hz != hz) {
		spi_set_clk_rate(pic32s, hz);
		pic32s->speed_hz = hz;
	}
}

__attribute__ ((weak))
int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	return 1;
}

__attribute__ ((weak))
void spi_cs_activate(struct spi_slave *slave)
{
}

__attribute__ ((weak))
void spi_cs_deactivate(struct spi_slave *slave)
{
}

__attribute__ ((weak)) void spi_init(void)
{
	debug("%s:%s\n", __FILE__, __func__);
}
