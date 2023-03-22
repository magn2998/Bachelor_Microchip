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

extern void dma_end(void);

extern const struct ddr_config lan966x_ddr_config;
static const struct ddr_config *current_ddr_config = &lan966x_ddr_config;

static const uintptr_t fip_base_addr = LAN966X_DDR_BASE;
static const uintptr_t fip_max_size = LAN966X_DDR_SIZE;
static uint32_t data_rcv_length;

static void int_to_hex_string(uint32_t num, char str[], int size) {
    const char hex_digits[] = "0123456789ABCDEF";
    uint32_t i = 0, len = 0, n;
    n = num;
    while (n != 0) {
        len++;
        n >>= 4;
    }
    // Calculate the number of leading zeros required to pad the hex value to [size] characters
    uint32_t leading_zeros = size - len;
    // Insert the leading zeros before the hex value
    for (i = 0; i < leading_zeros; i++) {
        str[i] = '0';
    }
    for (i = leading_zeros; i < size; i++) {
        uint32_t digit = num & 0xf;
        str[size - (i + 1) + leading_zeros] = hex_digits[digit];
        num >>= 4;
    }
    str[size] = '\0';
}


static void handle_toggle_cache(bootstrap_req_t *req, uint8_t toggle) { // Toggle = 1 => Enable and Toggle = 0 => Disable
	int res; // Handles result values from the various function calls
	// Remove The Dynamic Region in order to change it
	res = mmap_remove_dynamic_region(LAN966X_DDR_BASE, LAN966X_DDR_SIZE);

	if(res != 0) {
		bootstrap_TxAckData("Unable to Remove DDR MMAP region", 33);
		return;
	}
	// Define the attribute                   W. Cache    Wo. Cache
	unsigned int attribute = ((toggle == 1) ? MT_MEMORY : MT_DEVICE) | MT_RW | MT_SECURE | MT_EXECUTE_NEVER;


	res = mmap_add_dynamic_region(LAN966X_DDR_BASE, LAN966X_DDR_BASE, LAN966X_DDR_SIZE, attribute); 
	if(res != 0) {
		bootstrap_TxAckData((toggle == 1) ? "Unable to enable cache " : "Unable to disable cache", 24);
		return;
	}
	// Flush and clean cache - See: https://trustedfirmware-a.readthedocs.io/en/latest/getting_started/psci-lib-integration-guide.html 
	flush_dcache_range((uint64_t) LAN966X_DDR_BASE, LAN966X_DDR_SIZE);

	bootstrap_TxAckData((toggle == 1) ? "Successfully enabled cache " : "Successfully disabled cache", 28);
}

