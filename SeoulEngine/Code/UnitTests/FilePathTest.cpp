/**
 * \file FilePathTest.cpp
 * \brief Unit tests to verify that the FilePath class handles
 * and normalizes file paths as we expect.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FilePath.h"
#include "FilePathTest.h"
#include "GamePaths.h"
#include "GamePathsSettings.h"
#include "Logger.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "SeoulFile.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(FilePathTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasicFilePath)
	SEOUL_METHOD(TestAdvancedFilePath)
	SEOUL_METHOD(TestFilePathUtil)
SEOUL_END_TYPE()

void FilePathTest::TestBasicFilePath()
{
	static const String ksTestFilename = "test.png";
	static const String ksTestFilename2 = "TeSt.png";

	UnitTestsFileManagerHelper scoped;

	FilePath filePath;
	SEOUL_UNITTESTING_ASSERT(!filePath.IsValid());

	SEOUL_UNITTESTING_ASSERT(
		(filePath = FilePath::CreateConfigFilePath(ksTestFilename)) ==
		FilePath::CreateFilePath(GameDirectory::kConfig, ksTestFilename));
	SEOUL_UNITTESTING_ASSERT(
		(filePath = FilePath::CreateConfigFilePath(ksTestFilename)) ==
		FilePath::CreateFilePath(GameDirectory::kConfig, ksTestFilename2));
	SEOUL_UNITTESTING_ASSERT(filePath.IsValid());

	SEOUL_UNITTESTING_ASSERT(
		(filePath = FilePath::CreateContentFilePath(ksTestFilename)) ==
		FilePath::CreateFilePath(GameDirectory::kContent, ksTestFilename));
	SEOUL_UNITTESTING_ASSERT(
		(filePath = FilePath::CreateContentFilePath(ksTestFilename)) ==
		FilePath::CreateFilePath(GameDirectory::kContent, ksTestFilename2));
	SEOUL_UNITTESTING_ASSERT(filePath.IsValid());

	SEOUL_UNITTESTING_ASSERT(
		(filePath = FilePath::CreateLogFilePath(ksTestFilename)) ==
		FilePath::CreateFilePath(GameDirectory::kLog, ksTestFilename));
	SEOUL_UNITTESTING_ASSERT(
		(filePath = FilePath::CreateLogFilePath(ksTestFilename)) ==
		FilePath::CreateFilePath(GameDirectory::kLog, ksTestFilename2));
	SEOUL_UNITTESTING_ASSERT(filePath.IsValid());

	SEOUL_UNITTESTING_ASSERT(
		(filePath = FilePath::CreateSaveFilePath(ksTestFilename)) ==
		FilePath::CreateFilePath(GameDirectory::kSave, ksTestFilename));
	SEOUL_UNITTESTING_ASSERT(
		(filePath = FilePath::CreateSaveFilePath(ksTestFilename)) ==
		FilePath::CreateFilePath(GameDirectory::kSave, ksTestFilename2));
	SEOUL_UNITTESTING_ASSERT(filePath.IsValid());

	SEOUL_UNITTESTING_ASSERT(
		(filePath = FilePath::CreateToolsBinFilePath(ksTestFilename)) ==
		FilePath::CreateFilePath(GameDirectory::kToolsBin, ksTestFilename));
	SEOUL_UNITTESTING_ASSERT(
		(filePath = FilePath::CreateToolsBinFilePath(ksTestFilename)) ==
		FilePath::CreateFilePath(GameDirectory::kToolsBin, ksTestFilename2));
	SEOUL_UNITTESTING_ASSERT(filePath.IsValid());

	SEOUL_UNITTESTING_ASSERT(
		(filePath = FilePath::CreateVideosFilePath(ksTestFilename)) ==
		FilePath::CreateFilePath(GameDirectory::kVideos, ksTestFilename));
	SEOUL_UNITTESTING_ASSERT(
		(filePath = FilePath::CreateVideosFilePath(ksTestFilename)) ==
		FilePath::CreateFilePath(GameDirectory::kVideos, ksTestFilename2));
	SEOUL_UNITTESTING_ASSERT(filePath.IsValid());
}

void FilePathTest::TestAdvancedFilePath()
{
	const FilePathRelativeFilename kExpectedRelativePathWithoutExtension("test");
	const String ksTestFilenameWithPngExtension = String::Printf("%s.png", kExpectedRelativePathWithoutExtension.CStr());
	const String ksTestFilenameWithSif0Extension = String::Printf("%s.sif0", kExpectedRelativePathWithoutExtension.CStr());
	const String ksTestFilenameWithSif1Extension = String::Printf("%s.sif1", kExpectedRelativePathWithoutExtension.CStr());

	UnitTestsFileManagerHelper scoped;

	const String sTestContent = GamePaths::Get()->GetContentDir();
	const String sTestSource = GamePaths::Get()->GetSourceDir();
	String sAbsoluteContentPath = Path::Normalize(Path::Combine(sTestContent, ksTestFilenameWithSif0Extension));
	String sAbsoluteSourcePath = Path::Normalize(Path::Combine(sTestSource, ksTestFilenameWithPngExtension));

	FilePath filePath = FilePath::CreateContentFilePath(sAbsoluteContentPath);
	SEOUL_UNITTESTING_ASSERT(filePath.IsValid());
	SEOUL_UNITTESTING_ASSERT(filePath.GetRelativeFilenameWithoutExtension() == kExpectedRelativePathWithoutExtension);
	SEOUL_UNITTESTING_ASSERT(filePath.GetDirectory() == GameDirectory::kContent);
	SEOUL_UNITTESTING_ASSERT(filePath.GetType() == FileType::kTexture0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(sAbsoluteContentPath, filePath.GetAbsoluteFilename());
	SEOUL_UNITTESTING_ASSERT_EQUAL(sAbsoluteSourcePath, filePath.GetAbsoluteFilenameInSource());
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		Path::GetFileNameWithoutExtension(ksTestFilenameWithPngExtension),
		String(filePath.CStr()));
	SEOUL_UNITTESTING_ASSERT(
		GameDirectory::kContent == filePath.GetDirectory());
	SEOUL_UNITTESTING_ASSERT(
		FileType::kTexture0 == filePath.GetType());

	filePath.SetDirectory(GameDirectory::kConfig);
	SEOUL_UNITTESTING_ASSERT(
		GameDirectory::kConfig == filePath.GetDirectory());
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		filePath.GetAbsoluteFilenameInSource(),
		Path::Normalize(Path::Combine(GamePaths::Get()->GetConfigDir(), ksTestFilenameWithPngExtension)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		filePath.GetAbsoluteFilename(),
		Path::Normalize(Path::Combine(GamePaths::Get()->GetConfigDir(), ksTestFilenameWithSif0Extension)));

	filePath.SetType(FileType::kTexture1);
	SEOUL_UNITTESTING_ASSERT(
		FileType::kTexture1 == filePath.GetType());
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		filePath.GetAbsoluteFilenameInSource(),
		Path::Normalize(Path::Combine(GamePaths::Get()->GetConfigDir(), ksTestFilenameWithPngExtension)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		filePath.GetAbsoluteFilename(),
		Path::Normalize(Path::Combine(GamePaths::Get()->GetConfigDir(), ksTestFilenameWithSif1Extension)));

	filePath.Reset();
	SEOUL_UNITTESTING_ASSERT(!filePath.IsValid());
}

void FilePathTest::TestFilePathUtil()
{
	UnitTestsFileManagerHelper scoped;

	for (Int i = 0; i < (Int)GameDirectory::GAME_DIRECTORY_COUNT; ++i)
	{
		if ((Int)GameDirectory::kContent == i)
		{
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(
				GameDirectoryToString((GameDirectory)i),
				GameDirectoryToStringInSource((GameDirectory)i));
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				GameDirectoryToString((GameDirectory)i),
				GameDirectoryToStringInSource((GameDirectory)i));
		}
	}

	for (Int i = 0; i < (Int)FileType::FILE_TYPE_COUNT; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			ExtensionToFileType(FileTypeToCookedExtension((FileType)i)),
			(FileType)i);
		if (IsTextureFileType((FileType)i))
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ExtensionToFileType(FileTypeToSourceExtension((FileType)i)),
				FileType::kTexture0);
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ExtensionToFileType(FileTypeToSourceExtension((FileType)i)),
				(FileType)i);
		}
	}

	for (Int i = (Int)FileType::FIRST_TEXTURE_TYPE; i <= (Int)FileType::LAST_TEXTURE_TYPE; ++i)
	{
		SEOUL_UNITTESTING_ASSERT(IsTextureFileType((FileType)i));
	}

	for (Int i = 0; i < (Int)GameDirectory::GAME_DIRECTORY_COUNT; ++i)
	{
		auto const s(Path::Combine(
			GameDirectoryToStringInSource((GameDirectory)i),
			"test.png"));
		auto const e(GetGameDirectoryFromAbsoluteFilename(s));
		SEOUL_UNITTESTING_ASSERT_EQUAL((GameDirectory)i, e);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
