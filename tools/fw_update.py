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

ser = serial.Serial("/dev/ttyACM0", 115200)

RESP_OK = b"A"
FRAME_SIZE = 1024


def send_metadata(ser, metadata, IV, metadata_hmac, debug=False):
    # blob =  iv 16 | metadata version 4 | fw length 4 | len message 4 | pad 4 | meta data hmac 32
    assert(len(metadata) == 16)

    version = u16(metadata[:4], endian='little')
    fw_size = u16(metadata[4:8], endian='little')
    message_len = u16(metadata[8:12], endian='little')

    # Might want to remove after debugging
    print(f"Version: {version}\nFirmware is: {fw_size} bytes\n message is {message_len} bytes \n")

    # Handshake for update
    ser.write(b"U")

    print("Waiting for bootloader to enter update mode...")
    while ser.read(1).decode() != "U":
        print("got a byte")
        pass
    

    # Bootloader is now ready to accept metadata
    size = p16(64, endian="little") 
    ser.write(size + IV + metadata + metadata_hmac)

    

    # Wait for an OK from the bootloader.
    while ser.read(1).decode() != RESP_OK:  # charcter A
        print("Wating for confirmation...")

    # if resp != RESP_OK:  
    #     raise RuntimeError("ERROR: Bootloader responded with {}".format(repr(resp)))


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
    signature = firmware_blob[0:32]
    iv = p8(firmware_blob[32:48], endian="little")  
    metadata = firmware_blob[48:64]
    metadata_hmac = firmware_blob[64:96]

    fw_size = u32(metadata[4:8], endian="little")
    firmware = firmware_blob[fw_size:]

    send_metadata(ser, metadata, iv, metadata_hmac, debug=debug)
    
    # If you reach here you should have sucsessfully sent metadata and can start to send firmware frames


    for idx, frame_start in enumerate(range(0, len(firmware), FRAME_SIZE)):
        data = firmware[frame_start : frame_start + FRAME_SIZE]

        # Construct frame.
        frame_size = p16(len(data), endian='little')
        frame =  frame_size + data

        send_frame(ser, frame, debug=debug)
        print(f"Wrote frame {idx} ({len(frame)} bytes)")

    print("Done writing firmware.")

    # Send a zero length payload to tell the bootlader to finish writing it's page.
    ser.write(p16(0x0000, endian='little'))
    resp = ser.read(1)  # Wait for an OK from the bootloader
    if resp != RESP_OK:
        raise RuntimeError("ERROR: Bootloader responded to zero length frame with {}".format(repr(resp)))
    print(f"Wrote zero length frame (2 bytes)")

    return ser


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Firmware Update Tool")

    parser.add_argument("--port", help="Does nothing, included to adhere to command examples in rule doc", required=False)
    parser.add_argument("--firmware", help="Path to firmware image to load.", required=False)
    parser.add_argument("--debug", help="Enable debugging messages.", action="store_true")
    args = parser.parse_args()

    update(ser=ser, infile=args.firmware, debug=args.debug)
    ser.close()
