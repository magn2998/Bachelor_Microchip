/*
 * Copyright (C) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// BOOTSTRAP Start-Of-Frame
#define BOOTSTRAP_SOF          '>'

// Get version
#define BOOTSTRAP_VERS         'V'
// Send Code
#define BOOTSTRAP_SEND         'S'
// Data transmitted
#define BOOTSTRAP_DATA         'D'
// Gunzip (BL2U)
#define BOOTSTRAP_UNZIP        'Z'
// Authenticate & load BL2U
#define BOOTSTRAP_AUTH         'U'
// Override strapping
#define BOOTSTRAP_STRAP        'O'
// Set trace log-level
#define BOOTSTRAP_TRACE_LVL    'T'
// Program OTP data
#define BOOTSTRAP_OTPD         'P'
// Program OTP random data
#define BOOTSTRAP_OTPR         'R'
// Commit OTP emulation data
#define BOOTSTRAP_OTPC         'M'
// Program OTP regions
#define BOOTSTRAP_OTP_REGIONS  'G'
// Read OTP data (normal)
#define BOOTSTRAP_OTP_READ_EMU 'L' /* BL2U */
// Read RAW OTP data
#define BOOTSTRAP_OTP_READ_RAW 'l' /* BL2U */
// Continue boot
#define BOOTSTRAP_CONT         'C'
// SJTAG Read Challenge
#define BOOTSTRAP_SJTAG_RD     'Q'
// SJTAG Write Resonse
#define BOOTSTRAP_SJTAG_WR     'A'
// Write FIP data to eMMC/SD/NOR (BL2U)
#define BOOTSTRAP_WRITE        'W'
// Write raw image to eMMC/SD/NOR (BL2U)
#define BOOTSTRAP_IMAGE        'I'
// Binding operation for decrypt/encrypt (BL2U)
#define BOOTSTRAP_BIND         'B'
// Reset (BL2U)
#define BOOTSTRAP_RESET        'e'
// ACK
#define BOOTSTRAP_ACK          'a'
// NACK
#define BOOTSTRAP_NACK         'n'
// Enable Cache 
#define BOOTSTRAP_ENABLE_CACHE 'j'
// Disable Cache
#define BOOTSTRAP_DISABLE_CACHE 'J'
// Memory Configuration Read Out
#define BOOTSTRAP_DDR_CONFIG_READOUT   'h'
// Memory Chip Burst Write Test 
#define BOOTSTRAP_MEMORYTEST_BURST_WRITE 'H'
// DDR Memory Initialization Custom Configuration
#define BOOTSTRAP_MEMORY_INIT_CUSTOM 'f'
// Memory Data Bus Test
#define BOOTSTRAP_MEMORYTEST_DATABUS  'k'
// Memory Address Bus Test
#define BOOTSTRAP_MEMORYTEST_ADDRBUS  'K'
// Memory Chip Test: Random Pattern
#define BOOTSTRAP_MEMORYTEST_RND   'x'
// Memory Chip Test: Random Pattern + Reversed
#define BOOTSTRAP_MEMORYTEST_RND_REV   'X'
// Memory Chip Test: Walking Ones Pattern (000000001 -> 00000010 -> ...)
#define BOOTSTRAP_MEMORYTEST_ONES   'y'
// Memory Chip Test: Walking Ones Pattern + Reversed (Walking zeros)
#define BOOTSTRAP_MEMORYTEST_ONES_REV   'Y'
// Memory Chip Test: Address Pattern (Write address into address)
#define BOOTSTRAP_MEMORYTEST_ADDRESS   'p'
// Memory Chip Test: Address Pattern + Reversed
#define BOOTSTRAP_MEMORYTEST_ADDRESS_REV   'q'
// Memory Chip Test: Hammer Test
#define BOOTSTRAP_MEMORYTEST_HAMMER   'w'
// Upload Custom Pattern
#define BOOTSTRAP_CUSTOM_PATTERN   'g'
// Memory Chip Test: Bit Fade Pattern
#define BOOTSTRAP_MEMORYTEST_BITFADE 'F'
// Memory Chip Test: Bit Fade Pattern using all Zeros
#define BOOTSTRAP_MEMORYTEST_BITFADE_ALLZEROS 'E'

