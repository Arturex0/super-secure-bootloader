#ifndef __BOOTLOADER__BUTILS_H__
#define __BOOTLOADER__BUTILS_H__
#include <stdint.h>
long program_flash(void* page_addr, unsigned char * data, unsigned int data_len);
uint16_t read_short(void);
uint32_t read_frame(uint8_t *buffer);
bool verify_checksum(uint16_t given_checksum, uint8_t data[1024]);
void uart_write_hex_bytes(uint8_t uart, uint8_t *start, uint32_t len);
#endif
