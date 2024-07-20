# Bootloader and Firmware tools #

Run `bl_build.py` to generate bootloader in `bootloader_gen` directory.

The unchanged bootloader looks for a magic number in the 56th block
of flash to decide whether or not to store secrets in EEPROM

## NOTES ##

TODO

- Add configurable block offset
- Generate `secret_partion.h`
