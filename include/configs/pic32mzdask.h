/*
 * (c) 2015 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

/*
 * This file contains configuration for Microchip PIC32MZ[DA] StarterKit.
 */

#ifndef __PIC32MZDASK_CONFIG_H
#define __PIC32MZDASK_CONFIG_H

/* System Configuration */
#define CONFIG_SOC_PIC32
#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_DISPLAY_BOARDINFO

/*--------------------------------------------
 * CPU configuration
 */
#define CONFIG_MIPS	        1
#define CONFIG_MIPS32		1  /* MIPS32 CPU core	*/

/* Primary oscillator */
#define CONFIG_PIC32_POSC_FREQ		24000000

/* Reference Oscillator Frequency */
#define CONFIG_SYS_PIC32_REFO1_HZ	10000000
#define CONFIG_SYS_PIC32_REFO2_HZ	50000000
#define CONFIG_SYS_PIC32_REFO3_HZ	20000000
#define CONFIG_SYS_PIC32_REFO4_HZ	25000000

/* CPU Timer rate */
#define CONFIG_SYS_MHZ			100
#define CONFIG_SYS_MIPS_TIMER_FREQ	(CONFIG_SYS_MHZ * 1000000)

/* Cache Configuration */
#define CONFIG_SYS_DCACHE_SIZE		32768
#define CONFIG_SYS_ICACHE_SIZE		32768
#define CONFIG_SYS_CACHELINE_SIZE	16
#define CONFIG_SYS_MIPS_CACHE_MODE	CONF_CM_CACHABLE_NONCOHERENT
#undef CONFIG_SKIP_LOWLEVEL_INIT

/*----------------------------------------------------------------------
 * Memory Layout
 */
#define CONFIG_SYS_SRAM_BASE		0x80000000
#define CONFIG_SYS_SRAM_SIZE		0x00080000 /* 512K in PIC32MZSK */

/* Initial RAM for temporary stack, global data */
#define CONFIG_SYS_INIT_RAM_SIZE	0x10000
#define CONFIG_SYS_INIT_RAM_ADDR	(CONFIG_SYS_SRAM_BASE + CONFIG_SYS_SRAM_SIZE - CONFIG_SYS_INIT_RAM_SIZE)
#define CONFIG_SYS_GBL_DATA_OFFSET	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_OFFSET	CONFIG_SYS_GBL_DATA_OFFSET
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

/* SDRAM Configuration (for final code, data, stack, heap) */
#define CONFIG_SYS_SDRAM_BASE		0x88000000
#define CONFIG_SYS_MEM_SIZE		(128 << 20) /* 128M */

#define CONFIG_SYS_MALLOC_LEN		(256 << 10)
#define CONFIG_SYS_BOOTPARAMS_LEN	(4 << 10)
#define CONFIG_STACKSIZE		(4 << 10) /* regular stack */
#define CONFIG_SYS_EXCEPTION_ADDR	0xA000100 /* EBASE ADDR */

#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_MONITOR_LEN		(192 << 10)

#define CONFIG_SYS_LOAD_ADDR		0x88500000 /* default load address */
#define CONFIG_SYS_FDT_ADDR		0x88C00000
#define CONFIG_SYS_ENV_ADDR		0x88300000

/* Memory Test */
#define CONFIG_SYS_MEMTEST_START	0x88000000
#define CONFIG_SYS_MEMTEST_END		0x88080000

/* EBI */
#define CONFIG_PIC32_NO_EBI_SRAM

/*----------------------------------------------------------------------
 * Commands
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_CMD_CLK
#define CONFIG_CMD_MEMINFO
#define CONFIG_CMD_MEMTEST

/*-------------------------------------------------
 * FLASH configuration
 */
