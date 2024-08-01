TOOL_ROOT=.
BOOT_LOADER_DIR=../../bootloader
SECRETS_FILE=../secret_build_output.txt

all: bl_ready.bin

bl_ready.bin: secret_build_output.txt ${BOOT_LOADER_DIR}/bin/bootloader_unready.bin
	python3 add_secrets.py --infile ${BOOT_LOADER_DIR}/bin/bootloader_unready.bin --contents secret_build_output.txt --outfile bl_ready.bin
	cp bl_ready.bin ${BOOT_LOADER_DIR}/bin/bootloader.bin


secret_build_output.txt: ${SECRETS_FILE}
	python3 add_magic.py ${SECRETS_FILE} secret_build_output.txt

clean:
	rm secret_build_output.txt bl_ready.bin
