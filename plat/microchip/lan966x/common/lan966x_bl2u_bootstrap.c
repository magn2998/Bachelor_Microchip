/*
 * Copyright (C) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <common/debug.h>
#include <drivers/auth/crypto_mod.h>
#include <drivers/microchip/qspi.h>
#include <drivers/mmc.h>
#include <drivers/partition/partition.h>
#include <endian.h>
#include <errno.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <tf_gunzip.h>
#include <ddr_init.h>
#include <ddr_reg.h>

#include "lan966x_def.h"
#include "lan966x_private.h"
#include "lan966x_fw_bind.h"
#include "lan966x_bootstrap.h"
#include "lan966x_bl2u_bootstrap.h"
#include "otp.h"

// Include functionality to disable Cache
#include <lib/xlat_tables/xlat_tables_v2.h>

#define MAX_OTP_DATA	1024

#define PAGE_ALIGN(x, a)	(((x) + (a) - 1) & ~((a) - 1))

extern const struct ddr_config lan966x_ddr_config;
static const struct ddr_config *current_ddr_config = &lan966x_ddr_config;

static const uintptr_t fip_base_addr = LAN966X_DDR_BASE;
static const uintptr_t fip_max_size = LAN966X_DDR_SIZE;
static uint32_t data_rcv_length;

// static void int_to_string(uint32_t num, char str[]) 
// {
//     uint32_t i = 0, rem, len = 0, n;
//     n = num;
//     while (n != 0) {
//         len++;
//         n /= 10;
//     }
//     for (i = 0; i < len; i++) {
//         rem = num % 10;
//         str[len - (i + 1)] = rem + '0';
//         num = num / 10;
//     }
//     str[len] = '\0';
// }

static void handle_memoryTest_rnd(bootstrap_req_t *req, uint8_t reversed)
{
	// Numbers to generate seemingly random pattern
	uint8_t modulo = 251;
	uint8_t factor = 13;
	uint8_t start = 17;
	uint8_t cntr = 0;
	
	uint64_t progressStep = LAN966X_DDR_SIZE / (reversed ? 33 : 50);
	uint64_t nextProgressUpdate = 0;

	uint8_t* memoryBase = (uint8_t*)LAN966X_DDR_BASE;
	uint64_t memorySize = LAN966X_DDR_SIZE;


	int a = 12;
	int b =4;
    //        b into a aka. a = b
	// asm ("mov %1, %0;"
	//     :"=r"(a) // related to %0 - Output - Address of variable A
	//     :"r"(b) // related to %1 - Input - 
 //    :);

	//    Load word from address of b into register a with an offset of 0
    asm ("ldr %0, [%1, #0];"
	    :"=r"(a) // related to %0 - Output - Address of variable A
	    :"r"(&b) // related to %1 - Input - 
    :);


	// if(a == 2) {
	// 	bootstrap_TxAckData("A Equals 2", 11);
	// 	return;
	// }
	// bootstrap_TxAckData("A Doesnt Equal 2", 17);
	// return;

	// Populate Memory with data
	cntr = start;
	for(uint64_t i = 0; i < memorySize; i++) {
		*memoryBase = cntr;
		cntr = cntr * factor % modulo;
		memoryBase++;

		if((nextProgressUpdate--) <= 0) { // If it is time to update - update it
			bootstrap_TxAck(); 
			nextProgressUpdate = progressStep;
		}
	}


	// Check memory is identical
	memoryBase = (uint8_t*)LAN966X_DDR_BASE;
	cntr = start;
	for(uint64_t i = 0; i < memorySize; i++) {
		if(*memoryBase != cntr) {
			bootstrap_TxAckData("Test Failed", 12);
			return;
		}
		if(reversed) { // If it runs reversed, store the reversed value
			(*memoryBase) = (~cntr);
		}

		cntr = cntr * factor % modulo;
		memoryBase++;

		if((nextProgressUpdate--) <= 0) { // If it is time to update - update it
			bootstrap_TxAck();
			nextProgressUpdate = progressStep;
		}
	}

	// Check memory is identical to reverse, in case the reverse is checked
	if(reversed == 1) {
		memoryBase = (uint8_t*)LAN966X_DDR_BASE;
		cntr = start;
		for(uint64_t i = 0; (i < memorySize); i++) { // If it should also check reversed
			if((uint8_t)(*memoryBase ^ ~cntr) > 0) {
				bootstrap_TxAckData("Test Failed", 12);
				return;
			}

			cntr = cntr * factor % modulo;
			memoryBase++;

			if((nextProgressUpdate--) <= 0) { // If it is time to update - update it
				bootstrap_TxAck();
				nextProgressUpdate = progressStep;
			}
		}
	}


	// test is succesfull if it hasn't returned by now
	bootstrap_TxAckData("Test Success", 13);
}


static void handle_memoryTest_ones(bootstrap_req_t *req, uint8_t reversed)
{
	mmap_remove_dynamic_region(LAN966X_DDR_BASE, LAN966X_DDR_SIZE);
	int res = mmap_add_dynamic_region(LAN966X_DDR_BASE, LAN966X_DDR_BASE, LAN966X_DDR_SIZE, MT_DEVICE | MT_RW | MT_SECURE | MT_EXECUTE_NEVER); // LAN966X_MAP_DDR_MEM


	if(res != 0) {
		bootstrap_TxAckData("Unable to disable cache", 24);
		return;
	}
	// Flush cache - ensures no data is accidently pushed to memory while running the test
	flush_dcache_range((uint64_t) LAN966X_DDR_BASE, LAN966X_DDR_SIZE);
	
	bootstrap_TxAckData("Unable to disable kjscd", 24);
	return;
	



	uint8_t val = 1; // 00000001
	
	uint64_t progressStep = LAN966X_DDR_SIZE / (reversed ? 33 : 50);
	uint64_t nextProgressUpdate = 0;

	uint8_t* memoryBase = (uint8_t*)LAN966X_DDR_BASE;
	uint64_t memorySize = LAN966X_DDR_SIZE;

	// Populate Memory with data
	for(uint64_t i = 0; i < memorySize; i++) {
		*memoryBase = val;
		memoryBase++;

		val = (val == 0x80 ? 1 : val << 1); // If val = 10000000b (0x80) -> set val = 1 else bitshift value

		if((nextProgressUpdate--) <= 0) { // If it is time to update - update it
			bootstrap_TxAck(); 
			nextProgressUpdate = progressStep;
		}
	}


	// Check memory is identical
	memoryBase = (uint8_t*)LAN966X_DDR_BASE;
	val = 1;
	for(uint64_t i = 0; i < memorySize; i++) {
		if(*memoryBase != val) {
			bootstrap_TxAckData("Test Failed", 12);
			return;
		}
		if(reversed) { // If it runs reversed, store the revered value
			(*memoryBase) = (~val);
		}

		memoryBase++;
		val = (val == 0x80 ? 1 : val << 1); 

		if((nextProgressUpdate--) <= 0) { // If it is time to update - update it
			bootstrap_TxAck();
			nextProgressUpdate = progressStep;
		}
	}

	// Check memory is identical to reverse, in case the reverse is checked
	if(reversed == 1) {
		memoryBase = (uint8_t*)LAN966X_DDR_BASE;
		val = 1;
		for(uint64_t i = 0; (i < memorySize); i++) { // If it should also check reversed
			if((uint8_t)(*memoryBase ^ ~val) > 0) {
				bootstrap_TxAckData("Test Failed", 12);
				return;
			}

			memoryBase++;
			val = (val == 0x80 ? 1 : val << 1); 

			if((nextProgressUpdate--) <= 0) { // If it is time to update - update it
				bootstrap_TxAck();
				nextProgressUpdate = progressStep;
			}
		}
	}


	// attr = MT_MEMORY | MT_RW | MT_SECURE;
	// ret = xlat_change_mem_attributes(fip_base_addr, fip_max_size, attr);
	// if(ret != 0) {
	// 	bootstrap_TxAckData("Unable to enable cache", 23);
	// }

	// Test Succesfull if it hasn't returned by now
	bootstrap_TxAckData("Test Success", 13);
}


static void handle_databusTest(bootstrap_req_t *req)
{
	// Define the size of the data bus in amount of bits
	uint8_t size = 32; // 16 Bit wise address bus used
	uint32_t* baseAddr = (uint32_t*)LAN966X_DDR_BASE; // Address to write to
	uint64_t val = 0xbc; // Value to write - Ise uint64 to account for any sized data bus
	for(uint8_t i = 0; i < size; i++) {
		*baseAddr = val;

		if((*baseAddr) != val) {
			bootstrap_TxAckData("Data Bus Test Failed", 21);
			return;
		}
	}
	bootstrap_TxAckData("Data Bus Test Success", 22);
}  


static void handle_addrBusTest(bootstrap_req_t *req)
{
	uint8_t pattern    = 0xdb;
	uintptr_t addrMask = 0x1;
	uintptr_t baseAddr = LAN966X_DDR_BASE;
	uint8_t* addr; // Actual address to write to

	// Write to all addresses
	for(int i = 0; i < 31; i++) {
		addr = (uint8_t*)(baseAddr | addrMask); // Update Address 
		*addr = pattern; // Write Pattern
		addrMask = addrMask << 0x1; // Update Mask for next iteration
	}

	// Check they're all correct
	addrMask = 0x1; // Reset Address Mask
	for(int i = 0; i < 30; i++) {
		addr = (uint8_t*)(baseAddr | addrMask); // Update Address for next iteration
		
		if(*addr != pattern) {
			bootstrap_TxAckData("Test Failed", 12); // Error
			return;
		}

		*addr = ~pattern; // Write Anti-Pattern

		uintptr_t secAddrMask = 0x1; // Mask for checking other addresses
		uint8_t* secAddr; // Address for checking 
		for(int o = 0; o < 30; o++) {
			secAddr = (uint8_t*)(baseAddr | secAddrMask); // Use second address Mask to iterate again
			if((*secAddr != pattern && i!=o)) {
				bootstrap_TxAckData("Test Failed", 12); // Error if pattern is not correct in other addresses or the current address is not inverted
				return; 
			}
			secAddrMask = secAddrMask << 1;
		}
		
		*addr = pattern; // Write Back original pattern

		addrMask = addrMask << 0x1; // Update Mask for next iteration
	}

	bootstrap_TxAckData("Test Success", 13);
}


static void handle_read_ddr_configuration(bootstrap_req_t *req) {
	bootstrap_TxAckData((void*)current_ddr_config, sizeof lan966x_ddr_config);
}


static void handle_setup_ddr_memory_configuration(bootstrap_req_t *req) {
	uint8_t data[sizeof lan966x_ddr_config]; // 
	int num_bytes = 0;

	// Signal it is ready to receive data from client
	bootstrap_TxAck();

	// Read Data from request into data
	num_bytes = bootstrap_RxData(data, 1, sizeof lan966x_ddr_config); // Read the request - 336 bytes long

	if(num_bytes != (sizeof lan966x_ddr_config)) {
		bootstrap_TxAckData("Failed uploading config", 24);
	} 
	

	// Next Step is to load data into the ddr_config type, which is defined in include/ddr_config.h
	// Since the size of the values are matching, we can simply point to the data!
	current_ddr_config = (struct ddr_config*) data;
	ddr_init(current_ddr_config);

	bootstrap_TxAckData("Successfully Uploaded Configuration", 36);
}

static void handle_otp_read(bootstrap_req_t *req, bool raw)
{
	uint8_t data[256];
	uint32_t datalen;

	if (req->len == sizeof(uint32_t) && bootstrap_RxDataCrc(req, (uint8_t *)&datalen)) {
		datalen = __ntohl(datalen);
		if (datalen > 0 && datalen < sizeof(data) &&
		    req->arg0 >= 0 && (req->arg0 + datalen) <= OTP_MEM_SIZE) {
			int rc = (raw ?
				  otp_read_bytes_raw(req->arg0, datalen, data) :
				  otp_read_bytes(req->arg0, datalen, data));
			if (rc < 0)
				bootstrap_TxNack_rc("OTP read fails", rc);
			else
				bootstrap_TxAckData(data, datalen);
		} else
			bootstrap_TxNack("OTP read illegal length");
	}
}

static void handle_otp_data(bootstrap_req_t *req)
{
	uint8_t data[MAX_OTP_DATA];

	if (req->len > 0 && req->len < MAX_OTP_DATA &&
	    bootstrap_RxDataCrc(req, data)) {
		if (otp_write_bytes(req->arg0, req->len, data) == 0)
			bootstrap_Tx(BOOTSTRAP_ACK, req->arg0, 0, NULL);
		else
			bootstrap_TxNack("OTP program failed");

		/* Wipe data */
		memset(data, 0, req->len);
	} else
		bootstrap_TxNack("OTP rx data failed or illegal data size");
}

