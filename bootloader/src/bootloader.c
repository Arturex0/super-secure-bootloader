#include "bootloader.h"
#include "secret_partition.h"
#include "secrets.h"
#include "metadata.h"
#include "storage.h"
#include "butils.h"
#include "user_settings.h"
#include "public.h"

// Hardware Imports
#include "inc/hw_memmap.h"    // Peripheral Base Addresses
#include "inc/hw_types.h"     // Boolean type
#include "inc/tm4c123gh6pm.h" // Peripheral Bit Masks and Registers
// #include "inc/hw_ints.h" // Interrupt numbers

// Driver API Imports
#include "driverlib/flash.h"     // FLASH API
#include "inc/hw_flash.h"
#include "driverlib/interrupt.h" // Interrupt API
#include "driverlib/sysctl.h"    // System control API (clock/reset)

#include "driverlib/eeprom.h"	 // EEPROM API

#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/uart.h"

// Application Imports
#include "uart/uart.h"

// Cryptography Imports
#include "wolfssl/wolfcrypt/settings.h"
#include "wolfssl/wolfcrypt/aes.h"
#include "wolfssl/wolfcrypt/sha.h"
#include "wolfssl/wolfcrypt/hmac.h"

#include "wolfssl/wolfcrypt/ecc.h"

// Forward Declarations
void update_firmware(void);
void boot_firmware(void);
void uart_write_hex_bytes(uint8_t, uint8_t *, uint32_t);
bool verify_hmac(uint8_t * data, uint32_t data_len, uint8_t * key, uint8_t * test_hash);
void copy_fw_to_ram(uint32_t *fw_ptr, uint32_t *sram_ptr, uint32_t fw_size, Aes *cipher);
void jump_to_fw(uint32_t sram_start, uint32_t sram_end);

typedef void (*pFunction)(void);

#undef SCREW_OVER_MY_BOARD

//crypto state

uint8_t message[READ_BUFFER_SIZE];

// FLOW CHART: Initialize import state


int main(void) {

	// Initialze the serail port
    initialize_uarts();

#ifdef SCREW_OVER_MY_BOARD
	if ((HWREG(0x400FE1D0) & 0x00000003) != 0) {

	// 	HWREG(0x400FD000) = 0x75100000;
   	// 	HWREG(0x400FD004) = HWREG(0x400FE1D0) & 0x7FFFFFFC;
    // 	HWREG(0x400FD008) = 0xA4420000 |  0x00000008;

	}
#endif

	uint32_t eeprom_status;

	// Enable EEPROM
	SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);

	// While true if EEPROM is not ready
	while (!SysCtlPeripheralReady(SYSCTL_PERIPH_EEPROM0)) {
	}

	//make sure EEPROM is working
	eeprom_status = EEPROMInit();
	if (eeprom_status == EEPROM_INIT_ERROR) {
		uart_write_str(UART0, "Fatal EEPROM error\n");
		//Reboot
		SysCtlReset();
	}

	uart_write_str(UART0, "boot\n");

	setup_secrets();

	uart_write_str(UART0, "Found no secrets in secret block!, retrieving secrets\n");

	// TODO: Should only read vault decryption keays. Other keys are not needed rn 

    uart_write_str(UART0, "Welcome to the BWSI Vehicle Update Service!\n");
	// TODO: Boot fw, if we dont recieve an update request in 500ms 
    uart_write_str(UART0, "Press B to run the firmware.\n");

	// Not needed, but uart requres this var. Indicates that a response has beed read. In blocking function always true. 
    int resp;

    while (1) {
        uint32_t instruction = uart_read(UART0, BLOCKING, &resp);

        if (instruction == UPDATE) {
			update_firmware();

        } else if (instruction == BOOT) {
			boot_firmware();
        }
    }
}

