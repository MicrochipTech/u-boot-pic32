/*
 * (c) 2012 Steve Scott <steve.scott@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

/*
 * This file contains configuration for Microchip PIC32MZSK board.
 */

#ifndef __PIC32MZSK_CONFIG_H
#define __PIC32MZSK_CONFIG_H

/* System Configuration */
#define CONFIG_PIC32MZ
#define CONFIG_SOC_PIC32
#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_DISPLAY_BOARDINFO
#define CONFIG_SYS_PIC32_PLAND

/*--------------------------------------------
 * CPU configuration
 */
#define CONFIG_MIPS	        1
#define CONFIG_MIPS32		1  /* MIPS32 CPU core	*/

#define CONFIG_PIC32_POSC_FREQ	24000000 /* Primary Oscillator */

/* Reference Oscillator Frequency */
#define CONFIG_SYS_PIC32_REFO1_HZ 10000000
#define CONFIG_SYS_PIC32_REFO2_HZ 50000000
#define CONFIG_SYS_PIC32_REFO3_HZ 20000000
#define CONFIG_SYS_PIC32_REFO4_HZ 30000000

/*----------------------
 * CPU Timer rate
 */
#define CONFIG_SYS_MHZ			100
#define CONFIG_SYS_MIPS_TIMER_FREQ	(CONFIG_SYS_MHZ * 1000000)

/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CONFIG_SYS_DCACHE_SIZE		4096
#define CONFIG_SYS_ICACHE_SIZE		16384
#define CONFIG_SYS_CACHELINE_SIZE	16

/* --------------------------------------------
 * Memory Map
 */
/* Initial RAM (SRAM) for temporary stack */
#define CONFIG_SYS_INIT_RAM_ADDR	0x80000000
#define CONFIG_SYS_MIPS_CACHE_MODE	CONF_CM_CACHABLE_NONCOHERENT
#undef  CONFIG_SKIP_LOWLEVEL_INIT

#define CONFIG_SYS_INIT_RAM_SIZE	0x80000 /* 512K in PIC32MZSK */
#define CONFIG_SYS_GBL_DATA_OFFSET      (CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_OFFSET       CONFIG_SYS_GBL_DATA_OFFSET

/* SDRAM Configuration */
#define CONFIG_SYS_SDRAM_BASE	0xC0000000

#if defined (CONFIG_SYS_PIC32_PLANB)
#define CONFIG_SYS_MEM_SIZE	(8 << 20)
#elif defined(CONFIG_SYS_PIC32_PLANC) || defined(CONFIG_SYS_PIC32_PLAND)
#define CONFIG_SYS_MEM_SIZE	(16 << 20)
#else
#error "CONFIG_SYS_MEM_SIZE not defined."
#endif

#define CONFIG_SYS_MALLOC_LEN		(128 << 10)
#define CONFIG_SYS_BOOTPARAMS_LEN	(4 << 10)
#define CONFIG_STACKSIZE		(4 << 10) /* regular stack */

#define	CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE
#define	CONFIG_SYS_MONITOR_LEN		(192 << 10)
#define CONFIG_SYS_EXCEPTION_ADDR	0x9D000000 /* EBASE ADDR */

/* PLAN-B/C/D daughter board provides external SRAM mappable to CPU */
#define CONFIG_SYS_LOAD_ADDR		0xC0400000 /* default load address */

/* Memory Test */
#define CONFIG_SYS_MEMTEST_START	0xC0000000 /* MEM start */
#define CONFIG_SYS_MEMTEST_END		(0xC0000000 + (4 << 20)) /* MEM top */

/* Generic Board Setup */
#define CONFIG_SYS_PROMPT		"PIC32MZSK # "  /* Command Prompt */

/*----------------------------------------------------------------------
 * Commands
 */
#undef CONFIG_CMD_BDI
#undef CONFIG_CMD_BEDBUG
#undef CONFIG_CMD_ELF
#undef CONFIG_CMD_FAT
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_IDE
#undef CONFIG_CMD_DHCP
#undef CONFIG_CMD_LOADB
#undef CONFIG_CMD_LOADS
#undef CONFIG_CMD_NET
#undef CONFIG_ENV_IS_IN_FLASH
#undef CONFIG_ENV_IS_IN_SPI_FLASH
#undef CONFIG_CMD_FAT
#undef CONFIG_CMD_MEMORY

#define CONFIG_CMD_MEMTEST
#define CONFIG_CMD_MEMINFO
/*  -------------------------------------------------
 * FLASH configuration
 */