static void handle_memoryTest_rnd(bootstrap_req_t *req, uint8_t reversed) {
	uint8_t result = reversed; // Flag for result - Also indicates whether it is reversed or not
	uint32_t randomizer = 0xFFFFDA61;
	uint32_t startVal   = 0xFFFF00; 

	uint32_t memoryAddr = (uint32_t) LAN966X_DDR_BASE;
	asm volatile (
		// Prepare randomization algorithm
		"MOV r4, %[x];" // Lower 32 bits of x
		"MOV r5, #0;"   // Upper 32 bits of x
		// Prepare constants		
		"MOV r2, #0;"          // Max value for the address - Has to use Mov and Movt since imm is only 16 bits.
		"MOVT r2, #0x8000;"    // Write 16 last bits - set bit 31 (DDR mem goes from 0x60000000 to 0x80000000)
		"MOV r3, %[memAddr];"  // Save base address for later use
		// First loop - populate DDR memory
		"RNDLOOP1:" // Label for beginning first loop
		"STR r4, [%[memAddr]], #4;" // Store value
		"BL RANDOM;" // Randomize r4 value
		"CMP %[memAddr], r2;" // Check is max-address is reached
		"IT NE;"
		"BNE RNDLOOP1;"
		// Reset Randomization Algorithm
		"MOV r4, %[x];" // Lower 32 bits of x
		"MOV r5, #0;"   // Upper 32 bits of x
		"MOV %[memAddr], r3;" // Return to base memory address


		// Second Loop - Check DDR memory
		"RNDLOOP2:"
		"LDR r1, [%[memAddr]], #4;" // read from memory
		"CMP r1, r4;"  // Compare pattern and read value
		"IT NE;"       // Prepare branch
		"BNE ENDRNDTEST;" // Branch end-test if they're not equal

		"CMP %[resultOutput], 0x1;" // If it is reversed, write the inverse value, otherwise continue
		"ITTT EQ;"
		"MVNEQ r1, r4;" // MVN - Move Not - Move and perform bitwise not - Basically Negation on pattern - Reversing Pattern
		"SUBEQ %[memAddr], %[memAddr], #4;" // Move address one down again
		"STREQ r1, [%[memAddr]], #4;"         // Store negated value at address and increment address again

		"BL RANDOM;" // Randomize r4 value
		"CMP %[memAddr], r2;" // Check is max-address is reached
		"IT NE;"
		"BNE RNDLOOP2;"


		// Check if reversed check should be done
		"CMP %[resultOutput], 0x1;"
		"IT NE;"
		"BNE SKIPRNDLOOP3;" // Skip loop 3 if is shouldn't do reverse check

		// Reset Randomization Algorithm Again
		"MOV r4, %[x];" // Lower 32 bits of x
		"MOV r5, #0;"   // Upper 32 bits of x
		"MOV %[memAddr], r3;" // Return to base memory address
		// Third Loop - Check reversed DDR memory
		"RNDLOOP3:"
		"LDR r1, [%[memAddr]], #4;" // read from memory
		"MVN r1, r1;"               // Reverse value read from memory
		"CMP r1, r4;"  // Compare pattern and inversed read value
		"IT NE;"       // Prepare branch
		"BNE ENDRNDTEST;" // Branch end-test if they're not equal
		"BL RANDOM;" // Randomize r4 value
		"CMP %[memAddr], r2;" // Check is max-address is reached
		"IT NE;"
		"BNE RNDLOOP3;"

		// Set result to success and end test
		"SKIPRNDLOOP3:"
		"MOV %[resultOutput], #2;" 
		"B ENDRNDTEST;" // End test

		"RANDOM:" // Enter "function" to determine random value based on a[rnd] and lower bits of x[r4] stores in [r6,r7]
		"UMULL r6, r7, %[rnd], r4;" // Multiply a[32 bit] with lower bits of x[r4]
		"ADDS r4, r6, r5;"       // Add lower bits of previous result with upper bits of x[r5]. Store as the new lower bits of x[r4] + S for setting (carry) flags
		"ADC r5, r7, #0;"       // Adds any carry value to the upper bits of the new x value
		"BX r14;"                // Return to LR address. Result is stored in r7 and r8, lower and upper 32 bits respectively

		"ENDRNDTEST:"
	: [memAddr] "+&r" (memoryAddr), [resultOutput] "+&r" (result)
    : "r" (memoryAddr), [rnd] "r" (randomizer), [x] "r" (startVal)
	: "r1", "r2", "r3", "r4", "r5", "r6", "r7"); // Clobbered register for temp storage. In order: Pattern, MaxValue, Base Address, Read Value from memory register and three temp registers


	if(result == 0x2) {
		bootstrap_TxAckData("Random Pattern Test Succeded", 29);
		return;
	} 

	char addressStr[9]; // String of address - takes 8 characters, plus one null terminated character
	int_to_hex_string(memoryAddr-0x4, addressStr, 8); // Remember to remove 0x4 from the address, since it is added before the check in the assembly code
	char resultStr[50] = "Random Pattern Test Failed at Address: 0x"; // Size = 41 chars for text + 9 for address text
	strlcat(resultStr, addressStr, 50);
	bootstrap_TxAckData(resultStr, 50);
}


