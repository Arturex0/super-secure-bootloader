def encrypt(plaintext, key1, key2):
    part1 = []
    part2 = []
    for index, byte in enumerate(plaintext):
        if index % 2 == 0:
            encrypted_byte = (byte + key1 + index) % 256
            part1.append(encrypted_byte)
        else:
            encrypted_byte = ((byte ^ (key2 + index)) +1) % 256
            part2.append(encrypted_byte)
    
    ciphertext = []
    for i in range(max(len(part1), len(part2))):
        if i < len(part1):
            ciphertext.append(part1[i])
        if i < len(part2):
            ciphertext.append(part2[i])
    
    return ciphertext



def decrypt(ciphertext, key1, key2):

    part1 = []
    part2 = []
    for i in range(len(ciphertext)):
        if i % 2 == 0:
            part1.append(ciphertext[i])
        else:
            part2.append(ciphertext[i])

    print("PART1: " + str(len(part1)))
    print("PART2: " + str(len(part2)))


    plaintext = []
    for index, encrypted_byte in enumerate(part1):
        decrypted_byte = (encrypted_byte - key1 - (index * 2))
        plaintext.append(decrypted_byte)

    # print("mid: " + str(plaintext))
    # print("second part: " + str(part2))

    for index, encrypted_byte in enumerate(part2):
        decrypted_byte = (encrypted_byte - 1) ^ (key2 + (index * 2 + 1))
        # print(decrypted_byte)
        plaintext.insert(index * 2 + 1, decrypted_byte)

    return plaintext

# # Example usage
# plaintext = b"DZIALA TO NAWET NA DLUICH TEKSTACH"
# print("REAL PLAINTET: ")
# for index, byte in enumerate(plaintext):
#     print(byte, end=" ")
# # plaintext = b"Hello world!"
key1 = 21
key2 = 34
# ciphertext = encrypt(plaintext, key1, key2)
# print("Ciphertext:", ciphertext)


# plaintext = decrypt(ciphertext, key1, key2)
# print("Plaintet:", plaintext)
# Example usage


def read_file(filename):
    with open(filename, 'rb') as file:
        return bytearray(file.read())

def write_file(filename, data):
    with open(filename, 'wb') as file:
        file.write(bytearray(data))


input_filename = './something.bin'
encrypted_filename = 'encrypted.bin'
decrypted_filename = 'decrypted.txt'

# Read the plaintext from the file
plaintext = read_file(input_filename)


ciphertext = encrypt(plaintext, key1, key2)
write_file(encrypted_filename, ciphertext)