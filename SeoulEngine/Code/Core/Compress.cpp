/**
 * \file Compress.cpp
 * \brief Utilities for compressing and decompressing arbitrary blocks of data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#define ZDICT_STATIC_LINKING_ONLY
#define ZSTD_STATIC_LINKING_ONLY

#include "Algorithms.h"
#include "Compress.h"
#include "DiskFileSystem.h"
#include "ScopedAction.h"
#include "SeoulFile.h"
#include "Thread.h"

#include <stdint.h>

#include "lz4.h"
#include "lz4hc.h"
#include <zlib.h>
#include <dictBuilder/zdict.h>
#include <zstd.h>

// This constant is in zutil.h, which is an internal header that we don't have
// access to on all of our platforms.
static const int DEF_MEM_LEVEL = 8;

namespace Seoul
{

/** Header FourCC value written before compressed data. */
static const UInt32 kLZ4CompressedDataFourCC = ('L' << 0) | ('Z' << 8) | ('4' << 16) | ('C' << 24);

/** Total size of the header data inserted at the top of a compressed data chunk. */
static const UInt32 kLZ4CompressedHeaderDataSize = sizeof(kLZ4CompressedDataFourCC) + sizeof(UInt32);

/** Maximum size in bytes of data that is passed to Utilities.Compress. */
static const UInt32 kLZ4MaxCompressedDataInputSize = (1 << 30);

// Sanity check
SEOUL_STATIC_ASSERT(0 == (kLZ4CompressedHeaderDataSize % kLZ4MinimumAlignment));

/**
 * Compress data in pIn, compatible with decompression by LZ4Decompress
 *
 * @return True if compression was successful, false otherwise. If this method returns true,
 * ruOutputDataSizeInBytes will contain the size of the compressed data and rpOut will contain
 * the compressed data (in a buffer allocated with MemoryManager::AllocateAligned), otherwise
 * it will be left unchanged.
 */
Bool LZ4Compress(
	void const* pIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	LZ4CompressionLevel eLevel /* = LZ4CompressionLevel::kBest */,
	MemoryBudgets::Type eType /* = MemoryBudgets::Compression */,
	UInt32 uAlignmentOfOutputBuffer /*= kLZ4MinimumAlignment*/)
{
	// Make sure the alignment is at least our minimum.
	uAlignmentOfOutputBuffer = Max(kLZ4MinimumAlignment, uAlignmentOfOutputBuffer);

	UInt32 uFourCC = kLZ4CompressedDataFourCC;
	UInt32 uUncompressedDataSizeInBytes = uInputDataSizeInBytes;

	if (!IsSystemLittleEndian())
	{
		uFourCC = EndianSwap32(uFourCC);
		uUncompressedDataSizeInBytes = EndianSwap32(uUncompressedDataSizeInBytes);
	}

	// Allocate a worst case buffer and perform the compression.
	UInt32 const uInitialOutputDataSizeInBytes = (UInt32)(LZ4_compressBound(uInputDataSizeInBytes) + kLZ4CompressedHeaderDataSize);
	Byte* pStart = (Byte*)MemoryManager::AllocateAligned(
		// Round up the size to handle, what appears to be, read-only buffer
		// overrun in LZ4 functions. LZ4 functions can read passed the end of the
		// input/output buffer, which triggers a segfault on some platforms.
		RoundUpToAlignment(uInitialOutputDataSizeInBytes, uAlignmentOfOutputBuffer),
		eType,
		uAlignmentOfOutputBuffer);

	Byte* p = pStart;
	memcpy(p, &uFourCC, sizeof(uFourCC));
	p += sizeof(uFourCC);
	memcpy(p, &uUncompressedDataSizeInBytes, sizeof(uUncompressedDataSizeInBytes));
	p += sizeof(uUncompressedDataSizeInBytes);

	// Don't use LZ4_compress_HC here - it allocates the state structure on the stack,
	// which is very large, and can easily cause stack overflow crashes.
	Int iResult = 0;
	{
		void* pState = MemoryManager::AllocateAligned(
			(size_t)LZ4_sizeofStateHC(),
			MemoryBudgets::Compression,
			8u,
			__FILE__,
			__LINE__);
		iResult = LZ4_compress_HC_extStateHC(
			pState,
			(Byte const*)pIn,
			p,
			uInputDataSizeInBytes,
			(uInitialOutputDataSizeInBytes - kLZ4CompressedHeaderDataSize),
			(Int)eLevel);
		MemoryManager::Deallocate(pState);
	}

	if (iResult > 0)
	{
		ruOutputDataSizeInBytes = (UInt32)(iResult + kLZ4CompressedHeaderDataSize);

		// If the output data size is smaller than uInitialOutputDataSizeInBytes,
		// resize the buffer to be tight fitting.
		if (ruOutputDataSizeInBytes < uInitialOutputDataSizeInBytes)
		{
			pStart = (Byte*)MemoryManager::ReallocateAligned(
				pStart,
				// Round up the size to handle, what appears to be, read-only buffer
				// overrun in LZ4 functions. LZ4 functions can read passed the end of the
				// input/output buffer, which triggers a segfault on some platforms.
				RoundUpToAlignment(ruOutputDataSizeInBytes, uAlignmentOfOutputBuffer),
				uAlignmentOfOutputBuffer,
				eType);
		}

		rpOut = pStart;

		return true;
	}
	else
	{
		MemoryManager::Deallocate(pStart);
		return false;
	}
}

