#ifndef __BOOTLOADER_BF_H__
#define __BOOTLOADER_BF_H__

#include <stdint.h>
void bf_decrypt(uint8_t *encrypted_arr, uint8_t size);
void load_tape(unsigned char *tape, uint8_t *encrypted_arr, uint8_t size);
void save_tape(unsigned char *tape, uint8_t *encrypted_arr, uint8_t size);

#endif
