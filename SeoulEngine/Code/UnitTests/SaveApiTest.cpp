/**
 * \file SaveApiTest.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileManager.h"
#include "GenericSaveApi.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "SaveApiTest.h"
#include "UnitTesting.h"
#include "UnitTestsEngineHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(SaveApiTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestGeneric)
SEOUL_END_TYPE()

void SaveApiTest::TestGeneric()
{
	static const String ksTestData("Hello World");

	UnitTestsEngineHelper helper;
	GenericSaveApi api;

	auto const filePath(FilePath::CreateSaveFilePath("generic_api_test.dat"));

	// Basic, success.
	{
		StreamBuffer data;
		data.Write(ksTestData);
		SEOUL_UNITTESTING_ASSERT_EQUAL(SaveLoadResult::kSuccess, api.Save(filePath, data));
	}
	{
		StreamBuffer data;
		SEOUL_UNITTESTING_ASSERT_EQUAL(SaveLoadResult::kSuccess, api.Load(filePath, data));

		String s;
		SEOUL_UNITTESTING_ASSERT(data.Read(s));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ksTestData, s);
	}
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->Delete(filePath));
}

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