/**
 * Decompress data in pRawIn previously compressed with LZ4Compress
 *
 * @return True if decompression was successful, false otherwise. If this method returns true,
 * ruOutputDataSizeInBytes will contain the size of the uncompressed data and rpOut will contain
 * the uncompressed data (in a buffer allocated with MemoryManager::AllocateAligned), otherwise
 * it will be left unchanged.
 */
Bool LZ4Decompress(
	void const* pRawIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	MemoryBudgets::Type eType /* = MemoryBudgets::Compression */,
	UInt32 uAlignmentOfOutputBuffer /*= kLZ4MinimumAlignment*/)
{
	// Sanity check input data.
	SEOUL_ASSERT(0 == (size_t)pRawIn % (size_t)kLZ4MinimumAlignment);

	// Make sure the alignment is at least our minimum.
	uAlignmentOfOutputBuffer = Max(kLZ4MinimumAlignment, uAlignmentOfOutputBuffer);

	Byte const* pIn = (Byte const*)pRawIn;

	// Sanity check - data is too small, early out.
	if (uInputDataSizeInBytes < kLZ4CompressedHeaderDataSize)
	{
		return false;
	}

	// Read the header data.
	UInt32 uFourCC = 0u;
	UInt32 uOutputDataSizeInBytes = 0u;

	memcpy(&uFourCC, pIn, sizeof(uFourCC));
	pIn += sizeof(uFourCC);
	memcpy(&uOutputDataSizeInBytes, pIn, sizeof(uOutputDataSizeInBytes));
	pIn += sizeof(uOutputDataSizeInBytes);

	// Adjust for endianness of the current platform
	// (stored data is little endian).
	if (!IsSystemLittleEndian())
	{
		uFourCC = EndianSwap32(uFourCC);
		uOutputDataSizeInBytes = EndianSwap32(uOutputDataSizeInBytes);
	}

	// Sanity check input data.
	if (uFourCC != kLZ4CompressedDataFourCC ||
		uOutputDataSizeInBytes > kLZ4MaxCompressedDataInputSize)
	{
		return false;
	}

	// Allocate a buffer for decompression.
	Byte* p = (Byte*)MemoryManager::AllocateAligned(
		// Round up the size to handle, what appears to be, read-only buffer
		// overrun in LZ4 functions. LZ4 functions can read passed the end of the
		// input/output buffer, which triggers a segfault on some platforms.
		RoundUpToAlignment(uOutputDataSizeInBytes, uAlignmentOfOutputBuffer),
		eType,
		uAlignmentOfOutputBuffer);

	// Perform the decompression.
	Int iResult = LZ4_decompress_safe(
		pIn,
		p,
		(Int)(uInputDataSizeInBytes - kLZ4CompressedHeaderDataSize),
		(Int)uOutputDataSizeInBytes);

	// On success, populate output and return.
	if (iResult == (Int)uOutputDataSizeInBytes)
	{
		rpOut = p;
		ruOutputDataSizeInBytes = uOutputDataSizeInBytes;
		return true;
	}
	else
	{
		MemoryManager::Deallocate(p);
		return false;
	}
}

/**
 * Compresses the given data buffer as a zlib stream using the deflate
 * compression algorithm.
 *
 * @param[in] pInput Input buffer to compress
 * @param[in] uInputSize Size of the input buffer
 * @param[out] rpOutput On success, receives the pointer to the compressed data
 *               buffer
 * @param[out] ruOutputSize On Success, receives the size of the compressed
 *               output buffer
 * @param[in] eCompressionLevel Controls compression quality, size vs. speed tradeoff
 * @param[in] eMemoryBudget Memory budget with which to allocate the output
 *               buffer
 *
 * @return True if the compression succeeded, or false if an error occurred
 */
