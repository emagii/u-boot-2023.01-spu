/*
 * am335x_evm.h
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __CONFIG_AM335X_EVM_H
#define __CONFIG_AM335X_EVM_H

#include <configs/ti_am335x_common.h>
#include <linux/sizes.h>

#define	CONFIG_BOARD_NAME	"am335x-spu"
#define	CONFIG_BOARD_REV	"0.1"
#define	CONFIG_BOARD_SERIAL	"007"

#ifndef CONFIG_SPL_BUILD
//# define CONFIG_TIMESTAMP
#endif

//#define CFG_SYS_BOOTM_LEN		SZ_16M

#define CONFIG_MACH_TYPE		MACH_TYPE_AM335XEVM

/* Clock Defines */
#define V_OSCK				24000000  /* Clock output from T2 */
#define V_SCLK				(V_OSCK)

#ifdef CONFIG_MTD_RAW_NAND
#define NANDARGS \
	"mtdids=" CONFIG_MTDIDS_DEFAULT "\0" \
	"mtdparts=" CONFIG_MTDPARTS_DEFAULT "\0" \
	"nandargs=setenv bootargs console=${console} " \
		"${optargs} " \
		"root=${nandroot} " \
		"rootfstype=${nandrootfstype}\0" \
	"nandroot=ubi0:rootfs rw ubi.mtd=NAND.file-system,2048\0" \
	"nandrootfstype=ubifs rootwait=1\0" \
	"nandboot=echo Booting from nand ...; " \
		"run nandargs; " \
		"nand read ${fdtaddr} NAND.u-boot-spl-os; " \
		"nand read ${loadaddr} NAND.kernel; " \
		"bootz ${loadaddr} - ${fdtaddr}\0"
#else
#define NANDARGS ""
#endif

#define BOOTENV_DEV_LEGACY_MMC(devtypeu, devtypel, instance) \
	"bootcmd_" #devtypel #instance "=" \
	"setenv mmcdev " #instance"; "\
	"setenv bootpart " #instance":2 ; "\
	"run mmcboot\0"

#define BOOTENV_DEV_NAME_LEGACY_MMC(devtypeu, devtypel, instance) \
	#devtypel #instance " "

#define BOOTENV_DEV_NAND(devtypeu, devtypel, instance) \
	"bootcmd_" #devtypel "=" \
	"run nandboot\0"

#define BOOTENV_DEV_NAME_NAND(devtypeu, devtypel, instance) \
	#devtypel #instance " "

#if CONFIG_IS_ENABLED(CMD_PXE)
# define BOOT_TARGET_PXE(func) func(PXE, pxe, na)
#else
# define BOOT_TARGET_PXE(func)
#endif

#if CONFIG_IS_ENABLED(CMD_DHCP)
# define BOOT_TARGET_DHCP(func) func(DHCP, dhcp, na)
#else
# define BOOT_TARGET_DHCP(func)
#endif

#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(LEGACY_MMC, legacy_mmc, 0) \
	func(NAND, nand, 0) \
	BOOT_TARGET_PXE(func) \
	BOOT_TARGET_DHCP(func)

#include <config_distro_bootcmd.h>

#ifndef CONFIG_SPL_BUILD
#include <environment/ti/dfu.h>
#include <environment/ti/mmc.h>

