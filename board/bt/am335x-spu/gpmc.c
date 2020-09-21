// SPDX-License-Identifier: GPL-2.0+
/*
 * gpmc.c
 *
 * Bus Interface functions for BT AM335X based boards
 *
 * Copyright (C) 2020 Bombardier Transportation
 * Ulf Samuelsson <ulf@emagii.com>
 *
 */

#include <common.h>
#include <dm.h>
#include <env.h>
#include <errno.h>
#include <init.h>
#include <malloc.h>
#include <spl.h>
#include <serial.h>
#include <asm/arch/cpu.h>
#include <asm/arch/hardware.h>
#include <asm/arch/omap.h>
#include <asm/arch/ddr_defs.h>
#include <asm/arch/clock.h>
#include <asm/arch/clk_synthesizer.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mmc_host_def.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/mem.h>
#include <asm/io.h>
#include <asm/emif.h>
#include <asm/gpio.h>
#include <asm/omap_common.h>
#include <asm/omap_sec_common.h>
#include <asm/omap_mmc.h>
#include <i2c.h>
#include <miiphy.h>
#include <cpsw.h>
#include <power/tps65217.h>
#include <power/tps65910.h>
#include <env_internal.h>
#include <watchdog.h>
#include "../common/board_detect.h"
#include "board.h"
const struct gpmc *gpmc_cfg_spu = (struct gpmc *)GPMC_BASE;

#if defined(CONFIG_MTD_RAW_NAND) || defined(CONFIG_CMD_NAND)
const u32 gpmc_regs_nand[GPMC_MAX_REG] = {
	M_NAND_GPMC_CONFIG1,
	M_NAND_GPMC_CONFIG2,
	M_NAND_GPMC_CONFIG3,
	M_NAND_GPMC_CONFIG4,
	M_NAND_GPMC_CONFIG5,
	M_NAND_GPMC_CONFIG6,
	0
};
#endif

const u32 gpmc_regs_fpga[GPMC_MAX_REG] = {
	FPGA_GPMC_CONFIG1,
	FPGA_GPMC_CONFIG2,
	FPGA_GPMC_CONFIG3,
	FPGA_GPMC_CONFIG4,
	FPGA_GPMC_CONFIG5,
	FPGA_GPMC_CONFIG6,
	0
};

/*****************************************************
 * gpmc_init(): init gpmc bus
 * Init GPMC for x16, MuxMode (SDRAM in x32).
 * This code can only be executed from SRAM or SDRAM.
 *****************************************************/
void am335x_spu_gpmc_init(void)
{
	/* global settings */
	writel(0x00000008, &gpmc_cfg_spu->sysconfig);
	writel(0x00000000, &gpmc_cfg_spu->irqstatus);
	writel(0x00000000, &gpmc_cfg_spu->irqenable);
	/* disable timeout, set a safe reset value */
	writel(0x00001ff0, &gpmc_cfg_spu->timeout_control);
	writel(0x00000012, &gpmc_cfg_spu->config);
	enable_gpmc_cs_config(gpmc_regs_fpga, &gpmc_cfg_spu->cs[0], CFG_SYS_FPGA_BASE, GPMC_SIZE_256M);
#if defined(CONFIG_MTD_RAW_NAND) || defined(CONFIG_CMD_NAND)
	enable_gpmc_cs_config(gpmc_regs_nand, &gpmc_cfg_spu->cs[3], CFG_SYS_NAND_BASE, GPMC_SIZE_16M);
#endif
}