Bool ZlibCompress(
	const void *pInput,
	UInt32 uInputSize,
	void*& rpOutput,
	UInt32& ruOutputSize,
	ZlibCompressionLevel eCompressionLevel /* = ZlibCompressionLevel::kDefault */,
	MemoryBudgets::Type eMemoryBudget /* = MemoryBudgets::Compression */,
	UInt32 uAlignmentOfOutputBuffer /* = 0u */)
{
	rpOutput = nullptr;
	ruOutputSize = 0;

	uLong uCompressedSizeUpperBound = compressBound(uInputSize);
	if (uCompressedSizeUpperBound > (uLong)UINT32_MAX)
	{
		return false;  // Sanity check
	}

	void *pOutput = MemoryManager::AllocateAligned(
		uCompressedSizeUpperBound,
		eMemoryBudget,
		uAlignmentOfOutputBuffer);
	if (pOutput == nullptr)
	{
		return false;  // Out of memory
	}

	uLong uOutputSize = uCompressedSizeUpperBound;
	int iResult = compress2((UByte *)pOutput, &uOutputSize, (const UByte *)pInput, uInputSize, (int)eCompressionLevel);
	if (iResult != Z_OK)
	{
		// Failure compressing (non enough memory??)
		MemoryManager::Deallocate(pOutput);
		return false;
	}

	// If the output data size is smaller than uInitialOutputDataSizeInBytes,
	// resize the buffer to be tight fitting.
	if (uOutputSize < uCompressedSizeUpperBound)
	{
		pOutput = MemoryManager::ReallocateAligned(
			pOutput,
			uOutputSize,
			uAlignmentOfOutputBuffer,
			eMemoryBudget);
	}

	// Success
	SEOUL_ASSERT(uOutputSize <= (uLong)UINT32_MAX);
	rpOutput = pOutput;
	ruOutputSize = (UInt32)uOutputSize;
	return true;
}

/**
 * Decompress data in pIn compressed with ZlibCompress into pOut.
 *
 * @param[in] pIn The ZlibCompress data
 * @param[in] uInputDataSizeInBytes The total size of data in pIn
 * @param[in] pOut The data to write uncompressed data to, must be
 *            already allocate (unlike LZ4Decompress) and large
 *            enough to contain all of the uncompressed data.
 * @param[in] uOutputDataSizeInBytes The total size of pOut. Expected
 *            to be the uncompressed data size.
 *
 * @return True on successful decompress, false otherwise. If this method
 *         returns true, pOut will contain the uncompressed data, otherwise
 *         pOut will contain data in an undefined state.
 */
Bool ZlibDecompress(
	void const* pIn,
	UInt32 uInputDataSizeInBytes,
	void* pOut,
	UInt32 uOutputDataSizeInBytes)
{
	uLongf uZlibOutputDataSizeInBytes = (uLongf)uOutputDataSizeInBytes;
	int iResult = uncompress((Bytef*)pOut, &uZlibOutputDataSizeInBytes, (Bytef const*)pIn, (uLong)uInputDataSizeInBytes);
	if (iResult != Z_OK || uZlibOutputDataSizeInBytes != (uLongf)uOutputDataSizeInBytes)
	{
		return false;
	}

	// Success
	return true;
}

/** compressBound() is inaccurate for gzipped compressed data. */
static inline uLong GetGzipCompressBound(uLong uSizeInBytes)
{
	// See: http://stackoverflow.com/a/23578269
	// Need to add space for the header to the compress bound estimation
	// for gzip. Round up 18 bytes (the actual header size) to 32.
	return compressBound(uSizeInBytes) + 32;
}

/**
 * Compresses the given data buffer as a gzip stream using the deflate
 * compression algorithm.
 *
 * @param[in] pInput Input buffer to compress
 * @param[in] uInputSize Size of the input buffer
 * @param[out] rpOutput On success, receives the pointer to the compressed data
 *               buffer
 * @param[out] ruOutputSize On Success, receives the size of the compressed
 *               output buffer
 * @param[in] eCompressionLevel Controls compression quality, size vs. speed tradeoff
 * @param[in] eMemoryBudget Memory budget with which to allocate the output
 *               buffer
 *
 * @return True if the compression succeeded, or false if an error occurred
 */
