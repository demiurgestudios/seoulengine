/**
 * \file SaveLoadUtil.cpp
 * \brief Shared utility used for saving, converts a DataStore
 * to/from base64 encoded data with a checksum MD5.
 *
 * Implemented in Core, so it can be easily shared
 * client and server.
 *
 * This is our "wire" and cloud format. Local saves use a different
 * container format, defined in SaveLoadManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "DataStore.h"
#include "SaveLoadUtil.h"
#include "SeoulCrc32.h"
#include "SeoulFile.h"
#include "SeoulFileReaders.h"
#include "SeoulFileWriters.h"
#include "StringUtil.h"

namespace Seoul::SaveLoadUtil
{

// TODO: Implement big endian support.
SEOUL_STATIC_ASSERT(SEOUL_LITTLE_ENDIAN);

/** Encoding signature of our wire format. */
static const UInt32 kuEncodingSignature = 0x2ed2fc70;

/** Encoding version of our wire format. */
static const UInt32 kuEncodingVersion = 3u;

/** @return A Crc32 code for the members of this SaveFileMetadata. */
UInt32 SaveFileMetadata::ComputeCRC32() const
{
	UInt32 uReturn = 0u;
	uReturn = GetCrc32(uReturn, (UInt8 const*)m_sSessionGuid.CStr(), m_sSessionGuid.GetSize());
	uReturn = GetCrc32(uReturn, (UInt8 const*)&m_iTransactionIdMin, sizeof(m_iTransactionIdMin));
	uReturn = GetCrc32(uReturn, (UInt8 const*)&m_iTransactionIdMax, sizeof(m_iTransactionIdMax));
	uReturn = GetCrc32(uReturn, (UInt8 const*)&m_iVersion, sizeof(m_iVersion));
	return uReturn;
}

SaveLoadResult FromBlob(void const* pIn, UInt32 uInputSizeInBytes, SaveFileMetadata& rMetadata, DataStore& rSaveData)
{
	Byte const* const pInBytes = (Byte const*)pIn;

	// Get uncompressed size - it is a "footer", at the end of the
	// data blob.
	UInt32 uUncompressedSize = 0u;
	if (uInputSizeInBytes < sizeof(uUncompressedSize))
	{
		return SaveLoadResult::kErrorTooSmall;
	}
	memcpy(&uUncompressedSize, pInBytes + uInputSizeInBytes - sizeof(uUncompressedSize), sizeof(uUncompressedSize));

	// Sanity check size.
	if (uUncompressedSize > kuMaxDataSizeInBytes)
	{
		return SaveLoadResult::kErrorTooBig;
	}

	// Allocate and decompress into the allocated buffer.
	void* p = MemoryManager::Allocate(uUncompressedSize, MemoryBudgets::Saving);
	if (!ZlibDecompress(
		pInBytes,
		uInputSizeInBytes - sizeof(uUncompressedSize),
		p,
		uUncompressedSize))
	{
		MemoryManager::Deallocate(p);
		return SaveLoadResult::kErrorCompression;
	}

	// Deserialize and load.
	FullyBufferedSyncFile file(p, uUncompressedSize, true);

	// Check the encoding signature.
	{
		UInt32 uSignature = 0u;
		if (!ReadUInt32(file, uSignature)) { return SaveLoadResult::kErrorSignatureData; }
		if (uSignature != kuEncodingSignature) { return SaveLoadResult::kErrorSignatureCheck; }
	}

	// Check the encoding version.
	{
		UInt32 uVersion = 0u;
		if (!ReadUInt32(file, uVersion)) { return SaveLoadResult::kErrorVersionData; }
		if (uVersion != kuEncodingVersion) { return SaveLoadResult::kErrorVersionCheck; }
	}

	// Read the metadata checksum.
	UInt32 uMetadataCRC32 = 0u;
	if (!ReadUInt32(file, uMetadataCRC32)) { return SaveLoadResult::kErrorChecksumData; }

	// Populate metadata.
	SaveFileMetadata metadata;
	if (!ReadString(file, metadata.m_sSessionGuid)) { return SaveLoadResult::kErrorSessionGuid; }
	if (!ReadInt64(file, metadata.m_iTransactionIdMin)) { return SaveLoadResult::kErrorTransactionIdMin; }
	if (!ReadInt64(file, metadata.m_iTransactionIdMax)) { return SaveLoadResult::kErrorTransactionIdMax; }
	if (!ReadInt32(file, metadata.m_iVersion)) { return SaveLoadResult::kErrorUserVersion; }

	// Verify.
	if (metadata.ComputeCRC32() != uMetadataCRC32) { return SaveLoadResult::kErrorChecksumCheck; }

	// Get the MD5.
	String sMD5;
	if (!ReadString(file, sMD5)) { return SaveLoadResult::kErrorMD5Data; }

	// Read the DataStore binary.
	DataStore saveData;
	if (!saveData.Load(file)) { return SaveLoadResult::kErrorSaveData; }
	if (!saveData.VerifyIntegrity()) { return SaveLoadResult::kErrorSaveCheck; }

	// MD5 check.
	if (saveData.ComputeMD5() != sMD5) { return SaveLoadResult::kErrorMD5Check; }

	// Done, success - swap out results and return.
	rMetadata = metadata;
	rSaveData.Swap(saveData);
	return SaveLoadResult::kSuccess;
}