#define CONFIG_SYS_MAX_FLASH_BANKS	2	/* max number of memory banks */
#define CONFIG_SYS_MAX_FLASH_SECT	64	/* max number of sectors on one chip */
#define CONFIG_SYS_FLASH_SIZE		(1 << 20) /* 1M, size of one bank */
#define PHYS_FLASH_SECT_SIZE		(CONFIG_SYS_FLASH_SIZE / CONFIG_SYS_MAX_FLASH_SECT)
#define PHYS_FLASH_1			0x1D000000 /* Flash Bank #1 */
#define PHYS_FLASH_2			0x1D100000 /* Flash Bank #2 */
#define CONFIG_SYS_FLASH_BANKS_LIST	{PHYS_FLASH_1, PHYS_FLASH_2}
#define CONFIG_SYS_FLASH_BASE		PHYS_FLASH_1

/* FLASH erase/programming timeout (in ticks) */
#define CONFIG_SYS_FLASH_ERASE_TOUT	(2 * CONFIG_SYS_HZ) /* Timeout for Flash Erase */
#define CONFIG_SYS_FLASH_WRITE_TOUT	(25 * CONFIG_SYS_HZ) /* Timeout for Flash Write */

/* -------------------------------------------------
 * Serial Driver Configuration
 */
#define CONFIG_PIC32_SERIAL
#define CONFIG_PIC32_USART		2 /* UART2 */

/*------------------------------------------------------------
 * Console Configuration
 */
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}
#define CONFIG_SYS_PROMPT		"dask # "	/* Command Prompt */
#define CONFIG_SYS_CBSIZE		1024		/* Console I/O Buffer Size   */
#define CONFIG_SYS_MAXARGS		16		/* max number of command args*/
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_CMDLINE_EDITING		1

/*-----------------------------------------------------------------------
 * Networking Configuration
 */
#define CONFIG_PHYLIB
#define CONFIG_MII
#define CONFIG_PHY_SMSC
#define CONFIG_PHY_ICPLUS
#define CONFIG_PIC32_ENET

#define CONFIG_CMD_MII
#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP

/*
 * Phy			Phy addr
 *--------------------------------
 * SMSC LAN87XX		0
 * ICPlus IP101A/G	1
 * ICPlus IP101		1
 */
#define CONFIG_SYS_PHY_ADDR		0 /* LAN87XX */
#define CONFIG_ARP_TIMEOUT		500 /* millisec */
#define CONFIG_NET_RETRY_COUNT		20
#define CONFIG_SYS_RX_ETH_BUFFER	8
/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME

/*
 * Handover flattened device tree (dtb file) to Linux kernel
 */
#define CONFIG_OF_LIBFDT

/*-----------------------------------------------------------------------
 * SDHC Configuration
 */
#define CONFIG_PIC32_SDHCI
#define CONFIG_SDHCI
#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_CMD_MMC

/* -------------------------------------------------
 * SPI Driver Configuration
 */
/* Due to limitation in spi layer, we can use either SPI or SQI driver, but not both.*/
#if 1
#define CONFIG_PIC32_SPI		1
#else
/* SQI */
#define CONFIG_PIC32_SQI		1
#define CONFIG_PIC32_SQI_XIP		1/* use 'xspi' command */
#define CONFIG_SYS_PIC32_SQI_XIP_BASE	0xd0000000
#define CONFIG_SYS_PIC32_SQI_XIP_PHYS	0x30000000
#endif

#if defined(CONFIG_PIC32_SQI) || defined(CONFIG_PIC32_SPI)
#define CONFIG_HARD_SPI			1
#define CONFIG_CMD_SPI			1
#endif

/* -------------------------------------------------
 * SPI Flash Configuration
 */
#if defined(CONFIG_PIC32_SQI) || defined(CONFIG_PIC32_SPI)
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_SST

#define CONFIG_CMD_SF
#define CONFIG_SF_DEFAULT_SPEED         10000000
#define CONFIG_SF_DEFAULT_MODE          SPI_MODE_3
#define CONFIG_SF_DEFAULT_CS		0
#define CONFIG_SF_DEFAULT_BUS		0
#endif