static void handle_otp_random(bootstrap_req_t *req)
{
	/* Note: We keep work buffers on stack - must be large enough */
	uint32_t datalen, data[MAX_OTP_DATA / 4], cur_data[MAX_OTP_DATA / 4];
	int i;

	if (req->len == sizeof(uint32_t) && bootstrap_RxDataCrc(req, (uint8_t *)&datalen)) {
		datalen = __ntohl(datalen);
		if (datalen > 0 &&
		    datalen < MAX_OTP_DATA) {
			if (otp_read_bytes_raw(req->arg0, datalen, (uint8_t *)cur_data) != 0) {
				bootstrap_TxNack("Unable to read OTP data");
				return;
			}

			if (!otp_all_zero((uint8_t*) cur_data, datalen)) {
				bootstrap_TxNack("OTP data already non-zero");
				goto wipe_cur;
			}

			/* Read TRNG data */
			for (i = 0; i < div_round_up(datalen, sizeof(uint32_t)); i++)
				data[i] = lan966x_trng_read();

			/* Write to OTP */
			INFO("Write OTP\n");
			if (otp_write_bytes(req->arg0, datalen, (uint8_t *)data) == 0)
				bootstrap_Tx(BOOTSTRAP_ACK, req->arg0, 0, NULL);
			else
				bootstrap_TxNack("OTP program random failed");

			/* Wipe data */
			memset(data, 0, datalen);
		wipe_cur:
			memset(cur_data, 0, datalen);
		} else
			bootstrap_TxNack("OTP random data illegal length");
	} else
		bootstrap_TxNack("OTP random data illegal req length length");

}

