#include "bootloader.h"
#include "secret_partition.h"
#include "secrets.h"

// Hardware Imports
#include "inc/hw_memmap.h"    // Peripheral Base Addresses
#include "inc/hw_types.h"     // Boolean type
#include "inc/tm4c123gh6pm.h" // Peripheral Bit Masks and Registers

// Driver API Imports
#include "driverlib/flash.h"     // FLASH API
#include "driverlib/sysctl.h"    // System control API (clock/reset)

#include "driverlib/eeprom.h"	 // EEPROM API

// Checks for secrets and moves them to EEPROM if they exist, otherwise does nothing
// Before using this function, ensure:
// EEPROM is enabled (SysCtlPeripheralEnable())
// EEPROMInit() is called, *must* be done after SysCtlPeripheralEnable()
void setup_secrets(void) {
	uint32_t magic = *(uint32_t *)(SECRETS_BLOCK << 10);
	if (magic == SECRETS_MAGIC_INDICATOR) {

		// compute address where keys are stored (should just be right after magic)
		secrets_struct *secrets_location = (secrets_struct *) ((SECRETS_BLOCK << 10) + 4);
		secrets_struct secrets;


		//results of api calls for error checking
		uint32_t result = 0;

		//Erase EEPROM so we can write our keys
		result = EEPROMMassErase();
		if (result != 0) {
			SysCtlReset();
		}
		//backup secrets into memory before clearing flash
		secrets = *secrets_location;

		//clear flash before writing to EEPROM to ensure only one location contains secrets
		result = FlashErase((SECRETS_BLOCK << 10));
		if (result != 0) {
			SysCtlReset();
		}

		
		//store secrets into EEPROM
		result = EEPROMProgram((uint32_t *)&secrets, SECRETS_EEPROM_OFFSET, sizeof(secrets_struct));
		if (result != 0) {
			result = EEPROMProgram((uint32_t *)&secrets, SECRETS_EEPROM_OFFSET, sizeof(secrets_struct));
		}
		
		SysCtlReset();
		//counter to test write

	}

}
