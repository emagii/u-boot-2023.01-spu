// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for McSPI controller on OMAP3. Based on davinci_spi.c
 * Chip Selects handled using GPIO pins.
 *
 * Copyright (C) 2020 Bombardier Transportation
 *		Ulf Samuelsson <ext.ulf.samuelsson@rail.bombardier.com>
 *		Ulf Samuelsson <ulf@emagii.com>
 *
 * Copyright (C) 2016 Jagan Teki <jteki@openedev.com>
 *		      Christophe Ricard <christophe.ricard@gmail.com>
 *
 * Copyright (C) 2010 Dirk Behme <dirk.behme@googlemail.com>
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Copyright (C) 2007 Atmel Corporation
 *
 * Parts taken from linux/drivers/spi/omap2_mcspi.c
 * Copyright (C) 2005, 2006 Nokia Corporation
 *
 * Modified by Ruslan Araslanov <ruslan.araslanov@vitecmm.com>
 */

#include <common.h>
#include <ansi.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <fdtdec.h>
#include <log.h>
#include <malloc.h>
#include <spi.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/bitrev.h>
#include <linux/delay.h>
DECLARE_GLOBAL_DATA_PTR;

#define OMAP4_MCSPI_REG_OFFSET	0x100

#define CONFIG_OMAP3_SPI_TRACE

struct omap2_mcspi_platform_config {
	unsigned int regs_offset;
};

/* per-register bitmasks */
#define OMAP3_MCSPI_SYSCONFIG_SMARTIDLE (2 << 3)
#define OMAP3_MCSPI_SYSCONFIG_ENAWAKEUP BIT(2)
#define OMAP3_MCSPI_SYSCONFIG_AUTOIDLE	BIT(0)
#define OMAP3_MCSPI_SYSCONFIG_SOFTRESET BIT(1)

#define OMAP3_MCSPI_SYSSTATUS_RESETDONE BIT(0)

#define OMAP3_MCSPI_MODULCTRL_SINGLE	BIT(0)
#define OMAP3_MCSPI_MODULCTRL_MS	BIT(2)
#define OMAP3_MCSPI_MODULCTRL_STEST	BIT(3)

#define OMAP3_MCSPI_CHCONF_PHA		BIT(0)
#define OMAP3_MCSPI_CHCONF_POL		BIT(1)
#define OMAP3_MCSPI_CHCONF_CLKD_MASK	GENMASK(5, 2)
#define OMAP3_MCSPI_CHCONF_EPOL		BIT(6)
#define OMAP3_MCSPI_CHCONF_WL_MASK	GENMASK(11, 7)
#define OMAP3_MCSPI_CHCONF_TRM_RX_ONLY	BIT(12)
#define OMAP3_MCSPI_CHCONF_TRM_TX_ONLY	BIT(13)
#define OMAP3_MCSPI_CHCONF_TRM_MASK	GENMASK(13, 12)
#define OMAP3_MCSPI_CHCONF_DMAW		BIT(14)
#define OMAP3_MCSPI_CHCONF_DMAR		BIT(15)
#define OMAP3_MCSPI_CHCONF_DPE0		BIT(16)
#define OMAP3_MCSPI_CHCONF_DPE1		BIT(17)
#define OMAP3_MCSPI_CHCONF_IS		BIT(18)
#define OMAP3_MCSPI_CHCONF_TURBO	BIT(19)
#define OMAP3_MCSPI_CHCONF_FORCE	BIT(20)

#define OMAP3_MCSPI_CHSTAT_RXS		BIT(0)
#define OMAP3_MCSPI_CHSTAT_TXS		BIT(1)
#define OMAP3_MCSPI_CHSTAT_EOT		BIT(2)

#define OMAP3_MCSPI_CHCTRL_EN		BIT(0)
#define OMAP3_MCSPI_CHCTRL_DIS		(0 << 0)

