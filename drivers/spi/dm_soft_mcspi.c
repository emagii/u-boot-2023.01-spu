// SPDX-License-Identifier: GPL-2.0+
/*
 * Bitbang SPI driver supporting multiple chip selects
 * Adopted from drivers/spi/soft_spi.c
 *
 * Copyright (C) 2020 Bombardier Transportation
 *		Ulf Samuelsson <ext.ulf.samuelsson@rail.bombardier.com>
 *		Ulf Samuelsson <ulf@emagii.com>
 *
 * Copyright (c) 2014 Google, Inc
 *
 * Copyright (C) 2002
 * 		Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com.
 *
 * Influenced by code from:
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */
#define	DEBUG
#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <fdtdec.h>
#include <malloc.h>
#include <spi.h>
#include <asm/gpio.h>
#include <linux/delay.h>

DECLARE_GLOBAL_DATA_PTR;

extern void pinmux_print(void);

struct dm_soft_mcspi_priv {
	struct gpio_desc *cs_gpios;
	struct gpio_desc *cs;
	struct gpio_desc sclk;
	struct gpio_desc mosi;
	struct gpio_desc miso;
	unsigned int spi_delay_us;
	unsigned int num_cs;
	unsigned int csnum;
	unsigned int flags;
	unsigned int mode;
};

#define SPI_MASTER_NO_RX	BIT(0)
#define SPI_MASTER_NO_TX	BIT(1)

#define	SPI_CLK(bit)		dm_gpio_set_value(&priv->sclk, bit)
#define SPI_MOSI(bit)		dm_gpio_set_value(&priv->mosi, bit)
#define SPI_MISO		dm_gpio_get_value(&priv->miso)
#define	SPI_DEASSERT_CS		dm_gpio_set_value(priv->cs, 0);
#define	SPI_ASSERT_CS		dm_gpio_set_value(priv->cs, 1);
#define	SPI_DEASSERT_CS0	dm_gpio_set_value(&priv->cs_gpios[0], 0);
#define	SPI_ASSERT_CS0		dm_gpio_set_value(&priv->cs_gpios[0], 1);

#ifdef	SPI_TRACE
#define	SPI_ACTIVATE		\
	SPI_DEASSERT_CS;	\
	SPI_DEASSERT_CS0;	\
	SPI_CLK(0);		\
	SPI_ASSERT_CS0;		\
	SPI_ASSERT_CS

#define	SPI_DEACTIVATE		SPI_DEASSERT_CS0; \
				SPI_DEASSERT_CS
#else
#define	SPI_ACTIVATE		\
	SPI_DEASSERT_CS;	\
	SPI_CLK(0);		\
	SPI_ASSERT_CS

#define	SPI_DEACTIVATE		SPI_DEASSERT_CS
#endif

static int soft_mcspi_claim_bus(struct udevice *dev)
{
	/*
	 * Make sure the SPI clock is in idle state as defined for
	 * this slave.
	 */
	struct udevice *bus = dev_get_parent(dev);
	struct dm_soft_mcspi_priv *priv = dev_get_priv(bus);
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);
	int	cs = slave_plat->cs;
	if (cs < priv->num_cs) {
		priv->csnum = cs;
		priv->cs = &priv->cs_gpios[cs];
	} else {
		debug("%s: failed to select cs %d\n", __func__, cs);
		return -ENODEV;
	}
	debug("%s: selected cs %d\n", __func__, cs);
	SPI_CLK(0);
	return 0;
}

static int soft_mcspi_release_bus(struct udevice *dev)
{
	/* Nothing to do */
	return 0;
}

/*
 * Optimized routine for 8-bit transmits.
 * Mode 0
 * MSB first
 * Transmit only
 * 8-bit bytes first, then remaining bits
 */
static void soft_mcspi_tx8_msb_mode0(struct dm_soft_mcspi_priv *priv, unsigned int bitlen,
			 const void *dout)
{
	unsigned char		tmpdout = 0;
	const unsigned char	*txd = dout;
	unsigned int		bytes = bitlen >> 3;
	unsigned int 		bits = bitlen & 0x7;
	unsigned int		delay = priv->spi_delay_us;

	debug("fast 8-bit SPI transfer (MSB): %d bytes, %d bits\n", bytes, bitlen);
	for (unsigned int i = (bytes/sizeof(unsigned char)) ; i > 0 ; i -= 1) {
		tmpdout = *txd++;
		for (unsigned int bit = 0; bit < 8; bit++) {
			SPI_CLK(0);
			if (delay)
				udelay(delay);
			SPI_MOSI((tmpdout & 0x80)?1:0);
			SPI_CLK(1);
			if (delay)
				udelay(delay);
			tmpdout	<<= 1;
		}
	}
	if (bits != 0) {
		tmpdout = *txd++;
		for (unsigned int bit = bits; bit > 0; bit++) {
			SPI_CLK(0);
			if (delay)
				udelay(delay);
			SPI_MOSI((tmpdout & 0x80)?1:0);
			SPI_CLK(1);
			if (delay)
				udelay(delay);
			tmpdout	<<= 1;
		}
	}
	SPI_CLK(0);
}

