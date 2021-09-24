/*
 * Copyright (C) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <common/debug.h>
#include <drivers/console.h>
#include <drivers/io/io_storage.h>
#include <drivers/microchip/emmc.h>
#include <drivers/microchip/flexcom_uart.h>
#include <drivers/microchip/lan966x_clock.h>
#include <drivers/microchip/qspi.h>
#include <drivers/microchip/tz_matrix.h>
#include <drivers/microchip/vcore_gpio.h>
#include <drivers/microchip/sha.h>
#include <lib/mmio.h>
#include <plat/common/platform.h>
#include <platform_def.h>

#include <plat/common/platform.h>
#include <plat/arm/common/arm_config.h>
#include <plat/arm/common/plat_arm.h>

#include "lan966x_regs.h"
#include "lan966x_private.h"
#include "plat_otp.h"

CASSERT((BL1_RW_SIZE + BL2_SIZE) <= LAN996X_SRAM_SIZE, assert_sram_depletion);

static console_t lan966x_console;
shared_memory_desc_t shared_memory_desc;

#define FW_CONFIG_INIT_8(offset, value)		\
	.config[offset] = (uint8_t) (value)

#define FW_CONFIG_INIT_32(offset, value)				\
	.config[offset + 0] = (uint8_t) (value),			\
	.config[offset + 1] = (uint8_t) ((value) >> 8),			\
	.config[offset + 2] = (uint8_t) ((value) >> 16),		\
	.config[offset + 3] = (uint8_t) ((value) >> 24)

/* Define global fw_config, set default MMC settings */
lan966x_fw_config_t lan966x_fw_config = {
	FW_CONFIG_INIT_32(LAN966X_FW_CONF_MMC_CLK_RATE, MMC_DEFAULT_SPEED),
	FW_CONFIG_INIT_8(LAN966X_FW_CONF_MMC_BUS_WIDTH, MMC_BUS_WIDTH_1),
	FW_CONFIG_INIT_8(LAN966X_FW_CONF_QSPI_CLK, 10), /* 10Mhz */
};

#define LAN996X_MAP_QSPI0						\
	MAP_REGION_FLAT(						\
		LAN996X_QSPI0_MMAP,					\
		LAN996X_QSPI0_RANGE,					\
		MT_MEMORY | MT_RO | MT_SECURE)

#define LAN996X_MAP_AXI							\
	MAP_REGION_FLAT(						\
		LAN996X_DEV_BASE,					\
		LAN996X_DEV_SIZE,					\
		MT_DEVICE | MT_RW | MT_SECURE)

#define LAN966X_MAP_BL32					\
	MAP_REGION_FLAT(					\
		BL32_BASE,					\
		BL32_SIZE,					\
		MT_MEMORY | MT_RW | MT_SECURE)

#define LAN966X_MAP_NS_MEM					\
	MAP_REGION_FLAT(					\
		PLAT_LAN966X_NS_IMAGE_BASE,			\
		PLAT_LAN966X_NS_IMAGE_SIZE,			\
		MT_MEMORY | MT_RW | MT_NS)

#define LAN966X_MAP_USB						\
	MAP_REGION_FLAT(					\
		LAN996X_USB_BASE,				\
		LAN996X_USB_SIZE,				\
		MT_DEVICE | MT_RW | MT_SECURE)

#ifdef IMAGE_BL1
const mmap_region_t plat_arm_mmap[] = {
	LAN996X_MAP_QSPI0,
	LAN996X_MAP_AXI,
	LAN966X_MAP_USB,
	{0}
};
#endif
#ifdef IMAGE_BL2
const mmap_region_t plat_arm_mmap[] = {
	LAN996X_MAP_QSPI0,
	LAN996X_MAP_AXI,
	LAN966X_MAP_BL32,
	LAN966X_MAP_NS_MEM,
	LAN966X_MAP_USB,
	{0}
};
#endif
#ifdef IMAGE_BL32
const mmap_region_t plat_arm_mmap[] = {
	LAN996X_MAP_QSPI0,
	LAN996X_MAP_AXI,
	LAN966X_MAP_BL32,
	LAN966X_MAP_USB,
	{0}
};
#endif

enum lan966x_flexcom_id {
	FLEXCOM0 = 0,
	FLEXCOM1,
	FLEXCOM2,
	FLEXCOM3,
	FLEXCOM4,
};

