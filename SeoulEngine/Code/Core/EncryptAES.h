/**
 * \file EncryptAES.h
 * \brief Implements AES encryption (variable key sizes,
 * CFB algorithm). All encryption is performed in place.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ENCRYPT_AES_H
#define ENCRYPT_AES_H

#include "Prereqs.h"

namespace Seoul::EncryptAES
{

/** Size in bytes of the decryption/encryption nonce (once-number). */
static const size_t kEncryptionNonceLength = 16;

/** Size in bytes of the SHA512 digest/checksum. */
static const size_t kSHA512DigestLength = 64;

// Initialize a Nonce (once-number) for use
// with EncryptInPlace().
void InitializeNonceForEncrypt(UInt8 aNonce[kEncryptionNonceLength]);

// Decrypt pData with pKey from/to pData. Always succeeds,
// the caller must embed additional information (such as a checksum
// using SHA512Digest) to verify the integrity of the unencrypted
// data.
void DecryptInPlace(
	void* pData,
	UInt32 uDataSizeInBytes,
	UInt8 const* pKey,
	UInt32 uKeyLengthInBytes,
	UInt8 const aNonce[kEncryptionNonceLength]);

// Encrypt pData with pKey from/to pData. Always succeeds.
// aNonce should be initialized with InitializeNonceForEncrypt().
void EncryptInPlace(
	void* pData,
	UInt32 uDataSizeInBytes,
	UInt8 const* pKey,
	UInt32 uKeyLengthInBytes,
	UInt8 const aNonce[kEncryptionNonceLength]);

// TODO: Refactor this into a general purpose hashing class, like SeoulMD5.
//
// Generate a SHA512 digest/checksum of pData - typically used
// to verify the internal integrity of encrypted data after
// decryption.
void SHA512Digest(void const* pData, UInt32 uDataSizeInBytes, UInt8 aDigest[kSHA512DigestLength]);

} // namespace Seoul::EncryptAES

#endif // include guard