static void handle_memoryTest_ones(bootstrap_req_t *req, uint8_t reversed) {
	uint8_t result = 0; // Flag for result - Only set to true just before exiting the test, and only if all loops have been finished

	uint32_t memoryAddr = (uint32_t) LAN966X_DDR_BASE;
	asm volatile (
		"MOV r1, #1;"          // Initiate pattern
		"MOV r2, #0;"          // Max value for the pattern - Has to use Mov and Movt since imm is only 16 bits.
		"MOVT r2, #0x8000;"    // Write 16 last bits - set bit 31 (DDR mem goes from 0x60000000 to 0x80000000)
		"MOV r3, %[memAddr];"  // Save base address for later use
		"LOOP1:" // Label for beginning first loop
		"STR r1, [%[memAddr]], #4;"
		"CMP r1, r2;"       // Check that pattern has reached 0x80000000
		"ITE NE;"           // Prepare If-Then statement
		"LSLNE r1, r1, #1;" // If not equal, left shift bit ones
		"MOVEQ r1, #1;"     // Else (they're equal) set pattern to 0x1
		"CMP %[memAddr], r2;" // Check is max-address is reached
		"IT NE;"
		"BNE LOOP1;"
		// Beginning Loop no. 2
		"MOV r1, #1;"         // Reset Pattern
		"MOV %[memAddr], r3;" // Return to base memory address
		"LOOP2:"
		"LDR r4, [%[memAddr]], #4;" // read from memory
		"CMP r1, r4;"  // Compare pattern and read value
		"IT NE;"       // Prepare branch
		"BNE ENDTEST;" // Branch end-test if they're not equal

		"CMP %[isReversed], 0x1;" // If it is reversed, write the inverse value, otherwise continue
		"ITTT EQ;"
		"MVNEQ r4, r1;" // MVN - Move Not - Move and perform bitwise not - Basically Negation on pattern - Reversing Pattern
		"SUBEQ %[memAddr], %[memAddr], #4;" // Move address one down again
		"STREQ r4, [%[memAddr]], #4;"         // Store negated value at address and increment address again

		"CMP r1, r2;"       // Check that pattern has reached 0x80000000
		"ITE NE;"           // Prepare If-Then statement
		"LSLNE r1, r1, #1;" // If not equal, left shift bit ones
		"MOVEQ r1, #1;"     // Else (they're equal) set pattern to 0x1
		"CMP %[memAddr], r2;" // Check is max-address is reached
		"IT NE;"
		"BNE LOOP2;"
		// Check if it should continue
		"CMP %[isReversed], 0x1;"
		"IT NE;"
		"BNE SKIPLOOP3;" // Skip loop 3 if is shouldn't do reverse check
		// Beginning Loop no. 3
		"MOV r1, #1;"         // Reset Pattern
		"MOV %[memAddr], r3;" // Return to base memory address
		"LOOP3:"
		"LDR r4, [%[memAddr]], #4;" // read from memory
		"MVN r4, r4;"  // Negate Value from memory
		"CMP r1, r4;"  // Compare pattern and read value
		"IT NE;"       // Prepare branch
		"BNE ENDTEST;" // Branch end-test if they're not equal
		"CMP r1, r2;"       // Check that pattern has reached 0x80000000
		"ITE NE;"           // Prepare If-Then statement
		"LSLNE r1, r1, #1;" // If not equal, left shift bit ones
		"MOVEQ r1, #1;"     // Else (they're equal) set pattern to 0x1
		"CMP %[memAddr], r2;" // Check is max-address is reached
		"IT NE;"
		"BNE LOOP3;"

		"SKIPLOOP3:"
		"MOV %[resultOutput], #0x1;" // Set result to success
		"ENDTEST:"
	: [memAddr] "+&r" (memoryAddr), [resultOutput] "=r" (result)
    : "r" (memoryAddr), [isReversed] "r" (reversed) 
	: "r1", "r2", "r3", "r4"); // Clobbered register for temp storage. In order: Pattern, MaxValue, Base Address, Read Value from memory register


	if(result == 0x1) {
		bootstrap_TxAckData("Assembly Walking Ones Test Success", 35);
		return;
	} 


	char addressStr[9]; // String of address - takes 8 characters, plus one null terminated character
	int_to_hex_string(memoryAddr-0x4, addressStr, 8); // Remember to remove 0x4 from the address, since it is added before the check in the assembly code
	char resultStr[57] = "Assembly Walking Ones Test Failed at Address: 0x"; // Size = 48 chars for text + 9 for address text
	strlcat(resultStr, addressStr, 57);
	bootstrap_TxAckData(resultStr, 57);
}


