# # Load the .ibf file instructions


valid_chars = {'<', '>', '^', 'v', '+', '-', '.', ',', '[', ']', '*', '0'}

program = []

# Load the program 

with open("p.ibf", "r") as f:
    while True:
        char = f.read(1)
        if not char:
            break;
        if char in valid_chars:
            program.append(char)
        elif char == '#':
            f.readline()
        elif char == '~':
            f.read()


print(program)

# Execute 
memsize = 300

tape = [0] * memsize 

# values = [93, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 0, 0, 0, 0, 0, 0, 0, 21, 34]
# values =  0, 0, 0, 0, 0, 0, 0, 0, 21, 34]
# values = [0, 0, 0, 0, 0, 0, 0, 0, 8]
# values = [0, 255, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0]

def read_file_to_array(filename):
    with open(filename, 'rb') as file:
        byte_data = file.read()
    return list(byte_data)

# Example usage
input_filename = 'encrypted.bin'
# input_filename = 'something.bin'




# Read the file and load all bytes into an array
values = read_file_to_array(input_filename)

# Append the additional values
additional_values = [0, 0, 0, 0, 0, 0, 0, 0, 21, 34]
values.extend(additional_values)

print(values)



for i in range(len(values)):
    tape[i] = values[i]




print(tape)

ip = 0
pointer = 0

loop_stack = []

print(str(tape) + "\n\n")

while ip < len(program):

    # print("executing: " + str(program[ip]))

    if program[ip] == "<":
        if pointer != 0:
            pointer-=1
        else:
            print("Illegal argument exception at instruction: " + str(ip))
            break;
    elif program[ip] == ">":
        if pointer != memsize:
            pointer+=1
    elif program[ip] == "^":
        if tape[pointer] + pointer < memsize:
            pointer += tape[pointer]
    elif program[ip] == "v":
        if pointer - tape[pointer] >= 0:
            pointer -= tape[pointer]
    elif program[ip] == "+":
        tape[pointer] +=1
        if tape[pointer] == 256:
            tape[pointer] = 0
    elif program[ip] == "-":
        tape[pointer] -=1
        if tape[pointer] == -1:
            tape[pointer] = 255
    elif program[ip] == '0':
        pointer = 0
    elif program[ip] == '[':
        if tape[pointer] == 0:
            open_brackets = 1

            while open_brackets != 0:
                ip+=1

                if program[ip] == '[':
                    open_brackets += 1
                elif program[ip] == ']':
                    open_brackets -= 1

        else:

            loop_stack.append(ip)

    elif program[ip] == ']':
        if tape[pointer] != 0:
            ip = loop_stack[-1]
            # print("jumped to: " + str(ip+1))
        else:
            loop_stack.pop()

    ip+=1
    # print(tape)
    # print("pointer: " + str(pointer))
    # print(loop_stack)
        



    
print(tape)
print("pointer: " + str(pointer))
print(program)







# print("\n\n\n\n")
# print(program)
# print("\n\n\n\n")

# for i in program:
#     print(i, end="")









print(len(values))