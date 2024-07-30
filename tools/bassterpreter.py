import basscodes

class State:
    def __init__(self, instructions, writeb, readb, readr, writer):
        self.ip = 0
        self.fl = 0
        self.ra = 0
        self.rb = 0
        self.rc = 0
        self.rd = 0
        self.re = 0
        self.rf = 0
        self.instructions = instructions
        self.writeb = writeb
        self.readb = readb
        self.readr = readr
        self.writer = writer
        self.readp = 0
        self.memory = [0 for _ in range(256)]

    def interpret_instruction(self):
        instruction_data = self.instructions[self.ip * 3: self.ip * 3 + 3]
        opcode = instruction_data[0]
        a = instruction_data[1]
        b = instruction_data[2]

        inc_ins = True
        ending = False

        match opcode:
            case basscodes.COMP_ADD_CODE:
                self.add(a, b)
            case basscodes.COMP_SUB_CODE:
                self.sub(a, b)
            case basscodes.COMP_IMM_CODE:
                self.imm(a, b)
            case basscodes.COMP_CMP_CODE:
                self.cmp(a, b)
            case basscodes.COMP_STM_CODE:
                self.stm(a, b)
            case basscodes.COMP_LDM_CODE:
                self.ldm(a, b)
            case basscodes.COMP_SYS_CODE:
                ending = self.sys(a, b)

            case basscodes.COMP_JMP_CODE:
                inc_ins = False
                self.jmp(a, b)
            case basscodes.COMP_JNE_CODE:
                inc_ins = False
                self.JNE(a, b)
            case basscodes.COMP_JLT_CODE:
                inc_ins = False
                self.jlt(a, b)

            case basscodes.COMP_AND_CODE:
                self.int_and(a, b)
            case basscodes.COMP_NOT_CODE:
                self.int_not(a, b)
            case basscodes.COMP_ORR_CODE:
                self.orr(a, b)
            case basscodes.COMP_XOR_CODE:
                self.xor(a, b)
            case _:
                ending = True
                print("Wtf bad instruction")
        print("incing")
        if inc_ins:
            self.ip += 1
        return ending



    def rr(self, reg):
        match reg:
            case basscodes.COMP_RA_MASK:
                return self.ra
            case basscodes.COMP_RB_MASK:
                return self.rb
            case basscodes.COMP_RC_MASK:
                return self.rc
            case basscodes.COMP_RD_MASK:
                return self.rd
            case basscodes.COMP_RE_MASK:
                return self.re
            case basscodes.COMP_RF_MASK:
                return self.rf
            case _:
                print("Bad register number")

    def wr(self, reg, v):
        if (v < 0) or (v > 255):
            print(f"INT ERROR V IS {v}")

        match reg:
            case basscodes.COMP_RA_MASK:
                self.ra = v
            case basscodes.COMP_RB_MASK:
                self.rb = v
            case basscodes.COMP_RC_MASK:
                self.rc = v
            case basscodes.COMP_RD_MASK:
                self.rd = v
            case basscodes.COMP_RE_MASK:
                self.re = v
            case basscodes.COMP_RF_MASK:
                self.rf = v
            case _:
                print("Bad register number")
    
    def add(self, ra, rb):
        self.wr(ra, (self.rr(ra) + self.rr(rb)) & 0xFF)

    def sub(self, ra, rb):
        a = self.rr(ra)
        b = self.rr(rb)
        negb = 0x100 - b
        self.wr(ra, (a + negb) & 0xFF)

    def imm(self, ra, rb):
        self.wr(ra, rb)

    def cmp(self, ra, rb):
        a = self.rr(ra)
        b = self.rr(rb)
        negb = 0x100 - b
        res = a + negb

        signa = (a >> 7) & 1
        signb = (b >> 7) & 1

        signr = (result >> 7) & 1
        carry = (result >> 8) & 1
        zero = 1 if (result == 0) else 0
        overflow = 1 if ((not (signa ^ signb)) and (signr ^ signa)) else 0
        self.fl = signr << basscodes.COMP_FLAG_SIGN_SHIFT | \
            carry << basscodes.COMP_FLAG_CARRY_SHIFT | \
            zero << basscodes.COMP_FLAG_ZERO_SHIFT | \
            overflow << basscodes.COMP_FLAG_OVERFLOW_SHIFT;
    
    def stm(self, ra, rb):
        a = self.rr(ra)
        b = self.rr(rb)
        self.memory[a] = b
    def ldm(self, ra, rb):
        self.wr(ra, self.memory[self.rr(rb)])

    def sys(self, a, b):
        syscode = self.ra
        match syscode:
            case basscodes.COMP_SYS_WRITE:
                if (self.writer == 0):
                    self.ra = 33
                    print("no write")
                    return True
                self.writeb.append(self.rb)
                self.writer -= 1

            case basscodes.COMP_SYS_READ:
                if (self.readr == 0):
                    self.ra = 44
                    print("no read")
                    return True
                self.ra = self.readb[self.readp]
                self.readp += 1
                self.readr -= 1
            
            case basscodes.COMP_SYS_EXIT:
                return True

            case _:
                state.ra = 55
                return True

    def jmp(self, a, b):
        self.ip = a

    def jne(self, a, b):
        self.ip += 1
        if (self.fl >> basscodes.COMP_FLAG_ZERO_SHIFT) & 1:
            self.ip = a

    def int_and(self, ra, rb):
        self.wr(ra, self.rr(ra) & self.rr(rb))

    def int_not(self, ra, rb):
        self.wr(ra, 0x100 - self.rr(rb))

    def orr(self, ra, rb):
        self.wr(ra, self.rr(ra) | self.rr(rb))
    
    def xor(self, ra, rb):
        self.wr(ra, self.rr(ra) ^ self.rr(rb))

