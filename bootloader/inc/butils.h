#ifndef __BOOTLOADER__BUTILS_H__
#define __BOOTLOADER__BUTILS_H__
#include <stdint.h>
long program_flash(void* page_addr, unsigned char * data, unsigned int data_len);
uint16_t read_short(void);
uint32_t read_frame(uint8_t *buffer);
#endif