void update_firmware(void) {

	secrets_struct secrets;
	uint8_t ct_buffer[READ_BUFFER_SIZE];
	uint8_t pt_buffer[READ_BUFFER_SIZE];

	// FLOW CHART: Allocate space for IV + encrypted data + decrypted data
	uint8_t iv[SECRETS_IV_LEN];

	// EEPROM/flash results
	int result = 0;
	uint32_t size; 						// frame size read in

	uint32_t old_version;			 	// version of current firmware

	uint32_t start_block = 300; 		// current block to write into flash (initialize to write to invalid area)
	uint32_t flash_block_offset = 0; 	// blocks that have been written to flash (make sure to always update this if you increment write_block)

										// pointers to newly received metadata block and old metadata blocks
	metadata_blob *new_mb;

	uint32_t addr; 						// for calculating flash offsets

	bool passed; 						// did the metadata pass hmac
	bool ending = false; 				// did we receive a < BUFFER_LENGTH size when reading in firmware?

										// Useful crypto stuff
	wc_Sha256 hash;
	Aes aes;
	ecc_key ecc;

	// Set up ecc
	if (wc_ecc_init(&ecc)) {
		uart_write_str(UART0, "No memory for ecc key :(\n");
		while(UARTBusy(UART0_BASE)){}
		SysCtlReset();
	}

	// Import a key
	if (wc_ecc_import_x963(ecc_public_key, sizeof(ecc_public_key), &ecc)) {
		uart_write_str(UART0, "Can't init a key smfh\n");
		while (UARTBusy(UART0_BASE)) {}
		SysCtlReset();
	}
	uart_write_str(UART0, "hey look I have funny ecc key now lmao\n");

	vault_struct vault;
	uart_write_str(UART0, "U");

	// Read secrets from EEPROM and hide block, preventing further access
	EEPROMRead((uint32_t *) &secrets, SECRETS_EEPROM_OFFSET, sizeof(secrets));
	EEPROMBlockHide(EEPROMBlockFromAddr(SECRETS_EEPROM_OFFSET));

	// set up hash
	if (wc_InitSha256(&hash)) {
		uart_write_str(UART0, "sha256 skill issue\n");
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

	wc_Sha256Update(&hash, ct_buffer + sizeof(new_mb->iv), sizeof(metadata_blob) - sizeof(new_mb->iv));

	passed = verify_hmac((uint8_t *) &new_mb->metadata, sizeof(new_mb->metadata), secrets.hmac_key, (uint8_t *) &new_mb->hmac);

	// FLOW CHART: metadata signature good?
	if (!passed) {
		uart_write_str(UART0, "HMAC signature does not match :bangbang:\n");
		SysCtlReset();
	}
	uart_write_str(UART0, "you're did it\n");

	// Read into vault
	EEPROMRead((uint32_t *) &vault, SECRETS_VAULT_OFFSET, sizeof(vault));
	old_version = vault.fw_version;

	// Debug version should not reset 
	if (new_mb->metadata.fw_version != 0) {
		vault.fw_version = new_mb->metadata.fw_version;
	}

	vault.fw_length = new_mb->metadata.fw_length;
	vault.message_len = new_mb->metadata.message_length;

	switch (vault.s) {
		// Write to partition B, read old metadata from partition A
		case STORAGE_TRUST_A:
			start_block = STORAGE_PARTB;
			//new_permissions = STORAGE_TRUST_B;
			vault.s = STORAGE_TRUST_B;

			uart_write_str(UART0, "Trust a\n");

			break;
		// Write to partition A, read old metadata from partition B
		case STORAGE_TRUST_B:
			start_block = STORAGE_PARTA;
			//new_permissions = STORAGE_TRUST_A;
			vault.s = STORAGE_TRUST_A;
			uart_write_str(UART0, "Trust b\n");

			break;
		// By default write to A, firmware version is always 1	
		case STORAGE_TRUST_NONE:
			start_block = STORAGE_PARTA;
			//new_permissions = STORAGE_TRUST_A;
			vault.s = STORAGE_TRUST_A;

			uart_write_str(UART0, "Trust no one\n");
	}
	// Not needed afterwards so throw it away

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

	// Write **Encrypted** metadata to flash
	if (FlashProgram((uint32_t *) ct_buffer, addr, sizeof(metadata_blob))) {

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

		// //decrypt block
		// if (wc_AesCtrEncrypt(&aes, pt_buffer, ct_buffer, size)) {
		// 	uart_write_str(UART0, "idk how to do the crypto stuff all the cool kids are talking about\n");
		// 	SysCtlReset();
		// }

		// Write to flash
		program_flash((void *) addr, ct_buffer, size);

		// Update hash function (this is the cipher text, not the plaintext)
		if (wc_Sha256Update(&hash, (uint8_t *) ct_buffer, size)) {
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
	for (int i = 0; i < SECRETS_SIGNATURE_LENGTH; i++) {
		ct_buffer[i] = uart_read(UART0, BLOCKING, &read);
	}

	if (wc_Sha256Final(&hash, &ct_buffer[SECRETS_SIGNATURE_LENGTH]) != 0) {
		uart_write_str(UART0, "Couldn't compute hash");
		SysCtlReset();
	}

	// TODO: Use funny ecc thing instead of this crap
	passed = true;
	for (int i = 0; i < SECRETS_SIGNATURE_LENGTH; i++) {
		if (ct_buffer[i] != ct_buffer[SECRETS_SIGNATURE_LENGTH + i]) {
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
	program_flash((void *) addr, ct_buffer, SECRETS_SIGNATURE_LENGTH);

	// Store new vault
	result = EEPROMProgram((uint32_t *)&vault, SECRETS_VAULT_OFFSET, sizeof(vault));
	if (result != 0) {
		result = EEPROMProgram((uint32_t *)&vault, SECRETS_VAULT_OFFSET, sizeof(vault));
	}


	uart_write_str(UART0, "P");
	// can shorten this but it needs to not be so short that the string isn't written

	// wait for UART to finish
	while (UARTBusy(UART0_BASE)) {};
	SysCtlReset();
}

void boot_firmware(){

	// Values needed for decryption
	secrets_struct secrets;
	uint8_t iv[SECRETS_IV_LEN];

	metadata_blob *mb;
	metadata_blob decrypted_metadata;
	uint8_t *m_addr;
    uint32_t addr;

	// Decryption cipher
	Aes aes;

	uart_write_str(UART0, "B");
    uart_write_str(UART0, "Booting firmware...\n");
	vault_struct vault;


	// Read vault status
	EEPROMRead((uint32_t *) &vault, SECRETS_VAULT_OFFSET, sizeof(vault));


	// Read secrets from EEPROM and hide block, preventing further access
	EEPROMRead((uint32_t *) &secrets, SECRETS_EEPROM_OFFSET, sizeof(secrets));
	EEPROMBlockHide(EEPROMBlockFromAddr(SECRETS_EEPROM_OFFSET));

	if (vault.magic == VAULT_MAGIC) {
		uart_write_str(UART0, "No corrupted vault :D\n");
	}

	switch (vault.s) {
		case STORAGE_TRUST_A:

			// TODO: VERIFY FIRMWARE !!!

			mb = (metadata_blob *) ((STORAGE_PARTA << 10) + FLASH_PAGESIZE - sizeof(metadata_blob));
			uart_write_str(UART0, "I'm gonna boot from A :D\n");

			//Calculate the message address 
			m_addr = (uint8_t *) ((STORAGE_PARTA + 1) << 10);

			//Calculate the fw address
			addr = (STORAGE_PARTA + 2) << 10;
			break;

		case STORAGE_TRUST_B:
			mb = (metadata_blob *) ((STORAGE_PARTB << 10) + FLASH_PAGESIZE - sizeof(metadata_blob));

			// DELETE THE MESSAGE ||
			uart_write_str(UART0, "I'm gonna boot from B :D\n");

			//Calculate the message address 
			m_addr = (uint8_t *) ((STORAGE_PARTB + 1) << 10);
			//Calculate the fw address
			addr = (STORAGE_PARTB + 2) << 1;
			break;

		case STORAGE_TRUST_NONE:
			uart_write_str(UART0, "No fw in installed, plese update\n");
			SysCtlReset();
			return;

		default:
            uart_write_str(UART0, "Unknown trust state\n");
            SysCtlReset();
            return;
	}

	// Copy iv from metadata 
	// mb->iv
    memcpy(iv, mb, SECRETS_IV_LEN);

	// Setup crypto
	if (wc_AesInit(&aes, NULL, INVALID_DEVID)) {
		uart_write_str(UART0, "FATAL aes initialization error\n");
		while(UARTBusy(UART0_BASE)){}
		SysCtlReset();
    }

     // Set AES decryption key + IV
	// Direction for some modes (CFB and CTR) is always AES_ENCRYPTION.
    if (wc_AesSetKey(&aes, secrets.decrypt_key, sizeof(secrets.decrypt_key), iv, AES_ENCRYPTION)) {
        uart_write_str(UART0, "FATAL aes key setup error\n");
		while(UARTBusy(UART0_BASE)){}
        SysCtlReset();
    }


	// Decrypt the metadata
	if (wc_AesCtrEncrypt(&aes, \
				(uint8_t *) &decrypted_metadata.metadata, \
				((uint8_t *) mb) + SECRETS_IV_LEN,\
				sizeof(metadata_blob) - SECRETS_IV_LEN)) 
	{

        uart_write_str(UART0, "FATAL aes decrypt error\n");
		while(UARTBusy(UART0_BASE)){}
		SysCtlReset();
	}



	// decrypt + print the message
	// do NOT use metadata.message_length as the message is always 1024 bytes, message_length is only useful when printing the message
	if (wc_AesCtrEncrypt(&aes, message, m_addr, FLASH_PAGESIZE)) {
        uart_write_str(UART0, "FATAL aes decrypt error\n");
		while(UARTBusy(UART0_BASE)){}
		SysCtlReset();
	}

	// Verify metadata on boot
	uint32_t boot_fwversion = decrypted_metadata.metadata.fw_version;
	uint32_t vault_version = vault.fw_version;

	if((boot_fwversion != vault_version) && (boot_fwversion != 0)){
		uart_write_str(UART0, "The version did not check out\n");
		SysCtlReset();

	}
	// verify hmac signature of metadata
	bool passed;
	passed = verify_hmac((uint8_t *) &decrypted_metadata.metadata, sizeof(decrypted_metadata.metadata), secrets.hmac_key, decrypted_metadata.hmac);
	if (!passed) {
		uart_write_str(UART0, "Metadata wtf\n");
		SysCtlReset();
	}

	// Verify hmac signature of all data on boot
	uint32_t boot_size = decrypted_metadata.metadata.fw_length;
	//pad boot_size
	boot_size += SECRETS_ENCRYPTION_BLOCK_LENGTH - (boot_size % SECRETS_ENCRYPTION_BLOCK_LENGTH);
	uint32_t blocks = boot_size >> 10;
	if (boot_size % FLASH_PAGESIZE) {
		blocks += 1;
	}
	uint8_t* expected_hmac = addr + (uint8_t *) (blocks << 10);
	uint8_t* start_addr = (uint8_t*) mb + sizeof(mb->iv);

	//metadata size + message size + firmware size
	passed = verify_hmac((uint8_t *) start_addr, boot_size + FLASH_PAGESIZE + sizeof(decrypted_metadata) - sizeof(decrypted_metadata.iv), secrets.hmac_key, (uint8_t *) expected_hmac);

	// FLOW CHART: metadata signature good?
	if (!passed) {
		uart_write_str(UART0, "HMAC signature does not match on boot:bangbang:\n");
		SysCtlReset();
	}


	for (uint32_t i = 0; i < decrypted_metadata.metadata.message_length; i++) {
		uart_write(UART0, message[i]);
	}

	// Finish UART operations
	while(UARTBusy(UART0_BASE)){}

	// VERY DANGEROUS
	// Do not use globals after this function is called
	copy_fw_to_ram((uint32_t *) addr, \
			(uint32_t) 0x20000000, decrypted_metadata.metadata.fw_length, &aes);
	
	jump_to_fw(0x20000001, 0x20007FF0);

	while (1);
}


void jump_to_fw(uint32_t sram_start, uint32_t sram_end) {

	uint32_t fw_stack_pointer = sram_end;

	// Get the application's reset vector address from the SRAM start address (after initial SP)
    uint32_t fw_reset_vector = (volatile uint32_t)(sram_start);

	// Create a function pointer to the reset handler
    pFunction fw_entry = (pFunction) fw_reset_vector;

    // Set the application's stack pointer
    __asm volatile ("msr msp, %0" :: "r" (fw_stack_pointer) : );

    // Jump to the application's reset handler
    fw_entry();
}


void copy_fw_to_ram(uint32_t *fw_ptr, uint32_t *sram_ptr, uint32_t fw_size, Aes *cipher) {

	wc_AesCtrEncrypt(cipher, (uint8_t *) sram_ptr, (uint8_t *) fw_ptr, fw_size);

    // Clean up AES context
    wc_AesFree(cipher);
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
