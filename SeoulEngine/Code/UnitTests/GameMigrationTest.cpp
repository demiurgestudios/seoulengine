/**
* \file GameMigrationTest.cpp
* \brief Test for save game migrations.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "GameMigrationTest.h"

#include "Logger.h"
#include "ReflectionAny.h"
#include "ReflectionAttributes.h"
#include "ReflectionDefine.h"
#include "ReflectionRegistry.h"
#include "ReflectionType.h"
#include "UnitTesting.h"
#include "UnitTestsGameHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(GameMigrationTest, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestMigrations)
SEOUL_END_TYPE();

GameMigrationTest::GameMigrationTest()
	: m_pHelper()
{
	m_pHelper.Reset(SEOUL_NEW(MemoryBudgets::Developer) UnitTestsGameHelper(
		"http://localhost:8057", String(), String(), String()));
}

GameMigrationTest::~GameMigrationTest()
{
	m_pHelper.Reset();
}

void GameMigrationTest::TestMigrations()
{
	using namespace Reflection;
	using namespace Reflection::Attributes;

	UInt32 const nCount = Registry::GetRegistry().GetTypeCount();
	for (UInt32 iType = 0u; iType < nCount; ++iType)
	{
		Type const* pType = Registry::GetRegistry().GetType(iType);
		if (nullptr == pType)
		{
			continue;
		}

		UInt32 const nMethods = pType->GetMethodCount();
		for (UInt32 iMethod = 0u; iMethod < nMethods; ++iMethod)
		{
			Method const* pMethod = pType->GetMethod(iMethod);
			if (nullptr == pMethod)
			{
				continue;
			}

			for (const auto& attribute : pMethod->GetAttributes().GetAttributeVector())
			{
				if (attribute->GetId() == PersistenceDataMigrationTest::StaticId())
				{
					RunMigrationTest(pType->GetName(), pMethod, (const PersistenceDataMigrationTest*)attribute);
				}
			}
		}
	}
}

void GameMigrationTest::RunMigrationTest(HString sTypeName, const Reflection::Method *pMethod, const Reflection::Attributes::PersistenceDataMigrationTest* attribute)
{
	using namespace Reflection;
	using namespace Reflection::Attributes;

	if (nullptr == attribute)
	{
		SEOUL_UNITTESTING_FAIL("Data migration missing PersistenceDataMigrationTest attribute: %s::%s", sTypeName.CStr(), pMethod->GetName().CStr());
	}

	DataStore dataStoreBefore;
	if (!DataStoreParser::FromString(attribute->m_sBefore, StrLen(attribute->m_sBefore), dataStoreBefore))
	{
		SEOUL_UNITTESTING_FAIL("Failed to parse \"before\" JSON");
	}

	DataStore dataStoreAfter;
	if (!DataStoreParser::FromString(attribute->m_sAfter, StrLen(attribute->m_sAfter), dataStoreAfter))
	{
		SEOUL_UNITTESTING_FAIL("Failed to parse \"after\" JSON");
	}

	Any ret;
	MethodArguments a;
	a[0] = &dataStoreBefore;
	a[1] = dataStoreBefore.GetRootNode();
	SEOUL_UNITTESTING_ASSERT(pMethod->TryInvoke(ret, WeakAny(), a));

	Bool const bReturn = ret.Cast<Bool>();
	SEOUL_UNITTESTING_ASSERT_MESSAGE(bReturn, "Migration failed");

	// Before must have been changed to match expected
	Bool bEqual = DataStore::Equals(dataStoreBefore, dataStoreBefore.GetRootNode(), dataStoreAfter, dataStoreAfter.GetRootNode());
	if (!bEqual)
	{
		String s;
		dataStoreBefore.ToString(dataStoreBefore.GetRootNode(), s, true, 0, true);
		SEOUL_UNITTESTING_ASSERT_MESSAGE(
			bEqual,
			"Unexpected migration output (ignoring order, JSON syntax/commas):\n\n%s\n\n", s.CStr());
	}
}


#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
