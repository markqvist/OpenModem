#include "Crypto.h"
#include "util/Config.h"

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

	if (should_disable_enryption()) {
		if (config_crypto_lock) config_crypto_lock_disable();
	} else {
		if (load_key()) {
			if (load_entropy_index() && load_entropy()) {
				config_crypto_lock_enable();
				encryption_enabled = true;
			}
		}

		if (config_crypto_lock) {
			if (encryption_enabled) {
				LED_indicate_enabled_crypto();
			} else {
				LED_indicate_error_crypto();
			}
		}
	}
}

bool crypto_wait(void) {
	size_t wait_timer = 0;
	size_t interval_ms = 100;
	while (!crypto_enabled()) {
	    delay_ms(100);
	    wait_timer++;
	    sd_jobs();
	    if (wait_timer*interval_ms > CRYPTO_WAIT_TIMEOUT_MS) {
	    	return false;
	    }
	}

	return true;
}

void crypto_generate_hmac(uint8_t *data, size_t length) {
	hmac_md5(crypto_work_block, active_key, CRYPTO_KEY_SIZE_BITS, data, length*8);
}

bool crypto_enabled(void) {
	return encryption_enabled;
}

void crypto_prepare(void) {
  // Initialise the context with the key
  aes_128_init(&context, active_key);

  // Copy the IV into the current vector array
  memcpy(current_vector, active_iv, CRYPTO_KEY_SIZE);
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
			}

			crypto_fr = f_open(&crypto_fp, PATH_ENTROPY_INDEX, FA_READ);
		}

		if (crypto_fr == FR_OK) {
			UINT read = 0;
			crypto_fr = f_read(&crypto_fp, crypto_fb, sizeof(entropy_index), &read);
			f_close(&crypto_fp);
			if (crypto_fr == FR_OK && read == sizeof(entropy_index)) {
				memcpy(&entropy_index, crypto_fb, sizeof(entropy_index));
				return true;
			}
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
		f_close(&crypto_fp);
		
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
				
				crypto_fr = f_lseek(&crypto_fp, entropy_index);
				if (crypto_fr == FR_OK && crypto_fp.fptr < fsize-sizeof(entropy)) {
					UINT read = 0;
					crypto_fr = f_read(&crypto_fp, crypto_fb, sizeof(entropy), &read);
					f_close(&crypto_fp);

					if (crypto_fr == FR_OK) {
						memcpy(&entropy, crypto_fb, sizeof(entropy));
						srandom(entropy);
						entropy_loaded = true;
						ivs_generated = 0;
						return true;
					}

				} else {
					f_close(&crypto_fp);
					LED_indicate_error_crypto();
				}
			}
		}
	}

	f_close(&crypto_fp);
	return false;
}

bool should_disable_enryption(void) {
	if (sd_mounted()) {
		crypto_fr = f_open(&crypto_fp, PATH_CRYPTO_DISABLE, FA_READ);
		if (crypto_fr == FR_OK) {
			f_close(&crypto_fp);

			return true;
		}
	}

	return false;
}

bool load_key(void) {
	if (sd_mounted()) {
		crypto_fr = f_open(&crypto_fp, PATH_AES_128_KEY, FA_READ);
		if (crypto_fr == FR_OK) {
			UINT read = 0;
			crypto_fr = f_read(&crypto_fp, crypto_fb, CRYPTO_KEY_SIZE, &read);
			f_close(&crypto_fp);

			if (crypto_fr == FR_OK && read == CRYPTO_KEY_SIZE) {
				for (uint8_t i = 0; i < 16; i++) {
					active_key[i] = crypto_fb[i];
				}

				return true;
			}
		}
	}

	return false;
}

bool crypto_generate_iv(void) {
	if (entropy_loaded) {
		for (uint8_t i = 0; i < 16; i++) {
			active_iv[i] = (uint8_t)random();
		}
		ivs_generated++;

		if (ivs_generated >= MAX_IVS_PER_ENTROPY_BLOCK) {
			load_entropy();
		}

		return true;
	} else {
		return false;
	}
}

uint8_t *crypto_get_iv(void) {
	return active_iv;
}

void crypto_set_iv_from_workblock(void) {
	memcpy(active_iv, crypto_work_block, CRYPTO_KEY_SIZE);
}

// TODO: test entropy exhaustion