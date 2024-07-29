def read_file_to_array(filename):
    with open(filename, 'rb') as file:
        byte_data = file.read()
    return list(byte_data)

# Example usage
input_filename = 'encrypted.bin'
# input_filename = 'something.bin'




# Read the file and load all bytes into an array
ciphertext = read_file_to_array(input_filename)

part1 = []
part2 = []
for i in range(len(ciphertext)):
    if i % 2 == 0:
        part1.append(ciphertext[i])
    else:
        part2.append(ciphertext[i])

print(part2)