Bool GzipCompress(
	const void *pInput,
	UInt32 uInputSize,
	void*& rpOutput,
	UInt32& ruOutputSize,
	ZlibCompressionLevel eCompressionLevel /* = ZlibCompressionLevel::kDefault */,
	MemoryBudgets::Type eMemoryBudget /* = MemoryBudgets::Compression */,
	UInt32 uAlignmentOfOutputBuffer /* = 0u */)
{
	rpOutput = nullptr;
	ruOutputSize = 0;

	uLong uCompressedSizeUpperBound = GetGzipCompressBound(uInputSize);
	if (uCompressedSizeUpperBound > (uLong)UINT32_MAX)
	{
		return false;  // Sanity check
	}

	void *pOutput = MemoryManager::AllocateAligned(
		uCompressedSizeUpperBound,
		eMemoryBudget,
		uAlignmentOfOutputBuffer);
	if (pOutput == nullptr)
	{
		return false;  // Out of memory
	}

	uLong uOutputSize = uCompressedSizeUpperBound;

	z_stream stream;
	memset(&stream, 0, sizeof(stream));

	stream.next_in = (Bytef*)pInput;
	stream.avail_in = uInputSize;
	stream.next_out = (Bytef*)pOutput;
	stream.avail_out = (uInt)uOutputSize;

	// 16 + is secret magic for "write .gz compatible header". All other settings are defaults,
	// equivalent to calling compress2().
	if (Z_OK != deflateInit2(&stream, (int)eCompressionLevel, Z_DEFLATED, 16 + MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY))
	{
		MemoryManager::Deallocate(pOutput);
		return false;
	}

	// Perform the compression, any return code other than Z_STREAM_END is an error.
	if (Z_STREAM_END != deflate(&stream, Z_FINISH))
	{
		(void)deflateEnd(&stream);
		MemoryManager::Deallocate(pOutput);
		return false;
	}

	// Finalize the stream (this releases any memory allocation done by zlib).
	if (Z_OK != deflateEnd(&stream))
	{
		MemoryManager::Deallocate(pOutput);
		return false;
	}

	// Final output size.
	uOutputSize = stream.total_out;

	// If the output data size is smaller than uCompressedSizeUpperBound,
	// resize the buffer to be tight fitting.
	if (uOutputSize < uCompressedSizeUpperBound)
	{
		pOutput = MemoryManager::ReallocateAligned(
			pOutput,
			uOutputSize,
			uAlignmentOfOutputBuffer,
			eMemoryBudget);
	}

	// Success
	SEOUL_ASSERT(uOutputSize <= (uLong)UINT32_MAX);
	rpOutput = pOutput;
	ruOutputSize = (UInt32)uOutputSize;
	return true;
}

Bool GzipDecompress(
	void const* pIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	MemoryBudgets::Type eType /*= MemoryBudgets::Compression*/,
	UInt32 uAlignmentOfOutputBuffer /*= 0u*/)
{
	// Too small, no size.
	if (uInputDataSizeInBytes < sizeof(UInt32))
	{
		return false;
	}

	// ISIZE is the last 4 bytes.
	UInt32 uUncompressedSize = 0u;
	memcpy(&uUncompressedSize, (Byte const*)pIn + uInputDataSizeInBytes - sizeof(UInt32), sizeof(UInt32));

	// Sanity check ISIZE.
	if (uUncompressedSize > kDefaultMaxReadSize)
	{
		return false;
	}

	// Allocate a buffer for output.
	void* pOutput = MemoryManager::AllocateAligned(
		uUncompressedSize,
		eType,
		uAlignmentOfOutputBuffer);
	if (pOutput == nullptr)
	{
		return false;
	}

	// Perform decompression.
	uLong uOutputSize = uUncompressedSize;

	z_stream stream;
	memset(&stream, 0, sizeof(stream));

	stream.next_in = (Bytef*)pIn;
	stream.avail_in = (uInt)uInputDataSizeInBytes;
	stream.next_out = (Bytef*)pOutput;
	stream.avail_out = (uInt)uOutputSize;

	// 16 + is secret magic for "write .gz compatible header". All other settings are defaults,
	// equivalent to calling compress2().
	if (Z_OK != inflateInit2(&stream, 16 + MAX_WBITS))
	{
		MemoryManager::Deallocate(pOutput);
		return false;
	}

	// Perform the decompression, any return code other than Z_STREAM_END is an error.
	if (Z_STREAM_END != inflate(&stream, Z_FINISH))
	{
		(void)inflateEnd(&stream);
		MemoryManager::Deallocate(pOutput);
		return false;
	}

	// Finalize the stream (this releases any memory allocation done by zlib).
	if (Z_OK != inflateEnd(&stream))
	{
		MemoryManager::Deallocate(pOutput);
		return false;
	}

	// Final output size.
	uOutputSize = stream.total_out;

	// If the output data size is smaller than uUncompressedSize,
	// resize the buffer to be tight fitting.
	if (uOutputSize < uUncompressedSize)
	{
		pOutput = MemoryManager::ReallocateAligned(
			pOutput,
			uOutputSize,
			uAlignmentOfOutputBuffer,
			eType);
	}

	// Success
	SEOUL_ASSERT(uOutputSize <= (uLong)UINT32_MAX);
	rpOut = pOutput;
	ruOutputDataSizeInBytes = (UInt32)uOutputSize;
	return true;
}

/** Header FourCC value written before compressed data. */
static const UInt32 kZSTDCompressedDataFourCC = ('Z' << 0) | ('S' << 8) | ('T' << 16) | ('D' << 24);

/** Total size of the header data inserted at the top of a compressed data chunk. */
static const UInt32 kZSTDCompressedHeaderDataSize = sizeof(kZSTDCompressedDataFourCC) + sizeof(UInt32);