static void handle_databusTest(bootstrap_req_t *req) {
	uint8_t result    = 0; // Flag for result - Only set to true just before exiting the test, and only if all loops have been finished
	uint32_t expected = 0;
	uint32_t actual   = 0;

	uint32_t memoryAddr = (uint32_t) LAN966X_DDR_BASE;
	asm volatile (
		"MOV %[_expected], #1;" // Initiate Pattern
		"MOVW r2, #0;"          // Max value for the pattern - Has to use Mov and Movt since imm is only 16 bits.
		"MOVT r2, #0x8000;"    // Write 16 last bits - set bit 31

		"DATABUSLOOP1:" // Begin Test
		"STR %[_expected], [%[memAddr]];" // Store value at a chosen address in the DDR chip
		"DSB;"                  // Ensure value has been stored
		"LDR %[_actual], [%[memAddr]];" // Load value 
		"CMP %[_expected], %[_actual];"           // Compare read with written
		"IT NE;"
		"BNE ENDDATABUSTEST;"   // Stop test if mismatch
		"CMP %[_expected], r2;"          // Check is end is reached (Bit 31 is set)
		"ITT EQ;"
		"MOVEQ %[resultOutput], #1;" // Set Success flag
		"BEQ ENDDATABUSTEST;"        // End test
		"LSL %[_expected], %[_expected], #1;"            // Left shift pattern
		"B DATABUSLOOP1;"            // Continue loop
		"ENDDATABUSTEST:"

	: [memAddr] "+&r" (memoryAddr), [resultOutput] "=r" (result), [_expected] "+r" (expected), [_actual] "+r" (actual)
    : "r" (memoryAddr) 
	: "r2"); // Clobbered register for temp storage: MaxValue


	if(result == 1) {
		bootstrap_TxAckData("Data Bus Test Success", 22);
		return;
	} 

	char expectedStr[9]; 
	char actualStr[9]; 
	int_to_hex_string(expected, expectedStr, 8); 
	int_to_hex_string(actual, actualStr, 8);

	char resultStr[61] = "Data Bus Test Failed. Expected 0x"; // Size = 33 chars for text + 2*8 for values + 11 for middle text + 1 null character

	strlcat(resultStr, expectedStr, 61);
	strlcat(resultStr, " but got 0x", 61);
	strlcat(resultStr, actualStr, 61);

	bootstrap_TxAckData(resultStr, 61);
}  