#define OMAP3_MCSPI_WAKEUPENABLE_WKEN	BIT(0)
#define MCSPI_PINDIR_D0_IN_D1_OUT	0
#define MCSPI_PINDIR_D0_OUT_D1_IN	1

#define OMAP3_MCSPI_MAX_FREQ		48000000
#define SPI_WAIT_TIMEOUT		10

#define	SPI_ASSERT_CS		debug("cs+\n"); dm_gpio_set_value(priv->cs_gpio, 1);
#define	SPI_DEASSERT_CS		debug("cs-\n"); dm_gpio_set_value(priv->cs_gpio, 0);
#define	SPI_ASSERT_CS0		debug("cs0+\n"); dm_gpio_set_value(&priv->cs_gpios[0], 1);
#define	SPI_DEASSERT_CS0	debug("cs0-\n"); dm_gpio_set_value(&priv->cs_gpios[0], 0);

#ifdef	CONFIG_OMAP3_SPI_TRACE
#define	SPI_ACTIVATE		\
	SPI_DEASSERT_CS;	\
	SPI_DEASSERT_CS0;	\
	SPI_ASSERT_CS0;		\
	SPI_ASSERT_CS

#define	SPI_DEACTIVATE		SPI_DEASSERT_CS0; \
				SPI_DEASSERT_CS
#else
#define	SPI_ACTIVATE		\
	SPI_DEASSERT_CS;	\
	SPI_ASSERT_CS

#define	SPI_DEACTIVATE		SPI_DEASSERT_CS
#endif


/* OMAP3 McSPI registers */
struct mcspi_channel {
	unsigned int chconf;		/* 0x2C, 0x40, 0x54, 0x68 */
	unsigned int chstat;		/* 0x30, 0x44, 0x58, 0x6C */
	unsigned int chctrl;		/* 0x34, 0x48, 0x5C, 0x70 */
	unsigned int tx;		/* 0x38, 0x4C, 0x60, 0x74 */
	unsigned int rx;		/* 0x3C, 0x50, 0x64, 0x78 */
};

struct mcspi {
	unsigned char res1[0x10];
	unsigned int sysconfig;		/* 0x10 */
	unsigned int sysstatus;		/* 0x14 */
	unsigned int irqstatus;		/* 0x18 */
	unsigned int irqenable;		/* 0x1C */
	unsigned int wakeupenable;	/* 0x20 */
	unsigned int syst;		/* 0x24 */
	unsigned int modulctrl;		/* 0x28 */
	struct mcspi_channel channel[4];
	/* channel0: 0x2C - 0x3C, bus 0 & 1 & 2 & 3 */
	/* channel1: 0x40 - 0x50, bus 0 & 1 */
	/* channel2: 0x54 - 0x64, bus 0 & 1 */
	/* channel3: 0x68 - 0x78, bus 0 */
};

struct omap3_spi_priv {
	struct udevice *dev;
	struct mcspi *regs;
	struct gpio_desc *cs_gpios;
	struct gpio_desc *cs_gpio;
	unsigned int num_cs;
	unsigned int cs;
	unsigned int freq;
	unsigned int mode;
	unsigned int wordlen;
	unsigned int pin_dir:1;
};

#ifdef	DEBUG
typedef struct	{
	unsigned int	mode:2;
	unsigned int	clkd:4;
	unsigned int	epol:1;
	unsigned int	wl:5;
	unsigned int	trm:2;
	unsigned int	dmaw:1;
	unsigned int	dmar:1;
	unsigned int	dpe0:1;
	unsigned int	dpe1:1;
	unsigned int	is:1;
	unsigned int	turbo:1;
	unsigned int	force:1;
	unsigned int	spienslv:1;
	unsigned int	sbe:1;
	unsigned int	sbpol:1;
	unsigned int	tcs:2;
	unsigned int	ffew:1;
	unsigned int	ffer:1;
	unsigned int	clkg:1;
	unsigned int	reserved:2;
} chconf_t;
#endif