/*
 * Optimized routine for FPGA programming
 * Mode 0
 * LSB first
 * Transmit only
 * 8-bit bytes first, then remaining bits
 */
static void soft_mcspi_tx8_lsb_mode0(struct dm_soft_mcspi_priv *priv, unsigned int bitlen,
			 const void *dout)
{
	unsigned char 		tmpdout = 0;
	const unsigned char	*txd = dout;
	unsigned int		bytes = bitlen >> 3;
	unsigned int 		bits = bitlen & 0x7;
	unsigned int		delay = priv->spi_delay_us;
	debug("fast 8-bit SPI transfer (LSB): %d bytes, %d bits\n", bytes, bitlen);

	for (unsigned int i = (bytes/sizeof(unsigned char)) ; i > 0 ; i -= 1) {
		tmpdout = *txd++;
		for (unsigned int bit = 0; bit < 8; bit++) {
			SPI_CLK(0);
			if (delay)
				udelay(delay);
			SPI_MOSI((tmpdout & 0x01)?1:0);
			SPI_CLK(1);
			if (delay)
				udelay(delay);
			tmpdout	>>= 1;
		}
	}
	if (bits != 0) {
		tmpdout = *txd++;
		for (unsigned int bit = bits; bit > 0; bit++) {
			SPI_CLK(0);
			if (delay)
				udelay(delay);
			SPI_MOSI((tmpdout & 0x01)?1:0);
			SPI_CLK(1);
			if (delay)
				udelay(delay);
			tmpdout	>>= 1;
		}
	}
	SPI_CLK(0);
}

/*-----------------------------------------------------------------------
 * SPI transfer
 *
 * This writes "bitlen" bits out the SPI MOSI port and simultaneously clocks
 * "bitlen" bits in the SPI MISO port.  That's just the way SPI works.
 *
 * The source of the outgoing bits is the "dout" parameter and the
 * destination of the input bits is the "din" parameter.  Note that "dout"
 * and "din" can point to the same memory location, in which case the
 * input data overwrites the output data (since both are buffered by
 * temporary variables, this is OK).
 */
static int soft_mcspi_xfer(struct udevice *dev, unsigned int bitlen,
			 const void *dout, void *din, unsigned long flags)
{
	struct udevice *bus = dev_get_parent(dev);
	struct dm_soft_mcspi_priv *priv = dev_get_priv(bus);

	uchar		tmpdin  = 0;
	uchar		tmpdout = 0;
	const u8	*txd = dout;
	u8		*rxd = din;
	int		cpha = (priv->mode & SPI_CPHA)?1:0;
	int		ncpha = cpha ^ 1;
	unsigned int	j;
	int		rx = (priv->flags & SPI_MASTER_NO_RX) == 0;
	int		tx = (priv->flags & SPI_MASTER_NO_TX) == 0;
	unsigned int	mode	= priv->mode & 0x3;
	unsigned int	delay = priv->spi_delay_us;

	pinmux_print();

	debug("%s: slave %s:%s dout %08X din %08X bitlen %u\n",__func__ ,
	      dev->parent->name, dev->name, (uint) txd, (uint) rxd,
	      bitlen);

	if (flags & SPI_XFER_BEGIN) {
		debug("%s: activate cs\n", __func__);
		SPI_ACTIVATE;
	}
	if ((din == NULL) & (mode == SPI_MODE_0)) {
		// optimize for Transmit programming in mode 0
		if (flags & SPI_LSB_FIRST) {
			soft_mcspi_tx8_lsb_mode0(priv, bitlen, dout);
		} else {
			soft_mcspi_tx8_msb_mode0(priv, bitlen, dout);
		}
	} else {
		debug("%s: non-optimized transfer\n", __func__);
		if (flags & SPI_LSB_FIRST) {
			return -EINVAL;
		}
		for (j = 0; j < bitlen; j++) {
			/*
			 * Check if it is time to work on a new byte.
			 */
			if ((j & 0x7) == 0) {
				if (txd)
					tmpdout = *txd++;
				else
					tmpdout = 0;
				if (j != 0) {
					if (rxd)
						*rxd++ = tmpdin;
				}
				tmpdin  = 0;
			}

			if (ncpha) {
				SPI_CLK(0);
			}
			if (tx) {
				SPI_MOSI((tmpdout & 0x80)?1:0);
			}
			if (delay)
				udelay(delay);
			SPI_CLK(ncpha);

			tmpdin	<<= 1;
			if (rx) {
				tmpdin	|= SPI_MISO;
			}
			tmpdout	<<= 1;
			if (delay)
				udelay(delay);
			if (cpha) {
				SPI_CLK(1);
			}
		}
	/*
	 * If the number of bits isn't a multiple of 8, shift the last
	 * bits over to left-justify them.  Then store the last byte
	 * read in.
	 */
		if (rxd) {
			if ((bitlen % 8) != 0)
				tmpdin <<= 8 - (bitlen % 8);
			*rxd++ = tmpdin;
		}
		if (ncpha) {
			SPI_CLK(0);
		}
	}
	if (flags & SPI_XFER_END) {
		debug("%s: deactivate cs\n", __func__);
		SPI_DEACTIVATE;
	}
	return 0;
}