/** Maximum size in bytes of data that is passed to Utilities.Compress. */
static const UInt32 kZSTDMaxCompressedDataInputSize = (1 << 30);

// Sanity check
SEOUL_STATIC_ASSERT(0 == (kZSTDCompressedHeaderDataSize % kZSTDMinimumAlignment));

// Memory allocation hooks.
static void* ZSTDAllocate(void*, size_t zSize)
{
	return MemoryManager::Allocate(zSize, MemoryBudgets::Compression);
}

static void ZSTDDeallocate(void*, void* pAddress)
{
	MemoryManager::Deallocate(pAddress);
}
static const ZSTD_customMem s_kaZSTDCustomMem = { ZSTDAllocate, ZSTDDeallocate, nullptr };

/**
 * Compress data in pIn, compatible with decompression by ZSTDDecompress
 *
 * @return True if compression was successful, false otherwise. If this method returns true,
 * ruOutputDataSizeInBytes will contain the size of the compressed data and rpOut will contain
 * the compressed data (in a buffer allocated with MemoryManager::AllocateAligned), otherwise
 * it will be left unchanged.
 */
Bool ZSTDCompress(
	void const* pIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	ZSTDCompressionLevel eLevel /* = ZSTDCompressionLevel::kBest */,
	MemoryBudgets::Type eType /* = MemoryBudgets::Compression */,
	UInt32 uAlignmentOfOutputBuffer /*= kZSTDMinimumAlignment*/)
{
	// Make sure the alignment is at least our minimum.
	uAlignmentOfOutputBuffer = Max(kZSTDMinimumAlignment, uAlignmentOfOutputBuffer);

	// Values to write - adjust for endianess - we want
	// the output to always be little endian.
	UInt32 uFourCC = kZSTDCompressedDataFourCC;
	UInt32 uUncompressedDataSizeInBytes = uInputDataSizeInBytes;
	if (!IsSystemLittleEndian())
	{
		uFourCC = EndianSwap32(uFourCC);
		uUncompressedDataSizeInBytes = EndianSwap32(uUncompressedDataSizeInBytes);
	}

	// Allocate a worst case buffer and perform the compression.
	UInt32 const uInitialOutputDataSizeInBytes = (UInt32)(ZSTD_compressBound(uInputDataSizeInBytes) + kZSTDCompressedHeaderDataSize);
	Byte* pStart = (Byte*)MemoryManager::AllocateAligned(
		RoundUpToAlignment(uInitialOutputDataSizeInBytes, uAlignmentOfOutputBuffer),
		eType,
		uAlignmentOfOutputBuffer);

	// Write header data.
	Byte* p = pStart;
	memcpy(p, &uFourCC, sizeof(uFourCC));
	p += sizeof(uFourCC);
	memcpy(p, &uUncompressedDataSizeInBytes, sizeof(uUncompressedDataSizeInBytes));
	p += sizeof(uUncompressedDataSizeInBytes);

	// Perform the compression.
	auto pCtx = ZSTD_createCCtx_advanced(s_kaZSTDCustomMem);
	size_t const zResult = ZSTD_compressCCtx(
		pCtx,
		p,
		(uInitialOutputDataSizeInBytes - kZSTDCompressedHeaderDataSize),
		pIn,
		uInputDataSizeInBytes,
		(Int)eLevel);
	ZSTD_freeCCtx(pCtx); pCtx = nullptr;

	// No error indices successful compression.
	if (0 == ZSTD_isError(zResult))
	{
		// Populate output data.
		ruOutputDataSizeInBytes = (UInt32)(zResult + kZSTDCompressedHeaderDataSize);

		// If the output data size is smaller than uInitialOutputDataSizeInBytes,
		// resize the buffer to be tight fitting.
		if (ruOutputDataSizeInBytes < uInitialOutputDataSizeInBytes)
		{
			pStart = (Byte*)MemoryManager::ReallocateAligned(
				pStart,
				RoundUpToAlignment(ruOutputDataSizeInBytes, uAlignmentOfOutputBuffer),
				uAlignmentOfOutputBuffer,
				eType);
		}

		// Output buffer.
		rpOut = pStart;

		return true;
	}
	// On failure, free output buffer and return failure.
	else
	{
		MemoryManager::Deallocate(pStart);
		return false;
	}
}

/**
 * Decompress data in pRawIn previously compressed with ZSTDCompress
 *
 * @return True if decompression was successful, false otherwise. If this method returns true,
 * ruOutputDataSizeInBytes will contain the size of the uncompressed data and rpOut will contain
 * the uncompressed data (in a buffer allocated with MemoryManager::AllocateAligned), otherwise
 * it will be left unchanged.
 */
