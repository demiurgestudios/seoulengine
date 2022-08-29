/**
 * \file LocManagerTest.cpp
 * \brief Unit tests for the LocManager singleton.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DiskFileSystem.h"
#include "FileManager.h"
#include "LocManager.h"
#include "LocManagerTest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include "UnitTestsEngineHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(LocManagerTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestTimeFormat)
	SEOUL_METHOD(TestPatchAdditive)
	SEOUL_METHOD(TestPatchSubtractive)
	SEOUL_METHOD(TestPatchSubtractiveRegression)
SEOUL_END_TYPE()

void LocManagerTest::TestBasic()
{
	UnitTestsEngineHelper helper;

	auto& locManager = *LocManager::Get();
	SEOUL_UNITTESTING_ASSERT_EQUAL("English", locManager.GetCurrentLanguage());
	SEOUL_UNITTESTING_ASSERT_EQUAL("en", locManager.GetCurrentLanguageCode());
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize("yes_no_message_box_no_button_label"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize("yes_no_message_box_no_button_label", 34));
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize(HString("yes_no_message_box_no_button_label")));
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize(String("yes_no_message_box_no_button_label")));
	SEOUL_UNITTESTING_ASSERT_EQUAL("Yes", locManager.Localize("yes_no_message_box_yes_button_label"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("Yes", locManager.Localize("yes_no_message_box_yes_button_label", 35));
	SEOUL_UNITTESTING_ASSERT_EQUAL("Yes", locManager.Localize(HString("yes_no_message_box_yes_button_label")));
	SEOUL_UNITTESTING_ASSERT_EQUAL("Yes", locManager.Localize(String("yes_no_message_box_yes_button_label")));

	// number formatting
	SEOUL_UNITTESTING_ASSERT_EQUAL("0", locManager.FormatNumber(0, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("0.0", locManager.FormatNumber(0, 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL("0.00", locManager.FormatNumber(0, 2));
	SEOUL_UNITTESTING_ASSERT_EQUAL("10", locManager.FormatNumber(10, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("100", locManager.FormatNumber(100, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("1000", locManager.FormatNumber(1000, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("9999", locManager.FormatNumber(9999, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("99,999", locManager.FormatNumber(99999, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("100,999", locManager.FormatNumber(100999, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("1,000,000", locManager.FormatNumber(1000000, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("10", locManager.FormatNumber(10.5, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("4.1", locManager.FormatNumber(4.111, 1)); // rounding/truncation check
	SEOUL_UNITTESTING_ASSERT_EQUAL("4.4", locManager.FormatNumber(4.411, 1)); // rounding/truncation check
	SEOUL_UNITTESTING_ASSERT_EQUAL("4.8", locManager.FormatNumber(4.811, 1)); // rounding/truncation check

	SEOUL_UNITTESTING_ASSERT_EQUAL("4.2", locManager.FormatNumber(4.191, 1)); // rounding/truncation check
	SEOUL_UNITTESTING_ASSERT_EQUAL("4.5", locManager.FormatNumber(4.491, 1)); // rounding/truncation check
	SEOUL_UNITTESTING_ASSERT_EQUAL("4.9", locManager.FormatNumber(4.891, 1)); // rounding/truncation check
	SEOUL_UNITTESTING_ASSERT_EQUAL("445.5", locManager.FormatNumber(445.462, 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL("99,999.0", locManager.FormatNumber(99999, 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL("124,897", locManager.FormatNumber(124897.12312, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("124,897.1", locManager.FormatNumber(124897.12312, 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL("124,897.12", locManager.FormatNumber(124897.12312, 2));
	SEOUL_UNITTESTING_ASSERT_EQUAL("124,897.123", locManager.FormatNumber(124897.12312, 3));
	SEOUL_UNITTESTING_ASSERT_EQUAL("124,897.1231", locManager.FormatNumber(124897.12312, 4));
	SEOUL_UNITTESTING_ASSERT_EQUAL("124,897.12312", locManager.FormatNumber(124897.12312, 5));
	SEOUL_UNITTESTING_ASSERT_EQUAL("1,000,000.1", locManager.FormatNumber(1000000.14, 1));

	// negative.
	SEOUL_UNITTESTING_ASSERT_EQUAL("-1", locManager.FormatNumber(-1, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("-10", locManager.FormatNumber(-10, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("-100", locManager.FormatNumber(-100, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("-1000", locManager.FormatNumber(-1000, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("-10,000", locManager.FormatNumber(-10000, 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL("-5.5", locManager.FormatNumber(-5.5, 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL("-50.16", locManager.FormatNumber(-50.16, 2));
	SEOUL_UNITTESTING_ASSERT_EQUAL("-50.13", locManager.FormatNumber(-50.127, 2));
	SEOUL_UNITTESTING_ASSERT_EQUAL("-99,999.432", locManager.FormatNumber(-99999.432234, 3));
}

void LocManagerTest::TestTimeFormat()
{
	UnitTestsEngineHelper helper;

	auto& locManager = *LocManager::Get();
	HString d = HString("d");
	HString h = HString("h");
	HString m = HString("m");
	HString s = HString("s");

	SEOUL_UNITTESTING_ASSERT_EQUAL("0s", locManager.TimeToString(0,d,h,m,s));
	SEOUL_UNITTESTING_ASSERT_EQUAL("1s", locManager.TimeToString(1,d,h,m,s));
	SEOUL_UNITTESTING_ASSERT_EQUAL("0s", locManager.TimeToString(0.5f,d,h,m,s));
	SEOUL_UNITTESTING_ASSERT_EQUAL("59s", locManager.TimeToString(59,d,h,m,s));
	SEOUL_UNITTESTING_ASSERT_EQUAL("1m 1s", locManager.TimeToString(61,d,h,m,s));
	SEOUL_UNITTESTING_ASSERT_EQUAL("2m", locManager.TimeToString(120,d,h,m,s));
	SEOUL_UNITTESTING_ASSERT_EQUAL("59m 59s", locManager.TimeToString(3599,d,h,m,s));
	SEOUL_UNITTESTING_ASSERT_EQUAL("1h", locManager.TimeToString(3600,d,h,m,s));
	SEOUL_UNITTESTING_ASSERT_EQUAL("2h 1m", locManager.TimeToString(7261,d,h,m,s)); //We cap at 2 time types
	SEOUL_UNITTESTING_ASSERT_EQUAL("23h 59m", locManager.TimeToString(86399,d,h,m,s)); //We cap at 2 time types
	SEOUL_UNITTESTING_ASSERT_EQUAL("1d", locManager.TimeToString(86400,d,h,m,s));
	SEOUL_UNITTESTING_ASSERT_EQUAL("1d 1s", locManager.TimeToString(86401,d,h,m,s));
	SEOUL_UNITTESTING_ASSERT_EQUAL("7d", locManager.TimeToString(604800,d,h,m,s));
}

namespace
{

class LocPatchTestFileSystem SEOUL_SEALED : public IFileSystem
{
public:
	LocPatchTestFileSystem(Byte const* pData, UInt32 uData)
		: m_FilePath(FilePath::CreateConfigFilePath("Loc/English/locale_patch.json"))
		, m_vData(pData, pData + uData)
	{
	}

	// Absolute string paths are not supported.
	virtual Bool Copy(const String& sFrom, const String& sTo, Bool bAllowOverwrite = false) SEOUL_OVERRIDE { return false; }
	virtual Bool CreateDirPath(const String& sAbsoluteDir) SEOUL_OVERRIDE { return false; }
	virtual Bool Delete(const String& sAbsoluteFilename) SEOUL_OVERRIDE { return false; }
	virtual Bool DeleteDirectory(const String& sAbsoluteDir, Bool bRecursive) SEOUL_OVERRIDE { return false; }
	virtual Bool GetFileSize(const String& sAbsoluteFilename, UInt64& rzFileSize) const SEOUL_OVERRIDE { return false; }
	virtual Bool GetModifiedTime(const String& sAbsoluteFilename, UInt64& ruModifiedTime) const SEOUL_OVERRIDE { return false; }
	virtual Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime) SEOUL_OVERRIDE { return false; }
	virtual Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly) SEOUL_OVERRIDE { return false; }
	virtual Bool Exists(const String& sAbsoluteFilename) const SEOUL_OVERRIDE { return false; }
	virtual Bool IsDirectory(const String& sAbsoluteFilename) const SEOUL_OVERRIDE { return false; }
	virtual Bool Open(const String& sAbsoluteFilename, File::Mode eMode, ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE { return false; }
	virtual Bool GetDirectoryListing(const String& sAbsoluteDirectoryPath, Vector<String>& rvResults, Bool bIncludeDirectoriesInResults = true, Bool bRecursive = true, const String& sFileExtension = String()) const SEOUL_OVERRIDE { return false; }
	virtual Bool Rename(const String& sFrom, const String& sTo) SEOUL_OVERRIDE { return false; }
	// /End absolute paths.

	virtual Bool GetFileSize(FilePath filePath, UInt64& rzFileSize) const SEOUL_OVERRIDE
	{
		if (m_FilePath == filePath)
		{
			rzFileSize = m_vData.GetSize();
			return true;
		}
		return false;
	}

	virtual Bool GetModifiedTime(FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		if (m_FilePath == filePath)
		{
			ruModifiedTime = 1u;
			return true;
		}
		return false;
	}

	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) SEOUL_OVERRIDE { return false; }
	virtual Bool CreateDirPath(FilePath dirPath) SEOUL_OVERRIDE { return false; }
	virtual Bool Delete(FilePath filePath) SEOUL_OVERRIDE { return false; }
	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) SEOUL_OVERRIDE { return false; }
	virtual Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE { return false; }
	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) SEOUL_OVERRIDE { return false; }
	virtual Bool Exists(FilePath filePath) const SEOUL_OVERRIDE { return (m_FilePath == filePath); }
	virtual Bool IsDirectory(FilePath filePath) const SEOUL_OVERRIDE { return false; }
	virtual Bool Rename(FilePath from, FilePath to) SEOUL_OVERRIDE { return false; }

	virtual Bool Open(
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		if (m_FilePath == filePath && File::kRead == eMode)
		{
			void* p = MemoryManager::Allocate(m_vData.GetSize(), MemoryBudgets::Io);
			memcpy(p, m_vData.Data(), m_vData.GetSize());
			rpFile.Reset(SEOUL_NEW(MemoryBudgets::Io) FullyBufferedSyncFile(p, m_vData.GetSize(), true, filePath.GetAbsoluteFilename()));
			return true;
		}

		return false;
	}

	virtual Bool GetDirectoryListing(
		FilePath filePath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE { return false; }

private:
	FilePath const m_FilePath;
	Vector<Byte, MemoryBudgets::Developer> const m_vData;

	SEOUL_DISABLE_COPY(LocPatchTestFileSystem);
}; // class LocPatchTestFileSystem

} // namespace anonymous

static void LocManagerAdditivePatchTestFileSystems()
{
	static const String ksAdditivePatchTestData("{\"yes_no_message_box_yes_button_label\":\"Not Yes\"}");

	FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
	FileManager::Get()->RegisterFileSystem<LocPatchTestFileSystem>(ksAdditivePatchTestData.CStr(), ksAdditivePatchTestData.GetSize());
}

static void LocManagerSubtractivePatchTestFileSystems()
{
	static const String ksSubtractivePatchTestData("{\"yes_no_message_box_yes_button_label\":null}");

	FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
	FileManager::Get()->RegisterFileSystem<LocPatchTestFileSystem>(ksSubtractivePatchTestData.CStr(), ksSubtractivePatchTestData.GetSize());
}

// Need to generate a proper binary diff to verify that
// LocManager behaves correctly with SpecialErase is emitted.
static void LocManagerSubtractivePatchTestFileSystemsRegression()
{
	FileManager::Get()->RegisterFileSystem<DiskFileSystem>();

	// Load the base.
	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromFile(nullptr, FilePath::CreateConfigFilePath("Loc/English/locale.json"), dataStore));

	// Create a delta, just remove the one key.
	DataStore target;
	target.CopyFrom(dataStore);
	SEOUL_UNITTESTING_ASSERT(target.EraseValueFromTable(target.GetRootNode(), HString("yes_no_message_box_yes_button_label")));

	// Delta.
	DataStore delta;
	SEOUL_UNITTESTING_ASSERT(ComputeDiff(dataStore, target, delta));

	// Save.
	MemorySyncFile file;
	SEOUL_UNITTESTING_ASSERT(delta.Save(file, keCurrentPlatform));

	auto const& buffer = file.GetBuffer();
	FileManager::Get()->RegisterFileSystem<LocPatchTestFileSystem>(buffer.GetBuffer(), buffer.GetTotalDataSizeInBytes());
}

void LocManagerTest::TestPatchAdditive()
{
	UnitTestsEngineHelper helper(LocManagerAdditivePatchTestFileSystems);

	auto& locManager = *LocManager::Get();
	SEOUL_UNITTESTING_ASSERT_EQUAL("English", locManager.GetCurrentLanguage());
	SEOUL_UNITTESTING_ASSERT_EQUAL("en", locManager.GetCurrentLanguageCode());
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize("yes_no_message_box_no_button_label"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize("yes_no_message_box_no_button_label", 34));
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize(HString("yes_no_message_box_no_button_label")));
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize(String("yes_no_message_box_no_button_label")));
	SEOUL_UNITTESTING_ASSERT_EQUAL("Not Yes", locManager.Localize("yes_no_message_box_yes_button_label"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("Not Yes", locManager.Localize("yes_no_message_box_yes_button_label", 35));
	SEOUL_UNITTESTING_ASSERT_EQUAL("Not Yes", locManager.Localize(HString("yes_no_message_box_yes_button_label")));
	SEOUL_UNITTESTING_ASSERT_EQUAL("Not Yes", locManager.Localize(String("yes_no_message_box_yes_button_label")));
}

void LocManagerTest::TestPatchSubtractive()
{
	UnitTestsEngineHelper helper(LocManagerSubtractivePatchTestFileSystems);

	auto& locManager = *LocManager::Get();
	SEOUL_UNITTESTING_ASSERT_EQUAL("English", locManager.GetCurrentLanguage());
	SEOUL_UNITTESTING_ASSERT_EQUAL("en", locManager.GetCurrentLanguageCode());
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize("yes_no_message_box_no_button_label"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize("yes_no_message_box_no_button_label", 34));
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize(HString("yes_no_message_box_no_button_label")));
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize(String("yes_no_message_box_no_button_label")));
	SEOUL_UNITTESTING_ASSERT_EQUAL("yes_no_message_box_yes_button_label", locManager.Localize("yes_no_message_box_yes_button_label"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("yes_no_message_box_yes_button_label", locManager.Localize("yes_no_message_box_yes_button_label", 35));
	SEOUL_UNITTESTING_ASSERT_EQUAL("yes_no_message_box_yes_button_label", locManager.Localize(HString("yes_no_message_box_yes_button_label")));
	SEOUL_UNITTESTING_ASSERT_EQUAL("yes_no_message_box_yes_button_label", locManager.Localize(String("yes_no_message_box_yes_button_label")));
}

// Test for a bug in subtractive loc patches if a binary
// storage format is used, since subtractions are
// stored as SpecialErase in this case instead of a null value.
void LocManagerTest::TestPatchSubtractiveRegression()
{
	UnitTestsEngineHelper helper(LocManagerSubtractivePatchTestFileSystemsRegression);

	auto& locManager = *LocManager::Get();
	SEOUL_UNITTESTING_ASSERT_EQUAL("English", locManager.GetCurrentLanguage());
	SEOUL_UNITTESTING_ASSERT_EQUAL("en", locManager.GetCurrentLanguageCode());
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize("yes_no_message_box_no_button_label"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize("yes_no_message_box_no_button_label", 34));
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize(HString("yes_no_message_box_no_button_label")));
	SEOUL_UNITTESTING_ASSERT_EQUAL("No", locManager.Localize(String("yes_no_message_box_no_button_label")));
	SEOUL_UNITTESTING_ASSERT_EQUAL("yes_no_message_box_yes_button_label", locManager.Localize("yes_no_message_box_yes_button_label"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("yes_no_message_box_yes_button_label", locManager.Localize("yes_no_message_box_yes_button_label", 35));
	SEOUL_UNITTESTING_ASSERT_EQUAL("yes_no_message_box_yes_button_label", locManager.Localize(HString("yes_no_message_box_yes_button_label")));
	SEOUL_UNITTESTING_ASSERT_EQUAL("yes_no_message_box_yes_button_label", locManager.Localize(String("yes_no_message_box_yes_button_label")));
}

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
