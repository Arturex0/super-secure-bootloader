import sys
import struct
MAGIC = 0x00002137
bytestring = struct.pack('<I', MAGIC)

if len(sys.argv) != 3:
    print(f"Usage: {sys.argv[0]} <in file> <out file>")
    exit()

inf = sys.argv[1]
of = sys.argv[2]

with open(inf, 'rb') as f:
    data = f.read()

with open(of, 'wb') as f:
    payload = bytestring + data
    f.write(payload)
