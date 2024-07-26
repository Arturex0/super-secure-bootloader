#include "bootloader.h"

// Hardware Imports
#include "inc/hw_memmap.h"    // Peripheral Base Addresses
#include "inc/hw_types.h"     // Boolean type
#include "inc/tm4c123gh6pm.h" // Peripheral Bit Masks and Registers

// Driver API Imports
#include "driverlib/sysctl.h"    // System control API (clock/reset)
#include "driverlib/uart.h"

// Application Imports
#include "uart/uart.h"


#include "computer.h"
#include <stdbool.h>
/*
 * 8-bit Harvard architecture, 6 general purpose registers, 
 *
 */


uint8_t interpret_program(computer_state* state) {
	uint8_t opcode;
	uint8_t a;
	uint8_t b;
	computer_instruction ins;
	bool end = false;
	bool increment_ip; 
	while (!end) {
		increment_ip = true;
		ins = state->instructions[state->ip];
		switch (ins.opcode) {
			case COMP_ADD_CODE:
				computer_add(state, ins.a, ins.b);
				break;
			case COMP_SUB_CODE:
				computer_sub(state, ins.a, ins.b);
				break;
			case COMP_IMM_CODE:
				computer_imm(state, ins.a, ins.b);
				break;
			case COMP_CMP_CODE:
				computer_cmp(state, ins.a, ins.b);
				break;
			case COMP_STM_CODE:
				computer_stm(state, ins.a, ins.b);
				break;
			case COMP_LDM_CODE:
				computer_ldm(state, ins.a, ins.b);
				break;
			case COMP_SYS_CODE:
				end = computer_sys(state, ins.a, ins.b);
				break;

			case COMP_JMP_CODE:
				computer_jmp(state, ins.a, ins.b);
				increment_ip = false;
				break;
			case COMP_JNE_CODE:
				computer_jne(state, ins.a, ins.b);
				increment_ip = false;
				break;
			case COMP_JLT_CODE:
				computer_add(state, ins.a, ins.b);
				increment_ip = false;
				break;

			case COMP_AND_CODE:
				computer_and(state, ins.a, ins.b);
				break;
			case COMP_NOT_CODE:
				computer_not(state, ins.a, ins.b);
				break;
			case COMP_ORR_CODE:
				computer_orr(state, ins.a, ins.b);
				break;
			case COMP_XOR_CODE:
				computer_add(state, ins.a, ins.b);
				break;
			default:
				computer_badins();

		}
		if (increment_ip) {
			state->ip += 1;
		}
	}
	return 0;
}

void computer_badins(void) {
	uart_write_str(UART0, "Bad/unimplemented instruction\n");
	while(UARTBusy(UART0_BASE)){}
	SysCtlReset();
}

// Ra <- Ra + Rb
void computer_add(computer_state* state, uint8_t a, uint8_t b) {
	computer_write_reg(state, a, (computer_read_reg(state, a) + computer_read_reg(state, b)));
}

// Ra <- Ra - Rb
void computer_sub(computer_state* state, uint8_t a, uint8_t b) {
	computer_write_reg(state, a, (computer_read_reg(state, a) - computer_read_reg(state, b)));
}

// Ra <- b
void computer_imm(computer_state* state, uint8_t a, uint8_t b) {
	computer_write_reg(state, a, b);
}

//TODO: idk if this works lol
void computer_cmp(computer_state* state, uint8_t a, uint8_t b) {
	uint8_t ra = computer_read_reg(state, a);
	uint8_t rb = computer_read_reg(state, b);

	// negate rb
	uint8_t rbn = ((!computer_read_reg(state, b)) + 1);

	uint8_t signa = ((ra >> 7) & 1);
	uint8_t signb = ((rb >> 7) & 1);

	uint16_t result = (uint16_t) ra + (uint16_t) rbn;
	uint8_t signr = (result >> 7) & 1;
	uint8_t carry = (result >> 8) & 1;
	uint8_t zero = result == 0;
	uint8_t overflow = (!(signa ^ signb)) & (signr ^ signa);

	state->fl = signr << COMP_FLAG_SIGN_SHIFT | \
				carry << COMP_FLAG_CARRY_SHIFT | \
				zero << COMP_FLAG_ZERO_SHIFT | \
				overflow << COMP_FLAG_OVERFLOW_SHIFT;
}


// *Ma = Rb
void computer_stm(computer_state* state, uint8_t a, uint8_t b) {
	state->memory[a] = computer_read_reg(state, b);
}

// Ra = *Mb
void computer_ldm(computer_state* state, uint8_t a, uint8_t b) {
	computer_write_reg(state, a, state->memory[b]);
}

//TODO: fixme
bool computer_sys(computer_state* state, uint8_t a, uint8_t b) {
	computer_badins();
	return false;
}

// b is unused, ip = a
void computer_jmp(computer_state* state, uint8_t a, uint8_t b) {
	state->ip = a;
}

// b is unused, ip = a if zero is set
void computer_jne(computer_state* state, uint8_t a, uint8_t b) {
	state->ip+=1;
	if ((state->fl >> COMP_FLAG_ZERO_SHIFT) & 1) {
		state->ip = a;
	}
	
}

// sf <> of
void computer_jls(computer_state* state, uint8_t a, uint8_t b) {
	state->ip+=1;
	if (((state->fl >> COMP_FLAG_SIGN_SHIFT) & 1) ^ \
		((state->fl >> COMP_FLAG_OVERFLOW_SHIFT) & 1)) {
		state->ip = a;
	}
}

// Ra <- Ra & Rb
void computer_and(computer_state* state, uint8_t a, uint8_t b) {
	computer_write_reg(state, a, (computer_read_reg(state, a) & computer_read_reg(state, b)));
}

// Ra <- !Rb
void computer_not(computer_state* state, uint8_t a, uint8_t b) {
	computer_write_reg(state, a, !computer_read_reg(state, b));
}

// Ra <- Ra | Rb
void computer_orr(computer_state* state, uint8_t a, uint8_t b) {
	computer_write_reg(state, a, (computer_read_reg(state, a) | computer_read_reg(state, b)));
}

void computer_xor(computer_state* state, uint8_t a, uint8_t b) {
	computer_write_reg(state, a, (computer_read_reg(state, a) ^ computer_read_reg(state, b)));
}

uint8_t computer_read_reg(computer_state* state, uint8_t reg_num) {
	switch (reg_num) {
		case COMP_RA_MASK:
			return state->ra;
			break;
		case COMP_RB_MASK:
			return state->rb;
			break;
		case COMP_RC_MASK:
			return state->rc;
			break;
		case COMP_RD_MASK:
			return state->rd;
			break;
		case COMP_RE_MASK:
			return state->re;
			break;
		case COMP_RF_MASK:
			return state->rf;
			break;
		default:
			uart_write_str(UART0, "Invalid register number\n");
			while(UARTBusy(UART0_BASE)){}
			SysCtlReset();
	}
	return 0;

}

void computer_write_reg(computer_state* state, uint8_t reg_num, uint8_t value) {
	switch (reg_num) {
		case COMP_RA_MASK:
			state->ra = value;
			break;
		case COMP_RB_MASK:
			state->rb = value;
			break;
		case COMP_RC_MASK:
			state->rc = value;
			break;
		case COMP_RD_MASK:
			state->rd = value;
			break;
		case COMP_RE_MASK:
			state->re = value;
			break;
		case COMP_RF_MASK:
			state->rf = value;
			break;
		default:
			uart_write_str(UART0, "Invalid register number\n");
			while(UARTBusy(UART0_BASE)){}
			SysCtlReset();
	}
	return;
}
