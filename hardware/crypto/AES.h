/*
MIT License

Copyright (c) 2016 Andrew Carter

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef AES_H
#define AES_H

#include <stdint.h>

#define AES_256_ROUNDS 14
#define AES_192_ROUNDS 12
#define AES_128_ROUNDS 10

// Uncomment this (or compile with -DOPT_8_BIT) to optimise for an 8 bit architecture
#define AES_OPT_8_BIT

#ifdef AES_OPT_8_BIT
  typedef uint8_t counter;
#else
  typedef unsigned int counter;
#endif

// AES-256

typedef struct aes_256_context_t_ {
  uint8_t round_key[(AES_256_ROUNDS + 1) * 16];
} aes_256_context_t;

void aes_256_init    (aes_256_context_t *context, uint8_t key[32]);
void aes_256_encrypt (aes_256_context_t *context, uint8_t block[16]);
void aes_256_decrypt (aes_256_context_t *context, uint8_t block[16]);

// AES-192

typedef struct aes_192_context_t_ {
  uint8_t round_key[(AES_192_ROUNDS + 1) * 16];
} aes_192_context_t;

void aes_192_init    (aes_192_context_t *context, uint8_t key[24]);
void aes_192_encrypt (aes_192_context_t *context, uint8_t block[16]);
void aes_192_decrypt (aes_192_context_t *context, uint8_t block[16]);

// AES-128

typedef struct aes_128_context_t_ {
  uint8_t round_key[(AES_128_ROUNDS + 1) * 16];
} aes_128_context_t;

void aes_128_init    (aes_128_context_t *context, uint8_t key[16]);
void aes_128_encrypt (aes_128_context_t *context, uint8_t block[16]);
void aes_128_decrypt (aes_128_context_t *context, uint8_t block[16]);

#endif

