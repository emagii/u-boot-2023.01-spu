// SPDX-License-Identifier: GPL-2.0+
/*
 * mux.c
 *
 * Pin Mux functions for BT AM335X based boards
 *
 * Copyright (C) 2020 Bombardier Transportation
 * Ulf Samuelsson <ulf@emagii.com>
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/hardware.h>
#include <asm/arch/mux.h>
#include <asm/io.h>
#include <i2c.h>
#include "../common/board_detect.h"
#include "board.h"

#define	SMA2_REGISTER	(0x1320U)
#define	INP		(RXACTIVE)
#define	INP_PULL_UP	(RXACTIVE | PULLUDEN | PULLUP_EN)
#define	INP_PULL_DOWN	(RXACTIVE | PULLUDEN | PULLDOWN_EN)

#define	OUT		(PULLUDDIS)
#define	OUT_PULL_UP	(PULLUDEN | PULLUP_EN)
#define	OUT_PULL_DOWN	(PULLUDEN | PULLDOWN_EN)


static struct module_pin_mux uart0_pin_mux[] = {
	{OFFSET(uart0_rxd),	(MODE(0) | INP_PULL_UP)},	/* UART0_RXD */
	{OFFSET(uart0_txd),	(MODE(0) | OUT_PULL_DOWN)},	/* UART0_TXD */
	{-1},
};

static struct module_pin_mux uart1_pin_mux[] = {
	{OFFSET(uart1_rxd),	(MODE(0) | INP_PULL_UP)},	/* UART1_RXD */
	{OFFSET(uart1_txd),	(MODE(0) | OUT_PULL_DOWN)},	/* UART1_TXD */
	{-1},
};

static struct module_pin_mux uart2_pin_mux[] = {
	{OFFSET(mii1_txclk),	(MODE(1) | INP_PULL_UP)},	/* UART2_RXD */
	{OFFSET(mii1_rxclk),	(MODE(1) | OUT_PULL_DOWN)},	/* UART2_TXD */
	{-1},
};

static struct module_pin_mux uart5_pin_mux[] = {
	{OFFSET(lcd_data8),	(MODE(4) | OUT_PULL_DOWN)},	/* UART5_TXD */
	{OFFSET(lcd_data9),	(MODE(4) | INP_PULL_UP )},	/* UART5_RXD */
	{-1},
};

static struct module_pin_mux spi0_pin_mux[] = {
	{OFFSET(spi0_sclk), (MODE(0) | RXACTIVE | PULLUDEN)},	/* SPI0_SCLK */
	{OFFSET(spi0_d0), (MODE(0) | RXACTIVE |
			PULLUDEN | PULLUP_EN)},			/* SPI0_D0 */
	{OFFSET(spi0_d1), (MODE(0) | RXACTIVE | PULLUDEN)},	/* SPI0_D1 */
	{OFFSET(spi0_cs0), (MODE(0) | RXACTIVE |
			PULLUDEN | PULLUP_EN)},			/* SPI0_CS0 */
	{-1},
};

