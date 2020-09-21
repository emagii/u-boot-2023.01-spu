// SPDX-License-Identifier: GPL-2.0+
/*
 * MTD Driver for Passive Serial configuration of Cyclone 10
 *
 * Copyright (C) 2020 Bombardier Transportation
 * Ulf Samuelsson <ext.ulf.samuelsson@rail.bombardier.com>
 * Ulf Samuelsson <ulf@emagii.com>
 *
 */

#include <common.h>
#include <console.h>
#include <dm.h>
#include <errno.h>
#include <fdt_support.h>
//#include <flash.h>
#include <mtd.h>
#include <malloc.h>
#include <spi.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <dm/device_compat.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * How many milliseconds from CONF_DONE high to enter user mode
 * Datasheet says 650 us, Delay 2 ms to be safe...
 */
#define	USER_MODE_DELAY				2

struct cyc10_plat {
	struct udevice		*dev;
	struct spi_slave	*spi;
	char			name[8];
	struct gpio_desc	nconfig;
	struct gpio_desc	nstatus;
	struct gpio_desc	conf_done;
	struct gpio_desc	crc_error;
	u32			cs;
	int			flags;
	int			config_size;
};

static inline void write_nCONFIG(struct cyc10_plat *fpga, int value)
{
	dm_gpio_set_value(&fpga->nconfig, value);
}

static inline int read_nSTATUS(struct cyc10_plat *fpga)
{
	int val = dm_gpio_get_value(&fpga->nstatus);
	if (val < 0) {
		printf("%s: Failure reading nSTATUS; error=%d\n", fpga->name, val);
	}
	return val;
}

static inline int read_CONFIG_DONE(struct cyc10_plat *fpga)
{
	int val = dm_gpio_get_value(&fpga->conf_done);
	if (val < 0) {
		printf("%s: Failure reading CONFIG_DONE; error=%d\n", fpga->name, val);
	}
	return val;
}

static inline int read_CRC_ERROR(struct cyc10_plat *fpga)
{
	int val = dm_gpio_get_value(&fpga->crc_error);
	if (val < 0) {
		printf("%s: Failure reading CRC_ERROR; error=%d\n", fpga->name, val);
	}
	return val;
}

/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
static int cyc10_wait_until_conf_done(struct cyc10_plat *fpga)
{
	unsigned long timebase;
	timebase = get_timer(0);

	while (get_timer(timebase) < USER_MODE_DELAY) {
		if (read_nSTATUS(fpga) == 0)		/* Bad configuration */
			return -EIO;
		if (read_CONFIG_DONE(fpga) == 1)	/* Ready */
			return 0;
	}

	dev_err(fpga->dev, "fpga configuration timeout\n");
	return -ETIMEDOUT;
}


/* FPGA Passive Serial configuration is not erasable */
static int cyc10_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	instr->state = MTD_ERASE_DONE;
	return 0;
}

/* FPGA Passive Serial configuration is not readable, do dummy read */
static int cyc10_read(struct mtd_info *mtd, loff_t from, size_t len,
			    size_t *retlen, u_char *buf)
{
	for (int i = 0 ; i < len ; i++)
		buf[i] = 0xFF;
	*retlen = len;
	return 0;
}

static int cyc10_write(struct mtd_info *mtd, loff_t to, size_t len,
			     size_t *retlen, const u_char *buf)
{
	struct udevice *dev = mtd->dev;
	struct spi_slave *slave = dev_get_parent_priv(dev);
	struct cyc10_plat *fpga = dev_get_plat(dev);
	int ret;

	debug("%s: Claiming SPI Bus\n", fpga->name);
	ret = spi_claim_bus(slave);
	if (ret) {
		printf("Failed to claim SPI bus\n");
		return ret;
	}

	debug("%s: Starting configuration\n", fpga->name);
	/* The FPGA configuration start by a positive edge on nCONFIG */
	write_nCONFIG(fpga, 0);
	udelay(50);	// Should be low for at least 40 us according to spec.
	debug("nSTATUS = %d\n", read_nSTATUS(fpga));
	write_nCONFIG(fpga, 1);
	/* The FPGA is ready when nSTATUS is high */
	{
		int timeout = 200;
		while(1) {
			if (read_nSTATUS(fpga) != 0) {
				break;
			}
			udelay(10);
			timeout -= 10;
			if (timeout < 0) {
				debug("nSTATUS remains low\n");
				return -ETIMEDOUT;
			}
			debug("nSTATUS = %d\n", read_nSTATUS(fpga));
		}
	}

	/* FPGA needs some additional clocks to start up, add 8 bits */
	/* This means we read past the buffer, which is hopefully OK */
	debug("%s: Writing %d bytes\n", fpga->name, len);
	ret = spi_xfer(slave, (len * 8) + 8, buf, NULL,
		       SPI_XFER_ONCE | SPI_LSB_FIRST);

	debug("%s: Releasing SPI Bus\n", fpga->name);
	spi_release_bus(slave);

	debug("%s: Waiting for CONF_DONE\n", fpga->name);

	ret = cyc10_wait_until_conf_done(fpga);
	*retlen = len;
	return ret;
}

