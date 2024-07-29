#ifndef __BOOTLOADER_COMPUTER_H__
#define __BOOTLOADER_COMPUTER_H__
#include <stdint.h>
#include <stdbool.h>

//#define COMP_RA_MASK 0b00000001
#define COMP_RA_MASK 0x01
//#define COMP_RB_MASK 0b00000010
#define COMP_RB_MASK 0x02
//#define COMP_RC_MASK 0b00000100
#define COMP_RC_MASK 0x04
//#define COMP_RD_MASK 0b00001000
#define COMP_RD_MASK 0x08
//#define COMP_RE_MASK 0b00010000
#define COMP_RE_MASK 0x10
//#define COMP_RF_MASK 0b00100000
#define COMP_RF_MASK 0x20

//hex doesn't count up like this but whatever
#define COMP_MOV_CODE 0x39

#define COMP_ADD_CODE 0x40
#define COMP_SUB_CODE 0x41
#define COMP_IMM_CODE 0x42
#define COMP_CMP_CODE 0x43
#define COMP_STM_CODE 0x44
#define COMP_LDM_CODE 0x45
#define COMP_SYS_CODE 0x46

#define COMP_JMP_CODE 0x47
#define COMP_JNE_CODE 0x48
#define COMP_JLT_CODE 0x49

#define COMP_AND_CODE 0x50
#define COMP_NOT_CODE 0x51
#define COMP_ORR_CODE 0x52
#define COMP_XOR_CODE 0x53

#define COMP_SYS_WRITE	0x10
#define COMP_SYS_READ	0x20
#define COMP_SYS_EXIT	0x30

#define COMP_FLAG_SIGN_SHIFT 0
#define COMP_FLAG_CARRY_SHIFT 1
#define COMP_FLAG_ZERO_SHIFT 2
#define COMP_FLAG_OVERFLOW_SHIFT 3

typedef struct _ins {
	uint8_t opcode;
	uint8_t a;
	uint8_t b;
} computer_instruction;

typedef struct _comp {
	uint8_t ip;
	uint8_t fl;
	uint8_t ra;
	uint8_t rb;
	uint8_t rc;
	uint8_t rd;
	uint8_t re;
	uint8_t rf;
	// Care must be taken to ensure this buffer is 256 instructions
	// If we make the ip refer to instruction rather than offset we can fit 256 instructions
	// versus 256 // 3 = 85 instructions
	computer_instruction *instructions;
	uint8_t *sys_write_buffer;
	uint8_t *sys_read_buffer;
	uint32_t sys_read_remaining;
	uint32_t sys_write_remaining;
	uint8_t memory[256];
} computer_state;

// Returns exit code after SYS exit
uint8_t computer_interpret_program(computer_state *);
uint8_t computer_read_reg(computer_state* state, uint8_t reg_num);
void computer_write_reg(computer_state* state, uint8_t reg_num, uint8_t value);

void computer_badins(void);
void computer_mov(computer_state *state, uint8_t a, uint8_t b);
void computer_add(computer_state *state, uint8_t a, uint8_t b);
void computer_sub(computer_state *state, uint8_t a, uint8_t b);
void computer_imm(computer_state *state, uint8_t a, uint8_t b);
void computer_cmp(computer_state *state, uint8_t a, uint8_t b);
void computer_stm(computer_state *state, uint8_t a, uint8_t b);
void computer_ldm(computer_state *state, uint8_t a, uint8_t b);
bool computer_sys(computer_state *state, uint8_t a, uint8_t b);
void computer_jmp(computer_state *state, uint8_t a, uint8_t b);
void computer_jne(computer_state *state, uint8_t a, uint8_t b);
void computer_jlt(computer_state *state, uint8_t a, uint8_t b);
void computer_and(computer_state *state, uint8_t a, uint8_t b);
void computer_not(computer_state *state, uint8_t a, uint8_t b);
void computer_orr(computer_state *state, uint8_t a, uint8_t b);
void computer_xor(computer_state *state, uint8_t a, uint8_t b);

#endif
