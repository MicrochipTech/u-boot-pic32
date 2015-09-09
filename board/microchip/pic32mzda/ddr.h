/*
 * (c) 2015 Purna Chandra Mandal purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef __PIC32MZDA_DDR_H
#define __PIC32MZDA_DDR_H

#define DDR2TSEL	(PIC32_DDR2C_BASE + 0x00) /* ARB_AGENT_SEL       */
#define DDR2MINLIM	(PIC32_DDR2C_BASE + 0x04) /* MIN_LIMIT           */
#define DDR2REQPRD	(PIC32_DDR2C_BASE + 0x08) /* RQST_PERIOD         */
#define DDR2MINCMD	(PIC32_DDR2C_BASE + 0x0c) /* MIN_CMD_ACPT        */
#define DDR2MEMCON	(PIC32_DDR2C_BASE + 0x10) /* MEM_START           */
#define DDR2MEMCFG0	(PIC32_DDR2C_BASE + 0x14) /* MEM_CONFIG_1        */
#define DDR2MEMCFG1	(PIC32_DDR2C_BASE + 0x18) /* MEM_CONFIG_2        */
#define DDR2MEMCFG2	(PIC32_DDR2C_BASE + 0x1c) /* MEM_CONFIG_3        */
#define DDR2MEMCFG3	(PIC32_DDR2C_BASE + 0x20) /* MEM_CONFIG_4        */
#define DDR2MEMCFG4	(PIC32_DDR2C_BASE + 0x24) /* MEM_CONFIG_5        */
#define DDR2REFCFG	(PIC32_DDR2C_BASE + 0x28) /* REF_CONFIG          */
#define DDR2PWRCFG	(PIC32_DDR2C_BASE + 0x2c) /* PWR_SAVE_ECC_CONFIG */
#define DDR2DLYCFG0	(PIC32_DDR2C_BASE + 0x30) /* DLY_CONFIG1         */
#define DDR2DLYCFG1	(PIC32_DDR2C_BASE + 0x34) /* DLY_CONFIG2         */
#define DDR2DLYCFG2	(PIC32_DDR2C_BASE + 0x38) /* DLY_CONFIG3         */
#define DDR2DLYCFG3	(PIC32_DDR2C_BASE + 0x3c) /* DLY_CONFIG4         */
#define DDR2ODTCFG	(PIC32_DDR2C_BASE + 0x40) /* ODT_CONFIG          */
#define DDR2XFERCFG	(PIC32_DDR2C_BASE + 0x44) /* DATA_XFR_CONFIG     */
#define DDR2CMDISSUE	(PIC32_DDR2C_BASE + 0x48) /* HOST_CMD_ISSUE      */
#define DDR2ODTENCFG	(PIC32_DDR2C_BASE + 0x4c) /* ODT_EN_CONFIG       */
#define DDR2MEMWIDTH	(PIC32_DDR2C_BASE + 0x50) /* MEM_WIDTH           */
#define DDR2CMD10	(PIC32_DDR2C_BASE + 0x80) /* HOST_CMD1           */
#define DDR2CMD20	(PIC32_DDR2C_BASE + 0xc0) /* HOST_CMD2           */

/* DDR PHY */
#define DDR2SCLSTART	(PIC32_DDR2P_BASE + 0x00) /* SCL_START           */
#define DDR2SCLLAT	(PIC32_DDR2P_BASE + 0x0c) /* PHY_SCLLAAT         */
#define DDR2SCLCFG0	(PIC32_DDR2P_BASE + 0x18) /* SCL_CONFIG_1        */
#define DDR2SCLCFG1	(PIC32_DDR2P_BASE + 0x1c) /* SCL_CONFIG_2        */
#define DDR2PHYPADCON	(PIC32_DDR2P_BASE + 0x20) /* PHY_PAD_CTRL        */
#define DDR2PHYDLLR	(PIC32_DDR2P_BASE + 0x24) /* PHY_DLL_RECALIB     */

void ddr_phy_init(void);
void ddr_init(void);

#endif	/* __PIC32MZDA_DDR_H */
