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

#ifndef _FUNCTIONS_
#define _FUNCTIONS_

#include <stdint.h>
#include "crypto.h"
#include "definitions.h"

void init_context(void);
context_t *get_context(void);
void phrase_to_seed(const char *phrase, const uint32_t phrase_size, uint8_t *out_seed32);
void seed_to_kdf(const uint8_t *seed, size_t n, uint8_t *out_gen32, secp256k1_scalar *out_cof);
void generate_hash_id(uint64_t idx, uint32_t type, uint32_t sub_idx, uint8_t *out32);
void derive_key(const uint8_t *parent, uint8_t parent_size,
                const uint8_t *hash_id, uint8_t id_size, const secp256k1_scalar *cof_sk,
                secp256k1_scalar *out_sk);
void derive_pkey(const uint8_t *parent, uint8_t parent_size,
                 const uint8_t *hash_id, uint8_t id_size, secp256k1_scalar *out_sk);
void sk_to_pk(secp256k1_scalar *sk, const secp256k1_gej *generator_pts, uint8_t *out32);
void get_child_kdf(const uint8_t *parent_secret_32, const secp256k1_scalar *parent_cof,
                   uint32_t index, uint8_t *out32_child_secret,
                   secp256k1_scalar *out_child_cof);
uint32_t get_owner_key(const uint8_t *master_key, const secp256k1_scalar *master_cof,
                       const uint8_t *secret, size_t secret_size, uint8_t *out);
void get_HKdf(uint32_t index, const uint8_t *seed, HKdf_t *hkdf);

#endif //_FUNCTIONS_