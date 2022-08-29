/**
* \file CompressionTest.cpp
* \brief Benchmarks checks against SeoulEngine compression functions.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "Compress.h"
#include "CompressionTest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include "Vector.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(CompressionTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestGzipCompress)
	SEOUL_METHOD(TestGzipCompressSmall)
	SEOUL_METHOD(TestLZ4Compress)
	SEOUL_METHOD(TestLZ4CompressSmall)
	SEOUL_METHOD(TestZlibCompress)
	SEOUL_METHOD(TestZlibCompressSmall)
	SEOUL_METHOD(TestZSTDCompress)
	SEOUL_METHOD(TestZSTDCompressSmall)
	SEOUL_METHOD(TestZSTDCompressDict)
SEOUL_END_TYPE()

void CompressionTest::TestGzipCompress()
{
	static const Byte s_kaTestData[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };

	void* pCompressedData = nullptr;
	UInt32 zCompressedDataSizeInBytes = 0u;
	void* pUncompressedData = nullptr;
	UInt32 zUncompressedDataSizeInBytes = 0u;
	SEOUL_UNITTESTING_ASSERT(GzipCompress(s_kaTestData, sizeof(s_kaTestData), pCompressedData, zCompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT(GzipDecompress(pCompressedData, zCompressedDataSizeInBytes, pUncompressedData, zUncompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(s_kaTestData), zUncompressedDataSizeInBytes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(s_kaTestData, pUncompressedData, sizeof(s_kaTestData)));

	MemoryManager::Deallocate(pUncompressedData);
	MemoryManager::Deallocate(pCompressedData);
}

void CompressionTest::TestGzipCompressSmall()
{
	static const Byte s_kaTestData[] = { 1 };

	void* pCompressedData = nullptr;
	UInt32 zCompressedDataSizeInBytes = 0u;
	void* pUncompressedData = nullptr;
	UInt32 zUncompressedDataSizeInBytes = 0u;
	SEOUL_UNITTESTING_ASSERT(GzipCompress(s_kaTestData, sizeof(s_kaTestData), pCompressedData, zCompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT(GzipDecompress(pCompressedData, zCompressedDataSizeInBytes, pUncompressedData, zUncompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(s_kaTestData), zUncompressedDataSizeInBytes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(s_kaTestData, pUncompressedData, sizeof(s_kaTestData)));

	MemoryManager::Deallocate(pUncompressedData);
	MemoryManager::Deallocate(pCompressedData);
}

void CompressionTest::TestLZ4Compress()
{
	static const Byte s_kaTestData[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };

	void* pCompressedData = nullptr;
	UInt32 zCompressedDataSizeInBytes = 0u;
	void* pUncompressedData = nullptr;
	UInt32 zUncompressedDataSizeInBytes = 0u;
	SEOUL_UNITTESTING_ASSERT(LZ4Compress(s_kaTestData, sizeof(s_kaTestData), pCompressedData, zCompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT(LZ4Decompress(pCompressedData, zCompressedDataSizeInBytes, pUncompressedData, zUncompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(s_kaTestData), zUncompressedDataSizeInBytes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(s_kaTestData, pUncompressedData, sizeof(s_kaTestData)));

	MemoryManager::Deallocate(pUncompressedData);
	MemoryManager::Deallocate(pCompressedData);
}

void CompressionTest::TestLZ4CompressSmall()
{
	static const Byte s_kaTestData[] = { 1 };

	void* pCompressedData = nullptr;
	UInt32 zCompressedDataSizeInBytes = 0u;
	void* pUncompressedData = nullptr;
	UInt32 zUncompressedDataSizeInBytes = 0u;
	SEOUL_UNITTESTING_ASSERT(LZ4Compress(s_kaTestData, sizeof(s_kaTestData), pCompressedData, zCompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT(LZ4Decompress(pCompressedData, zCompressedDataSizeInBytes, pUncompressedData, zUncompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(s_kaTestData), zUncompressedDataSizeInBytes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(s_kaTestData, pUncompressedData, sizeof(s_kaTestData)));

	MemoryManager::Deallocate(pUncompressedData);
	MemoryManager::Deallocate(pCompressedData);
}

void CompressionTest::TestZlibCompress()
{
	static const Byte s_kaTestData[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };

	void* pCompressedData = nullptr;
	UInt32 zCompressedDataSizeInBytes = 0u;
	SEOUL_UNITTESTING_ASSERT(ZlibCompress(s_kaTestData, sizeof(s_kaTestData), pCompressedData, zCompressedDataSizeInBytes));
	UInt32 zUncompressedDataSizeInBytes = sizeof(s_kaTestData);
	void* pUncompressedData = MemoryManager::Allocate(zUncompressedDataSizeInBytes, MemoryBudgets::Compression, __FILE__, __LINE__);
	SEOUL_UNITTESTING_ASSERT(ZlibDecompress(pCompressedData, zCompressedDataSizeInBytes, pUncompressedData, zUncompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(s_kaTestData), zUncompressedDataSizeInBytes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(s_kaTestData, pUncompressedData, sizeof(s_kaTestData)));

	MemoryManager::Deallocate(pUncompressedData);
	MemoryManager::Deallocate(pCompressedData);
}

void CompressionTest::TestZlibCompressSmall()
{
	static const Byte s_kaTestData[] = { 1 };

	void* pCompressedData = nullptr;
	UInt32 zCompressedDataSizeInBytes = 0u;
	SEOUL_UNITTESTING_ASSERT(ZlibCompress(s_kaTestData, sizeof(s_kaTestData), pCompressedData, zCompressedDataSizeInBytes));
	UInt32 zUncompressedDataSizeInBytes = sizeof(s_kaTestData);
	void* pUncompressedData = MemoryManager::Allocate(zUncompressedDataSizeInBytes, MemoryBudgets::Compression, __FILE__, __LINE__);
	SEOUL_UNITTESTING_ASSERT(ZlibDecompress(pCompressedData, zCompressedDataSizeInBytes, pUncompressedData, zUncompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(s_kaTestData), zUncompressedDataSizeInBytes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(s_kaTestData, pUncompressedData, sizeof(s_kaTestData)));

	MemoryManager::Deallocate(pUncompressedData);
	MemoryManager::Deallocate(pCompressedData);
}

void CompressionTest::TestZSTDCompress()
{
	static const Byte s_kaTestData[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };

	void* pCompressedData = nullptr;
	UInt32 zCompressedDataSizeInBytes = 0u;
	void* pUncompressedData = nullptr;
	UInt32 zUncompressedDataSizeInBytes = 0u;
	SEOUL_UNITTESTING_ASSERT(ZSTDCompress(s_kaTestData, sizeof(s_kaTestData), pCompressedData, zCompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT(ZSTDDecompress(pCompressedData, zCompressedDataSizeInBytes, pUncompressedData, zUncompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(s_kaTestData), zUncompressedDataSizeInBytes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(s_kaTestData, pUncompressedData, sizeof(s_kaTestData)));

	MemoryManager::Deallocate(pUncompressedData);
	MemoryManager::Deallocate(pCompressedData);
}

void CompressionTest::TestZSTDCompressSmall()
{
	static const Byte s_kaTestData[] = { 1 };

	void* pCompressedData = nullptr;
	UInt32 zCompressedDataSizeInBytes = 0u;
	void* pUncompressedData = nullptr;
	UInt32 zUncompressedDataSizeInBytes = 0u;
	SEOUL_UNITTESTING_ASSERT(ZSTDCompress(s_kaTestData, sizeof(s_kaTestData), pCompressedData, zCompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT(ZSTDDecompress(pCompressedData, zCompressedDataSizeInBytes, pUncompressedData, zUncompressedDataSizeInBytes));
	SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(s_kaTestData), zUncompressedDataSizeInBytes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(s_kaTestData, pUncompressedData, sizeof(s_kaTestData)));

	MemoryManager::Deallocate(pUncompressedData);
	MemoryManager::Deallocate(pCompressedData);
}

void CompressionTest::TestZSTDCompressDict()
{
	// Generate some data.
	static const UInt32 kuDataSize = 32 * 1024;
	Vector<Byte, MemoryBudgets::Developer> vData(kuDataSize);
	for (UInt32 i = 0u; i < vData.GetSize(); ++i)
	{
		vData[i] = i;
	}

	// Allocate a dictionary of the same size.
	Vector<Byte, MemoryBudgets::Developer> vDict(kuDataSize);

	void* pCompressedData = nullptr;
	UInt32 zCompressedDataSizeInBytes = 0u;
	void* pUncompressedData = nullptr;
	UInt32 zUncompressedDataSizeInBytes = 0u;
	const size_t uFixedSampleSize = 16;
	SEOUL_UNITTESTING_ASSERT(vDict.GetSize() % uFixedSampleSize == 0u);
	Vector<size_t, MemoryBudgets::Developer> vSamples((UInt32)(vData.GetSizeInBytes() / uFixedSampleSize));
	vSamples.Fill(uFixedSampleSize);
	SEOUL_UNITTESTING_ASSERT(ZSTDPopulateDict(vData.Data(), vSamples.GetSize(), vSamples.Data(), vDict.Data(), vDict.GetSize()));
	auto pDictC = ZSTDCreateCompressionDictWeak(vDict.Data(), vDict.GetSize());
	SEOUL_UNITTESTING_ASSERT(ZSTDCompressWithDict(pDictC, vData.Data(), vData.GetSize(), pCompressedData, zCompressedDataSizeInBytes));
	ZSTDFreeCompressionDict(pDictC);
	auto pDictD = ZSTDCreateDecompressionDictWeak(vDict.Data(), vDict.GetSize());
	SEOUL_UNITTESTING_ASSERT(ZSTDDecompressWithDict(pDictD, pCompressedData, zCompressedDataSizeInBytes, pUncompressedData, zUncompressedDataSizeInBytes));
	ZSTDFreeDecompressionDict(pDictD);
	SEOUL_UNITTESTING_ASSERT_EQUAL(vData.GetSize(), zUncompressedDataSizeInBytes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vData.Data(), pUncompressedData, vData.GetSize()));

	MemoryManager::Deallocate(pUncompressedData);
	MemoryManager::Deallocate(pCompressedData);
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