static struct lan966x_flexcom_args {
	uintptr_t base;
	unsigned int clk_id;
	int rx_gpio;
	int tx_gpio;
} lan966x_flexcom_map[] = {
	[FLEXCOM0] = {
		LAN966X_FLEXCOM_0_BASE, LAN966X_CLK_ID_FLEXCOM0, 25, 26
	},
	[FLEXCOM1] = { 0 },
	[FLEXCOM2] = {
		LAN966X_FLEXCOM_2_BASE, LAN966X_CLK_ID_FLEXCOM2, 44, 45
	},
	[FLEXCOM3] = {
		LAN966X_FLEXCOM_3_BASE, LAN966X_CLK_ID_FLEXCOM3, 52, 53
	},
	[FLEXCOM4] = {
		LAN966X_FLEXCOM_4_BASE, LAN966X_CLK_ID_FLEXCOM4, 57, 58
	},
};

/*******************************************************************************
 * Returns ARM platform specific memory map regions.
 ******************************************************************************/
const mmap_region_t *plat_arm_get_mmap(void)
{
	return plat_arm_mmap;
}

static void lan966x_flexcom_init(int idx)
{
	struct lan966x_flexcom_args *fc;

	if (idx < 0)
		return;

	fc = &lan966x_flexcom_map[idx];

	if (fc->base == 0)
		return;

	/* GPIOs for RX and TX */
	vcore_gpio_set_alt(fc->rx_gpio, 1);
	vcore_gpio_set_alt(fc->tx_gpio, 1);

	/* Configure clock on UART */
	lan966x_clk_disable(fc->clk_id);
	lan966x_clk_set_rate(fc->clk_id, LAN966X_CLK_FREQ_FLEXCOM); /* 30MHz */
	lan966x_clk_enable(fc->clk_id);

	/* Initialize the console to provide early debug support */
	console_flexcom_register(&lan966x_console,
				 fc->base + FLEXCOM_UART_OFFSET,
				 FLEXCOM_DIVISOR(FACTORY_CLK, FLEXCOM_BAUDRATE));
	console_set_scope(&lan966x_console,
			  CONSOLE_FLAG_BOOT | CONSOLE_FLAG_RUNTIME);
}

void lan966x_console_init(void)
{
	vcore_gpio_init(GCB_GPIO_OUT_SET(LAN966X_GCB_BASE));

	switch (lan966x_get_strapping()) {
	case LAN966X_STRAP_BOOT_MMC_FC:
	case LAN966X_STRAP_BOOT_QSPI_FC:
	case LAN966X_STRAP_BOOT_SD_FC:
	case LAN966X_STRAP_BOOT_MMC_TFAMON_FC:
	case LAN966X_STRAP_BOOT_QSPI_TFAMON_FC:
	case LAN966X_STRAP_BOOT_SD_TFAMON_FC:
		lan966x_flexcom_init(FC_DEFAULT);
		break;
	case LAN966X_STRAP_TFAMON_FC0:
		lan966x_flexcom_init(FLEXCOM0);
		break;
	case LAN966X_STRAP_TFAMON_FC2:
		lan966x_flexcom_init(FLEXCOM2);
		break;
	case LAN966X_STRAP_TFAMON_FC3:
		lan966x_flexcom_init(FLEXCOM3);
		break;
	case LAN966X_STRAP_TFAMON_FC4:
		lan966x_flexcom_init(FLEXCOM4);
		break;
	case LAN966X_STRAP_TFAMON_USB:
#ifdef LAN966X_USE_USB
		lan966x_usb_init();
		lan966x_usb_register_console();
#endif /* LAN966X_USE_USB */
		break;
	default:
		/* No console */
		break;
	}
}

