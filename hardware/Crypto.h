#ifndef CRYPTO_H
#define CRYPTO_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "hardware/crypto/AES.h"
#include "hardware/crypto/HMAC_MD5.h"
#include "hardware/LED.h"
#include "hardware/SD.h"

#define PATH_ENTROPY_INDEX "OpenModem/entropy.index"
#define PATH_ENTROPY_SOURCE "OpenModem/entropy.source"
#define PATH_AES_128_KEY "OpenModem/aes128.key"
#define PATH_CRYPTO_DISABLE "OpenModem/aes128.disable"

#define CRYPTO_KEY_SIZE_BITS 128
#define CRYPTO_KEY_SIZE (CRYPTO_KEY_SIZE_BITS/8)
#define CRYPTO_HMAC_SIZE_BITS 128
#define CRYPTO_HMAC_SIZE (CRYPTO_HMAC_SIZE_BITS/8)
#define MAX_IVS_PER_ENTROPY_BLOCK 128

#define CRYPTO_WAIT_TIMEOUT_MS 2000

uint8_t crypto_work_block[CRYPTO_KEY_SIZE];

void crypto_init(void);
bool crypto_wait(void);

bool crypto_enabled(void);
bool crypto_generate_iv(void);
uint8_t* crypto_get_iv(void);
void crypto_set_iv_from_workblock(void);
void crypto_generate_hmac(uint8_t *data, size_t length);
void crypto_prepare(void);

void crypto_encrypt_block(uint8_t block[CRYPTO_KEY_SIZE]);
void crypto_decrypt_block(uint8_t block[CRYPTO_KEY_SIZE]);

void crypto_test(void);

bool should_disable_enryption(void);
bool load_key(void);
bool load_entropy(void);
bool load_entropy_index(void);

#endif

