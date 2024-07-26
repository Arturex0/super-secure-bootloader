import sys
from assembly_defs import *
from pwn import *

if (len(sys.argv) != 3):
    print(f"Usage: {sys.argv[0]} <infile> <outfile>")

ifile = sys.argv[1]
ofile = sys.argv[2]

user_labels = {}

final = b''

with open(ifile, 'r') as f:
    data = f.readlines()

#first run, gather labels
current_addr = 0
for line in data:
    tokens = line.split()
    if (len(tokens) == 0):
        continue
    first = tokens[0]
    if first in opcodes:
        current_addr += 1
        continue
    if first[0] == '<':
        user_labels[first] = current_addr
        print(f"Label {first} @ {current_addr}")



print("compiler run")
print(user_labels)
current_addr = 0
for line in data:
    tokens = line.split()

    #ignore whitespace
    if (len(tokens) == 0):
        continue
    first = tokens[0]

    #see if its a comment or label
    if first[0] == ';' or first[0] == '<':
        continue

    #otherwise its an instruction
    opcode = opcodes[first]
    a = tokens[1]
    b = tokens[2]

    #check if register
    if a in registers:
        a = registers[a]
    #check if syscode
    elif a in syscodes:
        a = syscodes[a]
    #check if address
    elif a in user_labels:
        a = user_labels[a]
    #make int
    else:
        a = int(a)

    #check if register
    if b in registers:
        b = registers[b]
    #check if syscode
    elif b in syscodes:
        b = syscodes[b]
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