void lan966x_io_bootsource_init(void)
{
	boot_source_type boot_source = lan966x_get_boot_source();

	switch (boot_source) {
	case BOOT_SOURCE_EMMC:
	case BOOT_SOURCE_SDMMC:
		/* Setup MMC */
		lan966x_mmc_plat_config(boot_source);
		break;

	case BOOT_SOURCE_QSPI:
		/* We own SPI */
		mmio_setbits_32(CPU_GENERAL_CTRL(LAN966X_CPU_BASE),
				CPU_GENERAL_CTRL_IF_SI_OWNER_M);

		/* Enable memmap access */
		qspi_init(LAN966X_QSPI_0_BASE);

		/* Ensure we have ample reach on QSPI mmap area */
		/* 16M should be more than adequate - EVB/SVB have 2M */
		matrix_configure_srtop(MATRIX_SLAVE_QSPI0,
				       MATRIX_SRTOP(0, MATRIX_SRTOP_VALUE_16M) |
				       MATRIX_SRTOP(1, MATRIX_SRTOP_VALUE_16M));

#if defined(LAN966X_TZ)
		/* Enable QSPI0 for NS access */
		matrix_configure_slave_security(MATRIX_SLAVE_QSPI0,
						MATRIX_SRTOP(0, MATRIX_SRTOP_VALUE_16M) |
						MATRIX_SRTOP(1, MATRIX_SRTOP_VALUE_16M),
						MATRIX_SASPLIT(0, MATRIX_SRTOP_VALUE_16M),
						MATRIX_LANSECH_NS(0));
#endif
	default:
		break;
	}
}

void lan966x_timer_init(void)
{
	uintptr_t syscnt = LAN966X_CPU_SYSCNT_BASE;

	mmio_write_32(CPU_SYSCNT_CNTCVL(syscnt), 0);	/* Low */
	mmio_write_32(CPU_SYSCNT_CNTCVU(syscnt), 0);	/* High */
	mmio_write_32(CPU_SYSCNT_CNTCR(syscnt),
		      CPU_SYSCNT_CNTCR_CNTCR_EN(1));	/*Enable */
}

unsigned int plat_get_syscnt_freq2(void)
{
	return SYS_COUNTER_FREQ_IN_TICKS;
}

uint8_t lan966x_get_strapping(void)
{
	uint32_t status;
	uint8_t strapping;

	status = mmio_read_32(CPU_GENERAL_STAT(LAN966X_CPU_BASE));
	strapping = CPU_GENERAL_STAT_VCORE_CFG_X(status);

#if defined(DEBUG)
	/*
	 * NOTE: This allows overriding the strapping switches through
	 * the GPR registers.
	 *
	 * In the DEBUG build, GPR(0) can be used to override the
	 * actual strapping. If any of the non-cfg (lower 4) bits are
	 * set, the the low 4 bits will override the actual
	 * strapping.
	 *
	 * You can set the GPR0 in the DSTREAM init like
	 * this:
	 *
	 * > memory set_typed S:0xE00C0000 (unsigned int) (0x10000a)
	 *
	 * This would override the strapping with the value: 0xa
	 */
	status = mmio_read_32(CPU_GPR(LAN966X_CPU_BASE, 0));
	if (status & ~CPU_GENERAL_STAT_VCORE_CFG_M) {
		VERBOSE("OVERRIDE CPU_GENERAL_STAT = 0x%08x\n", status);
		strapping = CPU_GENERAL_STAT_VCORE_CFG_X(status);
	}
#endif

	VERBOSE("VCORE_CFG = %d\n", strapping);

	return strapping;
}

void lan966x_set_strapping(uint8_t value)
{
#if defined(DEBUG)
	VERBOSE("OVERRIDE strapping = 0x%08x\n", value);
	mmio_write_32(CPU_GPR(LAN966X_CPU_BASE, 0), 0x10000 | value);
#endif
}

uint32_t lan966x_get_boot_source(void)
{
	boot_source_type boot_source;

	switch (lan966x_get_strapping()) {
	case LAN966X_STRAP_BOOT_MMC:
	case LAN966X_STRAP_BOOT_MMC_FC:
	case LAN966X_STRAP_BOOT_MMC_TFAMON_FC:
		boot_source = BOOT_SOURCE_EMMC;
		break;
	case LAN966X_STRAP_BOOT_QSPI:
	case LAN966X_STRAP_BOOT_QSPI_FC:
	case LAN966X_STRAP_BOOT_QSPI_TFAMON_FC:
		boot_source = BOOT_SOURCE_QSPI;
		break;
	case LAN966X_STRAP_BOOT_SD:
	case LAN966X_STRAP_BOOT_SD_FC:
	case LAN966X_STRAP_BOOT_SD_TFAMON_FC:
		boot_source = BOOT_SOURCE_SDMMC;
		break;
	default:
		boot_source = BOOT_SOURCE_NONE;
		break;
	}

	return boot_source;
}

