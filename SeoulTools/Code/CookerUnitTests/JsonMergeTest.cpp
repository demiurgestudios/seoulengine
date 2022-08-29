/**
 * \file JsonMergeTest.cpp
 * \brief Unit tests of the SlimCS compiler and toolchain.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DataStore.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "JsonMergeTest.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "SeoulProcess.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(JsonMergeTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestDirected)
SEOUL_END_TYPE()

namespace
{

struct Entry
{
	Byte const* m_sBase;
	Byte const* m_sTheirs;
	Byte const* m_sYours;
	Byte const* m_sExpected;
};

static const Entry kaTests[] =
{
	{
		R"([["$set", "ComicConversionBaseData", "Comic_XP_C1", {"Iso8Reward": 100, "AwardComicReplacement": "Comic_Iso8_C3"}]])",
		R"([["$set", "ComicConversionBaseData", "Comic_XP_C1", {"Iso8Reward": 100, "AwardComicReplacement": "Comic_Iso8_C4"}]])",
		R"([["$set", "ComicConversionBaseData", "Comic_XP_C1", {"Iso8Reward": 150, "AwardComicReplacement": "Comic_Iso8_C3"}]])",
		R"([["$set", "ComicConversionBaseData", "Comic_XP_C1", {"Iso8Reward": 150, "AwardComicReplacement": "Comic_Iso8_C4"}]])",
	},

	{
		R"(["a", "b", "c", "d"])",
		R"(["a", "b", "d"])",
		R"(["a", "b", "c", "d"])",
		R"(["a", "b", "d"])",
	},

	{
		R"(["a", "b", "c"])",
		R"(["a", "b", "d"])",
		R"(["a", "b", "e"])",
		nullptr,
	},

	{
		R"(["a", "b", {"a": 1, "b": 2}, "d"])",
		R"(["a", {"a": 1, "b": 3}, "d"])",
		R"(["a", "b", {"a": 1, "b": 2}, "d"])",
		R"(["a", {"a": 1, "b": 3}, "d"])",
	},

	{
		R"(["a", "b", {"a": 1, "b": 2}, "d"])",
		R"(["a", {"a": 1, "b": 3}, "d"])",
		R"(["a", "b", {"a": 2, "b": 2}, "d"])",
		R"(["a", {"a": 2, "b": 3}, "d"])",
	},

	{
		R"(["a", "b", "c"])",
		R"(["a", "b", "c", "d"])",
		R"(["a", "b", "c"])",
		R"(["a", "b", "c", "d"])",
	},

	{
		R"(["a", "b", "c"])",
		R"(["a", "b", "d", "c"])",
		R"(["a", "b", "c"])",
		R"(["a", "b", "d", "c"])",
	},

	{
		R"(["a", "b", "c"])",
		R"(["a", "b", "d", "c"])",
		R"(["b", "b", "c"])",
		R"(["b", "b", "d", "c"])",
	},

	{
		R"([
			"PVP_Spiderman2099.json",
			"PVP_Classof2013.json",
			"PVP_Classof2014.json",

			//// PVP Season Events
			"PVP_Season_31.json",
		])",
		R"([
			"PVP_Spiderman2099.json",
			"PVP_Classof2013.json",
			"PVP_Classof2014.json",
			"PVP_Wolfsbane.json",

			//// PVP Season Events
			"PVP_Season_31.json",
		])",
		R"([
			"PVP_Spiderman2099.json",
			"PVP_Classof2013.json",
			"PVP_Classof2014.json",
			"PVP_Classof2019.json",
			"PVP_Reunions.json",
			"PVP_FestivalOfFights.json",

			//// PVP Season Events
			"PVP_Season_31.json",
		])",
		R"([
			"PVP_Spiderman2099.json",
			"PVP_Classof2013.json",
			"PVP_Classof2014.json",
			"PVP_Wolfsbane.json",
			"PVP_Classof2019.json",
			"PVP_Reunions.json",
			"PVP_FestivalOfFights.json",

			//// PVP Season Events
			"PVP_Season_31.json",
		])",
	},

	{
		R"({"a": "b", "b": "a"})",
		R"({"a": "c", "b": "d"})",
		R"({"a": "b", "b": "a"})",
		R"({"a": "c", "b": "d"})",
	},

	{
		R"({"a": "b", "b": "a"})",
		R"({"a": "c", "b": "d"})",
		R"({"a": "d", "b": "a"})",
		R"({"a": "d", "b": "d"})",
	},

	// Append to end test
	{
		// Base
		R"([
	[
		"BaseData",
		"Comic_Token_Pack_Release_BetaRayBill_Legendary",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_Release_BetaRayBill_Vault",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_Release_KarolinaDean_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_PVP_KarolinaDean_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	]
		])",

		// Theirs
		R"([
	[
		"BaseData",
		"Comic_Token_Pack_Release_BetaRayBill_Legendary",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_Release_BetaRayBill_Vault",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_Release_KarolinaDean_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_PVP_KarolinaDean_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_Release_Northstar_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_PVP_Northstar_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	]
		])",

		// Yours
		R"([
	[
		"BaseData",
		"Comic_Token_Pack_Release_BetaRayBill_Legendary",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_Release_BetaRayBill_Vault",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_Release_KarolinaDean_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_PVP_KarolinaDean_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_2019_Lunar_Legendary",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_2019_Lunar_Vault_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
		])",

		// Expected
		R"([
	[
		"BaseData",
		"Comic_Token_Pack_Release_BetaRayBill_Legendary",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_Release_BetaRayBill_Vault",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_Release_KarolinaDean_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_PVP_KarolinaDean_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_Release_Northstar_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_PVP_Northstar_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_2019_Lunar_Legendary",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
	[
		"BaseData",
		"Comic_Token_Pack_2019_Lunar_Vault_HP",
		{
			"AttributionLine1": "Comic_Blank_Subtitle",
		}
	],
		])",
	},
};

static Bool Compare(Byte const* sBody, const String& sIn)
{
	void* p = nullptr;
	UInt32 u = 0u;
	if (!FileManager::Get()->ReadAll(sIn, p, u, 0u, MemoryBudgets::Developer))
	{
		return false;
	}

	DataStore expected, actual;

	auto b = true;
	b = b && DataStoreParser::FromString(sBody, expected, DataStoreParserFlags::kLeaveFilePathAsString);
	b = b && DataStoreParser::FromString((Byte const*)p, u, actual, DataStoreParserFlags::kLeaveFilePathAsString);
	b = b && DataStore::Equals(expected, expected.GetRootNode(), actual, actual.GetRootNode(), true);

	if (!b)
	{
		SEOUL_LOG_UNIT_TEST("%s", String((Byte const*)p, u).CStr());
	}

	return b;
}

static Bool Write(Byte const* sBody, const String& sOut)
{
	return FileManager::Get()->WriteAll(sOut, sBody, StrLen(sBody));
}

} // namespace anonymous

void JsonMergeTest::TestDirected()
{
	UnitTestsFileManagerHelper helper;

	// Process path.
	auto const sExe(Path::Combine(GamePaths::Get()->GetToolsBinDir(), "JsonMerge.exe"));

	// Run tests.
	auto const sTemp(Path::Combine(Path::GetTempDirectory(), "JsonMergeTest"));
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->CreateDirPath(sTemp));
	auto const sBase(Path::Combine(sTemp, "base.json"));
	auto const sTheirs(Path::Combine(sTemp, "theirs.json"));
	auto const sYours(Path::Combine(sTemp, "yours.json"));
	auto const sExpected(Path::Combine(sTemp, "expected.json"));
	for (auto const& entry : kaTests)
	{
		// Prep files.
		SEOUL_UNITTESTING_ASSERT(Write(entry.m_sBase, sBase));
		SEOUL_UNITTESTING_ASSERT(Write(entry.m_sTheirs, sTheirs));
		SEOUL_UNITTESTING_ASSERT(Write(entry.m_sYours, sYours));


		// Run.
		Int32 iResult = -1;
		{
			auto const aIn = {  sBase, sTheirs, sYours, sExpected };
			Process process(sExe, Process::ProcessArguments(aIn.begin(), aIn.end()));
			if (!process.Start())
			{
				iResult = -1;
			}
			else
			{
				iResult = process.WaitUntilProcessIsNotRunning();
			}
		}

		if (nullptr == entry.m_sExpected)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, iResult);
		}
		else
		{
			if (iResult != 0)
			{
				SEOUL_LOG("Failed test entry: %s", entry.m_sExpected);
			}
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, iResult);
			SEOUL_UNITTESTING_ASSERT(Compare(entry.m_sExpected, sExpected));
		}
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