static int omap3_spi_set_speed(struct udevice *dev, unsigned int speed);
static int omap3_spi_set_mode(struct udevice *dev, uint mode);

void bit_reverse(u8 *buffer, const u8 *txp, u32 len)
{
	u8	byte;
	for (u32 ix = 0; ix < len ; ix++) {
		byte = *txp++;
		*buffer++ = __bitrev8(byte);
	}
}

static	inline void red(const char *s)
{
	debug(ANSI_COLOR_RED);
	debug("%s: ",s);
	debug(ANSI_COLOR_WHITE);
}

static	inline void redlf(const char *s)
{
	red(s);
	debug("\n");
}

#ifndef	DEBUG
#define	omap_decode_chconf(priv, cs)
#else
static void omap_decode_chconf(struct omap3_spi_priv *priv, unsigned int cs)
{
	union {
		unsigned int 	chconf_val;
		chconf_t	chconf;
	} spi;

	spi.chconf_val = readl(&priv->regs->channel[cs].chconf);
	debug("cs%d: 0x%08x: ", cs, spi.chconf_val);
	debug("mode%d:", spi.chconf.mode);
	debug("divide by %d, ", (1 << spi.chconf.clkd));
	debug("cs active ");
	if (spi.chconf.epol) {
		debug("low, ");
	} else {
		debug("high, ");
	}
	if (spi.chconf.force == 0) {
		debug("force=0: ");
		if (spi.chconf.epol == 0) {
			debug("spien=0, ");
		} else {	/* epol == 0 */
			debug("spien=1, ");
		}
	} else {	/* force == 0 */
		debug("force=1: ");
		if (spi.chconf.epol == 0) {
			debug("spien=1, ");
		} else {	/* epol == 0 */
			debug("spien=0, ");
		}
	}
	debug("%d-bit, ", spi.chconf.wl+1);
	switch(spi.chconf.trm) {
	case 0:
		debug("txrx, ");
		break;
	case 1:
		debug("rx, ");
		break;
	case 2:
		debug("tx, ");
		break;
	}
	if (spi.chconf.dmaw)
		debug("txdma, ");
	if (spi.chconf.dmar)
		debug("rxdma, ");
	if (spi.chconf.dpe0 == 0) {
		debug("miso = tx!!!, ");
	}
	if (spi.chconf.dpe1 == 0) {
		debug("mosi = tx, ");
	}
	if (spi.chconf.is == 0) {
		debug("miso = rx, ");
	} else {
		debug("mosi = rx!!!, ");
	}
	if (spi.chconf.turbo) {
		debug("turbo, ");
	}
	debug("slcs=%d, ", spi.chconf.spienslv);
	if (spi.chconf.sbe) {
		debug("start bit = %d, ", spi.chconf.sbpol);
	}
	debug("delay=%d.5C, ", spi.chconf.tcs);
	if (spi.chconf.ffew) {
		debug("tx fifo, ");
	}
	if (spi.chconf.ffer) {
		debug("rx fifo, ");
	}
	if (spi.chconf.clkg) {
		debug("clk = x/n");
	} else {
		debug("clk = x/2^n");
	}
	debug("\n");
}
#endif

static void omap3_spi_write_chconf(struct omap3_spi_priv *priv, int val)
{
	writel(val, &priv->regs->channel[priv->cs].chconf);
	/* Flash post writes to make immediate effect */
	readl(&priv->regs->channel[priv->cs].chconf);
	omap_decode_chconf(priv, priv->cs);
}

static void omap3_spi_set_enable(struct omap3_spi_priv *priv, int enable)
{
	writel(enable, &priv->regs->channel[priv->cs].chctrl);
	/* Flash post writes to make immediate effect */
	readl(&priv->regs->channel[priv->cs].chctrl);
}