Bool ZSTDDecompress(
	void const* pRawIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	MemoryBudgets::Type eType /* = MemoryBudgets::Compression */,
	UInt32 uAlignmentOfOutputBuffer /*= kZSTDMinimumAlignment*/)
{
	// Sanity check input data.
	SEOUL_ASSERT(0 == (size_t)pRawIn % (size_t)kZSTDMinimumAlignment);

	// Make sure the alignment is at least our minimum.
	uAlignmentOfOutputBuffer = Max(kZSTDMinimumAlignment, uAlignmentOfOutputBuffer);

	Byte const* pIn = (Byte const*)pRawIn;

	// Sanity check input data - early out if too small.
	if (uInputDataSizeInBytes < kZSTDCompressedHeaderDataSize)
	{
		return false;
	}

	// Read header data.
	UInt32 uFourCC = 0u;
	UInt32 uOutputDataSizeInBytes = 0u;

	memcpy(&uFourCC, pIn, sizeof(uFourCC));
	pIn += sizeof(uFourCC);
	memcpy(&uOutputDataSizeInBytes, pIn, sizeof(uOutputDataSizeInBytes));
	pIn += sizeof(uOutputDataSizeInBytes);

	// Adjust for system endianess - data on disk is always little endian.
	if (!IsSystemLittleEndian())
	{
		uFourCC = EndianSwap32(uFourCC);
		uOutputDataSizeInBytes = EndianSwap32(uOutputDataSizeInBytes);
	}

	// Check header data.
	if (uFourCC != kZSTDCompressedDataFourCC ||
		uOutputDataSizeInBytes > kZSTDMaxCompressedDataInputSize)
	{
		return false;
	}

	// Allocate a buffer for decompression.
	Byte* p = (Byte*)MemoryManager::AllocateAligned(
		RoundUpToAlignment(uOutputDataSizeInBytes, uAlignmentOfOutputBuffer),
		eType,
		uAlignmentOfOutputBuffer);

	auto pCtx = ZSTD_createDCtx_advanced(s_kaZSTDCustomMem);
	size_t const zResult = ZSTD_decompressDCtx(
		pCtx,
		p,
		uOutputDataSizeInBytes,
		pIn,
		(size_t)(uInputDataSizeInBytes - kZSTDCompressedHeaderDataSize));
	ZSTD_freeDCtx(pCtx); pCtx = nullptr;

	// On success...
	if (0 == ZSTD_isError(zResult))
	{
		// Populate output buffers.
		rpOut = p;
		ruOutputDataSizeInBytes = (UInt32)zResult;

		// If the output data size is smaller than uOutputDataSizeInBytes,
		// resize the buffer to be tight fitting.
		if (ruOutputDataSizeInBytes < uOutputDataSizeInBytes)
		{
			rpOut = (Byte*)MemoryManager::ReallocateAligned(
				rpOut,
				RoundUpToAlignment(ruOutputDataSizeInBytes, uAlignmentOfOutputBuffer),
				uAlignmentOfOutputBuffer,
				eType);
		}

		return true;
	}
	// Otherwise, on failure, release the output buffer and return failure.
	else
	{
		MemoryManager::Deallocate(p);
		return false;
	}
}

Bool ZSTDCompressWithDict(
	ZSTDCompressionDict const* pDict,
	void const* pIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	MemoryBudgets::Type eType /*= MemoryBudgets::Compression*/,
	UInt32 uAlignmentOfOutputBuffer /*= kZSTDMinimumAlignment*/)
{
	// Make sure the alignment is at least our minimum.
	uAlignmentOfOutputBuffer = Max(kZSTDMinimumAlignment, uAlignmentOfOutputBuffer);

	// Values to write - adjust for endianess - we want
	// the output to always be little endian.
	UInt32 uFourCC = kZSTDCompressedDataFourCC;
	UInt32 uUncompressedDataSizeInBytes = uInputDataSizeInBytes;
	if (!IsSystemLittleEndian())
	{
		uFourCC = EndianSwap32(uFourCC);
		uUncompressedDataSizeInBytes = EndianSwap32(uUncompressedDataSizeInBytes);
	}

	// Allocate a worst case buffer and perform the compression.
	UInt32 const uInitialOutputDataSizeInBytes = (UInt32)(ZSTD_compressBound(uInputDataSizeInBytes) + kZSTDCompressedHeaderDataSize);
	Byte* pStart = (Byte*)MemoryManager::AllocateAligned(
		RoundUpToAlignment(uInitialOutputDataSizeInBytes, uAlignmentOfOutputBuffer),
		eType,
		uAlignmentOfOutputBuffer);

	// Write header data.
	Byte* p = pStart;
	memcpy(p, &uFourCC, sizeof(uFourCC));
	p += sizeof(uFourCC);
	memcpy(p, &uUncompressedDataSizeInBytes, sizeof(uUncompressedDataSizeInBytes));
	p += sizeof(uUncompressedDataSizeInBytes);

	// Perform the compression.
	auto pCtx = ZSTD_createCCtx_advanced(s_kaZSTDCustomMem);
	size_t const zResult = ZSTD_compress_usingCDict(
		pCtx,
		p,
		(uInitialOutputDataSizeInBytes - kZSTDCompressedHeaderDataSize),
		pIn,
		uInputDataSizeInBytes,
		(ZSTD_CDict const*)pDict);
	ZSTD_freeCCtx(pCtx); pCtx = nullptr;

	// No error indices successful compression.
	if (0 == ZSTD_isError(zResult))
	{
		// Populate output data.
		ruOutputDataSizeInBytes = (UInt32)(zResult + kZSTDCompressedHeaderDataSize);

		// If the output data size is smaller than uInitialOutputDataSizeInBytes,
		// resize the buffer to be tight fitting.
		if (ruOutputDataSizeInBytes < uInitialOutputDataSizeInBytes)
		{
			pStart = (Byte*)MemoryManager::ReallocateAligned(
				pStart,
				RoundUpToAlignment(ruOutputDataSizeInBytes, uAlignmentOfOutputBuffer),
				uAlignmentOfOutputBuffer,
				eType);
		}

		// Output buffer.
		rpOut = pStart;

		return true;
	}
	// On failure, free output buffer and return failure.
	else
	{
		MemoryManager::Deallocate(pStart);
		return false;
	}
}