static struct module_pin_mux spi1_pin_mux[] = {
#if	defined(CONFIG_OMAP3_SPI_CS_PERIPHERAL)
	{OFFSET(ecap0_in_pwm0_out),
				(MODE(4) | INP_PULL_DOWN)},	/* (C18) SPI1_SCLK */
	{OFFSET(uart0_ctsn),	(MODE(4) | INP_PULL_DOWN)},	/* (E18) SPI1_MISO(D0) */
	{OFFSET(uart0_rtsn),	(MODE(4) | INP_PULL_DOWN)},	/* (E17) SPI1_MOSI(D1) */
	{OFFSET(uart1_ctsn),	(MODE(4) | OUT_PULL_UP)},	/* (D18) SPI1_CS0 */
	{OFFSET(uart1_rtsn),	(MODE(4) | OUT_PULL_UP)},	/* (D17) SPI1_CS1 */
	{OFFSET(mii1_txd3), 	(MODE(7) | OUT_PULL_UP)},	/* (K15) SPI1_CS2 */
	{OFFSET(mii1_txd2), 	(MODE(7) | OUT_PULL_UP)},	/* (J18) SPI1_CS3 */
#elif	defined(CONFIG_OMAP3_SPI_CS_GPIO)
	{OFFSET(ecap0_in_pwm0_out),
				(MODE(4) | INP_PULL_DOWN)},	/* (C18) SPI1_SCLK */
	{OFFSET(uart0_ctsn),	(MODE(4) | INP_PULL_DOWN)},	/* (E18) SPI1_MISO(D0) */
	{OFFSET(uart0_rtsn),	(MODE(4) | INP_PULL_DOWN)},	/* (E17) SPI1_MOSI(D1) */
	{OFFSET(uart1_ctsn),	(MODE(7) | OUT_PULL_UP)},	/* (D18) SPI1_CS0 */
	{OFFSET(uart1_rtsn),	(MODE(7) | OUT_PULL_UP)},	/* (D17) SPI1_CS1 */
	{OFFSET(mii1_txd3), 	(MODE(7) | OUT_PULL_UP)},	/* (K15) SPI1_CS2 */
	{OFFSET(mii1_txd2), 	(MODE(7) | OUT_PULL_UP)},	/* (J18) SPI1_CS3 */
#else
	{OFFSET(ecap0_in_pwm0_out),
				(MODE(7) | INP_PULL_DOWN)},	/* (C18) SPI1_SCLK */
	{OFFSET(uart0_ctsn),	(MODE(7) | INP_PULL_DOWN)},	/* (E18) SPI1_MISO(D0) */
	{OFFSET(uart0_rtsn),	(MODE(7) | INP_PULL_DOWN)},	/* (E17) SPI1_MOSI(D1) */
	{OFFSET(uart1_ctsn),	(MODE(7) | OUT_PULL_UP)},	/* (D18) SPI1_CS0 */
	{OFFSET(uart1_rtsn),	(MODE(7) | OUT_PULL_UP)},	/* (D17) SPI1_CS1 */
	{OFFSET(mii1_txd3), 	(MODE(7) | OUT_PULL_UP)},	/* (K15) SPI1_CS2 */
	{OFFSET(mii1_txd2), 	(MODE(7) | OUT_PULL_UP)},	/* (J18) SPI1_CS3 */
#endif
	{-1},
};

static struct module_pin_mux mcasp_pin_mux[] = {
	{OFFSET(lcd_data10), 	(MODE(3) | INP_PULL_DOWN)},	/* U3: mcasp0_axr0	*/
	{OFFSET(lcd_data12), 	(MODE(3) | INP_PULL_DOWN)},	/* V2: mcasp0_aclkr	*/
	{OFFSET(lcd_data13), 	(MODE(3) | INP_PULL_DOWN)},	/* V3: mcasp0_fsr 	*/
	{OFFSET(lcd_data14), 	(MODE(3) | INP_PULL_DOWN)},	/* V4: mcasp0_axr1	*/
	{-1},
};

static struct module_pin_mux mmc0_pin_mux[] = {
	{OFFSET(mmc0_dat0),	(MODE(0) | INP_PULL_UP)},	/* G16: MMC0_DAT0 */
	{OFFSET(mmc0_dat1),	(MODE(0) | INP_PULL_UP)},	/* G15: MMC0_DAT1 */
	{OFFSET(mmc0_dat2),	(MODE(0) | INP_PULL_UP)},	/* F18: MMC0_DAT2 */
	{OFFSET(mmc0_dat3),	(MODE(0) | INP_PULL_UP)},	/* F17: MMC0_DAT3 */
	{OFFSET(mmc0_clk),	(MODE(0) | INP_PULL_UP)},	/* G17: MMC0_CLK */
	{OFFSET(mmc0_cmd),	(MODE(0) | INP_PULL_UP)},	/* G18: MMC0_CMD */
	{OFFSET(spi0_cs1),	(MODE(4) | INP_PULL_UP)},	/* A13: MMC0_CD */
	{OFFSET(gpmc_a3),	(MODE(7) | INP_PULL_UP)},	/* T14: GPIO1_19, SD_Eject */
	{-1},
};

static struct module_pin_mux rmii2_pin_mux[] = {
	{SMA2_REGISTER,		(0x0001U)},			/* Select MII2_CRS_DV */
	{OFFSET(mdio_clk),	(MODE(0) | OUT_PULL_UP)},	/* MDIO_CLK */
	{OFFSET(mdio_data),	(MODE(0) | INP_PULL_UP)},	/* MDIO_DATA */
	{OFFSET(gpmc_a0),	(MODE(3) | OUT)},		/* MII2_TXEN */
	{OFFSET(gpmc_a4),	(MODE(3) | OUT)},		/* MII2_TXD1 */
	{OFFSET(gpmc_a5),	(MODE(3) | OUT)},		/* MII2_TXD0 */
	{OFFSET(gpmc_a9),	(MODE(3) | INP)},		/* MII2_CRS_DV (requires SMA2==1) */
	{OFFSET(gpmc_a10),	(MODE(3) | INP)},		/* MII2_RXD1 */
	{OFFSET(gpmc_a11),	(MODE(3) | INP)},		/* MII2_RXD0 */
	{OFFSET(mii1_col),	(MODE(1) | INP)},		/* RMII2_REFCLK */
	{-1},
};