/* -------------------------------------------------
 * USB Configuration
 */
#define CONFIG_USB_MUSB_PIC32
#define CONFIG_ARCH_MISC_INIT
#define CONFIG_MUSB_PIO_ONLY
#define CONFIG_MUSB_DISABLE_BULK_COMBINE_SPLIT
#define CONFIG_MUSB_HOST

#define CONFIG_PIC32_USB0
#define CONFIG_PIC32_USB0_MODE	MUSB_HOST

#define CONFIG_USB_STORAGE
#define CONFIG_CMD_USB
#define CONFIG_CMD_STORAGE

/*-----------------------------------------------------------------------
 * File System Configuration
 */
/* FAT FS */
#define CONFIG_DOS_PARTITION
#define CONFIG_PARTITION_UUIDS
#define CONFIG_SUPPORT_VFAT
#define CONFIG_FS_FAT
#define CONFIG_FAT_WRITE
#define CONFIG_CMD_FS_GENERIC
#define CONFIG_CMD_PART
#define CONFIG_CMD_FAT

/* EXT4 FS */
#define CONFIG_FS_EXT4
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_EXT4_WRITE

/* --------------------------------
 * GPIO Configuration
 */
#define CONFIG_PIC32_GPIO	1
#define CONFIG_SYS_GPIO_NR_MAX	160
#define CONFIG_CMD_GPIO

/* -------------------------------------------------
 * Environment
 */
#define CONFIG_ENV_IS_IN_FLASH	1
#define CONFIG_ENV_SECT_SIZE	0x4000 /* 16K(one sector) for env */
#define CONFIG_ENV_SIZE		0x4000
#define CONFIG_ENV_ADDR		0x9d0fc000 /* Last sector from Bank 0 */

/* ---------------------------------------------------------------------
 * Board boot configuration
 */
#define CONFIG_TIMESTAMP	/* Print image info with timestamp */
#define CONFIG_BOOTDELAY	5 /* autoboot after X seconds     */
#undef	CONFIG_BOOTARGS

