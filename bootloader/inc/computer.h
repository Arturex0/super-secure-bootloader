#ifndef __BOOTLOADER_COMPUTER_H__
#define __BOOTLOADER_COMPUTER_H__
#include <stdint.h>

typedef struct _ {
	uint8_t ip;
	uint8_t fl;
	uint8_t ra;
	uint8_t rb;
	uint8_t rc;
	uint8_t rd;
	uint8_t re;
	uint8_t rf;
	uint8_t memory[256];
} computer_state;
#endif
