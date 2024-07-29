import argparse

BLOCK_SIZE = 1024
#reserve block 100
RESERVED_BLOCK = 100

def pad(infile, contentfile, outfile, reserved_block):
    with open(infile, 'rb') as f:
        data = f.read()
    with open(contentfile, 'rb') as f:
        contents = f.read()
    if len(contents) > BLOCK_SIZE:
        print("Secret file must fit inside one flash block")
        return
    # confirm there is space for block reserved block
    size = len(data)
    padding_needed = (reserved_block * BLOCK_SIZE) - BLOCK_SIZE - size
    if (padding_needed < 0):
        print("Not enough space for reserved block!")
        return

    payload = data + b'\xff' * padding_needed + contents
    with open(outfile, 'wb') as f:
        f.write(payload)
    print("Success!")

parser = argparse.ArgumentParser(description="Secret key integration tool")
parser.add_argument("--infile", required=True)
parser.add_argument("--outfile", required=True)
parser.add_argument("--contents", required=True)
parser.add_argument("--block", required=False)
args = parser.parse_args()

pad(args.infile, args.contents, args.outfile, int(args.block) if args.block else RESERVED_BLOCK)
