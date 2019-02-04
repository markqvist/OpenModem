#include "Crypto.h"

uint8_t active_key[16];
uint8_t active_iv[16];

aes_128_context_t context;
uint8_t current_vector[16];

void crypto_init(void) {
	crypto_test();
	return;
}

void crypto_prepare(uint8_t key[16], uint8_t initialization_vector[16]) {
  // Initialise the context with the key
  aes_128_init(&context, key);

  // Copy the IV into the current vector array
  memcpy(current_vector, initialization_vector, 16);
}

void crypto_encrypt_block(uint8_t block[16]) {
  int i;

  // XOR the current vector with the block before encrypting
  for (i = 0; i < 16; i++) {
    block[i] ^= current_vector[i];
  }

  // Encrypt the block
  aes_128_encrypt(&context, block);

  // Copy the cipher output to the current vector
  memcpy(current_vector, block, 16);
}

void crypto_decrypt_block(uint8_t block[16]) {
  uint8_t temp_vector[16];
  int i;

  // Copy the cipher output to the temporary vector
  memcpy(temp_vector, block, 16);

  // Decrypt the block
  aes_128_decrypt(&context, block);

  // XOR the output with the current vector to fully decrypt
  for (i = 0; i < 16; i++) {
    block[i] ^= current_vector[i];
  }

  // Copy the temporary vector to the current vector
  memcpy(current_vector, temp_vector, 16);
}

void load_key(void) {
	// TODO: implement
}

void generate_iv(void) {
	// TODO: implement
}

void crypto_test(void) {
	
}