#if 0 /* enable if FLASH is not required */
#define CONFIG_SYS_NO_FLASH
#undef CONFIG_CMD_IMLS
#else
#define CONFIG_SYS_MAX_FLASH_BANKS	2	/* max number of memory banks */
#define CONFIG_SYS_MAX_FLASH_SECT	64	/* max number of sectors on one chip */
#define CONFIG_SYS_FLASH_SIZE		(1 << 20) /* 1M, size of one bank */
#define PHYS_FLASH_1		0x1D000000 /* Flash Bank #1 */
#define PHYS_FLASH_2		0x1D100000 /* Flash Bank #2 */
#define CONFIG_SYS_FLASH_BANKS_LIST {PHYS_FLASH_1, PHYS_FLASH_2}

/* We boot from this flash, selected with dip switch */
#define CONFIG_SYS_FLASH_BASE		PHYS_FLASH_1
/* timeout values are in ticks */
#define CONFIG_SYS_FLASH_ERASE_TOUT	(2 * CONFIG_SYS_HZ) /* Timeout for Flash Erase */
#define CONFIG_SYS_FLASH_WRITE_TOUT	(25 * CONFIG_SYS_HZ) /* Timeout for Flash Write */
#define CONFIG_ENV_ADDR			0x9D0FC000 	/* Last sector from Bank 0 */
#endif

/*-----------------------------------------------------------------------
 * Networking Configuration
 */
#define CONFIG_CMD_NET
#define CONFIG_CMD_PING
#define CONFIG_PHYLIB
#define CONFIG_MII
#define CONFIG_CMD_MII
#define CONFIG_PHY_SMSC
#define CONFIG_PHY_ICPLUS
#define CONFIG_PIC32_ENET
/*
 * Phy			Phy addr
 *--------------------------------
 * SMSC LAN87XX		0
 * ICPlus IP101A/G	1
 * ICPlus IP101		1
 */
#define CONFIG_SYS_PHY_ADDR	0 /* LAN87XX */

#define CONFIG_ARP_TIMEOUT		500 /* millisec */
#define CONFIG_NET_RETRY_COUNT		1
#define CONFIG_SYS_RX_ETH_BUFFER	8
/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME

/* --------------------------------
 * GPIO Configuration
 */
#define CONFIG_PIC32_GPIO	1
#define CONFIG_SYS_GPIO_NR_MAX	160
#define CONFIG_CMD_GPIO

/* -------------------------------------------------
 * SPI Driver Configuration
 */
#if 0 /* SQI */
#define CONFIG_PIC32_SQI
#define CONFIG_PIC32_SQI_XIP /* use 'xspi' command */

/* -------------------
 * SQI XIP read base
 */
#if defined(CONFIG_SYS_PIC32_PLANB)
#define CONFIG_SYS_PIC32_SQI_XIP_BASE	0xc0800000
#elif defined(CONFIG_SYS_PIC32_PLANC) || defined(CONFIG_SYS_PIC32_PLAND)
#define CONFIG_SYS_PIC32_SQI_XIP_BASE	0xc1000000
#endif

#elif 1 /* SPI */
#define CONFIG_PIC32_SPI
#endif

#if defined(CONFIG_PIC32_SQI)||defined(CONFIG_PIC32_SPI)
#define CONFIG_HARD_SPI
#define CONFIG_CMD_SPI
#endif

/* -------------------------------------------------
 * SPI Flash Configuration
 */
#if 0
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_SST

#define CONFIG_CMD_SF
#define CONFIG_SF_DEFAULT_SPEED         10000000
#define CONFIG_SF_DEFAULT_MODE          SPI_MODE_3
#define CONFIG_SF_DEFAULT_CS		0
#define CONFIG_SF_DEFAULT_BUS		0
#endif

/* -------------------------------------------------
 * MMC_SPI Configuration
 */
#if (0 || defined(CONFIG_SYS_PIC32_PLAND))
#define CONFIG_MMC_SPI
#ifndef CONFIG_PIC32_SPI
#warning "define CONFIG_PIC32_SPI for MMC_SPI to work"
#endif
#endif

#ifdef CONFIG_MMC_SPI
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_CMD_MMC
#define CONFIG_CMD_MMC_SPI

#if defined(CONFIG_SYS_PIC32_PLAND)
#define CONFIG_MMC_SPI_BUS	1
#else
#define CONFIG_MMC_SPI_BUS	0
#endif
#define CONFIG_MMC_SPI_CS	0
#define CONFIG_MMC_SPI_SPEED	5000000
#define CONFIG_MMC_SPI_MODE	SPI_MODE_3

#define CONFIG_FS_FAT
#endif /* CONFIG_MMC_SPI */

/* -------------------------------------------------
 * USB Configuration
 */
