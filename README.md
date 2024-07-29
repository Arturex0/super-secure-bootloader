# Super Secure Bootloader

Installation and development guide for SSB

# Project Structure

```
├── bootloader *
│   ├── bin
│   │   ├── bootloader.bin
│   ├── src
│   │   ├── bootloader.c
│   │   ├── bass.c
│   │   ├── butils.c
│   │   ├── computer.c.c
│   │   ├── secret_partition.c
│   │   ├── startup_gcc.c
│   ├── inc
│   │   ├── bass.h
│   │   ├── bootloader.h
│   │   ├── butils.h
│   │   ├── computer.h
│   │   ├── metadata.h
│   │   ├── secret_partition.h
│   │   ├── secrets.h
│   │   ├── storage.h
│   │   ├── user_settings.h
│   ├── bootloader.ld
│   ├── Makefile
├── firmware
│   ├── bin
│   │   ├── firmware.bin
│   ├── lib
│   ├── src
├── lib
│   ├── driverlib
│   ├── inc
│   ├── uart
├── tools *
│   ├── bassembler.py
│   ├── bl_build.py
│   ├── fw_protect.py
│   ├── fw_update.py
│   ├── make_firmwares.sh
│   ├── util.py
│   ├── *.dumbbass
│   ├── bootloader_gen
│   │   ├── add_magic.py
│   │   ├── add_secrets.py
├── README.md

Directories marked with * are part of the CrASHBoot system
```

## Security Overview ##


### Formats ###

Our protected firmware has the following format:

```
| Length of signature | Signature | IV | Encrypted data ( Metadata + Message + Firmware) |
```

The message is always padded to be 1024 in order to occupy a block of flash

Our data is encrypted using `AES-CTR` and validated using `ec25519`

Furthermore, our metadata has the following format:

```
| fw_version | fw_length | message_length | unused padding | HMAC |
```

### Partitions ###

Our bootloader reserves two regions of flash called partitions to store firmware in. This way, one partition can contain verified firmware that is
safe to boot from while another serves as a temporary holding area for new firmware from the updater.

The blocks that the partitions occupy in flash are defined as `STORAGE_PARTA` and `STORAGE_PARTB` in `storage.h`, their sizes are defined as `STORAGE_PART_SIZE`

### Vaults and EEPROM ###

The vault contains information about the currently loaded firmware image. It contains a variable that indicates which partition to boot from, along
with metadata about the firmware in that partition. The vault is initialized to firmware version 1 and trusts neither partitions.

The vault is stored in an EEPROM block along with AES and HMAC keys. The public key for ec25519 verification
is not stored in EEPROM but flash.

The struct for the vault is defined as `vault_struct` in `storage.h`

The struct for the secrets is defined as `secrets_struct` in `secrets.h`

### Moving keys from flash ###

To store keys in EEPROM, the bootloader is first padded to a known length that is a multiple of the flash page size. A chunk of data containing a magic
number and some secret keys are then appended to the image. After flashing the bootloader onto the board, startup code checks for a magic number at this
block. If the magic is found, the bootloader resets the vault, erases and store keys into EEPROM, and erases flash.

The magic number and block to look for are defined as `SECRETS_MAGIC_INDICATOR` and `SECRETS_BLOCK` respectively in `secret_partition.h`

The code that checks for these secrets at startup is `setup_secrets` in `secret_partition.c`

### Updating ###

When updating, the updater sends a `U` and waits for a `U` back from the bootloader, it then uses the following frame format

```
| Frame byte ('F') | Data Length (2 bytes) | Data |
```

and waits for an acknowledgement byte back from the bootloader ('A')

The updater starts by sending in a frame containing encrypted metadata, the bootloader verifies the length of the frame
and decrypts it, along with verifying the HMAC signature. If this succeeds, the bootloader will compare the version of this metadata chunk with the
old version stored in the vault. If the version is satisfactory (version >= old version), the bootloader stores the metadata into flash and continues.

The updater must then send in as many frames of data length 1024 containing encrypted data as it can. It must send in a partial frame containing
the remaining data if the data size is not a multiple of 1024. The bootloader does no verification on these frames in this step and simply stores
them into flash, unless it finds that no space in flash is remaining.

When the updater is finished sending the data, it must send in a frame with the data length set to zero containing the signature. The bootloader
will take this signature and verify the data it has received. If this data is valid, it will change the vault settings to boot from the new partition, along
with storing the new metadata information.

Metadata data structures are defined in `metadata.h`

Frame code is handled by `fw_update.py` on the updater's side and the function `read_frame` in `butils.c` on the bootloader's side

### Booting ###

On boot, the bootloader verifies the metadata of the firmware stored in the partition matches that stored in the vault. The bootloader will also verify
the ed25119 signature before going on.

The bootloader will then decrypt the release message and prints it out, before decrypting the firmware into RAM and executing it.

## Tools

There are three python scripts in the `tools` directory which are used to:

1. Provision the bootloader (`bl_build.py`)
2. Package the firmware (`fw_protect.py`)
3. Update the firmware to a TM4C with a provisioned bootloader (`fw_update.py`)

### bl_build.py

This script calls `make` in the `bootloader` directory.
It also generates an AES and HMAC key, along with an ed25119 private key public key pair. The public key is made available to the bootloader in the form
of a header file called `public.h`

When the bootloader is created, the script will call `make` in the `bootloader_gen` directory to pad the bootloader and add secrets to it.

### fw_protect.py

This script bundles the version and release message with the firmware binary.
It also encrypts the firmware and adds signatures to it.

### fw_update.py

This script opens a serial channel with the bootloader, then writes the firmware metadata and binary broken into data frames to the bootloader.

# Building and Flashing the Bootloader

1. Enter the `tools` directory and run `bl_build.py`

```
cd ./tools
python bl_build.py
```

2. Flash the bootloader using `lm4flash` tool
   
```
sudo lm4flash bootloader_gen/bl_ready.bin
```

# Bundling and Updating Firmware

1. Enter the firmware directory and `make` the example firmware.

```
cd ./firmware
make
```

2. Enter the tools directory and run `fw_protect.py`

```
cd ../tools
python fw_protect.py --infile ../firmware/bin/firmware.bin --outfile firmware_protected.bin --version 2 --message "Firmware V2"
```

This creates a firmware bundle called `firmware_protected.bin` in the tools directory.

3. Reset the TM4C by pressig the RESET button

4. Run `fw_update.py`

```
python fw_update.py --firmware ./firmware_protected.bin
```

If the firmware bundle is accepted by the bootloader, the `fw_update.py` tool will exit

Additional firmwares can be updated by repeating steps 3 and 4, but only firmware versions higher than the one flashed to the board (or version 0) will be accepted.

# Interacting with the Bootloader

Using the custom `car-serial` script:
```
car-serial
```

Using `pyserial` module:

```
python -m serial.tools.miniterm /dev/ttyACM0 115200
```

You can now interact with the bootloader and firmware! Type 'B' to boot.

Exit miniterm: `Ctrl-]`
Exit picocom: `Ctrl-A X`

# Launching the Debugger
Use OpenOCD with the configuration files for the board to get it into debug mode and open GDB server ports:
```bash
openocd -f /usr/share/openocd/scripts/interface/ti-icdi.cfg -f /usr/share/openocd/scripts/board/ti_ek-tm4c123gxl.cfg
```

Start GDB and connect to the main OpenOCD debug port:
```bash
gdb-multiarch -ex "target extended-remote localhost:3333" bootloader/bin/bootloader.axf
```

Go to `main` function and set a breakpoint
```
layout src
list main
break bootloader.c:50
```

