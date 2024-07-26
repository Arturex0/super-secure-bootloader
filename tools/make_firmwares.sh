#!/bin/sh
python fw_protect.py --infile ../firmware/bin/firmware.bin --outfile debug.bin --version 0 --message "I am debug version lmao"
python fw_protect.py --infile ../firmware/bin/firmware.bin --outfile firmware_protected.bin --version 2 --message "I am most ordinary version"
python fw_protect.py --infile ../firmware/bin/firmware.bin --outfile new.bin --version 3 --message "I'm special new version :D"
