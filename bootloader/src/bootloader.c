#include "bootloader.h"
#include "secret_partition.h"
#include "secrets.h"
#include "metadata.h"
#include "storage.h"
#include "butils.h"
#include "user_settings.h"

// Hardware Imports
#include "inc/hw_memmap.h"    // Peripheral Base Addresses
#include "inc/hw_types.h"     // Boolean type
#include "inc/tm4c123gh6pm.h" // Peripheral Bit Masks and Registers
// #include "inc/hw_ints.h" // Interrupt numbers

// Driver API Imports
#include "driverlib/flash.h"     // FLASH API
#include "driverlib/interrupt.h" // Interrupt API
#include "driverlib/sysctl.h"    // System control API (clock/reset)

#include "driverlib/eeprom.h"	 // EEPROM API

// Application Imports
#include "driverlib/gpio.h"
#include "uart/uart.h"

// Cryptography Imports
#include "wolfssl/wolfcrypt/settings.h"
#include "wolfssl/wolfcrypt/aes.h"
#include "wolfssl/wolfcrypt/sha.h"
#include "wolfssl/wolfcrypt/rsa.h"
#include "wolfssl/wolfcrypt/hmac.h"

// Forward Declarations
void load_firmware(void);
void update_firmware(void);
void boot_firmware(void);
void uart_write_hex_bytes(uint8_t, uint8_t *, uint32_t);
bool verify_hmac(uint8_t * data, uint32_t data_len, uint8_t * key, uint8_t * test_hash);
void setup_vault(void);


//crypto state

// FLOW CHART: Initialize import state
vault_struct *vault_addr = (vault_struct *) (VAULT_BLOCK << 10);
secrets_struct secrets;
uint8_t ct_buffer[READ_BUFFER_SIZE];
uint8_t pt_buffer[READ_BUFFER_SIZE];

// FLOW CHART: Allocate space for IV + encrypted data + decrypted data
uint8_t iv[SECRETS_IV_LEN];

int main(void) {
	uint32_t eeprom_status;

    // Enable the GPIO port that is used for the on-board LED.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Check if the peripheral access is enabled.
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {
    }

    // Enable the GPIO pin for the LED (PF3).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);

    initialize_uarts(UART0);


	SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
	while (!SysCtlPeripheralReady(SYSCTL_PERIPH_EEPROM0)) {
	}

	//make sure EEPROM is working
	eeprom_status = EEPROMInit();
	if (eeprom_status == EEPROM_INIT_ERROR) {
		uart_write_str(UART0, "Fatal EEPROM error\n");
		SysCtlReset();
	}

	uart_write_str(UART0, "boot\n");
	setup_secrets();

	uart_write_str(UART0, "Found no secrets in secret block!, retrieving secrets\n");

	// Funny crypto shenanigans
	EEPROMRead((uint32_t *) &secrets, SECRETS_EEPROM_OFFSET, sizeof(secrets));
	

    uart_write_str(UART0, "Welcome to the BWSI Vehicle Update Service!\n");
    uart_write_str(UART0, "Send \"U\" to update, and \"B\" to run the firmware.\n");

    int resp;
    while (1) {
        uint32_t instruction = uart_read(UART0, BLOCKING, &resp);

        if (instruction == UPDATE) {
			update_firmware();

        } else if (instruction == BOOT) {
			metadata_blob *mb;
            uart_write_str(UART0, "B");
            uart_write_str(UART0, "Booting firmware...\n");

			if (vault_addr->magic == VAULT_MAGIC) {
				uart_write_str(UART0, "No corrupted vault :D\n");
			}

			uint8_t * m_addr;
			switch (vault_addr->s) {
				case STORAGE_TRUST_A:
					mb = (metadata_blob *) ((STORAGE_PARTA << 10) + FLASH_PAGESIZE - sizeof(metadata_blob));
					uart_write_str(UART0, "I'm gonna boot from A :D\n");

					// Write the message
					m_addr = (uint8_t *) ((STORAGE_PARTA + 1) << 10);
					for (uint32_t i = 0; i < mb->metadata.message_length; i++) {
						uart_write(UART0, m_addr[i]);
					}

					SysCtlDelay(700000);
					__asm("LDR R0,=0xe801\n\t"
						  "BX R0\n\t");

					break;
				case STORAGE_TRUST_B:
					mb = (metadata_blob *) ((STORAGE_PARTB << 10) + FLASH_PAGESIZE - sizeof(metadata_blob));
					uart_write_str(UART0, "I'm gonna boot from B :D\n");

					// Write the message
					m_addr = (uint8_t *) ((STORAGE_PARTB + 1) << 10);
					for (uint32_t i = 0; i < mb->metadata.message_length; i++) {
						uart_write(UART0, m_addr[i]);
					}

					SysCtlDelay(700000);
					__asm("LDR R0,=0x27801\n\t"
						  "BX R0\n\t");

					break;

				case STORAGE_TRUST_NONE:
					uart_write_str(UART0, "Sorry, my mind is blank\n");
					SysCtlReset();
			}
			while (1) {
				//be lazy
			}
        }
    }
}

 /*
 * Load the firmware into flash.
 */