static int omap3_spi_write(struct omap3_spi_priv *priv, unsigned int len,
			   const void *txp, unsigned long flags)
{
	ulong start;
	int i, chconf;
	redlf(__func__);;
	u8 *txp_rev = NULL;		/* txp_rev cannot be written */
	if (flags & SPI_LSB_FIRST) {	/* Only support with 8 bit wordlen */
		if (priv->wordlen != 8) {
			return -EINVAL;
		} else {
			txp_rev = malloc(len);
			if (txp_rev == NULL) {
				debug("%s: could not allocate %d bytes\n", __func__, len);
				return -ENOMEM;
			}
			bit_reverse(txp_rev,txp, len);
		}
	}
	chconf = readl(&priv->regs->channel[priv->cs].chconf);

	/* Enable the channel */
	omap3_spi_set_enable(priv, OMAP3_MCSPI_CHCTRL_EN);

	chconf &= ~(OMAP3_MCSPI_CHCONF_TRM_MASK | OMAP3_MCSPI_CHCONF_WL_MASK);
	chconf |= (priv->wordlen - 1) << 7;
	chconf |= OMAP3_MCSPI_CHCONF_TRM_TX_ONLY;
	chconf |= OMAP3_MCSPI_CHCONF_FORCE;
	omap3_spi_write_chconf(priv, chconf);
	if (flags & SPI_XFER_BEGIN) {
		SPI_ACTIVATE;
	}
	for (i = 0; i < len; i++) {
		/* wait till TX register is empty (TXS == 1) */
		start = get_timer(0);
		while (!(readl(&priv->regs->channel[priv->cs].chstat) &
			 OMAP3_MCSPI_CHSTAT_TXS)) {
			if (get_timer(start) > SPI_WAIT_TIMEOUT) {
				printf("SPI TXS timed out, status=0x%08x\n",
					readl(&priv->regs->channel[priv->cs].chstat));
				return -1;
			}
		}
		/* Write the data */
		unsigned int *tx = &priv->regs->channel[priv->cs].tx;
		if (priv->wordlen > 16)
			writel(((u32 *)txp)[i], tx);
		else if (priv->wordlen > 8)
			writel(((u16 *)txp)[i], tx);
		else if (flags & SPI_LSB_FIRST)
			writel(((u8 *)txp_rev)[i], tx);
		else
			writel(((u8 *)txp)[i], tx);
	}

	/* wait to finish of transfer */
	while ((readl(&priv->regs->channel[priv->cs].chstat) &
			(OMAP3_MCSPI_CHSTAT_EOT | OMAP3_MCSPI_CHSTAT_TXS)) !=
			(OMAP3_MCSPI_CHSTAT_EOT | OMAP3_MCSPI_CHSTAT_TXS))
		;

	/* Disable the channel otherwise the next immediate RX will get affected */
	omap3_spi_set_enable(priv, OMAP3_MCSPI_CHCTRL_DIS);

	if (flags & SPI_XFER_END) {

		chconf &= ~OMAP3_MCSPI_CHCONF_FORCE;
		omap3_spi_write_chconf(priv, chconf);
		SPI_DEACTIVATE;
	}
	return 0;
}

