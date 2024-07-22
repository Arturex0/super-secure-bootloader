#include "bootloader.h"
#include "butils.h"
#include "uart/uart.h"

#include "inc/hw_memmap.h"    // Peripheral Base Addresses
#include "inc/hw_types.h"     // Boolean type
#include "inc/tm4c123gh6pm.h" // Peripheral Bit Masks and Registers

#include "driverlib/flash.h"
#include "driverlib/sysctl.h"

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

// Reads in at most BUFFER_SIZE bytes into buffer 
// This function does not perform error checking if block size is zero
uint32_t read_frame(uint8_t * buffer) {
	//TODO: Comute checksum and verify (Wait for acknowledge)
	
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
	if (data_size > BUFFER_SIZE) {
		uart_write_str(UART0, "Frame size too big!\n");
		SysCtlReset();
	}
	for (int i = 0; i < data_size; i++) {
		buffer[i] = uart_read(UART0, BLOCKING, &read);
	}

	uart_write_str(UART0, "A");
	return data_size;

	//resend, checksum failed
	//uart_write_str(UART0, "R");
}