void load_firmware(void) {
    int read = 0;
    uint32_t rcv = 0;

    uint32_t version = 0;
    uint32_t size = 0;

    // Get version.
    rcv = uart_read(UART0, BLOCKING, &read);
    version = (uint32_t)rcv;
    rcv = uart_read(UART0, BLOCKING, &read);
    version |= (uint32_t)rcv << 8;

    // Get size.
    rcv = uart_read(UART0, BLOCKING, &read);
    size = (uint32_t)rcv;
    rcv = uart_read(UART0, BLOCKING, &read);
    size |= (uint32_t)rcv << 8;

    // Compare to old version and abort if older (note special case for version 0).
    // If no metadata available (0xFFFF), accept version 1
}
void update_firmware(void) {

	// ========== Funny metadata shenanigans =========
	uint32_t size; 						// frame size read in
	uint32_t old_version; 				// version of current firmware
	uint32_t start_block = 300; 		// current block to write into flash (initialize to write to invalid area)
	uint32_t flash_block_offset = 0; 	// blocks that have been written to flash (make sure to always update this if you increment write_block)

										// pointers to newly received metadata block and old metadata blocks
	metadata_blob *old_mb;
	metadata_blob *new_mb;
	enum STORAGE_PART_STATUS \
		new_permissions = \
		STORAGE_TRUST_NONE;  			// partition to change trust to

	uint32_t addr; 						// for calculating flash offsets

	bool passed; 						// did the metadata pass hmac
	bool ending = false; 				// did we receive a < BUFFER_LENGTH size when reading in firmware?

										// Useful crypto stuff
	Hmac hmac;
	Aes aes;

	uart_write_str(UART0, "U");

	// setup hmac
	if (wc_HmacSetKey(&hmac, WC_SHA256, secrets.hmac_key, sizeof(secrets.hmac_key)) != 0) {
		uart_write_str(UART0, "FATAL hmac key error\n");
		SysCtlReset();
	}

	// FLOW CHART: Read in IV + metadata chunk into memory
	size = read_frame(ct_buffer);
	// New metadata blob points into plaintext buffer
	new_mb = (metadata_blob *) &pt_buffer;
	if (size != sizeof(metadata_blob)) {
		uart_write_str(UART0, "You did not give me metadata and now I am angry\n");
		SysCtlReset();
	}

	// save iv in global (useful) and plaintext
	memcpy(&iv, ct_buffer, sizeof(new_mb->iv));
	memcpy(pt_buffer, ct_buffer, sizeof(new_mb->iv));

	// setup decryption
	if (wc_AesInit(&aes, NULL, INVALID_DEVID)) {
		uart_write_str(UART0, "FATAL cipher error\n");
		SysCtlReset();
	}
	// Direction for some modes (CFB and CTR) is always AES_ENCRYPTION.
	if (wc_AesSetKey(&aes, secrets.decrypt_key, sizeof(secrets.decrypt_key), iv, AES_ENCRYPTION)){
		uart_write_str(UART0, "second FATAL cipher error\n");
		SysCtlReset();
	}
	// copy in the rest of the unencrypted firmware blob into pt
	if (wc_AesCtrEncrypt(&aes, pt_buffer + sizeof(new_mb->iv), ct_buffer + sizeof(new_mb->iv), sizeof(metadata_blob) - sizeof(new_mb->iv))) {
		uart_write_str(UART0, "Idk how to do the funny unencryption thing /shrug\n");
		SysCtlReset();
	}
	// FLOW CHART: update hash function w/encrypted metadata block, verify meta data signature

	wc_HmacUpdate(&hmac, ct_buffer + sizeof(new_mb->iv), sizeof(metadata_blob) - sizeof(new_mb->iv));
	//wc_HmacUpdate(&hmac, (uint8_t *) &new_mb->metadata, sizeof(new_mb->metadata));
	//wc_HmacUpdate(&hmac, (uint8_t *) &new_mb->hmac, sizeof(new_mb->hmac));

	passed = verify_hmac((uint8_t *) &new_mb->metadata, sizeof(new_mb->metadata), secrets.hmac_key, (uint8_t *) &new_mb->hmac);
	// FLOW CHART: metadata signature good?
	if (!passed) {
		uart_write_str(UART0, "HMAC signature does not match :bangbang:\n");
		SysCtlReset();
	}
	uart_write_str(UART0, "you're did it\n");

	switch (vault_addr->s) {
		// Write to partition B, read old metadata from partition A
		case STORAGE_TRUST_A:
			start_block = STORAGE_PARTB;
			new_permissions = STORAGE_TRUST_B;
			uart_write_str(UART0, "Trust a\n");

			old_mb = (metadata_blob *) ((STORAGE_PARTA << 10) + FLASH_PAGESIZE - sizeof(metadata_blob));
			old_version = old_mb->metadata.fw_version;
			break;
		// Write to partition A, read old metadata from partition B
		case STORAGE_TRUST_B:
			start_block = STORAGE_PARTA;
			new_permissions = STORAGE_TRUST_A;
			uart_write_str(UART0, "Trust b\n");

			old_mb = (metadata_blob *) ((STORAGE_PARTB << 10) + FLASH_PAGESIZE - sizeof(metadata_blob));
			old_version = old_mb->metadata.fw_version;
			break;
		// By default write to A, firmware version is always 1	
		case STORAGE_TRUST_NONE:
			start_block = STORAGE_PARTA;
			new_permissions = STORAGE_TRUST_A;
			old_version = 1;
			uart_write_str(UART0, "Trust no one\n");
	}
	// Not needed afterwards so throw it away
	old_mb = NULL;

	// Handle debug case (if zero just set it to old version)
	if (new_mb->metadata.fw_version == 0) {
		new_mb->metadata.fw_version = old_version;
	}

	// FLOW CHART: Metadata version good?
	if (new_mb->metadata.fw_version < old_version) {
		uart_write_str(UART0, "It is evolving, just backwards\n");
		SysCtlReset();
	}

	// Flash the funny metadata into memory

	// This is to check that metadata_blob is multiple of 4 bytes which it should be unless I screwed up badly
	if (sizeof(metadata_blob) % 4) {
		uart_write_str(UART0, "oops messed up struct alignment\n");
		SysCtlReset();
	}

	if (flash_block_offset >= STORAGE_PART_SIZE) {
		uart_write_str(UART0, "no storage :<\n");
		SysCtlReset();
	}

	addr = (start_block + flash_block_offset) << 10;
	if (FlashErase(addr)) {
		uart_write_str(UART0, "couldn't erase flash :sob:\n");
		SysCtlReset();
	}
	addr = addr + FLASH_PAGESIZE - sizeof(metadata_blob);
	if (FlashProgram((uint32_t *) new_mb, addr, sizeof(metadata_blob))) {

		uart_write_str(UART0, "couldn't write metadata :skull:\n");
		SysCtlReset();
	}
	flash_block_offset++;

	// Acknowledge this frame because it is legitimate, prepare for another frame
	uart_write_str(UART0, "A");
	// New_mb is no longer needed
	new_mb = NULL;

	// ========== FIRMWARE ==========

	// Start reading in message data + other data
	while (true) {

		size = read_frame(ct_buffer);

		// FLOW CHART: Size = 0?
		if (size == 0) {
			break;
		}

		// When a partial block is sent data should stop being read
		if (ending) {
			uart_write_str(UART0, "do not write non signature data after a partial block\n");
			SysCtlReset();
		}

		if (size != READ_BUFFER_SIZE) {
			ending = true;
		}

		// is this size a multiple of AES/other function block size (16)?
		// ensuring this just makes decryption easier :D
		if (size % SECRETS_ENCRYPTION_BLOCK_LENGTH) {
			uart_write_str(UART0, "partial block received, can't decrypt\n");
			SysCtlReset();
		}

		// FLOW CHART: is flash_block_offset < 99?
		if (flash_block_offset >= STORAGE_PART_SIZE - 1) {
			uart_write_str(UART0, "We not beaver balling\n");
			SysCtlReset();
		}
		addr = (start_block + flash_block_offset) << 10;
		flash_block_offset++;

		//decrypt block
		if (wc_AesCtrEncrypt(&aes, pt_buffer, ct_buffer, size)) {
			uart_write_str(UART0, "idk how to do the crypto stuff all the cool kids are talking about\n");
			SysCtlReset();
		}

		// Write to flash
		program_flash((void *) addr, pt_buffer, size);

		// Update hash function (this is the cipher text, not the plaintext)
		if (wc_HmacUpdate(&hmac, (uint8_t *) ct_buffer, size)) {
			uart_write_str(UART0, "Crypto oopsie\n");
			SysCtlReset();

		}

		// Acknowledge this block
		uart_write_str(UART0, "A");
	}
	// This shouldn't happen but added for redundency
	if (flash_block_offset >= STORAGE_PART_SIZE) {
		uart_write_str(UART0, "No space for signature :(");
		SysCtlReset();
	}
	//handle signature :D
	int read = 0;
	for (int i = 0; i < SECRETS_HASH_LENGTH; i++) {
		ct_buffer[i] = uart_read(UART0, BLOCKING, &read);
	}
	if (wc_HmacFinal(&hmac, &ct_buffer[SECRETS_HASH_LENGTH]) != 0) {
		uart_write_str(UART0, "Couldn't compute hash");
		SysCtlReset();
	}
	passed = true;
	for (int i = 0; i < SECRETS_HASH_LENGTH; i++) {
		if (ct_buffer[i] != ct_buffer[SECRETS_HASH_LENGTH + i]) {
			passed = false;
		}
	}
	if (passed) {
		uart_write_str(UART0, "omg you are pro gamer!!!!\n");
	} else {
		uart_write_str(UART0, "smh so bad at math can't even calculate a signature\n");
		SysCtlReset();
	}
	addr = (start_block + flash_block_offset) << 10;
	program_flash((void *) addr, ct_buffer, SECRETS_HASH_LENGTH);

	vault_struct new_vault = {
		VAULT_MAGIC,
		new_permissions
	};

	// This is to check that metadata_blob is multiple of 4 bytes which it should be unless I screwed up badly
	if (sizeof(new_vault) % 4) {
		uart_write_str(UART0, "oops messed up struct alignment\n");
		SysCtlReset();
	}

	if (program_flash((void *) (VAULT_BLOCK << 10), (uint8_t *) &new_vault, sizeof(new_vault))) {
		uart_write_str(UART0, "Can't program flash :thonk:\n");
	}

	uart_write_str(UART0, "A");
	// can shorten this but it needs to not be so short that the string isn't written
	SysCtlDelay(700000);
	SysCtlReset();
}