#define CONFIG_EXTRA_ENV_SETTINGS				\
	"ftfile=pic32mzda.dtb\0"				\
	"ftaddr="__stringify(CONFIG_SYS_FDT_ADDR)"\0"		\
	"bootfile=uImage\0"					\
	"loadaddr="__stringify(CONFIG_SYS_LOAD_ADDR)"\0"	\
	"uenvfile=uEnv.txt\0"					\
	"uenvaddr="__stringify(CONFIG_SYS_ENV_ADDR)"\0"		\
	"scriptfile=boot.scr\0"					\
	"ubootfile=u-boot.bin\0"				\
	"nfsrootpath=/srv/nfsboot\0"				\
	"importbootenv= "					\
		"env import -t -r ${uenvaddr} ${filesize};\0"	\
	"extraargs=earlyprintk=ttyS1,115200n8r "		\
		"console=ttyS1,115200n8 loglevel=10\0"		\
	"ftprepare=fdt addr ${ftaddr}; fdt resize; "		\
		"fdt chosen; fdt print /chosen\0"		\
								\
	"nfsargs=setenv bootargs ${extraargs} root=/dev/nfs "	\
		"nfsroot=${serverip}:${nfsrootpath},tcp,vers=3 "\
		"ip=dhcp init=linuxrc rootwait rw\0"		\
	"tftpload=tftp ${loadaddr} ${bootfile}\0"		\
	"tftploadft=tftp ${ftaddr} ${ftfile}\0"			\
	"tftploadenv=tftp ${uenvaddr} ${uenvfile} \0"		\
	"tftploadscr=tftp ${uenvaddr} ${scriptfile} \0"		\
	"tftploadub=tftp ${loadaddr} ${ubootfile} \0"		\
	"bootcmd_tftp=run nfsargs; run tftpload; "		\
		"if run tftploadft; then "			\
			"echo Booting from network; "		\
			"run ftprepare; "			\
			"bootm ${loadaddr} - ${ftaddr}; "	\
		"else "						\
			"bootm ${loadaddr}; "			\
		"fi \0"						\
								\
	"mmcargs=setenv bootargs ${extraargs} "			\
		"root=/dev/mmcblk0p2 init=linuxrc "		\
		"rootdelay=10 rootwait ro\0"			\
	"mmcloadenv=fatload mmc 0 ${uenvaddr} ${uenvfile}\0"	\
	"mmcloadscr=fatload mmc 0 ${uenvaddr} ${scriptfile}\0"	\
	"mmcfatload=fatload mmc 0 ${loadaddr} ${bootfile}\0"	\
	"mmcfatloadft=fatload mmc 0 ${ftaddr} ${ftfile}\0"	\
	"mmcfatloadub=fatload mmc 0 ${loadaddr} ${ubootfile}\0"	\
	"bootcmd_mmcfat=run mmcargs; run mmcfatload; "		\
		"if run mmcfatloadft; then "			\
			"echo Booting from MMC w/ DTB; "	\
			"run ftprepare; "			\
			"bootm ${loadaddr} - ${ftaddr}; "	\
		"else "						\
			"echo Booting from MMC w/o DTB; "	\
			"bootm ${loadaddr}; "			\
		"fi\0"						\
								\
	"usbargs=setenv bootargs ${extraargs} "			\
		"root=/dev/sda2 init=linuxrc "			\
		"rootdelay=10 rootwait ro\0"			\
	"usbloadenv=fatload usb 0 ${uenvaddr} ${uenvfile}\0"	\
	"usbloadscr=fatload usb 0 ${uenvaddr} ${scriptfile}\0"	\
	"usbfatload=fatload usb 0 ${loadaddr} ${bootfile}\0"	\
	"usbfatloadft=fatload usb 0 ${ftaddr} ${ftfile}\0"	\
	"usbfatloadub=fatload usb 0 ${loadaddr} ${ubootfile}\0"	\
	"bootcmd_usbfat=usb start; usb info; "			\
		"run usbargs; run usbfatload; "			\
		"if run usbfatloadft; then "			\
			"echo Booting from USB w/ DTB; "	\
			"run ftprepare; "			\
			"bootm ${loadaddr} - ${ftaddr}; "	\
		"else "						\
			"echo Booting from USB w/o DTB; "	\
			"bootm ${loadaddr}; "			\
		"fi\0"						\
								\
	"flashub=protect off bank 1; "				\
		"erase.b 0x9d004000 0x9d0f3fff; "		\
		"cp.b ${loadaddr} 0x9d004000 ${filesize}; "	\
		"cmp.b ${loadaddr} 0x9d004000 ${filesize}; "	\
		"protect on bank 1; \0"				\
								\
	"loadbootenv=run mmcloadenv || run tftploadenv\0"	\
	"loadbootscr=run mmcloadscr || run tftploadscr\0"	\
	"bootcmd_root= "					\
		"if run loadbootenv; then "			\
			"echo Loaded environment ${uenvfile}; "	\
			"run importbootenv; "			\
		"fi; "						\
		"if test -n \"${bootcmd_uenv}\" ; then "	\
			"echo Running bootcmd_uenv ...; "	\
			"run bootcmd_uenv; "			\
		"fi; "						\
		"if run loadbootscr; then "			\
			"echo Jumping to ${scriptfile}; "	\
			"source ${uenvaddr}; "			\
		"fi; "						\
		"echo Trying mmcboot, tftpboot (in order)...; "	\
		"run bootcmd_mmcfat || run bootcmd_tftp;\0"	\
	""

#define CONFIG_BOOTCOMMAND		"run bootcmd_root"
#define CONFIG_MEMSIZE_IN_BYTES		/* pass 'memsize=' in bytes */
#endif	/* __PIC32MZDASK_CONFIG_H */