#define CONFIG_EXTRA_ENV_SETTINGS \
	DEFAULT_LINUX_BOOT_ENV \
	DEFAULT_MMC_TI_ARGS \
	DEFAULT_FIT_TI_ARGS \
	"bootpart=0:2\0" \
	"bootdir=/boot\0" \
	"bootfile=zImage\0" \
	"fdtfile=undefined\0" \
	"console=ttyO0,115200n8\0" \
	"partitions=" \
		"uuid_disk=${uuid_gpt_disk};" \
		"name=bootloader,start=384K,size=1792K," \
			"uuid=${uuid_gpt_bootloader};" \
		"name=rootfs,start=2688K,size=-,uuid=${uuid_gpt_rootfs}\0" \
	"optargs=\0" \
	"ramroot=/dev/ram0 rw\0" \
	"ramrootfstype=ext2\0" \
	"spiroot=/dev/mtdblock4 rw\0" \
	"spirootfstype=jffs2\0" \
	"spisrcaddr=0xe0000\0" \
	"spiimgsize=0x362000\0" \
	"spibusno=0\0" \
	"spiargs=setenv bootargs console=${console} " \
		"${optargs} " \
		"root=${spiroot} " \
		"rootfstype=${spirootfstype}\0" \
	"ramargs=setenv bootargs console=${console} " \
		"${optargs} " \
		"root=${ramroot} " \
		"rootfstype=${ramrootfstype}\0" \
	"loadramdisk=load mmc ${mmcdev} ${rdaddr} ramdisk.gz\0" \
	"spiboot=echo Booting from spi ...; " \
		"run spiargs; " \
		"sf probe ${spibusno}:0; " \
		"sf read ${loadaddr} ${spisrcaddr} ${spiimgsize}; " \
		"bootz ${loadaddr}\0" \
	"ramboot=echo Booting from ramdisk ...; " \
		"run ramargs; " \
		"bootz ${loadaddr} ${rdaddr} ${fdtaddr}\0" \
	"findfdt="\
		"if test $board_name = A335SPU; then " \
			"setenv fdtfile am335x-spu.dtb; fi; " \
		"if test $fdtfile = undefined; then " \
			"echo WARNING: Could not determine device tree to use; fi; \0" \
	"init_console=setenv console ttyO0,115200n8\0" \
	NANDARGS \
	NETARGS \
	DFUARGS \
	BOOTENV
#endif

/* NS16550 Configuration */
#define CFG_SYS_NS16550_COM1		0x44e09000	/* Base EVM has UART0 */
#define CFG_SYS_NS16550_COM2		0x48022000	/* UART1 */
#define CFG_SYS_NS16550_COM3		0x48024000	/* UART2 */
#define CFG_SYS_NS16550_COM4		0x481a6000	/* UART3 */
#define CFG_SYS_NS16550_COM5		0x481a8000	/* UART4 */
#define CFG_SYS_NS16550_COM6		0x481aa000	/* UART5 */

#define CONFIG_ENV_EEPROM_IS_ON_I2C
#define CFG_SYS_I2C_EEPROM_ADDR	0x50	/* Main EEPROM */
#define CFG_SYS_I2C_EEPROM_ADDR_LEN	2

/* PMIC support */
#define CONFIG_POWER_TPS65218

/* SPL */
#ifndef CONFIG_NOR_BOOT
/* Bootcount using the RTC block */
// #define CFG_SYS_BOOTCOUNT_BE

/* USB gadget RNDIS */
#endif

#define CFG_SYS_MAX_NAND_DEVICE	1

#ifdef CONFIG_MTD_RAW_NAND
/* NAND: device related configs */
/* #define	CFG_SYS_NAND_SELF_INIT	1 */
#define CFG_SYS_NAND_5_ADDR_CYCLE
#define CFG_SYS_NAND_PAGE_COUNT	(CFG_SYS_NAND_BLOCK_SIZE / \
					 CFG_SYS_NAND_PAGE_SIZE)
//#define CFG_SYS_NAND_PAGE_SIZE	2048
//#define CFG_SYS_NAND_OOBSIZE		64
#define CFG_SYS_NAND_BLOCK_SIZE	(128*1024)
/* NAND: driver related configs */
#define CFG_SYS_NAND_BAD_BLOCK_POS	NAND_LARGE_BADBLOCK_POS
#define CFG_SYS_NAND_ECCPOS		{ 2, 3, 4, 5, 6, 7, 8, 9, \
					 10, 11, 12, 13, 14, 15, 16, 17, \
					 18, 19, 20, 21, 22, 23, 24, 25, \
					 26, 27, 28, 29, 30, 31, 32, 33, \
					 34, 35, 36, 37, 38, 39, 40, 41, \
					 42, 43, 44, 45, 46, 47, 48, 49, \
					 50, 51, 52, 53, 54, 55, 56, 57, }