void lan966x_fwconfig_apply(void)
{
	boot_source_type boot_source = lan966x_get_boot_source();

	/* Update storage drivers with new values from fw_config */
	switch (boot_source) {
	case BOOT_SOURCE_QSPI:
		qspi_reinit();
		break;
	case BOOT_SOURCE_SDMMC:
	case BOOT_SOURCE_EMMC:
		lan966x_mmc_plat_config(boot_source);
		break;
	default:
		break;
	}
}

/*
 * Note: The FW_CONFIG is read *wo* authentication, as the OTP
 * emulation data may contain the actual (emulated) rotpk. This is
 * only called when a *hw* ROTPK has *not* been deployed.
 */
int lan966x_load_fw_config(unsigned int image_id)
{
	uintptr_t dev_handle, image_handle, image_spec = 0;
	size_t bytes_read;
	int result;

	result = plat_get_image_source(image_id, &dev_handle, &image_spec);
	if (result != 0) {
		WARN("Failed to obtain reference to image id=%u (%i)\n",
		     image_id, result);
		return result;
	}

	result = io_open(dev_handle, image_spec, &image_handle);
	if (result != 0) {
		WARN("Failed to access image id=%u (%i)\n", image_id, result);
		return result;
	}

	result = io_read(image_handle, (uintptr_t)&lan966x_fw_config,
			 sizeof(lan966x_fw_config), &bytes_read);
	if (result != 0)
		WARN("Failed to read data (%i)\n", result);

	io_close(image_handle);

#ifdef IMAGE_BL1
	/* This is fwd to BL2 */
	flush_dcache_range((uintptr_t)&lan966x_fw_config, sizeof(lan966x_fw_config));
#endif

	return result;
}

static int fw_config_read_bytes(unsigned int offset,
				unsigned int num_bytes,
				uint8_t *dst)
{
	int ret_val = -1;
	int cnt;
	uint8_t data;

	assert(num_bytes > 0);
	assert((offset + num_bytes) < (sizeof(lan966x_fw_config.config)));

	if (offset < LAN966X_FW_CONF_NUM_OF_ITEMS) {
		for (cnt = 0; cnt < num_bytes; cnt++) {
			data = lan966x_fw_config.config[offset + cnt];
			*dst++ = data;
		}

		ret_val = 0;
	} else {
		ERROR("Illegal offset access to fw_config structure\n");
	}

	return ret_val;
}

int lan966x_fw_config_read_uint8(unsigned int offset, uint8_t *dst)
{
	return fw_config_read_bytes(offset, 1, (uint8_t *)dst);
}

int lan966x_fw_config_read_uint16(unsigned int offset, uint16_t *dst)
{
	return fw_config_read_bytes(offset, 2, (uint8_t *)dst);
}

int lan966x_fw_config_read_uint32(unsigned int offset, uint32_t *dst)
{
	return fw_config_read_bytes(offset, 4, (uint8_t *)dst);
}

/*
 * Derive a 32 byte key with a 32 byte salt, output a 32 byte key
 */
int lan966x_derive_key(const lan966x_key32_t *in,
		       const lan966x_key32_t *salt,
		       lan966x_key32_t *out)
{
	uint8_t buf[LAN966X_KEY32_LEN * 2];
	int ret;

	/* Use one contiguous buffer for now */
	memcpy(buf, in->b, LAN966X_KEY32_LEN);
	memcpy(buf + LAN966X_KEY32_LEN, salt->b, LAN966X_KEY32_LEN);

	ret = sha_calc(SHA_MR_ALGO_SHA256, buf, sizeof(buf), out->b);

	/* Don't leak */
	memset(buf, 0, sizeof(buf));

	return ret;
}

/*
 * Some release build strapping modes will only show error traces by default
 */
void lan966x_set_max_trace_level(void)
{
#if !DEBUG
	switch (lan966x_get_strapping()) {
	case LAN966X_STRAP_BOOT_MMC:
	case LAN966X_STRAP_BOOT_QSPI:
	case LAN966X_STRAP_BOOT_SD:
	case LAN966X_STRAP_PCIE_ENDPOINT:
	case LAN966X_STRAP_TFAMON_FC0:
	case LAN966X_STRAP_TFAMON_FC2:
	case LAN966X_STRAP_TFAMON_FC3:
	case LAN966X_STRAP_TFAMON_FC4:
	case LAN966X_STRAP_TFAMON_USB:
	case LAN966X_STRAP_SPI_SLAVE:
		tf_log_set_max_level(LOG_LEVEL_ERROR);
		break;
	default:
		/* No change in trace level */
		break;
	}
#endif
}
