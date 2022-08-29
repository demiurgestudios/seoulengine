/**
 * \file Compress.h
 * \brief Utilities for compressing and decompressing arbitrary blocks of data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COMPRESS_H
#define COMPRESS_H

#include "Prereqs.h"

namespace Seoul
{

/** Minimum alignment of LZ input/output buffers. */
static const UInt32 kLZ4MinimumAlignment = 4u;

enum class LZ4CompressionLevel
{
	/** Lowest compression ratio, fastest compression speed. */
	kFastest = 1,

	/** Compromise between compression speed and size. */
	kNormal = 4,

	/** Highest compression, smallest output size, much slower compression speed. */
	kBest = 16,
};

Bool LZ4Compress(
	void const* pIn,
	UInt32 zInputDataSizeInBytes,
	void*& rpOut,
	UInt32& rzOutputDataSizeInBytes,
	LZ4CompressionLevel eLevel = LZ4CompressionLevel::kBest,
	MemoryBudgets::Type eType = MemoryBudgets::Compression,
	UInt32 zAlignmentOfOutputBuffer = kLZ4MinimumAlignment);

Bool LZ4Decompress(
	void const* pIn,
	UInt32 zInputDataSizeInBytes,
	void*& rpOut,
	UInt32& rzOutputDataSizeInBytes,
	MemoryBudgets::Type eType = MemoryBudgets::Compression,
	UInt32 zAlignmentOfOutputBuffer = kLZ4MinimumAlignment);

/**
 * Compression level used by ZLibCompress and GZipCompress.
 */
enum class ZlibCompressionLevel
{
	kNone = 0,
	kFast = 1,
	kBest = 9,
	kDefault = -1,
};

// Compresses data as a zlib stream.
Bool ZlibCompress(
	const void *pInput,
	UInt32 zInputSize,
	void*& rpOutput,
	UInt32& rzOutputSize,
	ZlibCompressionLevel eCompressionLevel = ZlibCompressionLevel::kDefault,
	MemoryBudgets::Type eMemoryBudget = MemoryBudgets::Compression,
	UInt32 zAlignmentOfOutputBuffer = 0u);

// Decompresses data from a zlib stream.
//
// NOTE: Different API than LZ4Decompress. pOut must be already allocated, and
// zOutputDataSizeInBytes must be already set to the complete output buffer size.
// This requires the caller to know the size of the uncompressed data.
Bool ZlibDecompress(
	void const* pIn,
	UInt32 zInputDataSizeInBytes,
	void* pOut,
	UInt32 zOutputDataSizeInBytes);

// Compresses data as a gzip stream. Equivalent compression to ZlibCompress,
// but includes a .gz compatible header.
Bool GzipCompress(
	const void *pInput,
	UInt32 zInputSize,
	void*& rpOutput,
	UInt32& rzOutputSize,
	ZlibCompressionLevel eCompressionLevel = ZlibCompressionLevel::kDefault,
	MemoryBudgets::Type eMemoryBudget = MemoryBudgets::Compression,
	UInt32 zAlignmentOfOutputBuffer = 0u);

// Decompresses data from a gzip stream.
Bool GzipDecompress(
	void const* pIn,
	UInt32 zInputDataSizeInBytes,
	void*& rpOut,
	UInt32& rzOutputDataSizeInBytes,
	MemoryBudgets::Type eType = MemoryBudgets::Compression,
	UInt32 zAlignmentOfOutputBuffer = 0u);

/** Minimum alignment of ZSTD input/output buffers. */
static const UInt32 kZSTDMinimumAlignment = 4u;

enum class ZSTDCompressionLevel
{
	/** Lowest compression ratio, fastest compression speed. */
	kFastest = 1,

	/** Compromise between compression speed and size. */
	kNormal = 4,

	/** Highest compression, smallest output size, much slower compression speed. */
	kBest = 22,
};

Bool ZSTDCompress(
	void const* pIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	ZSTDCompressionLevel eLevel = ZSTDCompressionLevel::kBest,
	MemoryBudgets::Type eType = MemoryBudgets::Compression,
	UInt32 uAlignmentOfOutputBuffer = kZSTDMinimumAlignment);

Bool ZSTDDecompress(
	void const* pIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	MemoryBudgets::Type eType = MemoryBudgets::Compression,
	UInt32 uAlignmentOfOutputBuffer = kZSTDMinimumAlignment);

struct ZSTDCompressionDict;
struct ZSTDDecompressionDict;

Bool ZSTDCompressWithDict(
	ZSTDCompressionDict const* pDict,
	void const* pIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	MemoryBudgets::Type eType = MemoryBudgets::Compression,
	UInt32 uAlignmentOfOutputBuffer = kZSTDMinimumAlignment);

Bool ZSTDDecompressWithDict(
	ZSTDDecompressionDict const* pDict,
	void const* pIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	MemoryBudgets::Type eType = MemoryBudgets::Compression,
	UInt32 uAlignmentOfOutputBuffer = kZSTDMinimumAlignment);

Bool ZSTDPopulateDict(
	void const* pInputData,
	UInt32 uInputSamples,
	size_t const* pSampleSizes,
	void* pDict,
	UInt32 uDict);

// Creates a decompression dictionary that can be used
// to (quickly) perform decompression using a dictionary.
//
// IMPORTANT: The memory passed in must
// stay alive for the life of the returned pointer.
ZSTDCompressionDict* ZSTDCreateCompressionDictWeak(void const* pDict, UInt32 uDict, ZSTDCompressionLevel eLevel = ZSTDCompressionLevel::kBest);
void ZSTDFreeCompressionDict(ZSTDCompressionDict*& rp);
ZSTDDecompressionDict* ZSTDCreateDecompressionDictWeak(void const* pDict, UInt32 uDict);
void ZSTDFreeDecompressionDict(ZSTDDecompressionDict*& rp);

// Special variation, uses ZSTD to decompress a file to another file.
// Useful for very large data that you do not want to exist in memory all at once.
//
// Currently, the input for this function is generated only by CI and tools using
// the zstd.exe command-line.
Bool ZSTDDecompressFile(const String& sIn, const String& sOut);

} // namespace Seoul

#endif // include guard