#define CFG_SYS_NAND_ECCSIZE		512
#define CFG_SYS_NAND_ECCBYTES	14
#define CFG_SYS_NAND_ONFI_DETECTION
// #define CONFIG_NAND_OMAP_ECCSCHEME	OMAP_ECC_BCH8_CODE_HW
#define CFG_SYS_NAND_U_BOOT_OFFS	0x000c0000
/* NAND: SPL related configs */
#ifdef CONFIG_SPL_OS_BOOT
#define CFG_SYS_NAND_SPL_KERNEL_OFFS	0x00200000 /* kernel offset */
#endif
#endif /* !CONFIG_MTD_RAW_NAND */

/*
 * For NOR boot, we must set this to the start of where NOR is mapped
 * in memory.
 */

/*
 * USB configuration.  We enable MUSB support, both for host and for
 * gadget.  We set USB0 as peripheral and USB1 as host, based on the
 * board schematic and physical port wired to each.  Then for host we
 * add mass storage support and for gadget we add both RNDIS ethernet
 * and DFU.
 */
#define CONFIG_AM335X_USB0
#define CONFIG_AM335X_USB0_MODE	MUSB_PERIPHERAL
#define CONFIG_AM335X_USB1
#define CONFIG_AM335X_USB1_MODE MUSB_HOST

/*
 * Disable MMC DM for SPL build and can be re-enabled after adding
 * DM support in SPL
 */
#ifdef CONFIG_SPL_BUILD
#undef CONFIG_DM_MMC
#undef CONFIG_TIMER
#endif

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_USB_ETHER)
/* Remove other SPL modes. */
/* disable host part of MUSB in SPL */
/* disable EFI partitions and partition UUID support */
#endif

/* USB Device Firmware Update support */
#ifndef CONFIG_SPL_BUILD
#define DFUARGS \
	DFU_ALT_INFO_EMMC \
	DFU_ALT_INFO_MMC \
	DFU_ALT_INFO_RAM \
	DFU_ALT_INFO_NAND
#endif

#define	OMAP3_SPI_NUM_CS	4
#define	OMAP3_SPI_HW_CS		2
#define	MCSPI1_MAX_CS		1
#define	MCSPI1_GPIO_CS		0
#define	MCSPI2_MAX_CS		4
#define	MCSPI2_GPIO_CS		0xE
#define	MCSPI3_MAX_CS		0
#define	MCSPI3_GPIO_CS		0
#define	MCSPI4_MAX_CS		0
#define	MCSPI4_GPIO_CS		0

/*
 * Default to using SPI for environment, etc.  (OBSOLETE)
 * 0x000000 - 0x020000 : SPL (128KiB)
 * 0x020000 - 0x0A0000 : U-Boot (512KiB)
 * 0x0A0000 - 0x0BFFFF : First copy of U-Boot Environment (128KiB)
 * 0x0C0000 - 0x0DFFFF : Second copy of U-Boot Environment (128KiB)
 * 0x0E0000 - 0x442000 : Linux Kernel
 * 0x442000 - 0x800000 : Userland
 */
#if defined(CONFIG_SPI_BOOT)
/* SPL related */
#elif defined(CONFIG_EMMC_BOOT)
#define CFG_SYS_MMC_ENV_DEV		0
#define CFG_SYS_MMC_ENV_PART		0
#define CFG_SYS_MMC_MAX_DEVICE	2
#elif defined(CONFIG_ENV_IS_IN_NAND)
#define CFG_SYS_ENV_SECT_SIZE	CFG_SYS_NAND_BLOCK_SIZE
#endif

