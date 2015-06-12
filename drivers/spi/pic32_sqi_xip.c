/*
 * PIC32 Quad Flash XIP mode
 *
 * Copyright (c) 2014, Microchip Technology Inc.
 *      Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
/*#define DEBUG*/

#ifdef CONFIG_PIC32_SQI_XIP
#include <dm.h>
#include <common.h>
#include <command.h>
#include <dm.h>
#include <spi.h>
#include <malloc.h>
#include <errno.h>
#include <linux/compat.h>
#include <linux/list.h>
#include <asm/types.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch-pic32/pic32.h>
#include <asm/arch-pic32/ap.h>

#include "pic32_sqi.h"

/*-----------------------------------------------------------------------
 * Definitions
 */

#ifndef MAX_SPI_BYTES
#define MAX_SPI_BYTES 32	/* Maximum number of bytes we can handle */
#endif

#ifndef CONFIG_DEFAULT_SPI_BUS
#define CONFIG_DEFAULT_SPI_BUS	0
#endif
#ifndef CONFIG_DEFAULT_SPI_MODE
#define CONFIG_DEFAULT_SPI_MODE	SPI_MODE_0
#endif
#ifndef CONFIG_SF_DEFAULT_CS
#define CONFIG_SF_DEFAULT_CS	0
#endif
/*
 * Values from last command.
 */
static unsigned int	bus;
static unsigned int	cs;
static unsigned int	mode;
static int		bitlen = MAX_SPI_BYTES * 8;
static uchar		dout[MAX_SPI_BYTES];
static uchar		*din = (uchar *)CONFIG_SYS_PIC32_SQI_XIP_BASE;
static uchar		code1[3], code2[3];
static int		code1_len, code2_len;

static int spi_xfer_xip(struct spi_slave *slave, unsigned int bitlen,
	     const void *tx_buf, void *rx_buf, unsigned long flags)
{
	u8 opcode;
	u32 v = 0;
	int cs = slave->cs;
	struct pic32_sqi *sqi;

	print_buffer8("tx:", tx_buf, 1);

	sqi = to_pic32_sqi(slave);

	/* lane type */
	v = (SQI_LANE_SINGLE << SQI_XIP_CMD_TYPE_SHIFT);
	v |= (SQI_LANE_SINGLE << SQI_XIP_ADDR_TYPE_SHIFT);
	v |= (SQI_LANE_SINGLE << SQI_XIP_MODE_TYPE_SHIFT);
	v |= (SQI_LANE_SINGLE << SQI_XIP_DUMM_TYPE_SHIFT);
	v |= (SQI_LANE_SINGLE << SQI_XIP_DATA_TYPE_SHIFT);

	/* opcode */
	opcode = *((u8 *)tx_buf);
	v |= (opcode << SQI_XIP_OPCODE_SHIFT);

	/* addr byte count */
	v |= (3 << SQI_XIP_ADDR_SHIFT);

	/* dummy byte count */
	if (opcode == 0x0b)
		v |= (1 << SQI_XIP_DUMMY_SHIFT);

	writel(v, sqi->regs + SQI_XIP_CTRL1_REG);

	/* devsel */
	v = (cs << SQI_XIP_DEVSEL_SHIFT);
	writel(v, sqi->regs + SQI_XIP_CTRL2_REG);

	/* init code1 */
	if (code1_len) {
		v = (code1[2] << 16)|(code1[1] << 8)|code1[0];
		v |= (SQI_LANE_SINGLE << SQI_XIP_INIT_TYPE_SHIFT);
		v |= (code1_len << SQI_XIP_INIT_CODE_CNT_SHIFT);
		writel(v, sqi->regs + SQI_XIP_CTRL3_REG);
	}

	/* init code2 */
	if (code2_len) {
		v = (code2[2] << 16)|(code2[1] << 8)|code2[0];
		v |= (code2_len << SQI_XIP_INIT_CODE_CNT_SHIFT);
		v |= (SQI_LANE_SINGLE << SQI_XIP_INIT_TYPE_SHIFT);
		writel(v, sqi->regs + SQI_XIP_CTRL4_REG);
	}

	debug(" -- XIP: enable spi(opcode %02x)\n", opcode);
	sqi_enable_spi(sqi);

	debug("XIPCTL1: %08x\n", readl(sqi->regs + SQI_XIP_CTRL1_REG));
	debug("XIPCTL2: %08x\n", readl(sqi->regs + SQI_XIP_CTRL2_REG));
	debug("XIPCTL3: %08x\n", readl(sqi->regs + SQI_XIP_CTRL3_REG));
	debug("XIPCTL4: %08x\n", readl(sqi->regs + SQI_XIP_CTRL4_REG));
	debug("SQICFG: %08x\n", readl(sqi->regs + SQI_CONF_REG));

	/* set mode: XIP */
	sqi_set_mode(sqi, SQI_MODE_XIP);

	debug("XIPCTL1: %08x\n", readl(sqi->regs + SQI_XIP_CTRL1_REG));
	debug("XIPCTL2: %08x\n", readl(sqi->regs + SQI_XIP_CTRL2_REG));
	debug("SQICFG: %08x\n", readl(sqi->regs + SQI_CONF_REG));

	/* Poll for packet completion */
	if (sqi_poll_for_completion(sqi, 0x1000000) <= 0) {
		printf("wait timedout/interrupted\n");
		pic32_debug_sqi(sqi, " -- TIMEDOUT -- ");
		return 1;
	}

	return 0;
}

