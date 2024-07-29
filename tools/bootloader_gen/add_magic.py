import sys
import struct
MAGIC = 0x00002137
KEY_SIZE=16

bytestring = struct.pack('<I', MAGIC)

if len(sys.argv) != 3:
    print(f"Usage: {sys.argv[0]} <in file> <out file>")
    exit()

inf = sys.argv[1]
of = sys.argv[2]

with open(inf, 'r') as f:
    decrypt_key = bytes.fromhex(f.readline())
    hmac_key = bytes.fromhex(f.readline())
data = decrypt_key + hmac_key

with open(of, 'wb') as f:
    payload = bytestring + data
    f.write(payload)
