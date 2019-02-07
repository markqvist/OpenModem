#include "Crypto.h"

bool encryption_enabled = false;
uint8_t active_key[CRYPTO_KEY_SIZE];
uint8_t active_iv[CRYPTO_KEY_SIZE];

aes_128_context_t context;
uint8_t current_vector[CRYPTO_KEY_SIZE];
uint32_t entropy;
uint32_t entropy_index = 0;
bool entropy_loaded = false;
uint8_t ivs_generated = 0;

FIL crypto_fp;						// File buffer
char crypto_fb[CRYPTO_KEY_SIZE];	// File read buffer
FRESULT crypto_fr;					// Result codes

void crypto_init(void) {
	encryption_enabled = false;

	if (load_key()) {
		if (load_entropy_index() && load_entropy()) {
			encryption_enabled = true;
		}
	}

	if (encryption_enabled) {
		// TODO: Set flags for crypto enabled

		// TODO: Remove
		// for (uint8_t i = 0; i < 130; i++) {
		// 	crypto_test();
		// }
	} else {
		LED_indicate_error_crypto();
	}
}

void crypto_prepare(uint8_t key[CRYPTO_KEY_SIZE], uint8_t initialization_vector[CRYPTO_KEY_SIZE]) {
  // Initialise the context with the key
  aes_128_init(&context, key);

  // Copy the IV into the current vector array
  memcpy(current_vector, initialization_vector, CRYPTO_KEY_SIZE);
}

void crypto_encrypt_block(uint8_t block[CRYPTO_KEY_SIZE]) {
  int i;

  // XOR the current vector with the block before encrypting
  for (i = 0; i < CRYPTO_KEY_SIZE; i++) {
    block[i] ^= current_vector[i];
  }

  // Encrypt the block
  aes_128_encrypt(&context, block);

  // Copy the cipher output to the current vector
  memcpy(current_vector, block, CRYPTO_KEY_SIZE);
}

void crypto_decrypt_block(uint8_t block[CRYPTO_KEY_SIZE]) {
  uint8_t temp_vector[CRYPTO_KEY_SIZE];
  int i;

  // Copy the cipher output to the temporary vector
  memcpy(temp_vector, block, CRYPTO_KEY_SIZE);

  // Decrypt the block
  aes_128_decrypt(&context, block);

  // XOR the output with the current vector to fully decrypt
  for (i = 0; i < CRYPTO_KEY_SIZE; i++) {
    block[i] ^= current_vector[i];
  }

  // Copy the temporary vector to the current vector
  memcpy(current_vector, temp_vector, CRYPTO_KEY_SIZE);
}

bool load_entropy_index(void) {
	if (sd_mounted()) {
		crypto_fr = f_open(&crypto_fp, PATH_ENTROPY_INDEX, FA_READ);
		if (crypto_fr == FR_NO_FILE) {
			//printf("Entropy index file does not exist\r\n");
			f_close(&crypto_fp);
			crypto_fr = f_open(&crypto_fp, PATH_ENTROPY_INDEX, FA_CREATE_NEW | FA_WRITE);

			if (crypto_fr == FR_OK) {
				entropy_index = 0x00000000;
				memcpy(crypto_fb, &entropy_index, sizeof(entropy_index));
				
				UINT written = 0;
				crypto_fr = f_write(&crypto_fp, crypto_fb, sizeof(entropy_index), &written);
				f_close(&crypto_fp);
				
				if (crypto_fr == FR_OK && written == sizeof(entropy_index)) {
					//printf("Wrote new index to index file\r\n");
				} else {
					//printf("Could not write index to index file\r\n");
				}
			} else {
				//printf("Could not create index file\r\n");
			}

			crypto_fr = f_open(&crypto_fp, PATH_ENTROPY_INDEX, FA_READ);
		}

		if (crypto_fr == FR_OK) {
			//printf("Opened entropy index file\r\n");
			UINT read = 0;
			crypto_fr = f_read(&crypto_fp, crypto_fb, sizeof(entropy_index), &read);
			f_close(&crypto_fp);
			if (crypto_fr == FR_OK && read == sizeof(entropy_index)) {
				memcpy(&entropy_index, crypto_fb, sizeof(entropy_index));
				//printf("Entropy index is now: %lX\r\n", entropy_index);
				return true;
			}
		} else {
			//printf("Error opening entropy index file\r\n");
		}

	}

	f_close(&crypto_fp);
	return false;
}

