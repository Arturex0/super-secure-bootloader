#!/usr/bin/env python3

"""
Bootloader Build Tool

This tool is responsible for building the bootloader from source and copying
the build outputs into the host tools directory for programming.
"""
import os
import pathlib
import subprocess

#change this once we decide on algorithm
KEY_SIZE=16

REPO_ROOT = pathlib.Path(__file__).parent.parent.absolute()
TOOL_DIR = pathlib.Path(__file__).parent.absolute()
GEN_DIR = os.path.join(TOOL_DIR, "bootloader_gen")
BOOTLOADER_DIR = os.path.join(REPO_ROOT, "bootloader")
SECRETS_FILE=os.path.join(TOOL_DIR, "secret_build_output.txt")

def generate_keys():
    print("making funny keys")
    vault_key = os.urandom(KEY_SIZE)
    decrypt_key=os.urandom(KEY_SIZE)
    hmac_key=os.urandom(KEY_SIZE)
    with open(SECRETS_FILE, 'w') as f:
        f.write(vault_key.hex() + '\n')
        f.write(decrypt_key.hex()+ '\n')
        f.write(hmac_key.hex() + '\n')

def make_bootloader() -> bool:
    # Build the bootloader from source.

    os.chdir(BOOTLOADER_DIR)

    subprocess.call("make clean", shell=True)
    status = subprocess.call("make")

    os.chdir(GEN_DIR)

    if status == 0:
        generate_keys()
        subprocess.call("make clean", shell=True)
        status = subprocess.call("make", shell=True)

    # Return True if make returned 0, otherwise return False.
    return status == 0


if __name__ == "__main__":
    make_bootloader()
