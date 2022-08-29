/**
 * \file PathTest.cpp
 * \brief Unit tests to verify that the functions in namespace Path
 * correctly handle file paths.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Directory.h"
#include "DiskFileSystem.h"
#include "Logger.h"
#include "GamePaths.h"
#include "Path.h"
#include "PathTest.h"
#include "ReflectionDefine.h"
#include "SeoulFile.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(PathTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAdvancedPath)
	SEOUL_METHOD(TestBasicPath)
	SEOUL_METHOD(TestCombine)
	SEOUL_METHOD(TestGetExactPathName)
	SEOUL_METHOD(TestGetDirectoryNameN)
	SEOUL_METHOD(TestGetProcessDirectory)
	SEOUL_METHOD(TestGetTempDirectory)
	SEOUL_METHOD(TestGetTempFileAbsoluteFilename)
	SEOUL_METHOD(TestSingleDotRegression)
SEOUL_END_TYPE()

void PathTest::TestAdvancedPath()
{
#if SEOUL_PLATFORM_WINDOWS
	static const String ksUnnormalizedPath = "/app_home/Project/Seoul/Dev/SeoulEngine/Data/Content/Textures/../Meshes/Test/../Test2/test_image.PNG";
	static const String ksNormalizedPath = "\\app_home\\Project\\Seoul\\Dev\\SeoulEngine\\Data\\Content\\Meshes\\Test2\\test_image.PNG";
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	static const String ksUnnormalizedPath = "D:\\Project\\Seoul\\Dev\\SeoulEngine\\Data\\Content\\Textures\\..\\Meshes\\Test\\..\\Test2\\test_image.PNG";
	static const String ksNormalizedPath = "D:/Project/Seoul/Dev/SeoulEngine/Data/Content/Meshes/Test2/test_image.PNG";
#else
#error "Define for this platform."
#endif

	String sResult;
	SEOUL_UNITTESTING_ASSERT(Path::CombineAndSimplify(
		String(),
		ksUnnormalizedPath,
		sResult));
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		sResult,
		ksNormalizedPath);
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		String(".PNG"),
		Path::GetExtension(sResult));

	SEOUL_UNITTESTING_ASSERT(Path::HasExtension(sResult));
	SEOUL_UNITTESTING_ASSERT(Path::IsRooted(sResult));
}

void PathTest::TestBasicPath()
{
#if SEOUL_PLATFORM_WINDOWS
	static const String ksTestPath = "D:\\Project\\Seoul\\Dev\\SeoulEngine\\Data\\Content\\";
	static const String ksAltTestPath = "/app_home/Project/Seoul/Dev/SeoulEngine/Data/Content/";
	SEOUL_UNITTESTING_ASSERT("\\" == Path::DirectorySeparatorChar());
	SEOUL_UNITTESTING_ASSERT("/" == Path::AltDirectorySeparatorChar());
#elif SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	static const String ksTestPath = "/Project/Seoul/Dev/SeoulEngine/Data/Content/";
	static const String ksAltTestPath = "D:\\Project\\Seoul\\Dev\\SeoulEngine\\Data\\Content\\";
	SEOUL_UNITTESTING_ASSERT("/" == Path::DirectorySeparatorChar());
	SEOUL_UNITTESTING_ASSERT("\\" == Path::AltDirectorySeparatorChar());
#elif SEOUL_PLATFORM_IOS
	static const String ksTestPath = "/Project/Seoul/Dev/SeoulEngine/Data/Content/";
	static const String ksAltTestPath = "D:\\Project\\Seoul\\Dev\\SeoulEngine\\Data\\Content\\";
	SEOUL_UNITTESTING_ASSERT("/" == Path::DirectorySeparatorChar());
	SEOUL_UNITTESTING_ASSERT("\\" == Path::AltDirectorySeparatorChar());
#else
#error "Define for this platform."
#endif

	// Directory
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		ksTestPath.Substring(0u, ksTestPath.GetSize() - 1u),
		Path::GetDirectoryName(ksTestPath));

	SEOUL_UNITTESTING_ASSERT_EQUAL(
		ksAltTestPath.Substring(0u, ksAltTestPath.GetSize() - 1u),
		Path::GetDirectoryName(ksAltTestPath));

	// Extension
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		String(),
		Path::GetExtension(ksTestPath));

	SEOUL_UNITTESTING_ASSERT_EQUAL(
		String(),
		Path::GetExtension(ksAltTestPath));

	// FileName
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		String(),
		Path::GetFileName(ksTestPath));

	SEOUL_UNITTESTING_ASSERT_EQUAL(
		String(),
		Path::GetFileName(ksAltTestPath));

	// GetFileNameWithoutExtension
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		String(),
		Path::GetFileNameWithoutExtension(ksTestPath));

	SEOUL_UNITTESTING_ASSERT_EQUAL(
		String(),
		Path::GetFileNameWithoutExtension(ksAltTestPath));

	// GetPathWithoutExtension
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		ksTestPath,
		Path::GetPathWithoutExtension(ksTestPath));

	SEOUL_UNITTESTING_ASSERT_EQUAL(
		ksAltTestPath,
		Path::GetPathWithoutExtension(ksAltTestPath));

	// HasExtension
	SEOUL_UNITTESTING_ASSERT(!Path::HasExtension(ksTestPath));
	SEOUL_UNITTESTING_ASSERT(!Path::HasExtension(ksAltTestPath));

	// IsRooted
	SEOUL_UNITTESTING_ASSERT(Path::IsRooted(ksTestPath));
	SEOUL_UNITTESTING_ASSERT(Path::IsRooted(ksAltTestPath));
}

void PathTest::TestCombine()
{
	SEOUL_UNITTESTING_ASSERT_EQUAL("A" SEOUL_DIR_SEPARATOR "B", Path::Combine("A", "B"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("A" SEOUL_DIR_SEPARATOR "B" SEOUL_DIR_SEPARATOR "C", Path::Combine("A", "B", "C"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("A" SEOUL_DIR_SEPARATOR "B" SEOUL_DIR_SEPARATOR "C" SEOUL_DIR_SEPARATOR "D", Path::Combine("A", "B", "C", "D"));
	SEOUL_UNITTESTING_ASSERT_EQUAL("A" SEOUL_DIR_SEPARATOR "B" SEOUL_DIR_SEPARATOR "C" SEOUL_DIR_SEPARATOR "D" SEOUL_DIR_SEPARATOR "E", Path::Combine("A", "B", "C", "D", "E"));
}

void PathTest::TestGetExactPathName()
{
	UnitTestsFileManagerHelper helper;

	// File test.
	{
		String const sFileName(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", "HTTP", "file1.txt"));
		String const sLowerFileName(sFileName.ToLower("en"));
		String const sUpperFileName(sFileName.ToUpper("en"));
		{
			String const sExactFileName(Path::GetExactPathName(sLowerFileName));
			if (Path::PlatformFileNamesAreCaseSensitive())
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(sLowerFileName, sExactFileName);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(sFileName, sExactFileName);
			}
		}
		{
			String const sExactFileName(Path::GetExactPathName(sUpperFileName));
			if (Path::PlatformFileNamesAreCaseSensitive())
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(sUpperFileName, sExactFileName);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(sFileName, sExactFileName);
			}
		}
	}

	// Directory test.
	{
		String const sFileName(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", "HTTP"));
		String const sLowerFileName(sFileName.ToLower("en"));
		String const sUpperFileName(sFileName.ToUpper("en"));
		{
			String const sExactFileName(Path::GetExactPathName(sLowerFileName));
			if (Path::PlatformFileNamesAreCaseSensitive())
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(sLowerFileName, sExactFileName);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(sFileName, sExactFileName);
			}
		}
		{
			String const sExactFileName(Path::GetExactPathName(sUpperFileName));
			if (Path::PlatformFileNamesAreCaseSensitive())
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(sUpperFileName, sExactFileName);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(sFileName, sExactFileName);
			}
		}
	}

	// Directory test (trailing slash).
	{
		String const sFileName(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", "HTTP") + Path::DirectorySeparatorChar());
		String const sLowerFileName(sFileName.ToLower("en"));
		String const sUpperFileName(sFileName.ToUpper("en"));
		{
			String const sExactFileName(Path::GetExactPathName(sLowerFileName));
			if (Path::PlatformFileNamesAreCaseSensitive())
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(sLowerFileName, sExactFileName);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(sFileName, sExactFileName);
			}
		}
		{
			String const sExactFileName(Path::GetExactPathName(sUpperFileName));
			if (Path::PlatformFileNamesAreCaseSensitive())
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(sUpperFileName, sExactFileName);
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(sFileName, sExactFileName);
			}
		}
	}

	auto const caseTest = [](const String& s)
	{
		if (Path::PlatformSupportsDriveLetters())
		{
			String sCopy(s);
			sCopy[0] += ('A' - 'a'); // Drive letter should always be normalized as uppercase.
			return sCopy;
		}
		else
		{
			return s;
		}
	};

	// Case test.
	{
		String const sFileName(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", "ThisDoesNotExist"));
		String const sLowerFileName(caseTest(sFileName.ToLower("en")));
		String const sUpperFileName(caseTest(sFileName.ToUpper("en")));
		{
			String const sExactFileName(Path::GetExactPathName(sLowerFileName));
			SEOUL_UNITTESTING_ASSERT_EQUAL(sLowerFileName, sExactFileName);
		}
		{
			String const sExactFileName(Path::GetExactPathName(sUpperFileName));
			SEOUL_UNITTESTING_ASSERT_EQUAL(sUpperFileName, sExactFileName);
		}
	}

	// Case test (trailing slash).
	{
		String const sFileName(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", "ThisDoesNotExist") + Path::DirectorySeparatorChar());
		String const sLowerFileName(caseTest(sFileName.ToLower("en")));
		String const sUpperFileName(caseTest(sFileName.ToUpper("en")));
		{
			String const sExactFileName(Path::GetExactPathName(sLowerFileName));
			SEOUL_UNITTESTING_ASSERT_EQUAL(sLowerFileName, sExactFileName);
		}
		{
			String const sExactFileName(Path::GetExactPathName(sUpperFileName));
			SEOUL_UNITTESTING_ASSERT_EQUAL(sUpperFileName, sExactFileName);
		}
	}
}

void PathTest::TestGetDirectoryNameN()
{
#if SEOUL_PLATFORM_WINDOWS
	static const String ksTestPath = "D:\\Project\\Seoul\\Dev\\SeoulEngine\\Data\\Content\\";
#elif SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	static const String ksTestPath = "/Project/Seoul/Dev/SeoulEngine/Data/Content/";
#elif SEOUL_PLATFORM_IOS
	static const String ksTestPath = "/Project/Seoul/Dev/SeoulEngine/Data/Content/";
#else
#error "Define for this platform."
#endif

	SEOUL_UNITTESTING_ASSERT_EQUAL(
		ksTestPath,
		Path::GetDirectoryName(ksTestPath, 0));

	String s = ksTestPath;
	for (Int i = 1; i <= 7; ++i)
	{
		s = Path::GetDirectoryName(s);
		SEOUL_UNITTESTING_ASSERT_EQUAL(s, Path::GetDirectoryName(ksTestPath, i));
	}
}

void PathTest::TestGetProcessDirectory()
{
	auto const s(Path::GetProcessDirectory());
	SEOUL_UNITTESTING_ASSERT(!s.IsEmpty());
	SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(s));
}

void PathTest::TestGetTempDirectory()
{
	auto const s(Path::GetTempDirectory());
	SEOUL_UNITTESTING_ASSERT(!s.IsEmpty());
	SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(s));
}

void PathTest::TestGetTempFileAbsoluteFilename()
{
	static const String ksTest("THIS IS A TEST");

	auto const s(Path::GetTempFileAbsoluteFilename());
	SEOUL_UNITTESTING_ASSERT(!s.IsEmpty());
	SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(s));
	SEOUL_UNITTESTING_ASSERT(DiskSyncFile::WriteAll(s, (void*)ksTest.CStr(), ksTest.GetSize()));

	void* p = nullptr;
	UInt32 u = 0u;
	SEOUL_UNITTESTING_ASSERT(DiskSyncFile::ReadAll(s, p, u, 0u, MemoryBudgets::TBD));
	SEOUL_UNITTESTING_ASSERT_EQUAL(ksTest.GetSize(), u);
	SEOUL_UNITTESTING_ASSERT(0 == memcmp(ksTest.CStr(), p, u));
	MemoryManager::Deallocate(p);

	SEOUL_UNITTESTING_ASSERT(DiskSyncFile::DeleteFile(s));
}

// Test for a regression where .\ could be mishandled
// and result in the wrong resulting path.
void PathTest::TestSingleDotRegression()
{
	auto const sA = R"(E:\projects\seoul\App\Source\Authored\Animations\TwoTwoSocket_Twoih)";
	auto const sB = R"(.\..\TwoTwoSocket_TwoTwo\images\legRightPath.png)";

	String sOut;
	SEOUL_UNITTESTING_ASSERT(Path::CombineAndSimplify(sA, sB, sOut));

	String sExpected(R"(E:\projects\seoul\App\Source\Authored\Animations\TwoTwoSocket_TwoTwo\images\legRightPath.png)");
	sExpected = sExpected.ReplaceAll("\\", Path::DirectorySeparatorChar());
	SEOUL_UNITTESTING_ASSERT_EQUAL(sExpected, sOut);
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