bool update_entropy_index(void) {
	crypto_fr = f_open(&crypto_fp, PATH_ENTROPY_INDEX, FA_WRITE);
	if (crypto_fr == FR_OK) {
		entropy_index += sizeof(entropy);
		memcpy(crypto_fb, &entropy_index, sizeof(entropy_index));

		UINT written = 0;
		crypto_fr = f_write(&crypto_fp, crypto_fb, sizeof(entropy_index), &written);

		if (crypto_fr == FR_OK && written == sizeof(entropy_index)) {
			return true;
		}
	}
	return false;
}

bool load_entropy(void) {
	if (sd_mounted()) {
		if (update_entropy_index()) {
			crypto_fr = f_open(&crypto_fp, PATH_ENTROPY_SOURCE, FA_READ);
			if (crypto_fr == FR_OK) {
				uint32_t fsize = f_size(&crypto_fp);
				//uint32_t fpoint = crypto_fp.fptr;

				//printf("Opened entropy file\r\n\tSize is %lu\r\n\tPointer is at %lX \r\n\tSeeking to index: %lX\r\n", fsize, fpoint, entropy_index);

				crypto_fr = f_lseek(&crypto_fp, entropy_index);

				//fpoint = crypto_fp.fptr;
				//printf("After seek, pointer is now at %lX\r\n", fpoint);

				if (crypto_fr == FR_OK && crypto_fp.fptr < fsize-sizeof(entropy)) {
					UINT read = 0;
					crypto_fr = f_read(&crypto_fp, crypto_fb, sizeof(entropy), &read);
					f_close(&crypto_fp);

					if (crypto_fr == FR_OK) {
						memcpy(&entropy, crypto_fb, sizeof(entropy));
						//printf("Read entropy from SD: %lX\r\n", entropy);
						srandom(entropy);
						entropy_loaded = true;
						ivs_generated = 0;
						return true;
					} else {
						//printf("Could not read entropy data from SD\r\n");
					}

				} else {
					f_close(&crypto_fp);
					//printf("Could not seek in index file, entropy exhausted\r\n");
					LED_indicate_error_crypto();
				}
			}
		}
	}

	f_close(&crypto_fp);
	return false;
}

bool load_key(void) {
	if (sd_mounted()) {
		crypto_fr = f_open(&crypto_fp, PATH_AES_128_KEY, FA_READ);
		if (crypto_fr == FR_OK) {
			//printf("File open\r\n");
			UINT read = 0;
			crypto_fr = f_read(&crypto_fp, crypto_fb, CRYPTO_KEY_SIZE, &read);
			f_close(&crypto_fp);

			if (crypto_fr == FR_OK && read == CRYPTO_KEY_SIZE) {
				//printf("Loaded AES-128 Key: ");
				for (uint8_t i = 0; i < 16; i++) {
					active_key[i] = crypto_fb[i];
					//printf("%X ", crypto_fb[i]);
				}
				//printf("\r\n");	
				return true;
			} else {
				//printf("Error %d reading file, read %d bytes.\r\n", crypto_fr, read);
			}
		} else {
			//printf("Could not open file\r\n");
		}
	} else {
		//printf("SD not mounted\r\n");
	}

	return false;
}

bool generate_iv(void) {
	if (entropy_loaded) {
		for (uint8_t i = 0; i < 16; i++) {
			active_iv[i] = (uint8_t)random();
		}
		ivs_generated++;

		// TODO: remove
		/*printf("Generated IV: ");
		for (uint8_t i = 0; i < 16; i++) {
			printf("%X ", active_iv[i]);
		}
		printf("\r\n");*/

		if (ivs_generated >= MAX_IVS_PER_ENTROPY_BLOCK) {
			load_entropy();
		}

		return true;
	} else {
		return false;
	}
}

// TODO: Remove this
void crypto_test(void) {
	generate_iv();

	uint8_t work_block[16];
	memset(work_block, 0x70, 16);
	work_block[15] = 0x00;

	//printf("Work block plaintext:  ===%s===\r\n", work_block);
	
	crypto_prepare(active_key, active_iv);
	crypto_encrypt_block(work_block);
	//printf("Work block ciphertext: ===%s===\r\n", work_block);

	crypto_prepare(active_key, active_iv);
	crypto_decrypt_block(work_block);
	printf("Work block plaintext:  ===%s===\r\n", work_block);

}

// TODO: test entropy exhaustion