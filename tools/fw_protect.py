#!/usr/bin/env python

"""
Firmware Bundle-and-Protect Tool

"""
import argparse
import os
from pwn import *
from Crypto.Hash import HMAC, SHA256
from Crypto.Util.Padding import pad, unpad
import struct

DEFAULT_SECRETS="./secret_build_output.txt"


def protect_firmware(infile, outfile, version, message, secrets):
    # Load firmware binary from infile
    if secrets is None:
        secrets = DEFAULT_SECRETS
    with open(infile, "rb") as fp:
        firmware = fp.read()

    with open(secrets, 'r') as f:
        vault_key = bytes.fromhex(f.readline())
        decrypt_key = bytes.fromhex(f.readline())
        hmac_key = bytes.fromhex(f.readline())

    hmac_obj = HMAC.new(hmac_key, digestmod=SHA256)

    # Allocate one flash block to message that goes before firmware
    m = message.encode()
    m_pad = m + b'\xff' * (1024 - len(m))

    # Shorts required but pack into ints instead
    metadata = p32(version, endian='little') + p32(len(firmware), endian='little') + \
               p32(len(m), endian='little') + p32(0, endian='little')
    print(metadata)

    hmac_obj.update(metadata)
    metadata_hmac = hmac_obj.digest()

    hmac_obj.update(metadata_hmac + m_pad + firmware)

    signature = hmac_obj.digest()
    iv = os.urandom(16)

    #signature gets sent in at the end in seperate block
    firmware_blob = signature + iv + metadata + metadata_hmac + m_pad + pad(firmware, 16)


    # Write firmware blob to outfile
    with open(outfile, "wb+") as outfile:
        outfile.write(firmware_blob)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Firmware Update Tool")
    parser.add_argument("--infile", help="Path to the firmware image to protect.", required=True)
    parser.add_argument("--outfile", help="Filename for the output firmware.", required=True)
    parser.add_argument("--version", help="Version number of this firmware.", required=True)
    parser.add_argument("--message", help="Release message for this firmware.", required=True)
    parser.add_argument("--secrets", help="File containing secrets", required=False)
    args = parser.parse_args()

    protect_firmware(infile=args.infile, outfile=args.outfile, version=int(args.version), message=args.message, secrets=args.secrets)
