// Copyright 2019 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.#include "misc.h"

#include "crypto.h"
#include "internal.h"
#if defined (TREZOR_CRYPTO_MOD)
#include "crypto_trezor/rand.h"
#include "crypto_trezor/sha2.h"
#include "crypto_trezor/memzero.h"
#elif defined (LEDGER_SDK)
//TODO:
#elif defined (DESKTOP_SDK)
#   include "pkcs5_pbkdf2.h"
    // TODO
#else
#error "Include your rand, memzero, AES, SHA256, HMAC, etc. files!"
#endif

#ifdef DESKTOP_SDK
#   if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#   	pragma GCC diagnostic push
#   	pragma GCC diagnostic ignored "-Wunused-function"
#   else
#   	pragma warning (push, 0) // suppress warnings from secp256k1
#   	pragma warning (disable: 4706 4701) // assignment within conditional expression
#   endif

#undef SECP256K1_RESTRICT
#define SECP256K1_RESTRICT

#   include "secp256k1-zkp/src/group_impl.h"
#   include "secp256k1-zkp/src/scalar_impl.h"
#   include "secp256k1-zkp/src/field_impl.h"
#   include "secp256k1-zkp/src/hash_impl.h"

#   if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#   	pragma GCC diagnostic pop
#   else
#   	pragma warning (default: 4706 4701)
#   	pragma warning (pop)
#   endif
#endif // DESKTOP_SDK

void beam_gej_to_pxy(const secp256k1_gej *gej, pxy_t *pxy)
{
  secp256k1_gej pt = *gej;
  secp256k1_ge ge;
  secp256k1_ge_set_gej(&ge, &pt);

  // seems like normalization can be omitted (already done by
  // secp256k1_ge_set_gej), but not guaranteed according to docs.
  // But this has a negligible impact on the performance
  secp256k1_fe_normalize(&ge.x);
  secp256k1_fe_normalize(&ge.y);

  pxy->margin = 0x04;
  secp256k1_fe_get_b32(pxy->x, &ge.x);
  secp256k1_fe_get_b32(pxy->y, &ge.y);
}

void beam_pxy_to_point(const pxy_t *pxy, point_t *pt)
{
  os_memcpy(pt->x, pxy->x, DIGEST_LENGTH);
  secp256k1_fe y;
  secp256k1_fe_set_b32(&y, pxy->y);
  pt->y = (secp256k1_fe_is_odd(&y) != 0);
}

void beam_pxy_to_gej(const pxy_t *pxy, secp256k1_gej *pt)
{
  secp256k1_ge ge;
  secp256k1_fe_set_b32(&ge.x, pxy->x);
  secp256k1_fe_set_b32(&ge.y, pxy->y);
  ge.infinity = 0;
  secp256k1_gej_set_ge(pt, &ge);
}

void beam_pxy_mul_scalar(pxy_t *pxy, const secp256k1_scalar *sk)
{
#if defined (LEDGER_SDK)
  uint8_t scalar[DIGEST_LENGTH];
  secp256k1_scalar_get_b32(scalar, sk);
  cx_ecfp_scalar_mult(CX_CURVE_SECP256K1, (uint8_t*)pxy, sizeof(pxy_t), scalar, DIGEST_LENGTH);
#else
  secp256k1_gej pt;
  beam_pxy_to_gej(pxy, &pt);
  gej_mul_scalar(&pt, sk, &pt);
  beam_gej_to_pxy(&pt, pxy);
#endif // LEDGER_SDK | ..
}

void beam_pbkdf2_sha512(const uint8_t *password, unsigned short password_len, uint8_t *salt,
                        unsigned short salt_len, unsigned int num_iterations, uint8_t *out_hash, unsigned int out_hash_len)
{

#if defined (LEDGER_SDK)
  cx_pbkdf2_sha512(password, password_len, salt, salt_len, num_iterations, out_hash, out_hash_len);
#elif defined (TREZOR_CRYPTO_MOD)
  #include "pbkdf2.h"
  pbkdf2_hmac_sha512(password, password_len, salt, salt_len, num_iterations, out_hash, out_hash_len);
#elif defined (DESKTOP_SDK)

    int nRes = pkcs5_pbkdf2(
        password,
        password_len,
        salt,
        salt_len,
        out_hash,
        out_hash_len,
        num_iterations);

    if (nRes)
        __debugbreak();

#else
  #error "Add your own implementation of pbkdf2_sha512 here!"
#endif // LEDGER_SDK | TREZOR_CRYPTO_MOD | ..
}

void beam_hash_sha256_write_8(beam_sha256_ctx *hasher, uint8_t b)
{
  beam_hash_sha256_update(hasher, &b, sizeof(b));
}

void beam_hash_sha256_write_64(beam_sha256_ctx *hasher, uint64_t v)
{
  for (; v >= 0x80; v >>= 7) {
    beam_hash_sha256_write_8(hasher, (uint8_t)((uint8_t)v | 0x80));
  }

  beam_hash_sha256_write_8(hasher, (uint8_t)v);
}

void beam_hash_sha256_init(beam_sha256_ctx *hasher)
{
#if defined (LEDGER_SDK)
  cx_sha256_init(hasher);
#elif defined (TREZOR_CRYPTO_MOD)
  sha256_Init(hasher);
#elif defined (DESKTOP_SDK)
    secp256k1_sha256_initialize(hasher);
#else
  #error "Add your own implementation of sha256 here!"
#endif // LEDGER_SDK | TREZOR_CRYPTO_MOD | ..
}

