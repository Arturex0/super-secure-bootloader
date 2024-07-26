import sys
from assembly_defs import *
from pwn import *

if (len(sys.argv) != 3):
    print(f"Usage: {sys.argv[0]} <infile> <outfile>")

ifile = sys.argv[1]
ofile = sys.argv[2]

current_addr = 0
user_labels = {}

final = b''

with open(ifile, 'r') as f:
    data = f.readlines()

for line in data:
    tokens = line.split()
    if (len(tokens) == 0):
        continue
    first = tokens[0]

    #see if its a comment
    if first[0] == ';':
        continue

    #see if its a label
    if first[0] == '<':
        user_labels[first] = current_addr
        print(f"Label {first} referring to instruction {current_addr}")
        continue
    #otherwise its an instruction
    opcode = opcodes[first]
    a = tokens[1]
    b = tokens[2]

    #check if register
    if a in registers:
        a = registers[a]
    #check if address
    elif a in user_labels:
        a = user_labels[a]
    #make int
    else:
        a = int(a)

    #check if register
    if b in registers:
        b = registers[b]
    #check if address
    elif b in user_labels:
        b = user_labels[b]
    #make int
    else:
        b = int(b)
    current_addr += 1
    ins = p8(opcode) + p8(a) + p8(b)
    final += ins
print(final)