// Implement this in the future
void boot_firmware(void) {
	uart_write_str(UART0, "oopsie I forgot how to run code :(\n");
	while (1) {
	}
    __asm("LDR R0,=0x10001\n\t"
          "BX R0\n\t");
}

// verifies an hmac, given the data, key and hash to test against, returns boolean True if verification correct
bool verify_hmac(uint8_t * data, uint32_t data_len, uint8_t * key, uint8_t * test_hash){
    Hmac hmac;

    if (wc_HmacSetKey(&hmac, WC_SHA256, key, SECRETS_HMAC_KEY_LEN) != 0) {
        uart_write_str(UART0, "Couldn't init HMAC");
        SysCtlReset();
}

    if( wc_HmacUpdate(&hmac, data, data_len) != 0) {
    uart_write_str(UART0, "Couldn't init HMAC");
    SysCtlReset();
}

    uint8_t hash[SECRETS_HASH_LENGTH]; // 256/8 = 32
    if (wc_HmacFinal(&hmac, hash) != 0) {
        uart_write_str(UART0, "Couldn't compute hash");
        SysCtlReset();
	}
	bool ret = true;
	for (uint32_t i = 0; i < SECRETS_HASH_LENGTH; i++) {
		if (test_hash[i] != hash[i]) {
			ret = false;
		}
	}
	return ret;
}

