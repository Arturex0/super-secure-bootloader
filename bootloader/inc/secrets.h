#ifndef __BOOTLOADER_SECRETS_H__
#define __BOOTLOADER_SECRETS_H__

#include <stdint.h>
// proof of concept code, we don't need to use 16 byte keys
// struct size should be multiple of 4 bytes to play nice with EEPROM api functions
#define SECRETS_DECRYPT_KEY_LEN 16
#define SECRETS_HMAC_KEY_LEN 16
#define SECRETS_HASH_LENGTH 32
#define SECRETS_SIGNATURE_LENGTH 64
#define SECRETS_IV_LEN 16
#define SECRETS_ENCRYPTION_BLOCK_LENGTH 16
typedef struct _ {
	uint8_t decrypt_key[SECRETS_DECRYPT_KEY_LEN];
	uint8_t hmac_key[SECRETS_HMAC_KEY_LEN];
} secrets_struct;

#endif