static void handle_read_rom_version(const bootstrap_req_t *req)
{
	VERBOSE("BL2U handle read rom version\n");
	bootstrap_TxAckData(version_string, strlen(version_string));
}

static void handle_load_data(const bootstrap_req_t *req)
{
	uint32_t length = req->arg0;
	uint8_t *ptr;
	int num_bytes, offset;
	data_rcv_length = 0;

	VERBOSE("BL2U handle load data\n");

	if (length == 0 || length > fip_max_size) {
		bootstrap_TxNack("Length Error");
		return;
	}

	/* Store data at start address of DDR memory (offset 0x0) */
	ptr = (uint8_t *)fip_base_addr;

	// Go ahead, receive data
	bootstrap_TxAck();

	/* Gobble up the data chunks */
	num_bytes = 0;
	offset = 0;

	while (offset < length && (num_bytes = bootstrap_RxData(ptr, offset, length - offset)) > 0) {
		ptr += num_bytes;
		offset += num_bytes;
	}

	if (offset != length) {
		ERROR("RxData Error: n = %d, l = %d, o = %d\n", num_bytes, length, offset);
		return;
	}

	/* Store data length of received data */
	data_rcv_length = length;

	VERBOSE("Received %d bytes\n", length);
}

static void handle_unzip_data(const bootstrap_req_t *req)
{
	uint8_t *ptr = (uint8_t *)fip_base_addr;
	const char *resp = "Plain data";

	/* See if this is compressed data */
	if (ptr[0] == 0x1f && ptr[1] == 0x8b) {
		uintptr_t in_buf, work_buf, out_buf, out_start;
		size_t in_len, work_len, out_len;

		/* GZIP 'magic' seen, try to decompress */
		INFO("Looks like GZIP data\n");

		/* Set up decompress params */
		in_buf = fip_base_addr;
		in_len = data_rcv_length;
		work_buf = in_buf + PAGE_ALIGN(data_rcv_length, SIZE_M(1));
		work_len = SIZE_M(16);
		out_start = out_buf = work_buf + work_len;
		out_len = fip_max_size - (out_buf - in_buf);
		VERBOSE("gunzip(%p, %zd, %p, %zd, %p, %zd)\n",
			(void*) in_buf, in_len, (void*) work_buf, work_len, (void*) out_buf, out_len);
		if (gunzip(&in_buf, in_len, &out_buf, out_len, work_buf, work_len) == 0) {
			out_len = out_buf - out_start;
			memmove((void *)fip_base_addr, (const void *) out_start, out_len);
			data_rcv_length = out_len;
			INFO("Unzipped data, length now %d bytes\n", data_rcv_length);
			resp = "Decompressed data";
		} else {
			INFO("Non-zipped data, length %zd bytes\n", data_rcv_length);
		}
	}

	/* Send response */
	bootstrap_TxAckData_arg(resp, strlen(resp), data_rcv_length);
}