SaveLoadResult FromBase64(const String& sData, SaveFileMetadata& rMetadata, DataStore& rSaveData)
{
	// Decode base 64.
	Vector<Byte> v;
	if (!Base64Decode(sData, v))
	{
		return SaveLoadResult::kErrorBase64;
	}

	void const* const p = (v.IsEmpty() ? nullptr : v.Data());
	UInt32 const u = v.GetSizeInBytes();

	return FromBlob(p, u, rMetadata, rSaveData);
}

static SaveLoadResult ToBlobInternal(
	const SaveFileMetadata& metadata,
	const DataStore& saveData,
	void*& rp,
	UInt32& ru,
	Bool bCompact)
{
	MemorySyncFile file;

	// Write header data.
	if (!WriteUInt32(file, kuEncodingSignature)) { return SaveLoadResult::kErrorSignatureData; }
	if (!WriteUInt32(file, kuEncodingVersion)) { return SaveLoadResult::kErrorVersionData; }
	if (!WriteUInt32(file, metadata.ComputeCRC32())) { return SaveLoadResult::kErrorChecksumData; }
	if (!WriteString(file, metadata.m_sSessionGuid)) { return SaveLoadResult::kErrorSessionGuid; }
	if (!WriteInt64(file, metadata.m_iTransactionIdMin)) { return SaveLoadResult::kErrorTransactionIdMin; }
	if (!WriteInt64(file, metadata.m_iTransactionIdMax)) { return SaveLoadResult::kErrorTransactionIdMax; }
	if (!WriteInt32(file, metadata.m_iVersion)) { return SaveLoadResult::kErrorUserVersion; }

	// Compute an MD5 of the save data.
	String const sMD5(saveData.ComputeMD5());

	// Write the md5.
	if (!WriteString(file, sMD5)) { return SaveLoadResult::kErrorMD5Data; }

	// Write the DataStore binary.
	if (!saveData.Save(file, keCurrentPlatform, bCompact))
	{
		return SaveLoadResult::kErrorSaveData;
	}

	// Compress the data using zlib.
	void* pCompressedData = nullptr;
	UInt32 uCompressedDataSizeInBytes = 0u;
	if (!ZlibCompress(
		file.GetBuffer().GetBuffer(),
		file.GetBuffer().GetTotalDataSizeInBytes(),
		pCompressedData,
		uCompressedDataSizeInBytes,
		ZlibCompressionLevel::kDefault,
		MemoryBudgets::Saving))
	{
		return SaveLoadResult::kErrorCompression;
	}

	// Update the data and append the uncompressed size.
	UInt32 const uUncompressedSize = file.GetBuffer().GetTotalDataSizeInBytes();
	pCompressedData = MemoryManager::Reallocate(
		pCompressedData,
		uCompressedDataSizeInBytes + sizeof(UInt32),
		MemoryBudgets::Saving);
	memcpy((Byte*)pCompressedData + uCompressedDataSizeInBytes, &uUncompressedSize, sizeof(uUncompressedSize));
	uCompressedDataSizeInBytes += sizeof(UInt32);

	// Done, assign and return.
	rp = pCompressedData;
	ru = uCompressedDataSizeInBytes;

	// Done, success.
	return SaveLoadResult::kSuccess;
}

SaveLoadResult ToBlob(const SaveFileMetadata& metadata, const DataStore& saveData, void*& rp, UInt32& ru)
{
	// Verify DataStore integrity prior to save.
	if (!saveData.VerifyIntegrity()) { return SaveLoadResult::kErrorSaveCheck; }

	return ToBlobInternal(metadata, saveData, rp, ru, true);
}

SaveLoadResult ToBase64(const SaveFileMetadata& metadata, const DataStore& saveData, String& rsData)
{
	void* p = nullptr;
	UInt32 u = 0u;
	auto const e = ToBlob(metadata, saveData, p, u);
	if (SaveLoadResult::kSuccess != e)
	{
		return e;
	}

	// Base64-encode the data.
	rsData = Base64Encode(p, u);

	// Release the raw data.
	MemoryManager::Deallocate(p);

	return SaveLoadResult::kSuccess;
}

#if SEOUL_UNIT_TESTS
// Unit testing only variation of ToBase64, does not verify the integrity of the DataStore prior to save.
SaveLoadResult UnitTestHook_ToBase64NoVerify(const SaveFileMetadata& metadata, const DataStore& saveData, String& rsData)
{
	void* p = nullptr;
	UInt32 u = 0u;
	auto const e = ToBlobInternal(metadata, saveData, p, u, false);
	if (SaveLoadResult::kSuccess != e)
	{
		return e;
	}

	// Base64-encode the data.
	rsData = Base64Encode(p, u);

	// Release the raw data.
	MemoryManager::Deallocate(p);

	return SaveLoadResult::kSuccess;
}
#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul::SaveLoadUtil