#define BSTRAP_HEXFLD_LEN	8



// Defines for the pattern interpreter
#define BOOTSTRAP_INTERP_STOP 0
#define BOOTSTRAP_INTERP_REPEAT   1
#define BOOTSTRAP_INTERP_END      2
#define BOOTSTRAP_INTERP_STORE    3
#define BOOTSTRAP_INTERP_LOAD     4
#define BOOTSTRAP_INTERP_CMP      5
#define BOOTSTRAP_INTERP_MOV      6
#define BOOTSTRAP_INTERP_MOVI     7
#define BOOTSTRAP_INTERP_MOVETOP  8
#define BOOTSTRAP_INTERP_ADD      9
#define BOOTSTRAP_INTERP_ADDI    10
#define BOOTSTRAP_INTERP_SUB     11
#define BOOTSTRAP_INTERP_SUBI    12
#define BOOTSTRAP_INTERP_AND     13
#define BOOTSTRAP_INTERP_ANDI    14
#define BOOTSTRAP_INTERP_OR      15
#define BOOTSTRAP_INTERP_ORI     16
#define BOOTSTRAP_INTERP_XOR     17
#define BOOTSTRAP_INTERP_XORI    18
#define BOOTSTRAP_INTERP_LSL     19
#define BOOTSTRAP_INTERP_LSLI    20
#define BOOTSTRAP_INTERP_LSR     21
#define BOOTSTRAP_INTERP_LSRI    22
#define BOOTSTRAP_INTERP_MUL     23
#define BOOTSTRAP_INTERP_MULI    24
#define BOOTSTRAP_INTERP_NEGATE  25
#define BOOTSTRAP_INTERP_REPEATEQUALS 26





typedef struct {
	char cmd;                          /* C        */
	char delim1;		           /* ','      */
	char arg0[BSTRAP_HEXFLD_LEN];      /* HHHHHHHH */
	char delim2;			   /* ','      */
	char len[BSTRAP_HEXFLD_LEN];       /* HHHHHHHH */
	char pay_delim;			   /* '(#|%)'  */
} __packed bstrap_char_req_t;

#define BSTRAP_CMD_FIXED_LEN (sizeof(bstrap_char_req_t))

#define BSTRAP_REQ_FLAG_BINARY	BIT(0)

typedef struct {
	uint8_t  cmd;
	uint8_t  flags;
	uint32_t arg0;
	uint32_t len;
	uint32_t crc;
} bootstrap_req_t;

static inline bool is_cmd(const bootstrap_req_t *req, const char cmd)
{
	return req->cmd == cmd;
}

bool bootstrap_RxReq(bootstrap_req_t *req);

int bootstrap_RxData(uint8_t *data,
		     int offset,
		     int datasize);

void bootstrap_Tx(char cmd, int32_t status,
		  uint32_t length, const uint8_t *payload);

static inline void bootstrap_TxAck(void)
{
	bootstrap_Tx(BOOTSTRAP_ACK, 0, 0, NULL);
}

static inline void bootstrap_TxAckData_arg(const void *data, uint32_t len, uint32_t arg)
{
	bootstrap_Tx(BOOTSTRAP_ACK, arg, len, data);
}

static inline void bootstrap_TxAckData(const void *data, uint32_t len)
{
	bootstrap_Tx(BOOTSTRAP_ACK, 0, len, data);
}

static inline void bootstrap_TxNack_rc(const char *str, uint32_t rc)
{
	bootstrap_Tx(BOOTSTRAP_NACK, rc, strlen(str), (const uint8_t *)str);
}

static inline void bootstrap_TxNack(const char *str)
{
	bootstrap_Tx(BOOTSTRAP_NACK, 0, strlen(str), (const uint8_t *)str);
}

bool bootstrap_RxDataCrc(bootstrap_req_t *req, uint8_t *data);