static int omap3_spi_read(struct omap3_spi_priv *priv, unsigned int len,
			  void *rxp, unsigned long flags)
{
	int i, chconf;
	ulong start;
	redlf(__func__);;
	if (flags & SPI_LSB_FIRST) {	/* Only support LSB in write */
		return -EINVAL;
	}
	chconf = readl(&priv->regs->channel[priv->cs].chconf);

	/* Enable the channel */
	omap3_spi_set_enable(priv, OMAP3_MCSPI_CHCTRL_EN);

	chconf &= ~(OMAP3_MCSPI_CHCONF_TRM_MASK | OMAP3_MCSPI_CHCONF_WL_MASK);
	chconf |= (priv->wordlen - 1) << 7;
	chconf |= OMAP3_MCSPI_CHCONF_TRM_RX_ONLY;
	chconf |= OMAP3_MCSPI_CHCONF_FORCE;
	omap3_spi_write_chconf(priv, chconf);

	writel(0, &priv->regs->channel[priv->cs].tx);
	if (flags & SPI_XFER_BEGIN) {
		SPI_ACTIVATE;
	}
	for (i = 0; i < len; i++) {
		start = get_timer(0);
		/* Wait till RX register contains data (RXS == 1) */
		while (!(readl(&priv->regs->channel[priv->cs].chstat) &
			 OMAP3_MCSPI_CHSTAT_RXS)) {
			if (get_timer(start) > SPI_WAIT_TIMEOUT) {
				printf("SPI RXS timed out, status=0x%08x\n",
					readl(&priv->regs->channel[priv->cs].chstat));
				return -1;
			}
		}

		/* Disable the channel to prevent furher receiving */
		if (i == (len - 1))
			omap3_spi_set_enable(priv, OMAP3_MCSPI_CHCTRL_DIS);

		/* Read the data */
		unsigned int *rx = &priv->regs->channel[priv->cs].rx;
		if (priv->wordlen > 16)
			((u32 *)rxp)[i] = readl(rx);
		else if (priv->wordlen > 8)
			((u16 *)rxp)[i] = (u16)readl(rx);
		else
			((u8 *)rxp)[i] = (u8)readl(rx);
	}

	if (flags & SPI_XFER_END) {
		chconf &= ~OMAP3_MCSPI_CHCONF_FORCE;
		omap3_spi_write_chconf(priv, chconf);
		SPI_DEACTIVATE;
	}

	return 0;
}

/*McSPI Transmit Receive Mode*/
static int omap3_spi_txrx(struct omap3_spi_priv *priv, unsigned int len,
			  const void *txp, void *rxp, unsigned long flags)
{
	ulong start;
	int chconf, i = 0;
	redlf(__func__);;
	if (flags & SPI_LSB_FIRST) {	/* Only support LSB in write */
		return -EINVAL;
	}
	chconf = readl(&priv->regs->channel[priv->cs].chconf);

	/*Enable SPI channel*/
	omap3_spi_set_enable(priv, OMAP3_MCSPI_CHCTRL_EN);

	/*set TRANSMIT-RECEIVE Mode*/
	chconf &= ~(OMAP3_MCSPI_CHCONF_TRM_MASK | OMAP3_MCSPI_CHCONF_WL_MASK);
	chconf |= (priv->wordlen - 1) << 7;
	chconf |= OMAP3_MCSPI_CHCONF_FORCE;
	omap3_spi_write_chconf(priv, chconf);
	if (flags & SPI_XFER_BEGIN) {
		SPI_ACTIVATE;
	}
	/*Shift in and out 1 byte at time*/
	for (i=0; i < len; i++){
		/* Write: wait for TX empty (TXS == 1)*/
		start = get_timer(0);
		while (!(readl(&priv->regs->channel[priv->cs].chstat) &
			 OMAP3_MCSPI_CHSTAT_TXS)) {
			if (get_timer(start) > SPI_WAIT_TIMEOUT) {
				printf("SPI TXS timed out, status=0x%08x\n",
					readl(&priv->regs->channel[priv->cs].chstat));
				return -1;
			}
		}
		/* Write the data */
		unsigned int *tx = &priv->regs->channel[priv->cs].tx;
		if (priv->wordlen > 16)
			writel(((u32 *)txp)[i], tx);
		else if (priv->wordlen > 8)
			writel(((u16 *)txp)[i], tx);
		else
			writel(((u8 *)txp)[i], tx);

		/*Read: wait for RX containing data (RXS == 1)*/
		start = get_timer(0);
		while (!(readl(&priv->regs->channel[priv->cs].chstat) &
			 OMAP3_MCSPI_CHSTAT_RXS)) {
			if (get_timer(start) > SPI_WAIT_TIMEOUT) {
				printf("SPI RXS timed out, status=0x%08x\n",
					readl(&priv->regs->channel[priv->cs].chstat));
				return -1;
			}
		}
		/* Read the data */
		unsigned int *rx = &priv->regs->channel[priv->cs].rx;
		if (priv->wordlen > 16)
			((u32 *)rxp)[i] = readl(rx);
		else if (priv->wordlen > 8)
			((u16 *)rxp)[i] = (u16)readl(rx);
		else
			((u8 *)rxp)[i] = (u8)readl(rx);
	}
	/* Disable the channel */
	omap3_spi_set_enable(priv, OMAP3_MCSPI_CHCTRL_DIS);

	/*if transfer must be terminated disable the channel*/
	if (flags & SPI_XFER_END) {
		chconf &= ~OMAP3_MCSPI_CHCONF_FORCE;
		omap3_spi_write_chconf(priv, chconf);
		SPI_DEACTIVATE;
	}

	return 0;
}

