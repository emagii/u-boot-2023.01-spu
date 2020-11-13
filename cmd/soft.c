// SPDX-License-Identifier: GPL-2.0+
/*
 * Command for accessing SPI flash.
 *
 * Copyright (C) 2008 Atmel Corporation
 */
#define	DEBUG
#include <common.h>
#include <command.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <log.h>
#include <malloc.h>
#include <mapmem.h>
#include <spi.h>
#include <asm/io.h>
#include <asm/gpio.h>


#define	SLEW_CONTROL	0x40
#define	RX_ACTIVE	0x20
#define	PULL_MASK	0x18
#define	PULL_UP		0x10
#define	PULL_DOWN	0x00
#define	NO_PULL_DOWN	0x08
#define	NO_PULL_UP	0x18
#define	MODE_SEL	0x07
struct pinmux {
	u32	address;
	char	*pin;
	char	*name;
	char	*mode4;
	char	*mode7;
	u32	 hw_value;
	u32	 sw_value;
};

const struct pinmux spi1_pins[7] = {
	{ 0x44e10964, "C18", "spi1_sclk:", "sclk",  "gpio0_7", 0x24, 0x27, },
	{ 0x44e10968, "E18", "spi1_miso:", "miso",  "gpio1_8", 0x24, 0x27, },
	{ 0x44e1096c, "E17", "spi1_mosi:", "mosi",  "gpio1_9", 0x24, 0x27, },
	{ 0x44e10978, "D18", "spi1_cs0:",  "cs0",   "gpio0_12",0x17, 0x17, },
	{ 0x44e1097c, "D17", "spi1_cs1:",  "cs1",   "gpio0_13",0x17, 0x17, },
	{ 0x44e10920, "K15", "spi1_cs2:",  "<bad>", "gpio0_17",0x17, 0x17, },
	{ 0x44e1091c, "J18", "spi1_cs3:",  "<bad>", "gpio0_16",0x17, 0x17, }
};

void pinmux_print(void)
{
	u32	*reg;
	u32	i;
	u32	value;
	u32	mode;
	u32	pull;
	for (i = 0; i < 7 ; i++) {
		reg = (u32 *) spi1_pins[i].address;
		debug("%s %-12s ", spi1_pins[i].pin, spi1_pins[i].name);
		value = *reg;
		mode = value & MODE_SEL;
		debug("mode %d ", mode);
		switch (mode) {
		case 4:
			debug("%-10s ", spi1_pins[i].mode4);
			break;
		case 7:
			debug("%-10s ", spi1_pins[i].mode7);
			break;
		default:
			debug("%-10s ", "<unknown>");
			break;
		}
		if (value & SLEW_CONTROL) {
			debug("slew ");
		} else {
			debug("     ");
		}
		if (value & RX_ACTIVE) {
			debug("rx ");
		} else {
			debug("   ");
		}
		pull = (value & PULL_MASK) >> 3;
		switch (pull) {
		case 0:
			debug("pull-down");
			break;
		case 2:
			debug("pull-up");

			break;
		default:
			debug("no pull");
			break;
		}
		debug("\n");
	}
}


static void	spi1_hwif(void)
{
	u32	*reg;
	for (u32 i = 0; i < 7; i++) {
		reg = (u32 *) spi1_pins[i].address;
		*reg = spi1_pins[i].hw_value;
	}
}

static void	spi1_soft(void)
{
	u32	*reg;
	for (u32 i = 0; i < 7; i++) {
		reg = (u32 *) spi1_pins[i].address;
		*reg = spi1_pins[i].sw_value;
	}
}

/* [0]  [1]   [2]      [3]  [4]
 * soft on
 * soft off
 */
static int do_soft(struct cmd_tbl *cmdtp, int flag, int argc,
			char * const argv[])
{
	const char *cmd;

	/* need at least two arguments */
	if (argc >= 2) {
		cmd = argv[1];	/* probe|read|write */
		if (strcmp(cmd, "off") == 0) {
			spi1_hwif();
		} else if (strcmp(cmd, "on") == 0) {
			spi1_soft();
		} else {
			goto usage;
		}
	}
	pinmux_print();
	return 0;
usage:
	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	soft,	5,	1,	do_soft,
	"SPI select sub-system",
	"soft off		 	- Use Soft SPI\n"
	"soft on			- Use H/W SPI\n"
);
