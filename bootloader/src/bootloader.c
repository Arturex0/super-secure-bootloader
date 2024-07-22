#include "bootloader.h"
#include "secret_partition.h"
#include "secrets.h"
#include "metadata.h"
#include "storage.h"
#include "butils.h"

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
void boot_firmware(void);
void uart_write_hex_bytes(uint8_t, uint8_t *, uint32_t);
bool verify_hmac(uint8_t * data, uint32_t data_len, uint8_t * key, uint8_t * test_hash);
void setup_vault(void);

//secrets buffer
#define BUFFER_SIZE 1024

// Reads into buffer and performs error checking, please ensure that buffer has minimum length of BUFFER_SIZE
uint32_t read_frame(uint8_t *buffer);
uint16_t read_short(void);

//crypto state
Hmac hmac;
Aes aes;
uint8_t iv[SECRETS_IV_LEN];
uint8_t ct_buffer[FLASH_PAGESIZE];
uint8_t pt_buffer[FLASH_PAGESIZE];
metadata global_metadata;

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
	/*
	 * ========================================= Setup Finished ==============================
	 */

	// Funny crypto shenanigans

	secrets_struct secrets;
	EEPROMRead((uint32_t *) &secrets, SECRETS_EEPROM_OFFSET, sizeof(secrets));
	
	if (wc_HmacSetKey(&hmac, WC_SHA256, secrets.hmac_key, sizeof(secrets.hmac_key)) != 0) {
		uart_write_str(UART0, "FATAL hmac key error\n");
		SysCtlReset();
	}

    uart_write_str(UART0, "Welcome to the BWSI Vehicle Update Service!\n");
    uart_write_str(UART0, "Send \"U\" to update, and \"B\" to run the firmware.\n");

    int resp;
    while (1) {
        uint32_t instruction = uart_read(UART0, BLOCKING, &resp);

        if (instruction == UPDATE) {

			// ========== Funny metadata shenanigans =========
			//Read a frame and ensure it matches the size of metadata
			uint32_t size;
			bool passed;
            uart_write_str(UART0, "U");

			size = read_frame(ct_buffer);
			metadata_blob *m = (metadata_blob *) &ct_buffer;

			if (size != sizeof(metadata_blob)) {
				uart_write_str(UART0, "You did not give me metadata and now I am angry\n");
				SysCtlReset();
			}
			//save a bunch of stuff
			memcpy(iv, &m->iv, sizeof(m->iv));
			memcpy(&global_metadata, &m->metadata, sizeof(m->metadata));
			passed = verify_hmac((uint8_t *) &global_metadata, sizeof(metadata), secrets.hmac_key, (uint8_t *) &m->hmac);
			if (!passed) {
				uart_write_str(UART0, "HMAC signature does not match :bangbang:\n");
				SysCtlReset();
			}
			// Flash the funny metadata into memory

			uart_write_str(UART0, "TODO: finish firmware loading :sob:\n");
            //load_firmware();
            //uart_write_str(UART0, "Loaded new firmware.\n");
            //nl(UART0);
        } else if (instruction == BOOT) {
            uart_write_str(UART0, "B");
            uart_write_str(UART0, "Booting firmware...\n");
            boot_firmware();
        }
    }
}

 /*
 * Load the firmware into flash.
 */
void load_firmware(void) {
    int frame_length = 0;
    int read = 0;
    uint32_t rcv = 0;

    uint32_t data_index = 0;
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

