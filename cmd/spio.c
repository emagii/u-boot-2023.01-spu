// SPDX-License-Identifier: GPL-2.0+
/*
 * Command for accessing registers in SPI peripherals.
 *
 * Copyright (C) 2020 Bombardier Transportation
 *		Ulf Samuelsson <ext.ulf.samuelsson@rail.bombardier.com>
 *		Ulf Samuelsson <ulf@emagii.com>
 *
 */
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

static int spio_write(struct udevice *dev, unsigned int addr, unsigned int value)
{
	gpio_get_ops(dev)->set_value(dev, addr, value);
	return 0;
}

static int spio_read(struct udevice *dev, unsigned int addr)
{
	unsigned int value;
	value = gpio_get_ops(dev)->get_value(dev, addr);
	printf("%s: [0x%02X] = 0x%02X\n", dev->name /*argv[2]*/, addr, value & 0xFF);
	return 0;
}

/* [0]  [1]   [2]      [3]  [4]
 * spio write <device> addr value
 * spio read  <device> addr
 */
static int do_spi_gpio(struct cmd_tbl *cmdtp, int flag, int argc,
			char * const argv[])
{
	struct udevice	*dev;
	unsigned int	addr;
	unsigned int	value=0;
	const char	*cmd;
	char 		*endp;
	int		ret;


	/* need at least three arguments */
	if (argc < 4)
		goto usage;

	cmd = argv[1];	/* read|write */
	ret = uclass_get_device_by_name(UCLASS_GPIO, argv[2], &dev);
	if (ret) {
		printf("Unknown GPIO device %s\n", argv[2]);
		goto usage;
	}
	debug("%s: %s\n", __func__, dev->driver->name);

	/* Read address as hex */
	addr = simple_strtol(argv[3], &endp, 16);
	if (*argv[3] == 0 || *endp != 0) {
		goto usage;
	}

	switch (argc) {
	case	5:
		if (strcmp(cmd, "write") == 0) {
			value = simple_strtol(argv[4], &endp, 16);
			if (*argv[4] == 0 || *endp != 0) {
				goto usage;
			}
			ret = spio_write(dev, addr, value);
		} else {
			goto usage;
		}
		break;
	case	4:
		if (strcmp(cmd, "read") == 0) {
			ret = spio_read(dev, addr);
		} else  {
			goto usage;
		}
		break;
	default:
		goto usage;
	}

	if (ret >= 0)
		return ret;
usage:
	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	spio,	5,	1,	do_spi_gpio,
	"SPI GPIO sub-system",
	"spio read  <device> addr 	- read byte starting at 'addr'\n"
	"spio write <device> addr data	- write byte to 'addr'\n"
);