#define CONFIG_USB_MUSB_PIC32
#define CONFIG_ARCH_MISC_INIT
#define CONFIG_MUSB_PIO_ONLY
#define CONFIG_MUSB_DISABLE_BULK_COMBINE_SPLIT
#define CONFIG_MUSB_HOST

#define CONFIG_PIC32_USB0
#define CONFIG_PIC32_USB0_MODE MUSB_HOST

#define CONFIG_USB_STORAGE
#define CONFIG_CMD_USB
#define CONFIG_CMD_FAT
#define CONFIG_CMD_STORAGE
#define CONFIG_DOS_PARTITION

/* -------------------------------------------------
 * Serial Driver Configuration
 */
#define CONFIG_PIC32_SERIAL
#define CONFIG_PIC32_USART		2

/*------------------------------------------------------------
 * Console Configuration
 */
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}

#define CONFIG_SYS_PROMPT		"PIC32MZSK # "  /* Command Prompt */

#define	CONFIG_SYS_CBSIZE		1024 /* Console I/O Buffer Size   */
#define	CONFIG_SYS_PBSIZE		\
	(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define	CONFIG_SYS_MAXARGS		16  /* max number of command args*/

#define CONFIG_CMDLINE_EDITING 1

/* -------------------------------------------------
 * Environment
 */
#if 0		/* environment nowhere */
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SECT_SIZE	0x1000 /* 4K(one sector) for env */
#define CONFIG_ENV_SIZE		0x1000
#undef CONFIG_CMD_SAVEENV
#elif 0		/* environment in spi flash */
#define CONFIG_ENV_IS_IN_SPI_FLASH	1
#define CONFIG_ENV_SECT_SIZE	0x1000 /* 4K(one sector) for env */
#define CONFIG_ENV_SIZE		0x1000
#define CONFIG_ENV_OFFSET	0 /* 1st sector */
#else		/* environment in flash */
#define CONFIG_ENV_IS_IN_FLASH	1
#define CONFIG_ENV_SECT_SIZE	0x4000 /* 16K(one sector) for env */
#define CONFIG_ENV_SIZE		0x4000
#define CONFIG_ENV_ADDR		0x9D0FC000 /* Last sector from Bank 0 */
#endif

/* Filesystem support
 */
#ifdef CONFIG_FS_FAT
#define CONFIG_DOS_PARTITION
#define CONFIG_PARTITION_UUIDS
#define CONFIG_SUPPORT_VFAT
#define CONFIG_FAT_WRITE

#define CONFIG_CMD_FS_GENERIC
#define CONFIG_CMD_PART
#define CONFIG_CMD_FAT
#endif


/* ---------------------------------------------------------------------
 * Board boot configuration
 */
#define	CONFIG_TIMESTAMP		/* Print image info with timestamp */
#define CONFIG_BOOTDELAY       6       /* autoboot after X seconds     */
#undef	CONFIG_BOOTARGS

#define	CONFIG_EXTRA_ENV_SETTINGS				\
	"addmisc=setenv bootargs ${bootargs} "			\
		"console=ttyS0,${baudrate} "			\
		"panic=1\0"					\
	"bootfile=uImage\0"					\
	"loadaddr="__stringify(CONFIG_SYS_LOAD_ADDR)"\0"	\
	"tftpload=tftp ${loadaddr} ${bootfile}\0"		\
	"sfcs="__stringify(CONFIG_SF_DEFAULT_CS)"\0"		\
	"sfload=sf probe 0:${sfcs};"				\
		"sf read ${loadaddr} 0 0x400000 \0"		\
	"sfupdate=sf probe ${sfcs}; run tftpload;"		\
		  "sf update ${loadaddr} 0 ${filesize} \0"	\
	"ipaddr=192.168.100.10\0"				\
	"ethaddr=00:04:A3:3E:37:D2\0"				\
	"serverip=192.168.100.1\0"				\
	"gatewayip=192.168.100.1\0"				\
	"netmask=255.255.0.0\0"					\
	"bootcmd_tftp=run tftpload; bootm ${loadaddr}\0"	\
	"bootcmd_sf=run sfload; bootm ${loadaddr}\0"		\
	"bootcmd_sd=mmc_spi 0; "				\
		    "fatload mmc 0 ${loadaddr} uimage;"		\
		    "bootm ${loadaddr}\0"			\
	""

#define CONFIG_BOOTCOMMAND	"run bootcmd_tftp"
#define CONFIG_MEMSIZE_IN_BYTES		/* pass 'memsize=' in bytes */

#endif	/* __PIC32MZSK_CONFIG_H */