static int _spi_xfer(struct omap3_spi_priv *priv, unsigned int bitlen,
		     const void *dout, void *din, unsigned long flags)
{
	unsigned int	len;
	int ret = -1;
	redlf(__func__);;
	if (priv->wordlen < 4 || priv->wordlen > 32) {
		printf("omap3_spi: invalid wordlen %d\n", priv->wordlen);
		return -1;
	}

	if (bitlen % priv->wordlen)
		return -1;

	len = bitlen / priv->wordlen;

	if (bitlen == 0) {	 /* only change CS */
		int chconf = readl(&priv->regs->channel[priv->cs].chconf);

		if (flags & SPI_XFER_BEGIN) {
			omap3_spi_set_enable(priv, OMAP3_MCSPI_CHCTRL_EN);
			chconf |= OMAP3_MCSPI_CHCONF_FORCE;
			omap3_spi_write_chconf(priv, chconf);
			SPI_ACTIVATE;
		}
		if (flags & SPI_XFER_END) {
			chconf &= ~OMAP3_MCSPI_CHCONF_FORCE;
			omap3_spi_write_chconf(priv, chconf);
			omap3_spi_set_enable(priv, OMAP3_MCSPI_CHCTRL_DIS);
			SPI_DEACTIVATE;
		}
		ret = 0;
	} else {
		if (dout != NULL && din != NULL)
			ret = omap3_spi_txrx(priv, len, dout, din, flags);
		else if (dout != NULL)
			ret = omap3_spi_write(priv, len, dout, flags);
		else if (din != NULL)
			ret = omap3_spi_read(priv, len, din, flags);
	}
	return ret;
}

static void _omap3_spi_set_speed(struct omap3_spi_priv *priv)
{
	uint32_t confr, div = 0;

	confr = readl(&priv->regs->channel[priv->cs].chconf);

	/* Calculate clock divisor. Valid range: 0x0 - 0xC ( /1 - /4096 ) */
	if (priv->freq) {
		while (div <= 0xC && (OMAP3_MCSPI_MAX_FREQ / (1 << div))
					> priv->freq)
			div++;
	} else {
		 div = 0xC;
	}

	/* set clock divisor */
	confr &= ~OMAP3_MCSPI_CHCONF_CLKD_MASK;
	confr |= div << 2;

	omap3_spi_write_chconf(priv, confr);
}