/*
 * NOTE: Instead of calling mmc_write_blocks() directly we have to
 * spoon feed individual blocks. This is needed due to a constraint at a
 * lower level of the MMC driver.
 */
static uint32_t single_mmc_write_blocks(uint32_t lba, uintptr_t buf_ptr, uint32_t length)
{
	uint32_t written;

	for (written = 0; written < length; ) {
		if (mmc_write_blocks(lba, buf_ptr, MMC_BLOCK_SIZE) != MMC_BLOCK_SIZE) {
			ERROR("Incomplete write at LBA 0x%x, writted %d of %d bytes\n", lba, written, length);
			break;
		}
		buf_ptr += MMC_BLOCK_SIZE;
		written += MMC_BLOCK_SIZE;
		lba++;
	}

	return written;
}

static int emmc_write(uint32_t offset, uintptr_t buf_ptr, uint32_t length)
{
	uint32_t round_len, lba, written;

	/* Check multiple number of MMC_BLOCK_SIZE */
	assert((offset % MMC_BLOCK_SIZE) == 0);

	/* Convert offset to LBA */
	lba = offset / MMC_BLOCK_SIZE;

	/* Calculate whole pages for mmc_write_blocks() */
	round_len = DIV_ROUND_UP_2EVAL(length, MMC_BLOCK_SIZE) * MMC_BLOCK_SIZE;

	VERBOSE("Write image to offset: 0x%x, length: 0x%x, LBA 0x%x, round_len 0x%x\n",
		offset, length, lba, round_len);

	written = single_mmc_write_blocks(lba, buf_ptr, round_len);

	VERBOSE("Written 0x%0x of the requested 0x%0x bytes\n", written, round_len);

	return written == round_len ? 0 : -EIO;
}

