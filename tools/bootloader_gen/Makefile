TOOL_ROOT=.
BOOT_LOADER_DIR=../../bootloader
SECRETS_FILE=../secret_build_output.txt

all: bl_ready.bin

bl_ready.bin: super_secret.magic ${BOOT_LOADER_DIR}/bin/bootloader.bin
	python add_secrets.py --infile ${BOOT_LOADER_DIR}/bin/bootloader.bin --contents super_secret.magic --outfile bl_ready.bin

super_secret.magic: ${SECRETS_FILE}
	python add_magic.py ${SECRETS_FILE} super_secret.magic

clean:
	rm super_secret.magic bl_ready.bin
