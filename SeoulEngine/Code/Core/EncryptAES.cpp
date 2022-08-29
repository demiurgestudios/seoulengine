/**
 * \file EncryptAES.cpp
 * \brief Implements AES encryption (variable key sizes,
 * CFB algorithm). All encryption is performed in place.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EncryptAES.h"
#include "SecureRandom.h"
extern "C"
{
#	include "WjCryptLib_AesCfb.h"
#	include "WjCryptLib_Sha512.h"
}

namespace Seoul::EncryptAES
{

/** Initialize a Nonce (once-number) for use with EncryptInPlace(). */
void InitializeNonceForEncrypt(UInt8 aNonce[kEncryptionNonceLength])
{
	memset(aNonce, 0, kEncryptionNonceLength);
	SecureRandom::GetBytes(aNonce, kEncryptionNonceLength);
}

template <int FUNC(AesCfbContext*, void const*, void*, uint32_t)>
static void DecryptEncryptCommon(
	void* pData,
	UInt32 uDataSizeInBytes,
	UInt8 const* pKey,
	UInt32 uKeyLengthInBytes,
	UInt8 const aNonce[kEncryptionNonceLength])
{
	SEOUL_STATIC_ASSERT(kEncryptionNonceLength == AES_BLOCK_SIZE);
	SEOUL_STATIC_ASSERT(kEncryptionNonceLength == AES_CFB_IV_SIZE);

	AesCfbContext ctxt;
	SEOUL_VERIFY(0 == ::AesCfbInitialiseWithKey(&ctxt, pKey, uKeyLengthInBytes, aNonce));

	// WjCryptLib requires multiples of the block size, so we handle that here.
	if (0 != uDataSizeInBytes % kEncryptionNonceLength)
	{
		const UInt32 uReadSize = (uDataSizeInBytes > kEncryptionNonceLength
			? ((uDataSizeInBytes / kEncryptionNonceLength) * kEncryptionNonceLength)
			: 0u);
		const UInt32 uRemainingSize = (uDataSizeInBytes - uReadSize);
		UInt8 aBlock[kEncryptionNonceLength];

		// If we have more than 1 block of data, process everything but the last
		// chunk.
		if (uReadSize > 0u)
		{
			SEOUL_VERIFY(0 == FUNC(&ctxt, pData, pData, uReadSize));
		}

		// Now fill the last chunk into a temporary block and process it.
		memset(aBlock, 0, sizeof(aBlock));
		memcpy(aBlock, (UInt8 const*)pData + uReadSize, uRemainingSize);
		SEOUL_VERIFY(0 == FUNC(&ctxt, aBlock, aBlock, sizeof(aBlock)));
		memcpy((UInt8*)pData + uReadSize, aBlock, uRemainingSize);
	}
	else
	{
		SEOUL_VERIFY(0 == FUNC(&ctxt, pData, pData, uDataSizeInBytes));
	}
}

/**
 * Decrypt pData with pKey from/to pData. Always succeeds,
 * the caller must embed additional information (such as a checksum
 * using SHA512Digest) to verify the integrity of the unencrypted
 * data.
 */
void DecryptInPlace(
	void* pData,
	UInt32 uDataSizeInBytes,
	UInt8 const* pKey,
	UInt32 uKeyLengthInBytes,
	UInt8 const aNonce[kEncryptionNonceLength])
{
	DecryptEncryptCommon<&::AesCfbDecrypt>(pData, uDataSizeInBytes, pKey, uKeyLengthInBytes, aNonce);
}

/**
 * Encrypt pData with pKey from/to pData. Always succeeds.
 * aNonce should be initialized with InitializeNonceForEncrypt().
 */
void EncryptInPlace(
	void* pData,
	UInt32 uDataSizeInBytes,
	UInt8 const* pKey,
	UInt32 uKeyLengthInBytes,
	UInt8 const aNonce[kEncryptionNonceLength])
{
	DecryptEncryptCommon<&::AesCfbEncrypt>(pData, uDataSizeInBytes, pKey, uKeyLengthInBytes, aNonce);
}

/*
 * Generate a SHA512 digest/checksum of pData - typically used
 * to verify the internal integrity of encrypted data after
 * decryption.
 */
void SHA512Digest(void const* pData, UInt32 uDataSizeInBytes, UInt8 aDigest[kSHA512DigestLength])
{
	SEOUL_STATIC_ASSERT(sizeof(SHA512_HASH) == kSHA512DigestLength);

	Sha512Context ctxt;
	::Sha512Initialise(&ctxt);
	::Sha512Update(&ctxt, pData, uDataSizeInBytes);
	::Sha512Finalise(&ctxt, (SHA512_HASH*)&aDigest[0]);
}

} // namespace Seoul::EncryptAES