static struct module_pin_mux i2c0_pin_mux[] = {
	{OFFSET(i2c0_sda),	(MODE(0) | INP_PULL_DOWN | SLEWCTRL)},	/* I2C_DATA */
	{OFFSET(i2c0_scl),	(MODE(0) | INP_PULL_DOWN | SLEWCTRL)},	/* I2C_SCLK */
	{-1},
};

static struct module_pin_mux gpmc_pin_mux[] = {
	{OFFSET(gpmc_ad0),	(MODE(0) | INP)}, 		/* U7:  AD0  */
	{OFFSET(gpmc_ad1),	(MODE(0) | INP)}, 		/* V7:  AD1  */
	{OFFSET(gpmc_ad2),	(MODE(0) | INP)}, 		/* R8:  AD2  */
	{OFFSET(gpmc_ad3),	(MODE(0) | INP)}, 		/* T8:  AD3  */
	{OFFSET(gpmc_ad4),	(MODE(0) | INP)}, 		/* U8:  AD4  */
	{OFFSET(gpmc_ad5),	(MODE(0) | INP)}, 		/* V8:  AD5  */
	{OFFSET(gpmc_ad6),	(MODE(0) | INP)}, 		/* R9:  AD6  */
	{OFFSET(gpmc_ad7),	(MODE(0) | INP)}, 		/* T9:  AD7  */
	{OFFSET(gpmc_ad8),	(MODE(0) | INP)}, 		/* U10: AD8  */
	{OFFSET(gpmc_ad9),	(MODE(0) | INP)}, 		/* T10: AD9  */
	{OFFSET(gpmc_ad10),	(MODE(0) | INP)}, 		/* T11: AD10 */
	{OFFSET(gpmc_ad11),	(MODE(0) | INP)}, 		/* U12: AD11 */
	{OFFSET(gpmc_ad12),	(MODE(0) | INP)}, 		/* T12: AD12 */
	{OFFSET(gpmc_ad13),	(MODE(0) | INP)}, 		/* R12: AD13 */
	{OFFSET(gpmc_ad14),	(MODE(0) | INP)}, 		/* V13: AD14 */
	{OFFSET(gpmc_ad15),	(MODE(0) | INP)}, 		/* U13: AD15 */
	{OFFSET(gpmc_wait0),	(MODE(0) | INP_PULL_UP)}, 	/* T17: nWAIT */
	{OFFSET(gpmc_csn0),	(MODE(0) | OUT_PULL_UP)},	/* V6:  nCS0 FPGA */
	{OFFSET(gpmc_csn1),	(MODE(1) | INP_PULL_DOWN)},	/* U9:  GPMC_CLK */
	{OFFSET(gpmc_csn2),	(MODE(1) | OUT_PULL_UP)},	/* V9:  GPMC_BE1N */
	{OFFSET(gpmc_csn3),	(MODE(0) | OUT_PULL_UP)},	/* T13: nCS3 NAND */
	{OFFSET(gpmc_advn_ale),	(MODE(0) | OUT_PULL_DOWN)},	/* R7:  ADV_ALE */
	{OFFSET(gpmc_oen_ren),	(MODE(0) | OUT_PULL_UP)},	/* T7:  OE */
	{OFFSET(gpmc_wen),	(MODE(0) | OUT_PULL_UP)},	/* U6:  WEN */
	{OFFSET(gpmc_be0n_cle),	(MODE(0) | OUT_PULL_UP)},	/* T6:  BE_CLE */
	{-1},
};