static void cyc10_sync(struct mtd_info *mtd)
{
}

static int cyc10_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	return 0;
}

static int cyc10_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	return 0;
}

static int pin_request(struct udevice *dev, const char *pin_name, struct gpio_desc *pin, int mode)
{
	int	ret;
	debug("gpio request: %s", pin_name);
	if (gpio_request_by_name(dev, pin_name, 0, pin, mode )) {
		debug("FAIL");
		ret = -EINVAL;
	} else {
		ret = 0;
		debug("OK");
		debug("%s:%s: %s:[%d]\n", __func__, pin_name, pin->dev->name, pin->offset);
	}
	return ret;
}

static int cyc10_probe(struct udevice *dev)
{
	struct spi_slave *slave = dev_get_parent_priv(dev);
	struct dm_spi_slave_plat *plat = dev_get_parent_plat(dev);
	struct cyc10_plat *fpga = dev_get_plat(dev);
	struct mtd_info *mtd;
	int ret = 0;
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(dev);
	/*
	 * Use "fpga" property as device name if it exists.
	 * Otherwise, use "fpga#" as device name # € {0..3}«»©“”nµªßðđŋħ
	 */
	const unsigned char *name = fdtdec_locate_byte_array(blob, node, "fpga", 4);
	if (name == NULL) {
		for (int i = 0; i < 3; i++) {
			sprintf(fpga->name, "fpga%d", i);
			mtd = get_mtd_device_nm(fpga->name);
			if (PTR_ERR(mtd) == -ENODEV) {
				goto allocate;
			}
		}
		printf("Cannot allocate FPGA device\n");
		return -ENODEV;
	} else {
		strncpy(fpga->name, (char *) name, sizeof(fpga->name));
	}

allocate:
	fpga->spi = slave;
	fpga->dev = dev;
	fpga->cs  = plat->cs;

	debug("%s: slave=%p, cs=%d\n", __func__, slave, fpga->cs);
	fpga->config_size = fdtdec_get_int(blob, node, "config-size", 0);
	debug("%s:config-size: %s:[%d]\n", __func__, fpga->name, fpga->config_size);

	ret  = pin_request(dev, "nconfig-gpios",	&fpga->nconfig,   GPIOD_IS_OUT);
	ret |= pin_request(dev, "nstat-gpios",		&fpga->nstatus,   GPIOD_IS_IN);
	ret |= pin_request(dev, "confd-gpios",		&fpga->conf_done, GPIOD_IS_IN);
	if (ret != 0)
		return ret;
	ret  = pin_request(dev, "crc-error-gpios",	&fpga->crc_error, GPIOD_IS_IN);	// May fail.

	mtd = dev_get_uclass_priv(dev);
	mtd->dev = dev;
	mtd->name		= fpga->name;
	mtd->type		= MTD_FPGA;
	mtd->flags		= MTD_WRITEABLE;
	mtd->size		= fpga->config_size;
	mtd->writesize		= fpga->config_size;
	mtd->writebufsize	= mtd->writesize;
	mtd->_erase		= cyc10_erase;
	mtd->_read		= cyc10_read;
	mtd->_write		= cyc10_write;
	mtd->_sync		= cyc10_sync;
	mtd->_lock		= cyc10_lock;
	mtd->_unlock		= cyc10_unlock;
	mtd->numeraseregions = 0;
	mtd->erasesize = 0x10000;
	if (add_mtd_device(mtd)) {
		debug("%s: registering %s as MTD failed\n", __func__, fpga->name);
		return -ENOMEM;
	}
	return 0;
}

static const struct udevice_id cyc10_ids[] = {
	{ .compatible = "intel,cyclone10" },
	{}
};

U_BOOT_DRIVER(cyc10) = {
	.name	= "cyc10",
	.id	= UCLASS_MTD,
	.of_match = cyc10_ids,
//	.priv_auto_alloc_size	= sizeof(struct cyc10_priv),
	.plat_auto = sizeof(struct cyc10_plat),
	.probe	= cyc10_probe,
};
