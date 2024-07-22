#ifndef __BOOTLOADER_STORAGE_H__
#define __BOOTLOADER_STORAGE_H__
//first partition
#define STORAGE_PARTA 56
//second partition
#define STORAGE_PARTB 156

#define STORAGE_PART_SIZE 100
#define VAULT_MAGIC 0x05EC12E7
#define VAULT_BLOCK 54

enum STORAGE_PART_STATUS {
	STORAGE_TRUST_NONE,
	STORAGE_TRUST_A,
	STORAGE_TRUST_B
};

typedef struct vault_struct {
	enum STORAGE_PART_STATUS s;
} vault_struct;
#endif