static struct module_pin_mux gpio_pin_mux[] = {
	{OFFSET(gpmc_a1),	(MODE(7) | OUT_PULL_UP)},	/* GPIO1_17, ETH_nRST */
	{OFFSET(gpmc_a2),	(MODE(7) | INP_PULL_UP)},	/* GPIO1_18, ETH_nINTR */
	{OFFSET(gpmc_a6),	(MODE(7) | INP_PULL_UP)},	/* GPIO1_22, NETBOOT */
	{OFFSET(gpmc_a7),	(MODE(7) | INP_PULL_UP)},	/* GPIO1_23, PowDnWarn */
	{OFFSET(gpmc_a8),	(MODE(7) | INP_PULL_UP)},	/* GPIO1_24, Service_nENBuf */
	{OFFSET(lcd_pclk),	(MODE(7) | OUT_PULL_DOWN)},	/* GPIO2_24, GPB_Boot_Mode */
	{-1},
};

static struct module_pin_mux sp_if_pin_mux[] = {
	{OFFSET(gpmc_wpn),	(MODE(7) | OUT_PULL_DOWN)},	/* GPIO0_31, SP_IF_GP1 */
	{OFFSET(mcasp0_axr1),	(MODE(7) | OUT_PULL_DOWN)},	/* GPIO3_20, SP_IF_GP2 */
	{-1},
};

static struct module_pin_mux sysboot_pin_mux[] = {
	{OFFSET(lcd_data0),	(MODE(7) | INP)},		/* GPIO2_6,  SYSBOOT[0] */
	{OFFSET(lcd_data1),	(MODE(7) | INP)},		/* GPIO2_7,  SYSBOOT[1] */
	{OFFSET(lcd_data2),	(MODE(7) | INP)},		/* GPIO2_8,  SYSBOOT[2] */
	{OFFSET(lcd_data3),	(MODE(7) | INP)},		/* GPIO2_9,  SYSBOOT[3] */
	{OFFSET(lcd_data4),	(MODE(7) | INP)},		/* GPIO2_10, SYSBOOT[4] */
	{OFFSET(lcd_data5),	(MODE(7) | INP)},		/* GPIO2_11, SYSBOOT[5] */
	{OFFSET(lcd_data6),	(MODE(7) | INP)},		/* GPIO2_12, SYSBOOT[6] */
	{OFFSET(lcd_data7),	(MODE(7) | INP)},		/* GPIO2_13, SYSBOOT[7] */
	{OFFSET(lcd_data11),	(MODE(7) | INP)},		/* GPIO2_17, SYSBOOT[11] */
	{OFFSET(lcd_data15), 	(MODE(7) | INP)},		/* GPIO0_11, SYSBOOT[15] */
	{-1},
};

static struct module_pin_mux ddr3_pin_mux[] = {
	{OFFSET(lcd_ac_bias_en),(MODE(7) | OUT_PULL_UP)},	/* GPIO2_25, DDR3_VTT_EN */
	{-1},
};

static struct module_pin_mux usb_power_pin_mux[] = {
	{OFFSET(lcd_vsync),	(MODE(7) | INP_PULL_UP)},	/* GPIO2_22, USB_Host_Pwr_nError */
	{OFFSET(lcd_hsync),	(MODE(7) | OUT_PULL_DOWN)},	/* GPIO2_23, USB_Host_Pwr_En */
	{OFFSET(mii1_rxerr),	(MODE(7) | OUT_PULL_UP)},	/* GPIO3_2,  USBPWR */
	{-1},
};

static struct module_pin_mux spi_fpga_pin_mux[] = {
	{OFFSET(mcasp0_axr0),	(MODE(7) | INP_PULL_UP)},	/* GPIO3_16, SPI.nIRQ */
	{OFFSET(mcasp0_fsx),	(MODE(7) | OUT_PULL_UP)},	/* GPIO3_15, SPI.nCONFIG */
	{OFFSET(mcasp0_fsr),	(MODE(7) | INP_PULL_UP)},	/* GPIO3_19, SPI.nSTATUS */
	{OFFSET(mcasp0_aclkr),	(MODE(7) | INP_PULL_DOWN)},	/* GPIO3_18, SPI.CONFIG_DONE */
	{OFFSET(gpmc_clk),	(MODE(7) | INP_PULL_DOWN)},	/* GPIO2_1,  SPI.CRC_ERROR */
	{-1},
};

