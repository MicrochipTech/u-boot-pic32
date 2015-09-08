/*
 * Copyright (c) 2015 Microchip Technology Inc
 * Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <linux/compat.h>
#include <asm/arch/pic32.h>

#define BIT_REG_MASK(bit, reg, mask)		\
	do {					\
		reg = (bit) >> 4;		\
		mask = 1 << ((bit) & 0xf);	\
	} while (0)

enum {
	GPIO_DIR_OUT,
	GPIO_DIR_IN
};

int gpio_is_valid(unsigned gpio)
{
	return gpio < CONFIG_SYS_GPIO_NR_MAX;
}

int gpio_request(unsigned gpio, const char *label)
{
	return 0;
}

int gpio_free(unsigned gpio)
{
	return 0;
}

int gpio_set_value(unsigned gpio, int val)
{
	u32 reg;
	u32 mask;

	if (gpio >= CONFIG_SYS_GPIO_NR_MAX)
		BUG();

	BIT_REG_MASK(gpio, reg, mask);

	debug("%s:%d / gpio %d/ port %c, mask 0x%x / 0x%x <= %x\n",
	      __func__, __LINE__, gpio, reg + 'A', mask, PORT(reg), val);
	if (val)
		writel(mask, (void __iomem *)PORTSET(reg));
	else
		writel(mask, (void __iomem *)PORTCLR(reg));
	return 0;
}

int gpio_get_value(unsigned gpio)
{
	u32 reg;
	u32 mask;

	if (gpio >= CONFIG_SYS_GPIO_NR_MAX)
		BUG();

	BIT_REG_MASK(gpio, reg, mask);
	debug("%s:%d / gpio %d/ port %c, mask 0x%x / 0x%x\n",
	      __func__, __LINE__, gpio, reg + 'A', mask, PORT(reg));

	return readl((void __iomem *)PORT(reg)) & mask;
}

static int _gpio_set_direction(unsigned gpio, int dir)
{
	u32 reg;
	u32 mask;

	if (gpio >= CONFIG_SYS_GPIO_NR_MAX)
		BUG();

	BIT_REG_MASK(gpio, reg, mask);
	debug("%s:%d / gpio %d/ port %c, mask 0x%x / 0x%x <= %x\n",
	      __func__, __LINE__, gpio, reg + 'A', mask, ANSEL(reg), dir);


	writel(mask, (void __iomem *)ANSELCLR(reg));

	if (dir)
		writel(mask, (void __iomem *)TRISSET(reg));
	else
		writel(mask, (void __iomem *)TRISCLR(reg));

	return 0;
}

int gpio_direction_input(unsigned gpio)
{
	return _gpio_set_direction(gpio, GPIO_DIR_IN);
}

int gpio_direction_output(unsigned gpio, int value)
{
	_gpio_set_direction(gpio, GPIO_DIR_OUT);
	return gpio_set_value(gpio, value);
}
