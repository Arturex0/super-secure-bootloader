import struct
import sys
if (len(sys.argv) != 2):
    print(f"Usage: {sys.argv[0]} <out>")
    exit()

secret = 0xDEADBEEF
bytestring = struct.pack('<I', secret)
file = sys.argv[1]

with open(file, "wb") as f:
    f.write(bytestring)