Bool ZSTDDecompressWithDict(
	ZSTDDecompressionDict const* pDict,
	void const* pRawIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	MemoryBudgets::Type eType /*= MemoryBudgets::Compression*/,
	UInt32 uAlignmentOfOutputBuffer /*= kZSTDMinimumAlignment*/)
{
	// Sanity check input data.
	SEOUL_ASSERT(0 == (size_t)pRawIn % (size_t)kZSTDMinimumAlignment);

	// Make sure the alignment is at least our minimum.
	uAlignmentOfOutputBuffer = Max(kZSTDMinimumAlignment, uAlignmentOfOutputBuffer);

	Byte const* pIn = (Byte const*)pRawIn;

	// Sanity check input data - early out if too small.
	if (uInputDataSizeInBytes < kZSTDCompressedHeaderDataSize)
	{
		return false;
	}

	// Read header data.
	UInt32 uFourCC = 0u;
	UInt32 uOutputDataSizeInBytes = 0u;

	memcpy(&uFourCC, pIn, sizeof(uFourCC));
	pIn += sizeof(uFourCC);
	memcpy(&uOutputDataSizeInBytes, pIn, sizeof(uOutputDataSizeInBytes));
	pIn += sizeof(uOutputDataSizeInBytes);

	// Adjust for system endianess - data on disk is always little endian.
	if (!IsSystemLittleEndian())
	{
		uFourCC = EndianSwap32(uFourCC);
		uOutputDataSizeInBytes = EndianSwap32(uOutputDataSizeInBytes);
	}

	// Check header data.
	if (uFourCC != kZSTDCompressedDataFourCC ||
		uOutputDataSizeInBytes > kZSTDMaxCompressedDataInputSize)
	{
		return false;
	}

	// Allocate a buffer for decompression.
	Byte* p = (Byte*)MemoryManager::AllocateAligned(
		RoundUpToAlignment(uOutputDataSizeInBytes, uAlignmentOfOutputBuffer),
		eType,
		uAlignmentOfOutputBuffer);

	// Decompress.
	auto pCtx = ZSTD_createDCtx_advanced(s_kaZSTDCustomMem);
	size_t const zResult = ZSTD_decompress_usingDDict(
		pCtx,
		p,
		uOutputDataSizeInBytes,
		pIn,
		(size_t)(uInputDataSizeInBytes - kZSTDCompressedHeaderDataSize),
		(ZSTD_DDict const*)pDict);
	ZSTD_freeDCtx(pCtx); pCtx = nullptr;

	// On success...
	if (0 == ZSTD_isError(zResult))
	{
		// Populate output buffers.
		rpOut = p;
		ruOutputDataSizeInBytes = (UInt32)zResult;

		// If the output data size is smaller than uOutputDataSizeInBytes,
		// resize the buffer to be tight fitting.
		if (ruOutputDataSizeInBytes < uOutputDataSizeInBytes)
		{
			rpOut = (Byte*)MemoryManager::ReallocateAligned(
				rpOut,
				RoundUpToAlignment(ruOutputDataSizeInBytes, uAlignmentOfOutputBuffer),
				uAlignmentOfOutputBuffer,
				eType);
		}

		return true;
	}
	// Otherwise, on failure, release the output buffer and return failure.
	else
	{
		MemoryManager::Deallocate(p);
		return false;
	}
}

