#!/bin/sh

python tools/add_magic.py tools/super_secret.txt tools/super_secret.magic
python tools/add_secrets.py --infile bootloader/bin/bootloader.bin --outfile tools/bl_ready.bin --contents tools/super_secret.magic
echo finished
