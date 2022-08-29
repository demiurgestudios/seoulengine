/**
 * \file CookDatabaseTest.cpp
 * \brief Unit test header file for the CookDatabase project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CookDatabase.h"
#include "CookDatabaseTest.h"
#include "FileManager.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"
#include "Vector.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(CookDatabaseTest)
	SEOUL_ATTRIBUTE(UnitTest)
#if !SEOUL_PLATFORM_ANDROID && !SEOUL_PLATFORM_IOS // Disabled on mobile.
	SEOUL_METHOD(TestMetadata)
#endif
SEOUL_END_TYPE()

static inline Bool operator==(const CookMetadata::Source& a, const FilePath& b)
{
	return (a.m_Source == b);
}

void CookDatabaseTest::TestMetadata()
{
	UnitTestsFileManagerHelper scoped;

	CookDatabase database(keCurrentPlatform, false);

	FilePath dir;
	dir.SetDirectory(GameDirectory::kContent);
	Vector<String> vsFiles;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->GetDirectoryListing(dir, vsFiles, false, true));

	// TODO: We set a max number of files so the test terminates in a reasonable amount of time. Not ideal - probably,
	// should change this test to use a known fixed set of files, but I like the possibility of catching
	// a new issue with this test run on live data.
	static const UInt32 kuMaxFiles = 1024u;

	UInt32 uCount = 0u;
	for (auto const& s : vsFiles)
	{
		auto const filePath = FilePath::CreateContentFilePath(s);
		if (!filePath.IsValid())
		{
			continue;
		}

		// TODO: Generalize.
		if (filePath.GetType() == FileType::kJson)
		{
			continue;
		}

		// TODO: Update once sound banks have metadata.
		if (filePath.GetType() == FileType::kSoundBank)
		{
			continue;
		}

		// No metadata for one-to-one types.
		if (CookDatabase::IsOneToOneType(filePath.GetType()))
		{
			continue;
		}

		CookMetadata metadata;
		SEOUL_UNITTESTING_ASSERT_MESSAGE(database.UnitTestHook_GetMetadata(filePath, metadata), "Failed getting metadata for: %s", filePath.CStr());

		String sMetadataPath(
			filePath.GetAbsoluteFilename() + ".json");

		SEOUL_UNITTESTING_ASSERT_EQUAL(metadata.m_uCookedTimestamp, FileManager::Get()->GetModifiedTime(filePath));
		SEOUL_UNITTESTING_ASSERT_EQUAL(39u, metadata.m_uCookerVersion);
		SEOUL_UNITTESTING_ASSERT_EQUAL(CookDatabase::GetDataVersion(filePath.GetType()), metadata.m_uDataVersion);
		SEOUL_UNITTESTING_ASSERT_EQUAL(metadata.m_uMetadataTimestamp, FileManager::Get()->GetModifiedTime(sMetadataPath));
		SEOUL_UNITTESTING_ASSERT(!metadata.m_vSources.IsEmpty());

		// Resolve type for source lookup.
		auto sourceFilePath = filePath;
		if (IsTextureFileType(sourceFilePath.GetType()))
		{
			sourceFilePath.SetType(FileType::kTexture0);
		}

		auto const i = metadata.m_vSources.Find(sourceFilePath);
		if (metadata.m_vSources.End() == i)
		{
			SEOUL_LOG("Failed finding source: %s", sourceFilePath.GetAbsoluteFilenameInSource().CStr());
		}
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(metadata.m_vSources.End(), i);
		SEOUL_UNITTESTING_ASSERT_EQUAL(i->m_uTimestamp, FileManager::Get()->GetModifiedTime(i->m_Source.GetAbsoluteFilenameInSource()));

		SEOUL_UNITTESTING_ASSERT(database.CheckUpToDate(filePath));

		// Check dependents - not expected to contain itself.
		CookDatabase::Dependents v;
		database.GetDependents(filePath, v);
		SEOUL_UNITTESTING_ASSERT(!v.Contains(filePath));
		SEOUL_UNITTESTING_ASSERT(!v.Contains(sourceFilePath));
		database.GetDependents(sourceFilePath, v);
		SEOUL_UNITTESTING_ASSERT(!v.Contains(filePath));
		SEOUL_UNITTESTING_ASSERT(!v.Contains(sourceFilePath));

		++uCount;
		if (uCount >= kuMaxFiles)
		{
			break;
		}
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
