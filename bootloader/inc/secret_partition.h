#ifndef __BOATLOADER_SECRET_PARTION_H__
#define __BOATLOADER_SECRET_PARTION_H__

#define SECRETS_BLOCK 99
#define SECRETS_MAGIC_INDICATOR 0x00002137
// Ensure these are multiples of blocksize (0x40)
#define SECRETS_EEPROM_OFFSET 0x400
#define SECRETS_VAULT_OFFSET 0x3c0

void setup_secrets(void);

#endif
