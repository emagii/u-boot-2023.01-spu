// SPDX-License-Identifier: GPL-2.0+
/*
 * eMagii SPI driver
 *
 * based on bfin_spi.c
 * Copyright (c) 2005-2008 Analog Devices Inc.
 * Copyright (C) 2010 Thomas Chou <thomas@wytron.com.tw>
 *
 * This driver is based on a structure where both commands
 * and data can be written to a FIFO.
 * The address is used to determine if the written byte is a
 * command or data.
 * The FIFO is tagged to determine the class of the content.
 */
#include <common.h>
#include <dm.h>
#include <errno.h>
#include <malloc.h>
#include <fdtdec.h>
#include <spi.h>
#include <asm/io.h>

#define EMAGII_SPI_CS_CONTROL   0
#define EMAGII_SPI_CS_CONTROL0  0
#define EMAGII_SPI_CS_CONTROL1  1
#define EMAGII_SPI_WRITE_BYTE   101
#define EMAGII_SPI_READ_BYTE    102
#define EMAGII_SPI_READ         103
#define EMAGII_SPI_SPEED        104
#define EMAGII_SPI_MODE         105
#define EMAGII_SPI_CLAIM        106
#define EMAGII_SPI_RELEASE      107

#define EMAGII_SPI_STATUS_RRDY_MSK	BIT(7)
#define EMAGII_SPI_CONTROL_SSO_MSK	BIT(10)

#ifndef CONFIG_EMAGII_SPI_IDLE_VAL
#define CONFIG_EMAGII_SPI_IDLE_VAL	0xff
#endif

typedef	u16	spi_cmd[256];
#define	RANGE(x)	((x) - sizeof(spi_cmd)/sizeof(u16))

struct emagii_spi_regs {
	u16	FIFOOUT[RANGE(4096)];
	spi_cmd	cmd;
	u16	FIFOIN[RANGE(4096)];
	u32	slave_sel;
};

struct emagii_spi_plat {
	struct emagii_spi_regs *regs;
};

struct emagii_spi_priv {
	struct emagii_spi_regs *regs;
	u16    speed;
	u16    mode;
};

static void spi_cs_activate(struct udevice *dev)
{
#if CONFIG_IS_ENABLED(DM_GPIO)
	struct udevice *bus = dev_get_parent(dev);
	struct emagii_spi_priv *priv = dev_get_priv(bus);
	struct emagii_spi_regs *const regs = priv->regs;
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);
	u32 cs = slave_plat->cs;
	writel(0, &regs->cmd[EMAGII_SPI_CS_CONTROL + cs]);
#endif
}

static void spi_cs_deactivate(struct udevice *dev)
{
#if CONFIG_IS_ENABLED(DM_GPIO)
	struct udevice *bus = dev_get_parent(dev);
	struct emagii_spi_priv *priv = dev_get_priv(bus);
	struct emagii_spi_regs *const regs = priv->regs;
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);
	u32 cs = slave_plat->cs;
	writel(1, &regs->cmd[EMAGII_SPI_CS_CONTROL + cs]);
#endif
}

static int emagii_spi_claim_bus(struct udevice *dev)
{
	struct udevice *bus = dev->parent;
	struct emagii_spi_priv *priv = dev_get_priv(bus);
	struct emagii_spi_regs *const regs = priv->regs;
	writel(1, &regs->cmd[EMAGII_SPI_CLAIM]);
	return 0;
}

static int emagii_spi_release_bus(struct udevice *dev)
{
	struct udevice *bus = dev->parent;
	struct emagii_spi_priv *priv = dev_get_priv(bus);
	struct emagii_spi_regs *const regs = priv->regs;
	writel(1, &regs->cmd[EMAGII_SPI_RELEASE]);

	return 0;
}

static int emagii_spi_xfer(struct udevice *dev, unsigned int bitlen,
			    const void *dout, void *din, unsigned long flags)
{
	struct udevice *bus = dev->parent;
	struct emagii_spi_priv *priv = dev_get_priv(bus);
	struct emagii_spi_regs *const regs = priv->regs;
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);
	uint   cs;

	/* assume spi core configured to do 8 bit transfers */
	unsigned int words = bitlen / 16;		/* words to transfer */
	unsigned int bytes = (bitlen / 8) & 0x1;	/* only remaining bytes */
	const u16 *txp = dout;
	u16 *fifo = &regs->FIFOOUT[0];
	unsigned char *rxp = din;
