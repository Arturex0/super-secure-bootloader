TOOL_ROOT=.
BOOT_LOADER_DIR=../../bootloader

all: bl_ready.bin

bl_ready.bin: super_secret.magic ${BOOT_LOADER_DIR}/bin/bootloader.bin
	python add_secrets.py --infile ${BOOT_LOADER_DIR}/bin/bootloader.bin --contents super_secret.magic --outfile bl_ready.bin

super_secret.magic: super_secret.txt
	python add_magic.py super_secret.txt super_secret.magic

super_secret.txt:
	python gen_secret.py super_secret.txt

clean:
	rm super_secret.txt super_secret.magic bl_ready.bin