static void _omap3_spi_set_mode(struct omap3_spi_priv *priv)
{
	uint32_t confr;

	confr = readl(&priv->regs->channel[priv->cs].chconf);

	/* standard 4-wire master mode:  SCK, MOSI/out, MISO/in, nCS
	 * REVISIT: this controller could support SPI_3WIRE mode.
	 */
	if (priv->pin_dir == MCSPI_PINDIR_D0_IN_D1_OUT) {
		confr &= ~(OMAP3_MCSPI_CHCONF_IS|OMAP3_MCSPI_CHCONF_DPE1);
		confr |= OMAP3_MCSPI_CHCONF_DPE0;
	} else {
		confr &= ~OMAP3_MCSPI_CHCONF_DPE0;
		confr |= OMAP3_MCSPI_CHCONF_IS|OMAP3_MCSPI_CHCONF_DPE1;
	}

	/* set SPI mode 0..3 */
	confr &= ~(OMAP3_MCSPI_CHCONF_POL | OMAP3_MCSPI_CHCONF_PHA);
	if (priv->mode & SPI_CPHA)
		confr |= OMAP3_MCSPI_CHCONF_PHA;
	if (priv->mode & SPI_CPOL)
		confr |= OMAP3_MCSPI_CHCONF_POL;

	/* set chipselect polarity; manage with FORCE */
	if (!(priv->mode & SPI_CS_HIGH))
		confr |= OMAP3_MCSPI_CHCONF_EPOL; /* active-low; normal */
	else
		confr &= ~OMAP3_MCSPI_CHCONF_EPOL;

	/* Transmit & receive mode */
	confr &= ~OMAP3_MCSPI_CHCONF_TRM_MASK;

	omap3_spi_write_chconf(priv, confr);
}

static void _omap3_spi_set_wordlen(struct omap3_spi_priv *priv)
{
	unsigned int confr;

	/* McSPI individual channel configuration */
	confr = readl(&priv->regs->channel[priv->cs].chconf);

	/* wordlength */
	confr &= ~OMAP3_MCSPI_CHCONF_WL_MASK;
	confr |= (priv->wordlen - 1) << 7;

	omap3_spi_write_chconf(priv, confr);
}

static void spi_reset(struct mcspi *regs)
{
	unsigned int tmp;
	redlf(__func__);;
	writel(OMAP3_MCSPI_SYSCONFIG_SOFTRESET, &regs->sysconfig);
	do {
		tmp = readl(&regs->sysstatus);
	} while (!(tmp & OMAP3_MCSPI_SYSSTATUS_RESETDONE));

	writel(OMAP3_MCSPI_SYSCONFIG_AUTOIDLE |
	       OMAP3_MCSPI_SYSCONFIG_ENAWAKEUP |
	       OMAP3_MCSPI_SYSCONFIG_SMARTIDLE, &regs->sysconfig);

	writel(OMAP3_MCSPI_WAKEUPENABLE_WKEN, &regs->wakeupenable);
}

static void _omap3_spi_claim_bus(struct omap3_spi_priv *priv)
{
	unsigned int conf;
	/*
	 * setup when switching from (reset default) slave mode
	 * to single-channel master mode
	 */
	conf = readl(&priv->regs->modulctrl);
	conf &= ~(OMAP3_MCSPI_MODULCTRL_STEST | OMAP3_MCSPI_MODULCTRL_MS);
	conf |= OMAP3_MCSPI_MODULCTRL_SINGLE;

	writel(conf, &priv->regs->modulctrl);
	omap3_spi_set_speed(priv->dev, priv->freq);
	omap3_spi_set_mode(priv->dev, priv->mode);
}

static int omap3_spi_claim_bus(struct udevice *dev)
{
	struct udevice *bus = dev->parent;
	struct omap3_spi_priv *priv = dev_get_priv(bus);
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);
	int	cs = slave_plat->cs;
	redlf(__func__);;
	if (cs < priv->num_cs) {
		priv->cs = cs;
		priv->cs_gpio = &priv->cs_gpios[cs];
	} else {
		debug("%s: failed to select cs %d\n", __func__, cs);
		return -ENODEV;
	}
	priv->freq = slave_plat->max_hz;

	_omap3_spi_claim_bus(priv);

	return 0;
}

static int omap3_spi_release_bus(struct udevice *dev)
{
	struct udevice *bus = dev_get_parent(dev);
	struct omap3_spi_priv *priv = dev_get_priv(bus);
	redlf(__func__);;
	priv->cs_gpio = NULL;
	writel(OMAP3_MCSPI_MODULCTRL_SINGLE, &priv->regs->modulctrl);

	return 0;
}

