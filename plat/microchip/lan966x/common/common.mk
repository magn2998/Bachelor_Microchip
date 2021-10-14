#
# Copyright (c) 2021, Microchip Technology Inc. and its subsidiaries.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifeq (${ARCH},aarch64)
  $(error Error: AArch64 not supported on ${PLAT})
endif

ARM_CORTEX_A7                   := yes
ARM_ARCH_MAJOR			:= 7

# Default number of CPUs per cluster on FVP
LAN966x_MAX_CPUS_PER_CLUSTER	:= 1

# Default number of threads per CPU on FVP
LAN966x_MAX_PE_PER_CPU	:= 1

# To compile with highest log level (VERBOSE) set value to 50
LOG_LEVEL := 40

# Single-core system
WARMBOOT_ENABLE_DCACHE_EARLY	:=	1

# Pass LAN966x_MAX_CPUS_PER_CLUSTER to the build system.
$(eval $(call add_define,LAN966x_MAX_CPUS_PER_CLUSTER))

# Pass LAN966x_MAX_PE_PER_CPU to the build system.
$(eval $(call add_define,LAN966x_MAX_PE_PER_CPU))

# Non-volatile counter values
TFW_NVCTR_VAL		:=	2
NTFW_NVCTR_VAL		:=	3

include lib/xlat_tables_v2/xlat_tables.mk

ifneq (${TRUSTED_BOARD_BOOT},0)

    $(info Including platform TBBR)
    include plat/microchip/lan966x/common/plat_tbbr.mk

endif

# Default chip variant = platform
ifeq (${PLAT_VARIANT},)
PLAT_VARIANT			:=	${PLAT}
endif

PLAT_INCLUDES	:=	-Iplat/microchip/lan966x/${PLAT_VARIANT}/include	\
			-Iplat/microchip/lan966x/common/include			\
			-Idrivers/microchip/crypto/inc/				\
			-Iinclude/drivers/microchip

LAN966X_CONSOLE_SOURCES	:=	\
				drivers/microchip/gpio/vcore_gpio.c			\
				drivers/microchip/qspi/qspi.c				\
				drivers/microchip/flexcom_uart/flexcom_console.S	\
				drivers/gpio/gpio.c

ifneq (${LAN966X_USE_USB},)
$(eval $(call add_define,LAN966X_USE_USB))
LAN966X_CONSOLE_SOURCES	+=	drivers/microchip/usb/usb.c
endif

LAN966X_STORAGE_SOURCES	:=	\
				drivers/io/io_block.c					\
				drivers/io/io_fip.c					\
				drivers/io/io_memmap.c					\
				drivers/io/io_storage.c					\
				drivers/microchip/emmc/emmc.c				\
				drivers/mmc/mmc.c					\
				drivers/partition/gpt.c					\
				drivers/partition/partition.c				\
				plat/microchip/lan966x/common/lan966x_io_storage.c	\
				plat/microchip/lan966x/common/lan966x_mmc.c

ifneq (${TRUSTED_BOARD_BOOT},0)
LAN966X_STORAGE_SOURCES	+=	drivers/io/io_encrypted.c
endif

PLAT_BL_COMMON_SOURCES	+=	\
				${LAN966X_CONSOLE_SOURCES}				\
				${LAN966X_STORAGE_SOURCES}				\
				${XLAT_TABLES_LIB_SRCS}					\
				common/desc_image_load.c				\
				drivers/delay_timer/delay_timer.c			\
				drivers/delay_timer/generic_delay_timer.c		\
				drivers/microchip/clock/lan966x_clock.c			\
				drivers/microchip/crypto/lan966x_sha.c			\
				drivers/microchip/otp/otp.c				\
				drivers/microchip/tz_matrix/tz_matrix.c			\
				lib/cpus/aarch32/cortex_a7.S				\
				plat/common/${ARCH}/crash_console_helpers.S		\
				plat/microchip/lan966x/common/${ARCH}/plat_helpers.S	\
				plat/microchip/lan966x/common/lan966x_common.c		\
				plat/microchip/lan966x/common/lan966x_crc32.c		\
				plat/microchip/lan966x/common/lan966x_trng.c

BL1_SOURCES		+=	\
				plat/microchip/lan966x/common/lan966x_bl1_bootstrap.c	\
				plat/microchip/lan966x/common/lan966x_bl1_pcie.c	\
				plat/microchip/lan966x/common/lan966x_bl1_setup.c	\
				plat/microchip/lan966x/common/lan966x_bootstrap.c	\
				plat/microchip/lan966x/common/lan966x_sjtag.c

BL2_SOURCES		+=	\
				plat/microchip/lan966x/common/lan966x_bl2_mem_params_desc.c \
				plat/microchip/lan966x/common/lan966x_bl2_setup.c	\
				plat/microchip/lan966x/common/lan966x_ddr.c		\
				plat/microchip/lan966x/common/lan966x_image_load.c	\
				plat/microchip/lan966x/common/lan966x_sjtag.c		\
				plat/microchip/lan966x/common/lan966x_tz.c

BL2U_SOURCES		+=	\
				plat/microchip/lan966x/common/lan966x_bl2u_setup.c

ifeq (${BL2_VARIANT},NOOP)
override BL2_SOURCES		:=	\
				bl2/${ARCH}/bl2_entrypoint.S				\
				plat/microchip/lan966x/common/${ARCH}/plat_bl2_noop.S
endif

# Add the build options to pack Trusted OS Extra1 and Trusted OS Extra2 images
# in the FIP if the platform requires.
ifneq ($(BL32_EXTRA1),)
$(eval $(call TOOL_ADD_IMG,bl32_extra1,--tos-fw-extra1,,$(ENCRYPT_BL32)))
endif
ifneq ($(BL32_EXTRA2),)
$(eval $(call TOOL_ADD_IMG,bl32_extra2,--tos-fw-extra2,,$(ENCRYPT_BL32)))
endif

# Enable Activity Monitor Unit extensions by default
ENABLE_AMU			:=	1

# We have TRNG
TRNG_SUPPORT			:=	1

# Enable stack protection
ENABLE_STACK_PROTECTOR	 	:= strong

ifneq (${BL2_AT_EL3}, 0)
    override BL1_SOURCES =
endif

ifneq (${ENABLE_STACK_PROTECTOR},0)
PLAT_BL_COMMON_SOURCES  +=      plat/microchip/lan966x/common/lan966x_stack_protector.c
endif

LAN966X_FW_CONFIG	:=	${BUILD_PLAT}/fw_config.bin

${LAN966X_FW_CONFIG}: bin/fw_param.bin ${LAN966X_OTP_DATA}
	$(Q)ruby ./scripts/otpbin.rb $(if ${LAN966X_OTP_DATA},-y ${LAN966X_OTP_DATA}) -o $@
	$(Q)cat bin/fw_param.bin >> $@

# FW config
$(eval $(call TOOL_ADD_PAYLOAD,${LAN966X_FW_CONFIG},--fw-config,${LAN966X_FW_CONFIG}))