Bool ZSTDPopulateDict(
	void const* pInputData,
	UInt32 uInputSamples,
	size_t const* pSampleSizes,
	void* pDict,
	UInt32 uDict)
{
	ZDICT_fastCover_params_t params;
	memset(&params, 0, sizeof(params));
	params.nbThreads = Thread::GetProcessorCount();
	auto const uResult = ZDICT_optimizeTrainFromBuffer_fastCover(
		pDict,
		uDict,
		pInputData,
		pSampleSizes,
		uInputSamples,
		&params);

	return (0 == ZSTD_isError(uResult));
}

ZSTDCompressionDict* ZSTDCreateCompressionDictWeak(
	void const* pDict,
	UInt32 uDict,
	ZSTDCompressionLevel eLevel /*= ZSTDCompressionLevel::kBest*/)
{
	auto const cParams = ZSTD_getCParams((Int)eLevel, 0, uDict);
	return (ZSTDCompressionDict*)ZSTD_createCDict_advanced(
		pDict,
		uDict,
		ZSTD_dlm_byRef,
		ZSTD_dct_auto,
		cParams,
		s_kaZSTDCustomMem);
}

void ZSTDFreeCompressionDict(ZSTDCompressionDict*& rp)
{
	ZSTD_CDict* p = (ZSTD_CDict*)rp;
	rp = nullptr;

	if (nullptr != p)
	{
		(void)ZSTD_freeCDict(p);
	}
}

ZSTDDecompressionDict* ZSTDCreateDecompressionDictWeak(void const* pDict, UInt32 uDict)
{
	return (ZSTDDecompressionDict*)ZSTD_createDDict_advanced(pDict, uDict, ZSTD_dlm_byRef, ZSTD_dct_auto, s_kaZSTDCustomMem);
}

void ZSTDFreeDecompressionDict(ZSTDDecompressionDict*& rp)
{
	ZSTD_DDict* p = (ZSTD_DDict*)rp;
	rp = nullptr;

	if (nullptr != p)
	{
		(void)ZSTD_freeDDict(p);
	}
}

/**
 * Special variation, uses ZSTD to decompress a file to another file.
 * Useful for very large data that you do not want to exist in memory all at once.
 *
 * NOTE: Does not use the FileManager virtualized file system interface - both files
 * must exist on disk.
 */
Bool ZSTDDecompressFile(const String& sIn, const String& sOut)
{
	auto pMapIn = DiskSyncFile::MemoryMapReadFile(sIn);
	if (nullptr == pMapIn)
	{
		return false;
	}
	// Make sure this is closed - typically handled in paths that care about return value.
	auto const deferredIn(MakeDeferredAction([&]() { DiskSyncFile::CloseMemoryMap(pMapIn); }));

	auto const pIn = DiskSyncFile::GetMemoryMapReadPtr(pMapIn);
	auto const uIn = DiskSyncFile::GetMemoryMapSize(pMapIn);
	if (nullptr == pIn)
	{
		return false;
	}

	// Expected decompressed size.
	auto const uOut = ZSTD_getFrameContentSize(pIn, (size_t)uIn);
	if (ZSTD_CONTENTSIZE_UNKNOWN == uOut || ZSTD_CONTENTSIZE_ERROR == uOut)
	{
		return false;
	}

	// Map the output file for writing.
	auto pMapOut = DiskSyncFile::MemoryMapWriteFile(sOut, uOut);
	if (nullptr == pMapOut)
	{
		(void)DiskSyncFile::CloseMemoryMap(pMapIn);
		return false;
	}
	// Make sure this is closed - typically handled in paths that care about return value.
	auto const deferredOu(MakeDeferredAction([&]() { DiskSyncFile::CloseMemoryMap(pMapOut); }));
	auto const pOut = DiskSyncFile::GetMemoryMapWritePtr(pMapOut);
	if (nullptr == pOut)
	{
		return false;
	}

	// Perform the decompression.
	auto const zResult = ZSTD_decompress(
		pOut,
		(size_t)uOut,
		pIn,
		(size_t)uIn);

	// Cache success.
	auto bError = ZSTD_isError(zResult);

	// On error, close output and delete any intermediate state.
	if (bError)
	{
		(void)DiskSyncFile::CloseMemoryMap(pMapOut, 0);
		(void)DiskSyncFile::DeleteFile(sOut);
	}
	// On success, terminate the output - treat failure here as a failure.
	else
	{
		bError = !DiskSyncFile::CloseMemoryMap(pMapOut, zResult) || bError;
	}
	// We don't care about any error on closing input (there isn't expected to be any.
	(void)DiskSyncFile::CloseMemoryMap(pMapIn, uIn);

	return !bError;
}

} // namespace Seoul
