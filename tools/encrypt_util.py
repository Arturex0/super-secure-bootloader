import bassterpreter

def bf_encrypt(plaintext):
    
    with open('special.sdo', 'rb') as f:
        data = f.read()

    w = []
    thing = bassterpreter.State(data, w, plaintext, len(plaintext), len(plaintext))

    end = False
    while (not end):
        end = thing.interpret_instruction()

    r = b''
    for c in w:
        r += int.to_bytes(c)
    return r

