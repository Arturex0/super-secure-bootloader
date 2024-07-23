#ifndef __BOOTLOADER_METADATA_H__
#define __BOOTLOADER_METADATA_H__
#include <stdint.h>
#include <secrets.h>
typedef struct metadata {
	uint32_t fw_version;
	uint32_t fw_length;
	uint32_t message_length;
	uint32_t unused;
} metadata;
typedef struct metadata_blob {
	uint8_t iv[SECRETS_IV_LEN];
	metadata metadata;
	uint8_t hmac[SECRETS_HASH_LENGTH];
} metadata_blob;
#endif
