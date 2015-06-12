/*
 * PIC32 SQI controller driver (refer spi-pic32.c)
 *
 * Copyright (c) 2014, Microchip Technology Inc.
 *      Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
/*#define DEBUG*/
#include <dm.h>
#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <linux/compat.h>
#include <linux/list.h>
#include <asm/types.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch-pic32/pic32.h>
#include <asm/arch-pic32/ap.h>

#include "pic32_sqi.h"

static inline struct sw_desc *sw_desc_get(struct pic32_sqi *sqi)
{
	struct sw_desc *b;

	if (list_empty(&sqi->bd_list)) {
		b = NULL;
		goto done;
	}

	b = list_first_entry(&sqi->bd_list, struct sw_desc, list);
	list_del(&b->list);
	list_add_tail(&b->list, &sqi->bd_list_used);
done:
	return b;
}

static void sw_desc_put(struct pic32_sqi *sqi, struct sw_desc *bd)
{
	list_del(&bd->list);
	list_add(&bd->list, &sqi->bd_list);
}

static int sw_desc_fill(struct sw_desc *desc,
	const void *tx_buf, const void *rx_buf,
	int len, int remaining, u32 bd_ctrl, int zerocopy)
{
	struct hw_bd *bd;
	int offset = len - remaining;

	desc->xfer_len = min_t(u32, remaining, SQI_BD_BUF_SIZE);
	desc->xfer_buf = 0;

	/* Buffer Descriptor */
	bd = desc->bd;

	/* BD CTRL */
	bd->bd_ctrl = bd_ctrl;
	bd->bd_ctrl |= desc->xfer_len;

	/* BD STAT */
	bd->bd_status = 0;

	/* BD ADDR: zero-copy / bounce-buffer ? */
	if (zerocopy) {
		bd->bd_addr = tx_buf ? (ulong)tx_buf : (ulong)rx_buf;
		bd->bd_addr = pic32_virt_to_phys((void *)bd->bd_addr + offset);
		goto done_out;
	}

	/* use pre-allocated dma buffer for bouncing */
	if (tx_buf)
		memcpy(desc->buf, (void *)tx_buf + offset, desc->xfer_len);
	else
		desc->xfer_buf = (void *)rx_buf + offset;

	bd->bd_addr = desc->buf_dma;

done_out:
	return desc->xfer_len;
}

