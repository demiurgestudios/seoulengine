/**
 * \file ImageTest.cpp
 * \brief Unit tests for the Seoul Engine wrapper
 * around image reading functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileManager.h"
#include "GamePaths.h"
#include "Image.h"
#include "ImageTest.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(ImageTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestPngSuite)
SEOUL_END_TYPE()

static void GetExpected(
	const String& sCodes,
	Int32& riWidth,
	Int32& riHeight)
{
	auto const sPrefix(sCodes.Substring(0, 3));
	if ("cdf" == sPrefix)
	{
		riWidth = 8;
		riHeight = 32;
	}
	else if ("cdh" == sPrefix)
	{
		riWidth = 32;
		riHeight = 8;
	}
	else if ("cds" == sPrefix)
	{
		riWidth = 8;
		riHeight = 8;
	}
	else if ('s' == sPrefix[0] && ('0' <= sPrefix[1] && sPrefix[1] <= '9'))
	{
		auto sNumber = sPrefix.CStr() + 1;
		if ('0' == sNumber[0])
		{
			++sNumber;
		}
		FromString(sNumber, riWidth);
		riHeight = riWidth;
		return;
	}
	else
	{
		riWidth = 32;
		riHeight = 32;
	}
}

void ImageTest::TestPngSuite()
{
	UnitTestsFileManagerHelper helper;
	Vector<String> vs;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->GetDirectoryListing(
		Path::Combine(GamePaths::Get()->GetConfigDir(), R"(UnitTests\Image\pngsuite)"),
		vs,
		false,
		false,
		".png"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(175, vs.GetSize());

	for (auto const& s : vs)
	{
		Int iWidth = -1;
		Int iHeight = -1;
		UInt32 uSizeInBytes = 0u;
		UInt8* p = nullptr;
		{
			void* pTemp = nullptr;
			UInt32 uTemp = 0u;
			SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(s, pTemp, uTemp, 0u, MemoryBudgets::Developer));
			auto const deferred2(MakeDeferredAction([&]() { MemoryManager::Deallocate(pTemp); }));
			p = LoadImageFromMemory((UInt8 const*)pTemp, uTemp, &iWidth, &iHeight, &uSizeInBytes);
		}
		auto const deferred(MakeDeferredAction([&]()
		{
			FreeImage(p);
		}));

		auto const sCodes(Path::GetFileNameWithoutExtension(s));
		SEOUL_UNITTESTING_ASSERT(!sCodes.IsEmpty());
		if (!p)
		{
			// Expected to fail.
			if ('x' == sCodes[0])
			{
				continue;
			}

			SEOUL_UNITTESTING_FAIL("Load failed on expected success.");
		}

		// This test was expected to fail.
		// - "xcsn0g01" - incorrect IDAT checksum
		// - "xhdn0g08" - incorrect IHDR checksum
		//
		// Both of the above fail to load in libpng, fail to load in (e.g.) Firefox,
		// but succeed in the Windows thumbnail generator and the GNU Image Manipulation Program,
		// so we accept that our current image reading backend (stb_image.h) also successfully
		// loads these images.
		if ("xcsn0g01" != sCodes && "xhdn0g08" != sCodes)
		{
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL('x', sCodes[0]);
		}

		// Test expected size.
		SEOUL_UNITTESTING_ASSERT_EQUAL(uSizeInBytes, (UInt32)(iWidth * iHeight * 4));

		// Internal knowledge of the API.
		auto const uAllocationSize = MemoryManager::GetAllocationSizeInBytes(p);
		SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(uAllocationSize, uSizeInBytes);

		SEOUL_UNITTESTING_ASSERT_EQUAL(8, sCodes.GetSize());
		Int32 iExpectedWidth = 0;
		Int32 iExpectedHeight = 0;
		GetExpected(sCodes, iExpectedWidth, iExpectedHeight);
		SEOUL_UNITTESTING_ASSERT_EQUAL(iExpectedWidth, iWidth);
		SEOUL_UNITTESTING_ASSERT_EQUAL(iExpectedHeight, iHeight);

		// Verify against raw data.
#if 0
		auto const sRgba(Path::ReplaceExtension(s, ".rgba"));
		if (!FileManager::Get()->Exists(sRgba))
		{
			SEOUL_VERIFY(FileManager::Get()->WriteAll(sRgba, p, uSizeInBytes));
		}
		else
#endif
		{
			void* pExpected = nullptr;
			UInt32 uExpected = 0u;
			SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(Path::ReplaceExtension(s, ".rgba"), pExpected, uExpected, 0u, MemoryBudgets::Developer));
			auto const deferred2(MakeDeferredAction([&]() { MemoryManager::Deallocate(pExpected); }));
			SEOUL_UNITTESTING_ASSERT_EQUAL(uSizeInBytes, uExpected);
			for (UInt32 i = 0u; i < uSizeInBytes; ++i)
			{
				UInt8 const uExpect = ((UInt8*)pExpected)[i];
				UInt8 const uTest = p[i];
				SEOUL_UNITTESTING_ASSERT_EQUAL_MESSAGE(uExpect, uTest, "(%u == %u) at %u", uExpect, uTest, i);
			}
		}
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