static int fip_update(const char *name, uintptr_t buf_ptr, uint32_t len)
{
	const partition_entry_t *entry = get_partition_entry(name);

	if (entry) {
		if (len > entry->length) {
			NOTICE("Partition %s only can hold %d bytes, %d uploaded\n",
			       name, (uint32_t) entry->length, len);
			return false;
		}
		return emmc_write(entry->start, buf_ptr, len);
	}

	NOTICE("Partition %s not found\n", name);
	return -ENOENT;
}

static bool valid_write_dev(boot_source_type dev)
{
	if (dev == BOOT_SOURCE_EMMC ||
	    dev == BOOT_SOURCE_SDMMC ||
	    dev == BOOT_SOURCE_QSPI)
		return true;

	return false;
}

/* This routine will write the image data flash device */
static void handle_write_image(const bootstrap_req_t * req)
{
	int ret;

	VERBOSE("BL2U handle write image\n");

	if (data_rcv_length == 0) {
		bootstrap_TxNack("Flash Image not loaded");
		return;
	}

	if (!valid_write_dev(req->arg0)) {
		bootstrap_TxNack("Unsupported target device");
		return;
	}

	/* Init IO layer */
	lan966x_io_init_dev(req->arg0);

	/* Write Flash */
	switch (req->arg0) {
	case BOOT_SOURCE_EMMC:
	case BOOT_SOURCE_SDMMC:
		ret = emmc_write(0, fip_base_addr, data_rcv_length);
		break;
	case BOOT_SOURCE_QSPI:
		ret = qspi_write(0, (void*) fip_base_addr, data_rcv_length);
		break;
	default:
		ret = -ENOTSUP;
	}

	if (ret)
		bootstrap_TxNack("Image write failed");
	else
		bootstrap_TxAck();
}

