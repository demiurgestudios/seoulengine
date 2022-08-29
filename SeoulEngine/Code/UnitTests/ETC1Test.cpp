/**
 * \file ETC1Test.cpp
 * \brief Unit test header file for ETC1 decompression functions.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "DDS.h"
#include "ETC1Test.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "Image.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "SeoulETC1.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(ETC1Test)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestDecompress)
SEOUL_END_TYPE()

static void* LoadSIF(const String& sPath, UInt32& ruSizeInBytes)
{
	void* p = nullptr;
	UInt32 u = 0u;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
		sPath,
		p,
		u,
		0u,
		MemoryBudgets::Rendering));

	void* pp = nullptr;
	UInt32 uu = 0u;
	SEOUL_UNITTESTING_ASSERT(ZSTDDecompress(
		p,
		u,
		pp,
		uu));
	MemoryManager::Deallocate(p);

	ruSizeInBytes = uu;
	return pp;
}

struct ETC1TestEntry
{
	Byte const* m_sActual;
	Byte const* m_sExpected;
	Int32 m_iWidth;
	Int32 m_iHeight;
};

void ETC1Test::TestDecompress()
{
	UnitTestsFileManagerHelper helper;

	static const ETC1TestEntry s_kaEntries[] =
	{
		{ "UnitTests/ETC1/RGB/actual.sif0", "UnitTests/ETC1/RGB/expected.png", 256, 256, },
		{ "UnitTests/ETC1/RGBA/actual.sif0", "UnitTests/ETC1/RGBA/expected.png", 256, 512, },
	};

	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaEntries); ++i)
	{
		UInt32 uETC = 0u;
		void* pETC = LoadSIF(
			Path::Combine(GamePaths::Get()->GetConfigDir(), s_kaEntries[i].m_sActual),
			uETC);

		// Decompress the actual data.
		void* pActual = nullptr;
		UInt32 uActual = 0u;
		SEOUL_UNITTESTING_ASSERT(ETC1Decompress(
			pETC,
			uETC,
			pActual,
			uActual));
		MemoryManager::Deallocate(pETC);

		// Load the expected data.
		void* pExpected = nullptr;
		UInt32 uExpected = 0u;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
			Path::Combine(GamePaths::Get()->GetConfigDir(), s_kaEntries[i].m_sExpected),
			pExpected,
			uExpected,
			0u,
			MemoryBudgets::Rendering));

		Int32 iWidth = 0;
		Int32 iHeight = 0;
		UInt8* pImage = LoadImageFromMemory(
			(UInt8 const*)pExpected,
			(Int32)uExpected,
			&iWidth,
			&iHeight);
		MemoryManager::Deallocate(pExpected);
		SEOUL_UNITTESTING_ASSERT(nullptr != pImage);

		SEOUL_UNITTESTING_ASSERT_EQUAL(s_kaEntries[i].m_iWidth, iWidth);
		SEOUL_UNITTESTING_ASSERT_EQUAL(s_kaEntries[i].m_iHeight, iHeight);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp((Byte*)pActual + sizeof(DdsHeader), pImage, iWidth * iHeight * 4));

		FreeImage(pImage);
		MemoryManager::Deallocate(pActual);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