//	uint32_t reg, data, start;

	cs = (slave_plat->cs) & 0x1;	/* Only support two chip selects */
	debug("%s: bus:%i cs:%i bitlen:%i bytes:%i flags:%lx\n", __func__,
	      bus->seq_, cs, bitlen, bytes, flags);

	if (bitlen == 0)
		goto done;


	/* empty read buffer */

	if (flags & SPI_XFER_BEGIN)
		spi_cs_activate(dev);

	if (dout != NULL) {
		/* Should use DMA */
		words = bitlen / 16;
		while (words) {
			*fifo++ = *txp++;
			words--;
		}
		bytes = (bitlen / 8) & 0x01;
		if (bytes) {
			writew(*txp, &regs->cmd[EMAGII_SPI_WRITE_BYTE]);
		}
	} else {
		writew(bitlen / 8, &regs->cmd[EMAGII_SPI_READ]); /* H/W assisted read */
	}

	if (din != NULL) {
		/* Should use DMA */
		words = bitlen / 16;
		fifo = &regs->FIFOIN[0];
		while (words) {
			*rxp++ = *fifo++;
			words--;
		}
		bytes = (bitlen / 8) & 0x01;
		if (bytes) {
			*rxp++ = readb(&regs->cmd[EMAGII_SPI_READ_BYTE]);
		}
	}

done:
	if (flags & SPI_XFER_END)
		spi_cs_deactivate(dev);

	return 0;
}

static int emagii_spi_set_speed(struct udevice *bus, uint speed)
{
	struct emagii_spi_priv *priv = dev_get_priv(bus);
	struct emagii_spi_regs *const regs = priv->regs;
	priv->speed = speed;
	writel(speed, &regs->cmd[EMAGII_SPI_SPEED]);
	return 0;
}

static int emagii_spi_set_mode(struct udevice *bus, uint mode)
{
	struct emagii_spi_priv *priv = dev_get_priv(bus);
	struct emagii_spi_regs *const regs = priv->regs;
	priv->mode = mode;
	writel(mode, &regs->cmd[EMAGII_SPI_MODE]);
	return 0;
}

static int emagii_spi_probe(struct udevice *bus)
{
	struct emagii_spi_plat *plat = dev_get_plat(bus);
	struct emagii_spi_priv *priv = dev_get_priv(bus);

	priv->regs = plat->regs;
	priv->speed = 0;
	priv->mode = 0;
	return 0;
}

static int emagii_spi_ofdata_to_plat(struct udevice *bus)
{
	struct emagii_spi_plat *plat = dev_get_plat(bus);

	plat->regs = map_physmem(devfdt_get_addr(bus),
				 sizeof(struct emagii_spi_regs),
				 MAP_NOCACHE);

	return 0;
}

static const struct dm_spi_ops emagii_spi_ops = {
	.claim_bus	= emagii_spi_claim_bus,
	.release_bus	= emagii_spi_release_bus,
	.xfer		= emagii_spi_xfer,
	.set_speed	= emagii_spi_set_speed,
	.set_mode	= emagii_spi_set_mode,
	/*
	 * cs_info is not needed, since we require all chip selects to be
	 * in the device tree explicitly
	 */
};

static const struct udevice_id emagii_spi_ids[] = {
	{ .compatible = "emagii,spi-1.0" },
	{}
};

U_BOOT_DRIVER(emagii_spi) = {
	.name	= "emagii_spi",
	.id	= UCLASS_SPI,
	.of_match = emagii_spi_ids,
	.ops	= &emagii_spi_ops,
	.of_to_plat = emagii_spi_ofdata_to_plat,
	.plat_auto = sizeof(struct emagii_spi_plat),
	.priv_auto = sizeof(struct emagii_spi_priv),
	.probe	= emagii_spi_probe,
};