/* This routine will write the previous encrypted FIP data to the flash device */
static void handle_write_fip(const bootstrap_req_t * req)
{
	int ret;

	VERBOSE("BL2U handle write data\n");

	if (data_rcv_length == 0) {
		bootstrap_TxNack("FIP Image not loaded");
		return;
	}

	if (!valid_write_dev(req->arg0)) {
		bootstrap_TxNack("Unsupported target device");
		return;
	}

	if (!is_valid_fip_hdr((const fip_toc_header_t *)fip_base_addr)) {
		bootstrap_TxNack("Data is not a valid FIP");
		return;
	}

	/* Generic IO init */
	lan966x_io_setup();

	/* Init IO layer, explicit source */
	if (req->arg0 != lan966x_get_boot_source())
		lan966x_io_init_dev(req->arg0);

	/* Write Flash */
	switch (req->arg0) {
	case BOOT_SOURCE_EMMC:
	case BOOT_SOURCE_SDMMC:
		INFO("Write FIP %d bytes to %s\n", data_rcv_length, req->arg0 == BOOT_SOURCE_EMMC ? "eMMC" : "SD");

		/* Init GPT */
		partition_init(GPT_IMAGE_ID);

		/* All OK to start */
		ret = 0;

		/* Update primary FIP */
		if (fip_update(FW_PARTITION_NAME, fip_base_addr, data_rcv_length))
			ret++;

		/* Update backup FIP */
		if (fip_update(FW_BACKUP_PARTITION_NAME, fip_base_addr, data_rcv_length))
			ret++;

		break;

	case BOOT_SOURCE_QSPI:
		INFO("Write FIP %d bytes to QSPI NOR\n", data_rcv_length);
		ret = qspi_write(0, (void*) fip_base_addr, data_rcv_length);
		break;

	default:
		ret = -ENOTSUP;
	}

	if (ret < 0)
		bootstrap_TxNack("Write FIP failed");
	else if (ret == 1)
		bootstrap_TxNack("One partition failed to update");
	else if (ret == 2)
		bootstrap_TxNack("Both partitions failed to update");
	else
		bootstrap_TxAck();
}