static int pic32_sqi_one_transfer(struct pic32_sqi *sqi,
	const void *tx_buf, const void *rx_buf, int len, int cs)
{
	struct sw_desc *desc;
	int remaining, ret, zerocopy = 0;
	u32 bd_ctrl;

	/* BD CTRL */
	bd_ctrl = 0;

	/* Device selection */
	bd_ctrl |= (cs << BD_DEVSEL_SHIFT);

#if 0
	if (nbits & SPI_NBITS_QUAD)
		bd_ctrl |= BD_QUAD;
	else if (nbits & SPI_NBITS_DUAL)
		bd_ctrl |= BD_DUAL;
#endif

	/* Transfer/Receive mode selection */
	if (rx_buf) {
		bd_ctrl |= BD_DATA_RECV;

		if (len >= SQI_MIN_LEN_DMA) {
			invalidate_dcache_range((ulong)rx_buf,
						(ulong)rx_buf + len);
			zerocopy = 1;
		}

		goto mapping_done;
	}

	if (len >= SQI_MIN_LEN_DMA) {
		flush_dcache_range((ulong)tx_buf, (ulong)tx_buf + len);
		zerocopy = 1;
	}
mapping_done:

	/* LSB First */
	if (sqi->spi_mode & SPI_LSB_FIRST)
		bd_ctrl |= BD_LSBF;

	/* Ownership */
	bd_ctrl |= BD_EN;

	for (remaining = len; remaining;) {
		desc = sw_desc_get(sqi);
		if (!desc)
			break;
		ret = sw_desc_fill(desc, tx_buf, rx_buf, len,
				    remaining, bd_ctrl, zerocopy);
		remaining -= ret;
	}

	return 0;
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen,
	     const void *tx_buf, void *rx_buf, unsigned long flags)
{
	int ret = 0;
	int cs = slave->cs;
	struct hw_bd *bd;
	struct sw_desc *desc, *n;
	struct pic32_sqi *sqi;
	int len = bitlen >> 3;

	debug("new message %p, %p submitted, len %d\n", tx_buf, rx_buf, len);
	print_buffer8("tx:", tx_buf, len);

	if (!rx_buf && !tx_buf)
		return 0;

	if (rx_buf)
		flags |= SPI_XFER_END;

	sqi = to_pic32_sqi(slave);

	/* prepare BD(s) */
	ret = pic32_sqi_one_transfer(sqi, tx_buf, rx_buf, len, cs);
	if (ret) {
		printf("xfer failed.\n");
		goto xfer_done;
	}

	/* mark LAST_BD to last of list */
	if (flags & SPI_XFER_END) {
		debug("xfer_end: deassert on last BD\n");
		desc = list_entry(sqi->bd_list_used.prev,
				  struct sw_desc, list);
		bd = desc->bd;
		bd->bd_ctrl |= BD_LAST;
		bd->bd_ctrl |= BD_CS_DEASSERT;
		bd->bd_ctrl |= BD_LIFM|BD_PKT_INT_EN;
	} else {
		debug("just queued\n");
		ret = 0;
		goto out_done;
	}

	show_busy_list(sqi);

	/* set BD base address */
	desc = list_first_entry(&sqi->bd_list_used, struct sw_desc, list);
	writel(desc->bd_dma, sqi->regs + SQI_BD_BASE_ADDR_REG);

	sqi_enable_spi(sqi);

	sqi_enable_int(sqi);

	sqi_enable_dma(sqi);

	dump_regs(sqi);

	ret = sqi_poll_for_completion(sqi, 0x1000000);
	if (ret <= 0) {
		printf("wait timedout/interrupted\n");
		pic32_debug_sqi(sqi, " -- TIMEDOUT -- ");
		goto xfer_done;
	}

	show_busy_list(sqi);
	dump_regs(sqi);

	/* post-process: copy received bytes to rx_buf */
	list_for_each_entry(desc, &sqi->bd_list_used, list) {
		if (!desc->xfer_buf)
			continue;
		memcpy(desc->xfer_buf, desc->buf, desc->xfer_len);
	}

	ret = 0;

xfer_done:
	sqi_disable_int(sqi);

	sqi_disable_dma(sqi);

	sqi_disable_spi(sqi);

	print_buffer8("rx:", rx_buf, len);

	/* release all used bds */
	list_for_each_entry_safe_reverse(desc, n, &sqi->bd_list_used, list)
		sw_desc_put(sqi, desc);
out_done:
	return ret;
}


static int sw_desc_ring_alloc(struct pic32_sqi *sqi)
{
	int i = 0;
	int bd_size, buf_size = SQI_MIN_LEN_DMA;
	struct hw_bd *bd;
	void *tmp_buf, *buf;
	dma_addr_t tmp_buf_dma;
	struct sw_desc *desc, *d;

	/* allocate h/w descriptor and bounce-buffer */
	bd_size = sizeof(struct hw_bd) * SQI_BD_COUNT;
	buf = malloc(bd_size + (SQI_BD_COUNT * buf_size));
	if (!buf) {
		printf("error: allocating hw-bd\n");
		return -ENOMEM;
	}

	sqi->buffer = (void *)pic32_virt_to_uncac((ulong)buf);
	sqi->buffer_dma = pic32_virt_to_phys(buf);
	tmp_buf = sqi->buffer + bd_size;
	tmp_buf_dma = sqi->buffer_dma + bd_size;

	/* allocate software descriptor */
	desc = malloc(sizeof(*d) * SQI_BD_COUNT);
	if (!desc) {
		free(buf);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&sqi->bd_list);
	INIT_LIST_HEAD(&sqi->bd_list_used);

	/* prepare SWBDs */
	bd = (struct hw_bd *)sqi->buffer;
	for (i = 0, d = &desc[0]; i < SQI_BD_COUNT; i++, d++) {
		INIT_LIST_HEAD(&d->list);
		d->bd = &bd[i];
		d->bd_dma = sqi->buffer_dma + ((void *)d->bd - (void *)bd);
		d->buf = tmp_buf + (i * buf_size);
		d->buf_dma = tmp_buf_dma + (i * buf_size);
		list_add_tail(&d->list, &sqi->bd_list);
	}

	/* prepare BD: link buffer-addr & chain to next BD(s) */
	bd[0].bd_addr = desc[0].buf_dma;
	for (i = 1, d = &desc[i]; i < SQI_BD_COUNT; i++, d++) {
		bd[i].bd_addr = d->buf_dma;
		bd[i - 1].bd_nextp = d->bd_dma;
	}

	return 0;
}

