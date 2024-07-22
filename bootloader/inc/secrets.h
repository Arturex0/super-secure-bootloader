#ifndef __BOOTLOADER_SECRETS_H__
#define __BOOTLOADER_SECRETS_H__

#include <stdint.h>
//proof of concept code, we don't need to use 16 byte keys
// struct size should be multiple of 4 bytes to play nice with EEPROM api functions
typedef struct _ {
	uint8_t vault_key[16];
	uint8_t decrypt_key[16];
	uint8_t hmac_key[16];
} secrets_struct;

#define SECRETS_VAULT_KEY_LEN 16
#define SECRETS_DECRYPT_KEY_LEN 16
#define SECRETS_HMAC_KEY_LEN 16
#define SECRETS_HASH_LENGTH 32
#endif