static void handle_bind(const bootstrap_req_t *req)
{
	fw_bind_res_t result;

	VERBOSE("BL2U handle bind operation\n");

	if (data_rcv_length == 0 || data_rcv_length > fip_max_size) {
		bootstrap_TxNack("Image not loaded, length error");
		return;
	}

	/* Parse FIP for encrypted image files */
	result = lan966x_bind_fip(fip_base_addr, data_rcv_length);
	if (result) {
		bootstrap_TxNack(lan966x_bind_err2str(result));
	} else {
		VERBOSE("FIP image successfully accessed\n");
		bootstrap_TxAck();
	}
}

void lan966x_bl2u_bootstrap_monitor(void)
{
	bool exit_monitor = false;
	bootstrap_req_t req = { 0 };

	lan966x_reset_max_trace_level();
	INFO("*** ENTERING BL2U BOOTSTRAP MONITOR ***\n");

	while (!exit_monitor) {
		if (!bootstrap_RxReq(&req)) {
			bootstrap_TxNack("Garbled command");
			continue;
		}

		if (is_cmd(&req, BOOTSTRAP_RESET)) {		// e - Reset (by exit)
			bootstrap_TxAck();
			exit_monitor = true;
		} else if (is_cmd(&req, BOOTSTRAP_VERS))	// V - Get bootstrap version
			handle_read_rom_version(&req);
		else if (is_cmd(&req, BOOTSTRAP_SEND))		// S - Load data file
			handle_load_data(&req);
		else if (is_cmd(&req, BOOTSTRAP_UNZIP))		// Z - Unzip data
			handle_unzip_data(&req);
		else if (is_cmd(&req, BOOTSTRAP_IMAGE))		// I - Copy uploaded raw image from DDR memory to flash device
			handle_write_image(&req);
		else if (is_cmd(&req, BOOTSTRAP_WRITE))		// W - Copy uploaded fip from DDR memory to flash device
			handle_write_fip(&req);
		else if (is_cmd(&req, BOOTSTRAP_BIND))		// B - FW binding operation (decrypt and encrypt)
			handle_bind(&req);
		else if (is_cmd(&req, BOOTSTRAP_OTPD))
			handle_otp_data(&req);
		else if (is_cmd(&req, BOOTSTRAP_OTPR))
			handle_otp_random(&req);
		else if (is_cmd(&req, BOOTSTRAP_OTP_READ_EMU))	// L - Read OTP data
			handle_otp_read(&req, false);
		else if (is_cmd(&req, BOOTSTRAP_OTP_READ_RAW))	// l - Read RAW OTP data
			handle_otp_read(&req, true);
		else if(is_cmd(&req, BOOTSTRAP_DDR_CONFIG_READOUT))
			handle_read_ddr_configuration(&req);
		else if(is_cmd(&req, BOOTSTRAP_MEMORY_INIT_CUSTOM)) // f - Setup DDR Configuration
			handle_setup_ddr_memory_configuration(&req);
		else if(is_cmd(&req, BOOTSTRAP_MEMORYTEST_DATABUS)) // k - Data Bus Test
			handle_databusTest(&req);
		else if(is_cmd(&req, BOOTSTRAP_MEMORYTEST_ADDRBUS)) // K - Address Bus Test
			handle_addrBusTest(&req);
		else if(is_cmd(&req, BOOTSTRAP_MEMORYTEST_RND)) // x - Random Pattern Test
			handle_memoryTest_rnd(&req, 0);
		else if(is_cmd(&req, BOOTSTRAP_MEMORYTEST_RND_REV)) // X - Random Pattern Test + Reverse
			handle_memoryTest_rnd(&req, 1);
		else if(is_cmd(&req, BOOTSTRAP_MEMORYTEST_ONES)) // y - Walking Ones Test
			handle_memoryTest_ones(&req, 0);
		else if(is_cmd(&req, BOOTSTRAP_MEMORYTEST_ONES_REV)) // Y - Walking Ones Test + Reverse
			handle_memoryTest_ones(&req, 1);
		else
			bootstrap_TxNack("Unknown command");
	}

	INFO("*** EXITING BL2U BOOTSTRAP MONITOR ***\n");
	lan966x_set_max_trace_level();
}