static void handle_addrBusTest(bootstrap_req_t *req) {
	uint8_t result    = 0; // Flag for result - Only set to true just before exiting the test, and only if all loops have been finished
	uint32_t expected  = 0xAA;
	uint32_t actual    = 0x00;

	uint32_t memoryAddr = (uint32_t) LAN966X_DDR_BASE;
	uint32_t secondMemAddr = (uint32_t) LAN966X_DDR_BASE;
	asm volatile (
		"MOVW r1, #0;"          // Max value for the pattern - Has to use Mov and Movt since imm is only 16 bits.
		"MOVT r1, #0xE000;"    // Write 16 last bits - set bit 31
		"MOV r2, #1;"         // First Address mask is 1
		"MOV r4, %[memAddr];"// Store base address 
		"MOV r5, #0x55;"    // Inverse Pattern

		"ADDRBUSLOOP1:" // Begin Test - populate all addresses with the pattern (expected)
		"STRB %[_expected], [%[memAddr]];"
		"ORR %[memAddr], r4, r2;"
		"CMP %[memAddr], r1;"
		"ITT NE;"
		"LSLNE r2, r2, #1;" 
		"BNE ADDRBUSLOOP1;"
		"DSB;"

		//Reset Address & Address Mask
		"MOV r2, #1;"
		"MOV %[memAddr], r4;"

		"ADDRBUSLOOP2:"
		"STRB r5, [%[memAddr]];" // Store reversed Pattern

		"MOV %[secMemAddr], r4;" // Prepare Second Address 
		"MOV r3, #1;" // Prepare Second Address Mask
		"INNERADDRLOOP:" // Inner loop to check all other addresses have not changed
		"CMP %[secMemAddr], %[memAddr];"
		"IT EQ;"
		"BEQ SKIPADDRCHECK;" // Skip check if it is the address just written to
		
		"LDRB %[_actual], [%[secMemAddr]];"
		"CMP %[_actual], %[_expected];"
		"IT NE;"
		"BNE ENDADDRTEST;"

		"SKIPADDRCHECK:"
		"ORR %[secMemAddr], r4, r3;" // Set SecAddr = baseAddr | SecAddrMask
		"CMP %[secMemAddr], r1;"     // Check if max is reached
		"ITT NE;"
		"LSLNE r3, r3, #1;" 
		"BNE INNERADDRLOOP;"

		"STRB %[_expected], [%[memAddr]];" // Store original pattern at mem addr

		"ORR %[memAddr], r4, r2;" // Set SecAddr = baseAddr | SecAddrMask
		"CMP %[memAddr], r1;"     // Check if max is reached
		"ITT NE;"
		"LSLNE r2, r2, #1;" 
		"BNE ADDRBUSLOOP2;"

		"MOV %[resultOutput], #1;" // Set Success flag
		"ENDADDRTEST:"

	: [memAddr] "+&r" (memoryAddr), [secMemAddr] "+&r" (secondMemAddr), [resultOutput] "=r" (result), [_expected] "+r" (expected), [_actual] "+r" (actual)
    : "r" (memoryAddr) 
	: "r1", "r2", "r3", "r4" ,"r5"); // Clobbered register for temp storage: maxValue, addrMask, SecondAddr, secondAddressMask, baseAddr, inversePattern


	if(result == 1) {
		bootstrap_TxAckData("Address Bus Test Success", 25);
		return;
	} 

	char firstAddress[9]; 
	char secondAddress[9]; 
	char expectedStr[9]; 
	char actualStr[9]; 
	int_to_hex_string(memoryAddr, firstAddress, 8); 
	int_to_hex_string(secondMemAddr, secondAddress, 8);
	int_to_hex_string(expected, expectedStr, 2); 
	int_to_hex_string(actual, actualStr, 2);

	char resultStr[130] = "Address Bus Test Failed. Wrote to address 0x"; // Size = 130 . chars for text[109] + 2*8 for address + 2*2 for values + 1 null character

	strlcat(resultStr, firstAddress, 130);
	strlcat(resultStr, " which changed the value at address 0x", 130);	
	strlcat(resultStr, secondAddress, 130);
	strlcat(resultStr, ". It expected 0x", 130);	
	strlcat(resultStr, expectedStr, 130);
	strlcat(resultStr, " but got 0x", 130);
	strlcat(resultStr, actualStr, 130);

	bootstrap_TxAckData(resultStr, 130);
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
		else if(is_cmd(&req, BOOTSTRAP_DISABLE_CACHE))  // J - disable cache
			handle_toggle_cache(&req, 0);
		else if(is_cmd(&req, BOOTSTRAP_ENABLE_CACHE))   // j - Enable Cache
			handle_toggle_cache(&req, 1);
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
