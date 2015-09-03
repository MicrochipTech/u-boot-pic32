/*
 * (c) 2012 Steve Scott <steve.scott@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <config.h>
#include <serial.h>
#include <linux/compiler.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/pic32.h>
#include <asm/arch/ap.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_ENABLE           (1 << 15)
#define UART_ENABLE_RX        (1 << 12)
#define UART_ENABLE_TX        (1 << 10)
#define UART_RX_DATA_AVAIL    (1 << 0)
#define UART_RX_OERR          (1 << 1)
#define UART_TX_FULL          (1 << 9)
#define UART_LOOPBACK         (1 << 6)

void pic32_serial_setbrg(void)
{
	ulong pbclk = pic32_get_pbclk(2);

	writel(((pbclk / CONFIG_BAUDRATE) / 16) - 1,
	       U_BRG(CONFIG_PIC32_USART));
}

/*
 * Initialize the serial port with the given baudrate.
 * The settings are always 8 data bits, no parity, 1 stop bit, no start bits.
 */
int pic32_serial_init(void)
{
	/* disable and clear this UART to default mode */
	writel(0, U_MODE(CONFIG_PIC32_USART));

	/* set baud rate generator */
	serial_setbrg();

	/* enable the UART */
	writel(UART_ENABLE, U_MODE(CONFIG_PIC32_USART));

	/* enable the UART for TX and RX */
	writel(UART_ENABLE_TX | UART_ENABLE_RX, U_STASET(CONFIG_PIC32_USART));

	return 0;
}

/*
 * Read a single byte from the rx buffer.
 * Blocking: waits until a character is received, then returns.
 * Return the character read directly from the UART's receive register.
 *
 */
int pic32_serial_getc(void)
{
	char ch;

	/* wait here until data is available */
	while (!serial_tstc())
		;

	/* read the character from the rcv buffer */
	ch = readl(U_RXREG(CONFIG_PIC32_USART));
	return ch;
}

/* Output a single byte to the serial port */
void pic32_serial_putc(const char c)
{
	/* if \n, then add a \r */
	if (c == '\n')
		serial_putc('\r');

	/* Wait for Tx FIFO not full */
	while (readl(U_STA(CONFIG_PIC32_USART)) & UART_TX_FULL)
		;

	/* stuff the tx buffer with the character */
	writel(c, U_TXREG(CONFIG_PIC32_USART));
}

/* Test whether a character is in the RX buffer */
int pic32_serial_tstc(void)
{
	char __attribute__((unused)) throwaway;

	/* check if rcv buf overrun error has occurred */
	if (readl(U_STA(CONFIG_PIC32_USART)) & UART_RX_OERR) {
		/* throw away this character */
		throwaway      = readl(U_RXREG(CONFIG_PIC32_USART));

		/* clear OERR to keep receiving */
		writel(UART_RX_OERR, U_STACLR(CONFIG_PIC32_USART));
	}

	if (readl(U_STA(CONFIG_PIC32_USART)) & UART_RX_DATA_AVAIL)
		return 1;       /* yes, there is data in rcv buffer */
	else
		return 0;       /* no data in rcv buffer */
}

/* send a string to the serial port until null term reached */
void pic32_serial_puts(const char *s)
{
	while (*s)
		serial_putc(*s++);
}

static struct serial_device pic32_serial_dev = {
	.name   = "pic32_serial",
	.start  = pic32_serial_init,
	.stop   = NULL,
	.setbrg = pic32_serial_setbrg,
	.putc   = pic32_serial_putc,
	.puts   = pic32_serial_puts,
	.getc   = pic32_serial_getc,
	.tstc   = pic32_serial_tstc,
};

void pic32_serial_initialize(void)
{
	serial_register(&pic32_serial_dev);
}

__weak struct serial_device *default_serial_console(void)
{
	return &pic32_serial_dev;
}
