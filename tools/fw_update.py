#!/usr/bin/env python

# Copyright 2024 The MITRE Corporation. ALL RIGHTS RESERVED
# Approved for public release. Distribution unlimited 23-02181-25.

"""
Firmware Updater Tool

A frame consists of two sections:
1. Two bytes for the length of the data section
2. A data section of length defined in the length section

[ 0x02 ]  [ variable ]
--------------------
| Length | Data... |
--------------------

In our case, the data is from one line of the Intel Hex formated .hex file

We write a frame to the bootloader, then wait for it to respond with an
OK message so we can write the next frame. The OK message in this case is
just a zero
"""

import argparse
from pwn import *
import time
import serial

from util import *


ser = ""

RESP_OK = b"A"
RESP_UPDATE = b"U"
SEND_UPDATE = b"U"
SEND_FRAME = b"F"

FRAME_SIZE = 1024
DEBUG = True

SIZE_SIG = 72
SIZE_IV = 16
SIZE_METADATA = 16
SIZE_HMAC = 32


# calculates a crc 16- IBM checksum, becasue board had that funcitonality
def calc_checksum(data): 
    poly = 0xA001
    crc = 0x0000
    for byte in data:
        crc ^= byte

        for j in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ poly
            else:
                crc >>= 1

    return p16(crc & 0xFFFF, endian="little")

def wait_confirmation(response):
    print("Waiting for bootloader response....")
    b = ser.read(1)
    if DEBUG:
        print(b.decode(), end='')
    while b != response:
        b = ser.read(1)
        if DEBUG:
            print(b.decode(), end='')
        pass
    print()
    print(">")


def send_metadata(ser, metadata, IV, metadata_hmac, debug=False):
    # blob =  iv 16 | metadata version 4 | fw length 4 | len message 4 | pad 4 | meta data hmac 32
    assert(len(metadata) == 16)

    # Might want to remove after debugging
    print("Version: {???}\nFirmware is: {???} bytes\n message is {???} bytes \n")

    # Handshake for update
    ser.write(SEND_UPDATE)
    wait_confirmation(RESP_UPDATE)
    

    if DEBUG:
        print("Writing metadata")

    # Bootloader is now ready to accept metadata, send handshake
    ser.write(SEND_FRAME)

    size = p16(64, endian="little") 
    
    # calculate a checksum please
    frame_data = IV + metadata + metadata_hmac
    checksum = calc_checksum(frame_data)
    print(calc_checksum(b'sob'))
    ser.write(size + frame_data + checksum)

    #ser.write(size + IV + metadata + metadata_hmac + calc_checksum(IV + metadata + metadata_hmac)) # last bits to take place of check

    # Wait for an OK from the bootloader.
    wait_confirmation(RESP_OK)
    if DEBUG:
        print("Received confirmation :D")

    # if resp != RESP_OK:  
    #     raise RuntimeError("ERROR: Bootloader responded with {}".format(repr(resp)))
def send_firmware(firmware, signature):
    full_blocks = len(firmware) // 1024
    extra = len(firmware) % 1024
    for i in range(full_blocks):
        if DEBUG:
            print("Writing firmware frame!")
        firmware_chunk = firmware[i * 1024: (i + 1) * 1024]
        checksum = calc_checksum(firmware_chunk) # checksum is of only the (cyrpt) firmware chunk
        ser.write(SEND_FRAME)
        size = p16(1024, endian="little")
        ser.write(size + firmware_chunk + checksum)  
        print(f'The check is {checksum}')
        wait_confirmation(RESP_OK)
    if extra:
        print("Writing partial frame!")
        ser.write(SEND_FRAME)
        size = p16(extra)
        firmware_chunk = firmware[full_blocks * 1024:]
        checksum = calc_checksum(firmware_chunk)
        ser.write(size + firmware_chunk + checksum)
        wait_confirmation(RESP_OK)
    

    print("Sending in signature please pray for me")
    ser.write(SEND_FRAME)

    ser.write(p16(0) + p16(len(signature)) + signature)
    wait_confirmation(RESP_OK)


def send_frame(ser, frame, debug=False):
    ser.write(frame)  # Write the frame...

    if debug:
        print_hex(frame)

    resp = ser.read(1)  # Wait for an OK from the bootloader

    time.sleep(0.1)

    if resp != RESP_OK:
        raise RuntimeError("ERROR: Bootloader responded with {}".format(repr(resp)))

    if debug:
        print("Resp: {}".format(ord(resp)))


def update(ser, infile, debug):
    # Open serial port. Set baudrate to 115200. Set timeout to 2 seconds.
    with open(infile, "rb") as fp:
        firmware_blob = fp.read()


    # firmware_blob = signature 32  iv 16  metadata 16  metadata_hmac 32  m_pad  firmware (numbers in bytes)
    #
    # Extracts each portion of the firmware blob
    """
    signature = firmware_blob[0:32]
    iv = firmware_blob[32:48]  
    metadata = firmware_blob[48:64]
    metadata_hmac = firmware_blob[64:96]
    firmware = firmware_blob[96:]

    fw_size = u32(metadata[4:8], endian="little")
    """
    cur = 2
    SIZE_SIG = u16(firmware_blob[:2], endian = 'little')
    print(f"Signature is {SIZE_SIG} bytes")
    signature = firmware_blob[cur : cur + SIZE_SIG]
    cur += SIZE_SIG
    iv = firmware_blob[cur : cur + SIZE_IV]
    cur += SIZE_IV
    metadata = firmware_blob[cur: cur + SIZE_METADATA]
    cur += SIZE_METADATA
    metadata_hmac = firmware_blob[cur : cur + SIZE_HMAC]
    cur += SIZE_HMAC
    firmware = firmware_blob[cur:]


    send_metadata(ser, metadata, iv, metadata_hmac, debug=debug)
    send_firmware(firmware, signature)

    print("Yay you did it :bangbang:")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Firmware Update Tool")

    parser.add_argument("--port", help="Does nothing, included to adhere to command examples in rule doc", required=False)
    parser.add_argument("--firmware", help="Path to firmware image to load.", required=False)
    parser.add_argument("--debug", help="Enable debugging messages.", action="store_true")
    args = parser.parse_args()

    if args.port == None:
        ser = serial.Serial("/dev/ttyACM0", 115200)
    else:

        ser = serial.Serial(args.port, 115200)

    update(ser=ser, infile=args.firmware, debug=args.debug)
    ser.close()
