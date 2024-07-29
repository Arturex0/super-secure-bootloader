#ifndef __BOOTLOADER_STORAGE_H__
#define __BOOTLOADER_STORAGE_H__
#include <stdint.h>
/*
 * Partition layout:
 * | Iv Metadata | Message | ...Firmware... | Signature |
 */
//first partition
#define STORAGE_PARTA 100
//second partition
#define STORAGE_PARTB 178

#define STORAGE_PART_SIZE 78
#define VAULT_MAGIC 0x05EC12E7

enum STORAGE_PART_STATUS {
	STORAGE_TRUST_NONE,
	STORAGE_TRUST_A,
	STORAGE_TRUST_B
};

typedef struct vault_struct {
	uint32_t magic;
	enum STORAGE_PART_STATUS s;
	uint32_t fw_version;
	uint32_t fw_length;
	uint32_t message_len;
} vault_struct;
#endif
