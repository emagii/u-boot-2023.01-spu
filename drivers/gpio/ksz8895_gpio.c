// SPDX-License-Identifier: GPL-2.0+
/*
 *
 * KSZ8895 GPIO driver for registers accessible over SPI
 *
 * Copyright (C) 2020 Bombardier Transportation
 * Ulf Samuelsson <ulf@emagii.com>
 *
 */

#include <common.h>
#include <errno.h>
#include <dm.h>
#include <fdtdec.h>
#include <malloc.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <dt-bindings/gpio/gpio.h>
#include <spi.h>

#define	DM_ACCESS
DECLARE_GLOBAL_DATA_PTR;

#define	KSZ8895_READ	0x03
#define	KSZ8895_WRITE	0x02
#define	KSZ8895_CMDLEN	(3 * 8)

struct gen_ksz8895_plat {
	char			name[8];
	struct udevice		*dev;
	struct spi_slave	*spi;
	u32			cs;
	u32 			nregs;
	int			flags;
};

static int gen_ksz8895_get_value(struct udevice *dev, unsigned int offset)
{
#if	!defined(DM_ACCESS)
	struct spi_slave *slave = dev_get_parent_priv(dev);
#endif
	struct gen_ksz8895_plat *ksz = dev_get_plat(dev);
	u8	dout[3];
	u8	din[3];
	int	ret;

	if (offset > ksz->nregs)
		return -ENODEV;

#if	defined(DM_ACCESS)
	ret = dm_spi_claim_bus(dev);
#else
	ret = spi_claim_bus(slave);
#endif
	if (ret)
		return ret;

	dout[0] = KSZ8895_READ;
	dout[1] = offset & 0xFF;
	dout[2] = 0;

	debug("%s MOSI: 0x%02X 0x%02X 0x%02X\n", __func__, dout[0], dout[1], dout[2]);
#if	defined(DM_ACCESS)
	ret = dm_spi_xfer(dev, KSZ8895_CMDLEN, dout, din,
			  SPI_XFER_BEGIN | SPI_XFER_END);
#else
	ret = spi_xfer(slave, KSZ8895_CMDLEN, dout, din, SPI_XFER_ONCE);
#endif
	debug("%s MISO: 0x%02X 0x%02X 0x%02X\n", __func__, din[0], din[1], din[2]);

#if	defined(DM_ACCESS)
	dm_spi_release_bus(dev);
#else
	spi_release_bus(slave);
#endif
	return din[2];
}

static int gen_ksz8895_set_value(struct udevice *dev, unsigned offset,
				int value)
{
#if	!defined(DM_ACCESS)
	struct spi_slave *slave = dev_get_parent_priv(dev);
#endif
	struct gen_ksz8895_plat *ksz = dev_get_plat(dev);
	u8	dout[3];
	u8	din[3];
	int ret;

	if (offset > ksz->nregs)
		return -ENODEV;

#if	defined(DM_ACCESS)
	ret = dm_spi_claim_bus(dev);
#else
	ret = spi_claim_bus(slave);
#endif
	if (ret)
		return ret;

	dout[0] = KSZ8895_WRITE;
	dout[1] = offset & 0xFF;
	dout[2] = value & 0xFF;

	debug("%s MOSI: 0x%02X 0x%02X 0x%02X\n", __func__, dout[0], dout[1], dout[2]);
#if	defined(DM_ACCESS)
	ret = dm_spi_xfer(dev, KSZ8895_CMDLEN, dout, din,
			  SPI_XFER_BEGIN | SPI_XFER_END);
#else
	ret = spi_xfer(slave, KSZ8895_CMDLEN, dout, din, SPI_XFER_ONCE);
#endif
	debug("%s MISO: 0x%02X 0x%02X 0x%02X\n", __func__, din[0], din[1], din[2]);

#if	defined(DM_ACCESS)
	dm_spi_release_bus(dev);
#else
	spi_release_bus(slave);
#endif
	return 0;
}

static int gen_ksz8895_probe(struct udevice *dev)
{
	struct udevice *bus = dev->parent;
	struct spi_slave *slave = dev_get_parent_priv(dev);
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);
	struct gen_ksz8895_plat *ksz = dev_get_plat(dev);
#if	0
	u8	reg1;
//	const void *blob = gd->fdt_blob;
//	int node = dev_of_offset(dev);
#endif

	debug("Probing KSZ8895 [%s]!\n", dev->driver->name);
	debug("SPI bus %s, cs %d\n", bus->name, slave_plat->cs);

	strcpy(ksz->name,"switch");
	ksz->dev = dev;
	ksz->spi = slave;
	ksz->cs  = slave_plat->cs;
	ksz->dev = dev;
	ksz->nregs = 256;
	ksz->flags = 0;

#if	0
	reg1 = gen_ksz8895_get_value(dev, 1);
	debug("KSZ8895 reg1 = 0x%02x\n", reg1);

//	debug("Enabling switch!\n");
//	gen_ksz8895_set_value(dev, 1, 1);

//	reg1 = gen_ksz8895_get_value(dev, 1);
//	debug("KSZ8895 reg1 = 0x%02x\n", reg1);
#endif
	debug("%s is ready\n", dev->name);
	return 0;
}

static const struct dm_gpio_ops gen_ksz8895_ops = {
	.get_value		= gen_ksz8895_get_value,
	.set_value		= gen_ksz8895_set_value,
};

static const struct udevice_id gen_ksz8895_ids[] = {
	{ .compatible = "bt,ksz8895-regs" },
	{ }
};

U_BOOT_DRIVER(ksz8895_regs) = {
	.name		= "ksz8895-regs",
	.id		= UCLASS_GPIO,
	.ops		= &gen_ksz8895_ops,
	.probe		= gen_ksz8895_probe,
	.plat_auto = sizeof(struct gen_ksz8895_plat),
	.of_match	= gen_ksz8895_ids,
};