static inline void sw_desc_ring_free(struct pic32_sqi *sqi)
{
	struct sw_desc *b;

	/* remove DMA buffer & DMA descriptor */
	free(sqi->buffer);

	/* remove s/w buffer desc */
	b = list_first_entry(&sqi->bd_list, struct sw_desc, list);
	free(b);
}

static void pic32_sqi_hw_init(struct pic32_sqi *sqi, u8 mode)
{
	u32 v;

	/* soft reset */
	sqi_soft_reset(sqi);

	/* disable all interrupts */
	sqi_disable_int(sqi);

	/* tx fifo interrupt threshold */
	sqi_set_tx_thr(sqi, 1);
	sqi_set_tx_intr(sqi, 1);

	/* rx fifo interrupt threshold */
	sqi_set_rx_thr(sqi, 1);
	sqi_set_rx_intr(sqi, 1);

	/* enable default interrupt */
	sqi_enable_int(sqi);

	/* default configuration */
	v = readl(sqi->regs + SQI_CONF_REG);

	/* set mode */
	v &= ~(SQI_MODE << SQI_MODE_SHIFT);
	v |= (mode << SQI_MODE_SHIFT);
	writel(v, sqi->regs + SQI_CONF_REG);

	/* DATAEN - SQIID0-ID3 */
	v |= (SQI_LANE_QUAD << SQI_LANES_SHIFT);

	/* burst/INCR4 enable */
	v |= SQI_BURST_EN;

	/* CSEN - all CS */
	v |= (((1 << sqi->num_cs) - 1) << SQI_CSEN_SHIFT);

	if (sqi->flags & SQI_IP_V1) {
		/* SERMODE disabled */
		v &= ~SQI_SERMODE;

		/* CSCON - hardware */
		v |= SQI_CS_CTRL_HW;

		/* RXLATCH */
		v |= SQI_RXLATCH;
	}
	writel(v, sqi->regs + SQI_CONF_REG);

	/* write poll count */
	writel(0, sqi->regs + SQI_BD_POLL_CTRL_REG);

	/* disable module */
	sqi_disable_spi(sqi);

	sqi->speed_hz = 0;
	sqi->spi_mode = -1;
}

static struct pic32_sqi *sqi_ctlr;

struct spi_slave *spi_setup_slave(unsigned int bus,
	unsigned int cs, unsigned int max_hz, unsigned int mode)
{
	struct pic32_sqi *sqi = sqi_ctlr;

	debug("%s: bus.cs (%d.%d)\n", __func__, bus, cs);

	/* check maximum SPI clk rate */
	if (!max_hz) {
		printf("No max speed HZ parameter\n");
		return NULL;
	}

	if (max_hz > SQI_MAX_CLK) {
		printf("max speed %u HZ not supported\n", max_hz);
		return NULL;
	}

	if (cs >= SQI_MAX_CS) {
		printf("cs %u not supported\n", cs);
		return NULL;
	}

	if (!sqi) {
		sqi = spi_alloc_slave(struct pic32_sqi, bus, cs);
		if (sqi == NULL)
			return NULL;
		sqi_ctlr = sqi;
		sqi->regs = (void *)SQI_BASE;
		sqi->num_cs = SQI_MAX_CS;
		pic32_sqi_hw_init(sqi, SQI_MODE_DMA);
		if (sw_desc_ring_alloc(sqi)) {
			free(sqi);
			return NULL;
		}
		sqi->flags |= SQI_INIT;
	} else if (cs != sqi->slave.cs) {
		sqi->slave.cs = cs;
	}

	/* spi mode */
	sqi->spi_mode = mode;
	sqi_set_spi_mode(sqi, mode);

	/* spi clock */
	sqi_disable_clk(sqi);
	sqi_set_clk_rate(sqi, max_hz);
	sqi_enable_clk(sqi);

	return &sqi->slave;
}

void spi_free_slave(struct spi_slave *spi)
{
#if 0
	struct pic32_sqi *sqi;

	sqi = to_pic32_sqi(spi);

	/* disable spi */
	sqi_disable_clk(sqi);

	sqi_disable_spi(sqi);

	/* free */
	sw_desc_ring_free(sqi);

	free(sqi);
#endif
}

int spi_claim_bus(struct spi_slave *slave)
{
	struct pic32_sqi *sqi = to_pic32_sqi(slave);
	sqi_enable_spi(sqi);
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
}

__attribute__ ((weak)) int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	return 1;
}

__attribute__ ((weak)) void spi_init(void)
{
	printf("%s: %d\n", __func__, __LINE__);
}

