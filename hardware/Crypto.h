#ifndef CRYPTO_H
#define CRYPTO_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "hardware/crypto/aes.h"
#include "hardware/LED.h"
#include "hardware/SD.h"

#define PATH_ENTROPY_INDEX "OpenModem/entropy.index"
#define PATH_ENTROPY_SOURCE "OpenModem/entropy.source"
#define PATH_AES_128_KEY "OpenModem/aes128.key"

#define CRYPTO_KEY_SIZE_BITS 128
#define CRYPTO_KEY_SIZE (CRYPTO_KEY_SIZE_BITS/8)
#define MAX_IVS_PER_ENTROPY_BLOCK 128

void crypto_init(void);
void crypto_test(void);

bool load_key(void);
bool load_entropy(void);
bool load_entropy_index(void);

#endif