/* SPI flash. */

/* Network. */
//#define CONFIG_PHY_SMSC
#define CFG_SYS_FAULT_ECHO_LINK_DOWN	1U
#define CFG_SYS_FAULT_MII_ADDR	5U
/* Enable Atheros phy driver */
//#define CONFIG_PHY_ATHEROS

/*
 * NOR Size = 16 MiB
 * Number of Sectors/Blocks = 128
 * Sector Size = 128 KiB
 * Word length = 16 bits
 * Default layout:
 * 0x000000 - 0x07FFFF : U-Boot (512 KiB)
 * 0x080000 - 0x09FFFF : First copy of U-Boot Environment (128 KiB)
 * 0x0A0000 - 0x0BFFFF : Second copy of U-Boot Environment (128 KiB)
 * 0x0C0000 - 0x4BFFFF : Linux Kernel (4 MiB)
 * 0x4C0000 - 0xFFFFFF : Userland (11 MiB + 256 KiB)
 */
#if defined(CONFIG_NOR)
#define CFG_SYS_MAX_FLASH_SECT	128
#define CFG_SYS_MAX_FLASH_BANKS	1
#define CFG_SYS_FLASH_BASE		(0x08000000)
#define CFG_SYS_FLASH_CFI_WIDTH	FLASH_CFI_16BIT
#define CFG_SYS_FLASH_SIZE		0x01000000
#define CFG_SYS_MONITOR_BASE		CFG_SYS_FLASH_BASE
#define CFG_SYS_FLASH_BANKS_LIST { CFG_SYS_FLASH_BASE }
#endif  /* NOR support */

//#ifdef CONFIG_DRIVER_TI_CPSW
//#define CONFIG_CLOCK_SYNTHESIZER
//#define CLK_SYNTHESIZER_I2C_ADDR 0x65
//#endif

#define	CFG_SYS_FPGA_BASE	0x10000000

/*
 *	31	WRAPBURST	0	No Burst Wraparound
 *	30	READMULTIPLE	1	Burst Access
 *	29	READTYPE	1	Synchronous Read
 *	28	WRITEMULTIPLE	1	Burst Access
 *	27	WRITETYPE	1	Synchronous Write
 *	26:25	CLKACTIVATIONTIME 01	First rising edge of GPMC_CLK
 *					two GPMC_FCLK cycles after
 *					start access time
 *	24:23	ATT_DEV_PAGELEN	00	4 words of 16 bits
 *	22	WAITREADMONITOR	0	Wait Pin not monitored (read)
 *	21	WAITWRMONITOR	0	Wait pin not monitored (write)
 *	20	<reserved>	0
 *	19:18	WAITMONTIME	00	Wait pin monitored with valid data
 *	17:16	WAITPINSELECT	00	Wait pin is WAIT0
 *	15:14	<reserved>	00
 *	13:12	DEVICESIZE	01	16-bit interface
 *	11:10	DEVICETYPE	00	NOR-Flash like
 *	9:8	MUXADDDATA	01	AAD device
 *	7-5	<reserved>	000
 *	4	TIMEPARAGRAN	0	x1 latency
 *	3:2	<reserved>	00
 *	1:0	GPMCFCLKDIV	00	GPMC_CLK = GPMC_FCLK
 *	  3322_2222_2222_1111_1111_1100_0000_0000
 *	  1098_7654_3210_9876_5432_1098_7654_3210
 *	0x0010_1010_0000_0000_0001_0001_0000_0000
 *	0x2A001100
 */
#define FPGA_GPMC_CONFIG1	0x7A001100
/*
 *	31:21	<reserved>	00000000000
 *	20:16	CSWROFFTIME	01100	15 clocks
 *	15:13	<reserved>	000
 *	12:8	CSRDOFFTIME	01011	15 clocks
 *	7	CSEXTRADELAY	0	CS Timing control signal is not delayed
 *	6:4	<reserved>
 *	3:0	CSONTIME	0001	CS# asserted 1 FCLK from start cycle
 *	  3322_2222_2222_1111_1111_1100_0000_0000
 *	  1098_7654_3210_9876_5432_1098_7654_3210
 *	0x0000_0000_0000_1110_0000_1110_0000_0001
 *	0x000E0E01
 */
