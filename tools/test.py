import bassterpreter
with open('ban.do', 'rb') as f:
    data = f.read()

w = []
r = b'hello world'

thing = bassterpreter.State(data, w, r, 11, 11)

end = False
while (not end):
    end = thing.interpret_instruction()

print(w)
