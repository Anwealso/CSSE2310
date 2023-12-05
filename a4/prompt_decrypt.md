
Write a c function that decrypts a given encrypted password string, `char* ciphertext`. Make sure to use the standard c library (<crypt.h>) function crypt() or crypt_r()

Spec
"""
crackserver is to use a brute-force method for cracking passwords as follows: For each received ciphertext password
• Extract the password salt (the first two characters of the ciphertext) • For each word saved from the dictionary – encrypt the word with the salt using crypt() or crypt_r() – compare the sample ciphertext with the encrypted dictionary word – if the two ciphertexts are the same, terminate the search and return the plaintext dictionary word
Note – with just over 200,000 words of the right length in the default dictionary on moss and 642 possible salt strings, it is not feasible to pre-calculate and store all possible salt+word combinations. Your program is expected to use the brute-force method described above, trading CPU work for memory storage.
"""