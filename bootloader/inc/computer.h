#ifndef __BOOTLOADER_COMPUTER_H__
#define __BOOTLOADER_COMPUTER_H__
#include <stdint.h>

#define COMP_RA_MASK 0b00000001
#define COMP_RB_MASK 0b00000010
#define COMP_RC_MASK 0b00000100
#define COMP_RD_MASK 0b00001000
#define COMP_RE_MASK 0b00010000
#define COMP_RF_MASK 0b00100000
#define COMP_RG_MASK 0b01000000

#define COMP_ADD_CODE
#define COMP_SUB_CODE
#define COMP_IMM_CODE
#define COMP_CMP_CODE
#define COMP_STM_CODE
#define COMP_LDM_CODE
#define COMP_SYS_CODE
#define COMP_JMP_CODE
#define COMP_BNE_CODE
#define COMP_BLT_CODE

#define COMP_SYS_WRITE
#define COMP_SYS_READ
#define COMP_SYS_EXIT

typedef struct _ {
	uint8_t ip;
	uint8_t fl;
	uint8_t ra;
	uint8_t rb;
	uint8_t rc;
	uint8_t rd;
	uint8_t re;
	uint8_t rf;
	// Care must be taken to ensure this buffer is at least 256 bytes
	uint8_t *instructions;
	uint8_t memory[256];
} computer_state;

// Returns exit code after SYS exit
uint8_t interpret_program(computer_state *);



#endif
