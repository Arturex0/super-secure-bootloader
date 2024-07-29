registers = {"ra": 0b00000001,
             "rb": 0b00000010,
             "rc": 0b00000100,
             "rd": 0b00001000,
             "re": 0b00010000,
             "rf": 0b00100000}

opcodes = {"mov": 0x39,
           "add": 0x40,
           "sub": 0x41,
           "imm": 0x42,
           "cmp": 0x43,
           "stm": 0x44,
           "ldm": 0x45,
           "sys": 0x46,
           "jmp": 0x47,
           "jne": 0x48,
           "jlt": 0x49,

           "and": 0x50,
           "not": 0x51,
           "orr": 0x52,
           "xor": 0x53}

syscodes = {
        "sw": 0x10,
        "sr": 0x20,
        "sx": 0x30
        }