void beam_hash_sha256_update(beam_sha256_ctx *hasher, const uint8_t *buf, unsigned int len)
{
#if defined (LEDGER_SDK)
  cx_hash((cx_hash_t *)hasher, 0, buf, len, NULL, 0);
#elif defined (TREZOR_CRYPTO_MOD)
  sha256_Update(hasher, buf, len);
#elif defined (DESKTOP_SDK)
    secp256k1_sha256_write(hasher, buf, len);
#else
  #error "Add your own implementation of sha256 here!"
#endif // LEDGER_SDK | TREZOR_CRYPTO_MOD | ..
}

int beam_hash_sha256_final(beam_sha256_ctx *hasher, uint8_t *out)
{
#if defined (LEDGER_SDK)
  return cx_hash((cx_hash_t *)hasher, CX_LAST, NULL, 0, out, DIGEST_LENGTH);
#elif defined (TREZOR_CRYPTO_MOD)
  sha256_Final(hasher, out);
  return 0;
#elif defined (DESKTOP_SDK)
    secp256k1_sha256_finalize(hasher, out);
    return 0;
#else
  #error "Add your own implementation of sha256 here!"
#endif // LEDGER_SDK | TREZOR_CRYPTO_MOD | ..
}

void beam_hash_hmac_sha256_init(beam_hmac_sha256_ctx *hasher, const uint8_t *key, const uint32_t keylen)
{
#if defined (LEDGER_SDK)
  cx_hmac_sha256_init(hasher, key, keylen);
#elif defined (TREZOR_CRYPTO_MOD)
  hmac_sha256_Init(hasher, key, keylen);
#elif defined (DESKTOP_SDK)
    secp256k1_hmac_sha256_initialize(hasher, key, keylen);
#else
  #error "Add your own implementation of hmac_sha256 here!"
#endif // LEDGER_SDK | TREZOR_CRYPTO_MOD | ..
}

void beam_hash_hmac_sha256_update(beam_hmac_sha256_ctx *hasher, const uint8_t *buf, unsigned int len)
{
#if defined (LEDGER_SDK)
  cx_hmac((cx_hash_t *)hasher, 0, buf, len, NULL, 0);
#elif defined (TREZOR_CRYPTO_MOD)
  hmac_sha256_Update(hasher, buf, len);
#elif defined (DESKTOP_SDK)
    secp256k1_hmac_sha256_write(hasher, buf, len);
#else
  #error "Add your own implementation of hmac_sha256 here!"
#endif // LEDGER_SDK | TREZOR_CRYPTO_MOD | ..
}

int beam_hash_hmac_sha256_final(beam_hmac_sha256_ctx *hasher, uint8_t *out)
{
#if defined (LEDGER_SDK)
  return cx_hmac((cx_hash_t *)hasher, CX_LAST, NULL, 0, out, DIGEST_LENGTH);
#elif defined (TREZOR_CRYPTO_MOD)
  hmac_sha256_Final(hasher, out);
  return 0;
#elif defined (DESKTOP_SDK)
    secp256k1_hmac_sha256_finalize(hasher, out);
    return 0;
#else
  #error "Add your own implementation of hmac_sha256 here!"
#endif // LEDGER_SDK | TREZOR_CRYPTO_MOD | ..
}

void beam_get_private_key_data(uint8_t *data)
{
  uint32_t HARDENED_OFFSET = 0x80000000;
  uint32_t derivePath[BIP32_PATH] = {
      44 | HARDENED_OFFSET,
      1533 | HARDENED_OFFSET,
      0 | HARDENED_OFFSET,
      0 | HARDENED_OFFSET,
      0 | HARDENED_OFFSET};

#if defined (LEDGER_SDK)
  os_perso_derive_node_bip32(CX_CURVE_SECP256K1, derivePath, BIP32_PATH, data, NULL);
#else
  //TODO: use derive Path
  UNUSED(derivePath);
  // Test data
  os_memset(data, 3, DIGEST_LENGTH);
#endif // LEDGER_SDK | ..
}

void beam_aes_init(beam_aes_ctx* ctx, const uint8_t* key32)
{
#if defined (LEDGER_NATIVE_CRYPT)
  cx_aes_init_key(key32, DIGEST_LENGTH, ctx);
#elif defined (LEDGER_SDK) || defined (TREZOR_CRYPTO_MOD) || defined (DESKTOP_SDK)
  aes_encrypt_key256(key32, ctx);
#else
  #error "Add your own implementation of AES here!"
#endif // LEDGER_NATIVE_CRYPT | TREZOR_CRYPTO_MOD | ..
}

void beam_aes_encrypt(const beam_aes_ctx *ctx, const uint8_t *iv16, const uint8_t *in, uint8_t *out, uint32_t len)
{
#if defined(LEDGER_NATIVE_CRYPT)
  cx_aes_iv(ctx, CX_ENCRYPT | CX_CHAIN_CTR | CX_LAST | CX_PAD_NONE,
            iv16, 16, in, len, out, len);
#elif defined (LEDGER_SDK) || defined (TREZOR_CRYPTO_MOD) || defined (DESKTOP_SDK)
  aes_ctr_encrypt(in, out, len, (uint8_t*)iv16, aes_ctr_cbuf_inc, (beam_aes_ctx*)ctx);
#else
  #error "Add your own implementation of AES here!"
#endif // LEDGER_NATIVE_CRYPT | TREZOR_CRYPTO_MOD | ..
}

void beam_rng(uint8_t* dest, uint32_t len)
{
#if defined(DESKTOP_SDK) || defined(BEAM_DEBUG)
    memset(dest, 0, len);
#elif defined (LEDGER_SDK)
  cx_rng(dest, len);
#elif defined (TREZOR_CRYPTO_MOD)
  #include "crypto_trezor/rand.h"
  random_buffer(dest, len);
#else
  #error "Add your own implementation of random module here!"
#endif // LEDGER_SDK | TREZOR_CRYPTO_MOD | ..
}