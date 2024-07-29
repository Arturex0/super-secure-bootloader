#include "bootloader.h"
#include "butils.h"
#include "computer.h"
#include "bass.h"
#include "uart/uart.h"

#include "inc/hw_memmap.h"    // Peripheral Base Addresses
#include "inc/hw_types.h"     // Boolean type
#include "inc/tm4c123gh6pm.h" // Peripheral Bit Masks and Registers
							  
#define DPART_TM4C123GH6PM
#define DTARGET_IS_TM4C123_RB1
#define TARGET_IS_BLIZZARD_RB1

#include "driverlib/flash.h"
#include "driverlib/sysctl.h"

#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
/*
 * Program a stream of bytes to the flash.
 * This function takes the starting address of a 1KB page, a pointer to the
 * data to write, and the number of bytes to write.
 *
 * This functions performs an erase of the specified flash page before writing
 * the data.
 */
long program_flash(void* page_addr, unsigned char * data, unsigned int data_len) {
    uint32_t word = 0;
    int ret;
    int i;

    // Erase next FLASH page
    FlashErase((uint32_t) page_addr);

    // Clear potentially unused bytes in last word
    // If data not a multiple of 4 (word size), program up to the last word
    // Then create temporary variable to create a full last word
    if (data_len % FLASH_WRITESIZE) {
        // Get number of unused bytes
        int rem = data_len % FLASH_WRITESIZE;
        int num_full_bytes = data_len - rem;

        // Program up to the last word
        ret = FlashProgram((unsigned long *)data, (uint32_t) page_addr, num_full_bytes);
        if (ret != 0) {
            return ret;
        }

        // Create last word variable -- fill unused with 0xFF
        for (i = 0; i < rem; i++) {
            word = (word >> 8) | (data[num_full_bytes + i] << 24); // Essentially a shift register from MSB->LSB
        }
        for (i = i; i < 4; i++) {
            word = (word >> 8) | 0xFF000000;
        }

        // Program word
        return FlashProgram(&word, (uint32_t) page_addr + num_full_bytes, 4);
    } else {
        // Write full buffer of 4-byte words
        return FlashProgram((unsigned long *)data, (uint32_t) page_addr, data_len);
    }
}

//read a little endian short from serial
uint16_t read_short(void) {
	int read = 0;
	uint16_t r = 0;
	uint8_t c;
	c = uart_read(UART0, BLOCKING, &read);
	r = c;
	c = uart_read(UART0, BLOCKING, &read);
	r |= (c << 8);
	return r;
}

// Reads in at most READ_BUFFER_SIZE bytes into buffer 
// This function does not perform error checking if block size is zero
uint32_t read_frame(uint8_t * buffer) {
	
	
	int read = 0;
	
	//wait for a frame instruction
    uint32_t instruction = uart_read(UART0, BLOCKING, &read);
	while (instruction != FRAME) {
		instruction = uart_read(UART0, BLOCKING, &read);
	}

	uint16_t data_size;
	data_size = read_short();
	if (data_size == 0) {
		return data_size;
	}
	if (data_size > READ_BUFFER_SIZE) {
		uart_write_str(UART0, "Frame size too big!\n");
		SysCtlReset();
	}
	for (int i = 0; i < data_size; i++) {
		buffer[i] = uart_read(UART0, BLOCKING, &read);
	}

    uint16_t checksum = read_short();

	uart_write_str(UART0, "Looking at the checksum\n");
    if (verify_checksum(checksum, buffer, data_size)){
			uart_write_str(UART0, "Checksum did not checkout");
			SysCtlReset();

		}

		uart_write_str(UART0, "CHECKSUM CHECKED ;)");
	// Do not write acknowledge in this function, instead it is up to the caller to run logic and acknowledge the frame \
	// Only write a restart here if frame is corrupted
	
	//uart_write_str(UART0, "A");
	return data_size;

	//resend, checksum failed
	//uart_write_str(UART0, "R");
}

// Takes in proposed checksum and data returns bool of verification
bool verify_checksum(uint16_t given_checksum, uint8_t *data, uint32_t length) {

	uint16_t checksum = ROM_Crc16(0, data, length); 

	if (checksum == given_checksum) {

		return false; // returns false which means verified
	}
	return true;
}

void uart_write_hex_bytes(uint8_t uart, uint8_t * start, uint32_t len) {
    for (uint8_t * cursor = start; cursor < (start + len); cursor += 1) {
        uint8_t data = *((uint8_t *)cursor);
        uint8_t right_nibble = data & 0xF;
        uint8_t left_nibble = (data >> 4) & 0xF;
        char byte_str[3];
        if (right_nibble > 9) {
            right_nibble += 0x37;
        } else {
            right_nibble += 0x30;
        }
        byte_str[1] = right_nibble;
        if (left_nibble > 9) {
            left_nibble += 0x37;
        } else {
            left_nibble += 0x30;
        }
        byte_str[0] = left_nibble;
        byte_str[2] = '\0';

        uart_write_str(uart, byte_str);
        uart_write_str(uart, " ");
    }
}

void stupid_computer(void) {
	uart_write_str(UART0, "Running Dumb Bass program\n");
	uint8_t stuff[] = {43, 65, 15, 12, 78, 5, 20, 12, 0, 65, 12, 0, 29, 18, 0x00};
	computer_state state = {0};
	state.instructions = (computer_instruction *) instructions;
	state.sys_write_buffer = stuff;
	state.sys_read_buffer = stuff;
	state.sys_read_remaining = (sizeof(stuff) - 1);
	state.sys_write_remaining = (sizeof(stuff) - 1);
	computer_interpret_program(&state);

	uart_write_str(UART0, "Finishing dumb bass program\n");
	uart_write_str(UART0, stuff);
	uart_write_str(UART0, state.memory);
	while (1) {
	};
}