static int soft_mcspi_set_speed(struct udevice *dev, unsigned int speed)
{
	/* Accept any speed, use "spi-delay-us" to slow down */
	return 0;
}

static int soft_mcspi_set_mode(struct udevice *dev, unsigned int mode)
{
	struct dm_soft_mcspi_priv *priv = dev_get_priv(dev);

	priv->mode = mode;

	return 0;
}

static const struct dm_spi_ops soft_mcspi_ops = {
	.claim_bus	= soft_mcspi_claim_bus,
	.release_bus	= soft_mcspi_release_bus,
	.xfer		= soft_mcspi_xfer,
	.set_speed	= soft_mcspi_set_speed,
	.set_mode	= soft_mcspi_set_mode,
};

static int soft_mcspi_probe(struct udevice *dev)
{
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);
	struct dm_soft_mcspi_priv *priv =dev_get_priv(dev);
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(dev);

	int num_gpios;
	int cs_flags, clk_flags;
	int ret;

	priv->spi_delay_us = 1;
	if (slave_plat) {
		priv->mode = slave_plat->mode;
	} else {
		priv->mode = 0;
	}

	priv->num_cs = fdtdec_get_int(blob, node, "num-cs", 1);
	debug("%s: %d chipselects\n", dev->name, priv->num_cs);

	num_gpios = gpio_get_list_count(dev, "cs-gpios");
	if (num_gpios != priv->num_cs) {
		dev_err(dev,"Number of cs-gpios does not match num-cs");
		return -EINVAL;
	}
	if (num_gpios <= 0) {
		dev_err(dev, "Minimum number of chipselects == 1\n");
		return -EINVAL;
	}

	cs_flags = (priv->mode & SPI_CS_HIGH) ? 0 : GPIOD_ACTIVE_LOW;
	priv->cs_gpios = malloc(sizeof(struct gpio_desc) * num_gpios);
	ret = gpio_request_list_by_name(dev, "cs-gpios", priv->cs_gpios,
					num_gpios, GPIOD_IS_OUT | cs_flags);
	if (ret < 0) {
		dev_err(dev, "cs-gpios not defined\n");
		free(priv->cs_gpios);
		return -EINVAL;
	}

	clk_flags = (priv->mode & SPI_CPOL) ? GPIOD_ACTIVE_LOW : 0;
	ret = gpio_request_by_name(dev, "gpio-sck", 0, &priv->sclk,
				 GPIOD_IS_OUT | clk_flags);
	if (ret)
		return -EINVAL;
	debug("soft-mcspi allocate: sck: %s_%d\n", priv->sclk.dev->name, priv->sclk.offset);

	ret = gpio_request_by_name(dev, "gpio-mosi", 0, &priv->mosi,
				   GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	if (ret)
		priv->flags |= SPI_MASTER_NO_TX;
	debug("soft-mcspi allocate: mosi:%s_%d\n", priv->mosi.dev->name, priv->mosi.offset);

	ret = gpio_request_by_name(dev, "gpio-miso", 0, &priv->miso,
				   GPIOD_IS_IN);
	if (ret)
		priv->flags |= SPI_MASTER_NO_RX;
	debug("soft-mcspi allocate: miso:%s_%d:\n", priv->miso.dev->name, priv->miso.offset);


	if ((priv->flags & (SPI_MASTER_NO_RX | SPI_MASTER_NO_TX)) ==
	    (SPI_MASTER_NO_RX | SPI_MASTER_NO_TX))
		return -EINVAL;

	priv->spi_delay_us = fdtdec_get_int(blob, node, "spi-delay-us", 0);

	return 0;
}

static const struct udevice_id soft_mcspi_ids[] = {
	{ .compatible = "mcspi-gpio" },
	{ }
};

U_BOOT_DRIVER(soft_mcspi) = {
	.name	= "soft_mcspi",
	.id	= UCLASS_SPI,
	.of_match = soft_mcspi_ids,
	.ops	= &soft_mcspi_ops,
	.priv_auto = sizeof(struct dm_soft_mcspi_priv),
	.probe	= soft_mcspi_probe,
};