static int do_spi_xfer(int bus, int cs)
{
	struct spi_slave *slave;
	int ret = 0;

#ifdef CONFIG_DM_SPI
	char name[30], *str;
	struct udevice *dev;

	snprintf(name, sizeof(name), "generic_%d:%d", bus, cs);
	str = strdup(name);
	ret = spi_get_bus_and_cs(bus, cs, 1000000, mode, "spi_generic_drv",
				 str, &dev, &slave);
	if (ret)
		return ret;
#else
	slave = spi_setup_slave(bus, cs, 1000000, mode);
	if (!slave) {
		printf("Invalid device %d:%d\n", bus, cs);
		return -EINVAL;
	}
#endif

	ret = spi_claim_bus(slave);
	if (ret)
		goto done;
	ret = spi_xfer_xip(slave, bitlen, dout, din,
		       SPI_XFER_BEGIN | SPI_XFER_END);
#ifndef CONFIG_DM_SPI
	/* We don't get an error code in this case */
	if (ret)
		ret = -EIO;
#endif
	if (ret) {
		printf("Error %d during SPI transaction\n", ret);
	} else {
		int j;

		for (j = 0; j < ((bitlen + 7) / 8); j++)
			printf("%02X", din[j]);
		printf("\n");
	}
done:
	spi_release_bus(slave);
#ifndef CONFIG_DM_SPI
	spi_free_slave(slave);
#endif

	return ret;
}

/*
 * SPI read/write
 *
 * Syntax:
 *   spi {dev} {num_bits} {dout}
 *     {dev} is the device number for controlling chip select (see TBD)
 *     {num_bits} is the number of bits to send & receive (base 10)
 *     {dout} is a hexadecimal string of data to send
 * The command prints out the hexadecimal string received via SPI.
 */

int do_xspi(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char  *cp = 0;
	uchar tmp;
	int   j;

	/*
	 * We use the last specified parameters, unless new ones are
	 * entered.
	 */
	if ((flag & CMD_FLAG_REPEAT) == 0) {
		if (argc >= 2) {
			mode = CONFIG_DEFAULT_SPI_MODE;
			bus = simple_strtoul(argv[1], &cp, 10);
			if (*cp == ':') {
				cs = simple_strtoul(cp+1, &cp, 10);
			} else {
				cs = CONFIG_SF_DEFAULT_CS;
				bus = CONFIG_DEFAULT_SPI_BUS;
			}
		}
		if (argc >= 3) {
			cp = argv[2];
			for (j = 0; *cp; j++, cp++) {
				tmp = *cp - '0';
				if (tmp > 9)
					tmp -= ('A' - '0') - 10;
				if (tmp > 15)
					tmp -= ('a' - 'A');
				if (tmp > 15) {
					printf("Hex conversion error on %c\n",
					       *cp);
					return 1;
				}
				if ((j % 2) == 0)
					dout[j / 2] = (tmp << 4);
				else
					dout[j / 2] |= tmp;
			}
		}
		if (argc >= 4) {
			cp = argv[3];
			for (j = 0; *cp; j++, cp++) {
				tmp = *cp - '0';
				if (tmp > 9)
					tmp -= ('A' - '0') - 10;
				if (tmp > 15)
					tmp -= ('a' - 'A');
				if (tmp > 15) {
					printf("Hex conversion error on %c\n",
					       *cp);
					return 1;
				}
				if ((j % 2) == 0)
					code1[j / 2] = (tmp << 4);
				else
					code1[j / 2] |= tmp;
				code1_len = j / 2;
			}
		}
		if (argc >= 5) {
			cp = argv[4];
			for (j = 0; *cp; j++, cp++) {
				tmp = *cp - '0';
				if (tmp > 9)
					tmp -= ('A' - '0') - 10;
				if (tmp > 15)
					tmp -= ('a' - 'A');
				if (tmp > 15) {
					printf("Hex conversion error on %c\n",
					       *cp);
					return 1;
				}
				if ((j % 2) == 0)
					code2[j / 2] = (tmp << 4);
				else
					code2[j / 2] |= tmp;
				code2_len = j / 2;
			}
		}
	}

	if (do_spi_xfer(bus, cs))
		return 1;

	return 0;
}

/***************************************************/

U_BOOT_CMD(
	xspi,	6,	1,	do_xspi,
	"PIC32 Quad SPI Flash (read) through XIP",
	"[<bus>:]<cs> <opcode> [<init1code> <init2code>] - Send (fast)read_opcode bits\n"
	"<bus>     - Identifies the SPI bus\n"
	"<cs>      - Identifies the chip select\n"
	"<opcode>    - Hexadecimal string that gets sent(single lane)\n"
	"<init1code>    - Hexadecimal string for init1 code (single lane)\n"
	"<init2code>    - Hexadecimal string for init2 code (single lane)"
);
#endif