static int omap3_spi_set_wordlen(struct udevice *dev, unsigned int wordlen)
{
	struct udevice *bus = dev->parent;
	struct omap3_spi_priv *priv = dev_get_priv(bus);
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);

	priv->cs = slave_plat->cs;
	priv->wordlen = wordlen;
	_omap3_spi_set_wordlen(priv);

	return 0;
}

static int omap3_spi_probe(struct udevice *dev)
{
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);
	struct omap3_spi_priv *priv = dev_get_priv(dev);
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(dev);
	int num_gpios;
	int cs_flags;
	int ret;
	redlf(__func__);;
	if (slave_plat) {
		priv->mode = slave_plat->mode;
	} else {
		priv->mode = 0;
	}

	struct omap2_mcspi_platform_config* data =
		(struct omap2_mcspi_platform_config*)dev_get_driver_data(dev);

	priv->dev = dev;
	priv->regs = (struct mcspi *)(dev_read_addr(dev) + data->regs_offset);
	if (fdtdec_get_bool(blob, node, "ti,pindir-d0-out-d1-in"))
		priv->pin_dir = MCSPI_PINDIR_D0_OUT_D1_IN;
	else
		priv->pin_dir = MCSPI_PINDIR_D0_IN_D1_OUT;
	priv->wordlen = SPI_DEFAULT_WORDLEN;

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
	for (unsigned int cs = 0 ; cs < num_gpios ; cs++) {
		debug("cs%d: %s_%d\n", cs, priv->cs_gpios[cs].dev->name,priv->cs_gpios[cs].offset);
		dm_gpio_set_value(&priv->cs_gpios[cs], 0);
	}

	spi_reset(priv->regs);

	return 0;
}

static int omap3_spi_xfer(struct udevice *dev, unsigned int bitlen,
			    const void *dout, void *din, unsigned long flags)
{
	struct udevice *bus = dev->parent;
	struct omap3_spi_priv *priv = dev_get_priv(bus);

	return _spi_xfer(priv, bitlen, dout, din, flags);
}

static int omap3_spi_set_speed(struct udevice *dev, unsigned int speed)
{

	struct omap3_spi_priv *priv = dev_get_priv(dev);

	priv->freq = speed;
	_omap3_spi_set_speed(priv);

	return 0;
}

static int omap3_spi_set_mode(struct udevice *dev, uint mode)
{
	struct omap3_spi_priv *priv = dev_get_priv(dev);

	priv->mode = mode;

	_omap3_spi_set_mode(priv);

	return 0;
}

static const struct dm_spi_ops omap3_spi_ops = {
	.claim_bus      = omap3_spi_claim_bus,
	.release_bus    = omap3_spi_release_bus,
	.set_wordlen    = omap3_spi_set_wordlen,
	.xfer	    = omap3_spi_xfer,
	.set_speed      = omap3_spi_set_speed,
	.set_mode	= omap3_spi_set_mode,
	/*
	 * cs_info is not needed, since we require all chip selects to be
	 * in the device tree explicitly
	 */
};

static struct omap2_mcspi_platform_config omap2_pdata = {
	.regs_offset = 0,
};

static struct omap2_mcspi_platform_config omap4_pdata = {
	.regs_offset = OMAP4_MCSPI_REG_OFFSET,
};

static const struct udevice_id omap3_spi_ids[] = {
	{ .compatible = "ti,omap2-mcspi", .data = (ulong)&omap2_pdata },
	{ .compatible = "ti,omap4-mcspi", .data = (ulong)&omap4_pdata },
	{ }
};

U_BOOT_DRIVER(omap3_spi) = {
	.name   = "omap3_spi",
	.id     = UCLASS_SPI,
	.of_match = omap3_spi_ids,
	.probe = omap3_spi_probe,
	.ops    = &omap3_spi_ops,
	.priv_auto = sizeof(struct omap3_spi_priv),
};
