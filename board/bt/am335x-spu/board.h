/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * board.h
 *
 * Board header for BT AM335X based boards
 *
 * Copyright (C) 2020 Bombardier Transportation
 * Ulf Samuelsson <ulf@emagii.com>
 *
 * TI AM335x boards information header
 *
 * Copyright (C) 2011, Texas Instruments, Incorporated - http://www.ti.com/
 */

#ifndef _BOARD_H_
#define _BOARD_H_

/**
 * AM335X (EMIF_4D) EMIF REG_COS_COUNT_1, REG_COS_COUNT_2, and
 * REG_PR_OLD_COUNT values to avoid LCDC DMA FIFO underflows and Frame
 * Synchronization Lost errors. The values are the biggest that work
 * reliably with offered video modes and the memory subsystem on the
 * boards. These register have are briefly documented in "7.3.3.5.2
 * Command Starvation" section of AM335x TRM. The REG_COS_COUNT_1 and
 * REG_COS_COUNT_2 do not have any effect on current versions of
 * AM335x.
 */
#define EMIF_OCP_CONFIG_BEAGLEBONE_BLACK       0x00141414
#define EMIF_OCP_CONFIG_AM335X_EVM             0x003d3d3d

static inline int board_is_spu(void)
{
	return 1;
}

static inline int board_is_bone_lt(void)
{
	return 0;
}

static inline int board_is_pb(void)
{
	return 0;
}

static inline int board_is_bbg1(void)
{
	return 0;
}

static inline int board_is_bben(void)
{
	return 0;
}

static inline int board_is_beaglebonex(void)
{
	return 0;
}

static inline int board_is_evm_sk(void)
{
	return 0;
}

static inline int board_is_idk(void)
{
	return 0;
}

static inline int board_is_gp_evm(void)
{
	return 0;
}

static inline int board_is_evm_15_or_later(void)
{
	return 0;
}

static inline int board_is_icev2(void)
{
	return 0;
}

/*
 * We have three pin mux functions that must exist.  We must be able to enable
 * uart0, for initial output and i2c0 to read the main EEPROM.  We then have a
 * main pinmux function that can be overridden to enable all other pinmux that
 * is required on the board.
 */
void enable_uart0_pin_mux(void);
void enable_uart1_pin_mux(void);
void enable_uart2_pin_mux(void);
void enable_uart3_pin_mux(void);
void enable_uart4_pin_mux(void);
void enable_uart5_pin_mux(void);
void enable_i2c0_pin_mux(void);
void enable_board_pin_mux(void);
void am335x_spu_gpmc_init(void);
#endif
