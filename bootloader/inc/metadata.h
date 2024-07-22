#ifndef __BOOTLOADER_METADATA_H__
#define __BOOTLOADER_METADATA_H__
#include <stdint.h>
typedef struct metadata {
	uint32_t fw_version;
	uint32_t fw_length;
	uint32_t message_length;
	uint32_t unused;
} metadata;
#endif