static struct module_pin_mux spy_fpga_pin_mux[] = {
	{OFFSET(mii1_rxdv),	(MODE(7) | INP_PULL_UP)},	/* GPIO3_4,  SPY.nIRQ */
	{OFFSET(mii1_rxd2),	(MODE(7) | OUT_PULL_UP)},	/* GPIO2_19, SPY.nRESET */
	{OFFSET(mcasp0_ahclkx),	(MODE(7) | OUT_PULL_UP)},	/* GPIO3_21, SPY.nCONFIG */
	{OFFSET(mcasp0_ahclkr),	(MODE(7) | INP_PULL_UP)},	/* GPIO3_17, SPY.nSTATUS */
	{OFFSET(usb0_drvvbus),	(MODE(7) | INP_PULL_DOWN)},	/* GPIO0_18, SPY.CONF_DONE*/
	{OFFSET(mii1_rxd3),	(MODE(7) | INP_PULL_DOWN)},	/* GPIO2_18, SPY.CRC_ERROR */

	{OFFSET(rmii1_refclk),	(MODE(7) | INP_PULL_UP)},	/* GPIO0_29, GPIO0 */
	{OFFSET(mii1_rxd0),	(MODE(7) | INP_PULL_UP)},	/* GPIO2_21, GPIO1 */
	{OFFSET(mii1_rxd1),	(MODE(7) | INP_PULL_UP)},	/* GPIO2_20, GPIO2 */
	{OFFSET(mii1_txd0),	(MODE(7) | INP_PULL_UP)},	/* GPIO0_28, GPIO3 */
	{OFFSET(mii1_txd1),	(MODE(7) | INP_PULL_UP)},	/* GPIO0_21, GPIO4 */
	{OFFSET(mii1_txen),	(MODE(7) | INP_PULL_UP)},	/* GPIO3_3,  GPIO5 */
	{OFFSET(mii1_crs),	(MODE(7) | INP_PULL_UP)},	/* GPIO3_1,  GPIO6 */
	{-1},
};

void enable_uart0_pin_mux(void)
{
	configure_module_pin_mux(uart0_pin_mux);
}

void enable_uart1_pin_mux(void)
{
	configure_module_pin_mux(uart1_pin_mux);
}

void enable_uart2_pin_mux(void)
{
	configure_module_pin_mux(uart2_pin_mux);
}

void enable_uart3_pin_mux(void)
{
}

void enable_uart4_pin_mux(void)
{
}

void enable_uart5_pin_mux(void)
{
	configure_module_pin_mux(uart5_pin_mux);
}

void enable_i2c0_pin_mux(void)
{
	configure_module_pin_mux(i2c0_pin_mux);
}

/*
 * The AM335x GP EVM, if daughter card(s) are connected, can have 8
 * different profiles.  These profiles determine what peripherals are
 * valid and need pinmux to be configured.
 */
#define PROFILE_NONE	0x0
#define PROFILE_0	(1 << 0)
#define PROFILE_1	(1 << 1)
#define PROFILE_2	(1 << 2)
#define PROFILE_3	(1 << 3)
#define PROFILE_4	(1 << 4)
#define PROFILE_5	(1 << 5)
#define PROFILE_6	(1 << 6)
#define PROFILE_7	(1 << 7)
#define PROFILE_MASK	0x7
#define PROFILE_ALL	0xFF

/* CPLD registers */
#define I2C_CPLD_ADDR	0x35
#define CFG_REG		0x10

u32	pin_config(u32 reg)
{
	return __raw_readl(CTRL_BASE + reg);
}

typedef	struct pin_info {
	u32	addr;
	u32	data;
	unsigned char name[16];
} pin_info_t;

void am33xx_spl_board_init(void)
{
}

void enable_board_pin_mux(void)
{
	/* SPU pinmux */
	configure_module_pin_mux(uart0_pin_mux);
	configure_module_pin_mux(uart1_pin_mux);
	configure_module_pin_mux(uart2_pin_mux);
	configure_module_pin_mux(uart5_pin_mux);
	configure_module_pin_mux(spi0_pin_mux);
	configure_module_pin_mux(spi1_pin_mux);
	configure_module_pin_mux(mmc0_pin_mux);
	configure_module_pin_mux(rmii2_pin_mux);
	configure_module_pin_mux(i2c0_pin_mux);
	configure_module_pin_mux(mcasp_pin_mux);
	configure_module_pin_mux(gpmc_pin_mux);
	configure_module_pin_mux(gpio_pin_mux);
	configure_module_pin_mux(sp_if_pin_mux);
	configure_module_pin_mux(sysboot_pin_mux);
	configure_module_pin_mux(ddr3_pin_mux);
	configure_module_pin_mux(usb_power_pin_mux);
	configure_module_pin_mux(spi_fpga_pin_mux);
	configure_module_pin_mux(spy_fpga_pin_mux);
}