#define FPGA_GPMC_CONFIG2	0x000E0E01
/*
 *	31				0
 *	30:28	ADVAADMUXWROFFTIME	100	4 cycles
 *	27	<reserved>		0
 *	26:24	ADVAADMUXRDOFFTIME	100	4 cycles
 *	23:21	<reserved>		000
 *	20:16	ADVWROFFTIME		01000	8 cycles
 *	15:13	<reserved>		000
 *	12:8	ADVRDOFFTIME		01000	8 cycles
 *	7	ADVEXTRADELAY		0	Timing Control not delayed
 *	6:4	ADVAADMUXONTIME		010	2
 *	3:0	ADVONTIME		0100	6 cycles
 *	  3322_2222_2222_1111_1111_1100_0000_0000
 *	  1098_7654_3210_9876_5432_1098_7654_3210
 *	0x0100_0100_0000_1000_0000_1000_0010_0110
 *	0x44080826
 */
#define FPGA_GPMC_CONFIG3	0x44080826
/*
 *	31:29	<reserved>	000
 *	28:24	WEOFFTIME	01011	11 cycles
 *	23	WEEXTRADELAY	0	WE Timing control signal not delayed
 *	22:20	<reserved>	000
 *	19:16	WEONTIME	0110	6 cycles
 *	15:13	OEAADMUXOFFTIME	100	4 cycles
 *	12:8	OEOFFTIME	01100	12 cycles
 *	7	OEEXTRADELAY	0	OE Timing control signal not delayed
 *	6:4	OEAADMUXONTIME	001	1 cycle
 *	3:0	OEONTIME	0110	6 cycles
 *	  3322_2222_2222_1111_1111_1100_0000_0000
 *	  1098_7654_3210_9876_5432_1098_7654_3210
 *	0x0000_1011_0000_0110_1000_1011_0001_0110
 *	0x0B068B16
 */
#define FPGA_GPMC_CONFIG4	0x0D098C19
/*
 *	31:28	<reserved>		0000
 *	27:24	PAGEBURSTACCESSTIME	0010	2
 *	23:21	<reserved>		000
 *	20:16	RDACCESSTIME		01100	12
 *	15:13	<reserved>		000
 *	12:8	WRCYCLETIME		01111	15
 *	7:5	<reserved>		000
 *	4:0	RDCYCLETIME		01111	15
 *	  3322_2222_2222_1111_1111_1100_0000_0000
 *	  1098_7654_3210_9876_5432_1098_7654_3210
 *	0x0000_0010_0000_1100_0000_1111_0000_1111
 *	0x020C0F0F
 */
#define FPGA_GPMC_CONFIG5	0x020C0F0F
/*
 *	31:29	<reserved>		000
 *	28:24	WRACCESSTIME		01010
 *	23:20	<reserved>		0000
 *	19:16	WRDATAONADMUXBUS	1001
 *	15:12	<reserved>		0000
 *	11:8	CYCLE2CYCLEDELAY	0011
 *	7	CYCLE2CYCLESAMECSEN	1
 *	6	CYCLE2CYCLEDIFFCSEN	1
 *	5:4	<reserved>		00
 *	3:0	BUSTURNAROUND		0100
 *	  3322_2222_2222_1111_1111_1100_0000_0000
 *	  1098_7654_3210_9876_5432_1098_7654_3210
 *	0x0000_1010_0000_0110_0000_0011_1100_0100
 *	0x0A0603C4
 */
#define FPGA_GPMC_CONFIG6	0x0C0903C4

#endif	/* ! __CONFIG_AM335X_SPU_H */
