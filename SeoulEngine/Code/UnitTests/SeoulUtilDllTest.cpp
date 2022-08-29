/**
 * \file SeoulUtilDll.cpp
 * \brief Tests of the SeoulUtil.dll in the tools codebase.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DiskFileSystem.h"
#include "GamePaths.h"
#include "PackageFileSystem.h"
#include "Path.h"
#include "Platform.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "SeoulFile.h"
#include "SeoulUtilDllTest.h"
#include "UnitTesting.h"
#include "UnitTestsEngineHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(SeoulUtilDllTest, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAppendToJson)
	SEOUL_METHOD(TestCookJson)
	SEOUL_METHOD(TestMinifyJson)
	SEOUL_METHOD(TestGetModifiedTimeOfFileInSar)
SEOUL_END_TYPE()

#if SEOUL_PLATFORM_WINDOWS
namespace
{

typedef bool (SEOUL_STD_CALL *Seoul_AppendToJsonFunc)(const char* sInOutFilename, void const* pIn, unsigned int uIn);
typedef bool (SEOUL_STD_CALL *Seoul_CookJsonFunc)(void const* pIn, unsigned int uIn, int iPlatform, void** ppOut, unsigned int* rpOut);
typedef bool (SEOUL_STD_CALL *Seoul_MinifyJsonFunc)(void const* pIn, unsigned int uIn, void** ppOut, unsigned int* rpOut);
typedef bool (SEOUL_STD_CALL *Seoul_GetModifiedTimeOfFileInSarFunc)(const char* sSarPath, const char* sSerializedUrl, uint64_t* puModifiedTime);
typedef void (SEOUL_STD_CALL *Seoul_ReleaseJsonFunc)(void* p);

/**
 * @return The tools path for the current execution environment.
 */
static Byte const* GetToolsRelative()
{
#if SEOUL_PLATFORM_64
	return "..\\SeoulTools\\Binaries\\PC\\Developer\\x64\\";
#else
	return "..\\SeoulTools\\Binaries\\PC\\Developer\\x86\\";
#endif
}

// See also: https://docs.microsoft.com/en-us/cpp/build/reference/decorated-names?view=vs-2017
static String Decorate(Byte const* sBaseName, UInt32 uPointerArgs, UInt32 uAdditionalFixedSizeArgBytes = 0u)
{
#if SEOUL_PLATFORM_64
	// "Format of a C decorated name" ... "Note that in a 64-bit environment, functions are not decorated."
	return sBaseName;
#else
	return String::Printf("_%s@%u", sBaseName, (UInt32)(uPointerArgs * sizeof(void*) + uAdditionalFixedSizeArgBytes));
#endif
}

class SeoulUtilApi SEOUL_SEALED
{
public:
	SeoulUtilApi()
	{
		String sPath;
		SEOUL_VERIFY(Path::CombineAndSimplify(
			String(),
			Path::Combine(GamePaths::Get()->GetBaseDir(), GetToolsRelative(), "SeoulUtil.dll"),
			sPath));
		m_pDll = ::LoadLibraryW(sPath.WStr());
		SEOUL_UNITTESTING_ASSERT(m_pDll);

		Seoul_AppendToJson = (Seoul_AppendToJsonFunc)::GetProcAddress(m_pDll, Decorate("Seoul_AppendToJson", 2u, 4u).CStr());
		SEOUL_UNITTESTING_ASSERT(Seoul_AppendToJson);
		Seoul_CookJson = (Seoul_CookJsonFunc)::GetProcAddress(m_pDll, Decorate("Seoul_CookJson", 3u, 8u).CStr());
		SEOUL_UNITTESTING_ASSERT(Seoul_CookJson);
		Seoul_MinifyJson = (Seoul_MinifyJsonFunc)::GetProcAddress(m_pDll, Decorate("Seoul_MinifyJson", 3u, 4u).CStr());
		SEOUL_UNITTESTING_ASSERT(Seoul_MinifyJson);
		Seoul_GetModifiedTimeOfFileInSar = (Seoul_GetModifiedTimeOfFileInSarFunc)::GetProcAddress(m_pDll, Decorate("Seoul_GetModifiedTimeOfFileInSar", 3u).CStr());
		SEOUL_UNITTESTING_ASSERT(Seoul_GetModifiedTimeOfFileInSar);
		Seoul_ReleaseJson = (Seoul_ReleaseJsonFunc)::GetProcAddress(m_pDll, Decorate("Seoul_ReleaseJson", 1u).CStr());
		SEOUL_UNITTESTING_ASSERT(Seoul_ReleaseJson);
	}

	~SeoulUtilApi()
	{
		Seoul_ReleaseJson = nullptr;
		Seoul_GetModifiedTimeOfFileInSar = nullptr;
		Seoul_MinifyJson = nullptr;
		Seoul_CookJson = nullptr;
		Seoul_AppendToJson = nullptr;

		auto p = m_pDll;
		m_pDll = nullptr;
		SEOUL_VERIFY(FALSE != ::FreeLibrary(p));
	}

	Bool AppendToJson(const String& sInOutFilename, Byte const* sChunk)
	{
		return Seoul_AppendToJson(sInOutFilename.CStr(), sChunk, StrLen(sChunk));
	}

	Bool CookJson(Byte const* sJson, Platform ePlatform, DataStore& rDataStore)
	{
		void* p = nullptr;
		UInt32 u = 0u;
		if (!Seoul_CookJson(sJson, StrLen(sJson), (Int)ePlatform, &p, &u)) { return false; }

		FullyBufferedSyncFile file(p, u, false);
		auto const b = rDataStore.Load(file);
		Seoul_ReleaseJson(p);
		p = nullptr;
		u = 0u;

		return b;
	}

	Bool MinifyJson(Byte const* sJson, String& rsOut)
	{
		void* p = nullptr;
		UInt32 u = 0u;
		if (!Seoul_MinifyJson(sJson, StrLen(sJson), &p, &u)) { return false; }

		rsOut.Assign((Byte const*)p, u);

		Seoul_ReleaseJson(p);
		p = nullptr;
		u = 0u;

		return true;
	}

	Bool GetModifiedTimeOfFileInSar(const String& sSar, FilePath filePath, UInt64& ruOut)
	{
		return Seoul_GetModifiedTimeOfFileInSar(
			sSar.CStr(),
			filePath.ToSerializedUrl().CStr(),
			&ruOut);
	}

private:
	SEOUL_DISABLE_COPY(SeoulUtilApi);

	HMODULE m_pDll = nullptr;
	volatile Seoul_AppendToJsonFunc Seoul_AppendToJson = nullptr;
	volatile Seoul_CookJsonFunc Seoul_CookJson = nullptr;
	volatile Seoul_MinifyJsonFunc Seoul_MinifyJson = nullptr;
	volatile Seoul_GetModifiedTimeOfFileInSarFunc Seoul_GetModifiedTimeOfFileInSar = nullptr;
	volatile Seoul_ReleaseJsonFunc Seoul_ReleaseJson = nullptr;
};

static Bool ReadDataStore(const String& sFileName, DataStore& ds)
{
	{
		void* pFile = nullptr;
		UInt32 uFile = 0u;
		if (!DiskSyncFile::ReadAll(sFileName, pFile, uFile, 0u, MemoryBudgets::TBD))
		{
			return false;
		}

		// Parse the input file.
		auto const bOk = DataStoreParser::FromString((Byte const*)pFile, uFile, ds);
		MemoryManager::Deallocate(pFile);
		if (!bOk)
		{
			return false;
		}
	}

	return true;
}

SharedPtr<DataStore> IncludeResolver(const String& sFileName, Bool bResolveCommands)
{
	// Read the data.
	DataStore ds;
	if (!ReadDataStore(sFileName, ds))
	{
		return SharedPtr<DataStore>();
	}

	// If requested, resolve the commands.
	if (bResolveCommands && DataStoreParser::IsJsonCommandFile(ds))
	{
		if (!DataStoreParser::ResolveCommandFile(
			SEOUL_BIND_DELEGATE(&IncludeResolver),
			sFileName,
			ds,
			ds))
		{
			return SharedPtr<DataStore>();
		}
	}

	SharedPtr<DataStore> p(SEOUL_NEW(MemoryBudgets::Io) DataStore);
	p->Swap(ds);
	return p;
}

void TestAppend(
	SeoulUtilApi& api,
	Byte const* sAppendTo,
	Byte const* sToAppend,
	Byte const* sExpected)
{
	Bool b = true;
	{
		DataStore expected;
		b = b && DataStoreParser::FromString(sExpected, expected);

		DataStore actual;
		{
			String sTemp;
			auto const scoped(MakeScopedAction(
				[&]()
			{
				sTemp = Path::GetTempFileAbsoluteFilename();
			},
				[&]()
			{
				DiskSyncFile::DeleteFile(sTemp);
			}));

			// Commit to a temp file.
			{
				auto const uSize = StrLen(sAppendTo);
				DiskSyncFile file(sTemp, File::kWriteTruncate);
				b = b && (uSize == file.WriteRawData(sAppendTo, uSize));
			}

			// Process.
			b = b && api.AppendToJson(sTemp, sToAppend);

			// Read back in.
			if (b)
			{
				auto p(IncludeResolver(sTemp, true));
				if (p.IsValid())
				{
					actual.Swap(*p);
				}
			}
		}

		b = b && DataStore::Equals(expected, expected.GetRootNode(), actual, actual.GetRootNode());
	}

	SEOUL_UNITTESTING_ASSERT(b);
}

void TestCook(
	SeoulUtilApi& api,
	FilePath filePath)
{
	Bool b = true;
	{
		void* p = nullptr;
		UInt32 u = 0u;
		b = b && DiskSyncFile::ReadAll(filePath, p, u, 0u, MemoryBudgets::Io);

		String sJson;
		if (b)
		{
			sJson.Assign((Byte const*)p, u);
		}

		MemoryManager::Deallocate(p);
		p = nullptr;
		u = 0u;

		DataStore actual;
		b = b && api.CookJson(sJson.CStr(), keCurrentPlatform, actual);

		DataStore expected;
		b = b && DataStoreParser::FromString(sJson, expected);

		b = b && DataStore::Equals(expected, expected.GetRootNode(), actual, actual.GetRootNode(), true);
	}

	SEOUL_UNITTESTING_ASSERT(b);
}

void TestMinify(
	SeoulUtilApi& api,
	Byte const* sActual,
	Byte const* sExpected)
{
	Bool b = true;
	{
		String sMinified;
		b = b && api.MinifyJson(sActual, sMinified);

		b = b && (sMinified == sExpected);
	}

	SEOUL_UNITTESTING_ASSERT(b);
}

} // namespace
#endif // /#if SEOUL_PLATFORM_WINDOWS

SeoulUtilDllTest::SeoulUtilDllTest()
{
}

SeoulUtilDllTest::~SeoulUtilDllTest()
{
}

void SeoulUtilDllTest::TestAppendToJson()
{
#if SEOUL_PLATFORM_WINDOWS
	UnitTestsEngineHelper helper;
	SeoulUtilApi api;
	TestAppend(api,
		R"(
			[
				["$object", "DisplayConfig"],
				["$set", "BackgroundImages", [
					{
						"ImagePath": "content://Authored/Fx/Effects/Abilities/Other/ABIL_Other2.png",
						"Width": 250,
						"Height": 250,
						"XOffset": 0,
						"YOffset": 0
					}
				]]
			]
		)",
		R"(
			[
				["$object", "DisplayConfig"],
				["$set", "BackgroundImages", [
					{
						"ImagePath": "content://Authored/Fx/Effects/Abilities/Someone/Someone3.png",
						"Width": 330,
						"Height": 330,
						"XOffset": 0,
						"YOffset": 10
					}
				]]
			]
		)",
		R"(
			{
				"DisplayConfig": {
					"BackgroundImages": [
						{
							"ImagePath": "content://Authored/Fx/Effects/Abilities/Someone/Someone3.png",
							"Width": 330,
							"Height": 330,
							"XOffset": 0,
							"YOffset": 10
						}
					]
				}
			}
		)");

	TestAppend(api,
		R"(
			{
				"DisplayConfig": {
					"BackgroundImages": [
						{
							"ImagePath": "content://Authored/Fx/Effects/Abilities/Other/ABIL_Other2.png",
							"Width": 250,
							"Height": 250,
							"XOffset": 0,
							"YOffset": 0
						}
					]
				}
			}
		)",
		R"(
			[
				["$object", "DisplayConfig"],
				["$set", "BackgroundImages", [
					{
						"ImagePath": "content://Authored/Fx/Effects/Abilities/Someone/Someone3.png",
						"Width": 330,
						"Height": 330,
						"XOffset": 0,
						"YOffset": 10
					}
				]]
			]
		)",
		R"(
			{
				"DisplayConfig": {
					"BackgroundImages": [
						{
							"ImagePath": "content://Authored/Fx/Effects/Abilities/Someone/Someone3.png",
							"Width": 330,
							"Height": 330,
							"XOffset": 0,
							"YOffset": 10
						}
					]
				}
			}
		)");

	TestAppend(api,
		R"(
			{
				"DisplayConfig": {
					"BackgroundImages": [
						{
							"ImagePath": "content://Authored/Fx/Effects/Abilities/Other/ABIL_Other2.png",
							"Width": 250,
							"Height": 250,
							"XOffset": 0,
							"YOffset": 0
						},
						{
							"ImagePath": "content://Authored/Fx/Effects/Abilities/Someone/Someone3.png",
							"Width": 330,
							"Height": 330,
							"XOffset": 0,
							"YOffset": 10
						}
					]
				}
			}
		)",
		R"(
			[
				["$object", "DisplayConfig"],
				["$set", "BackgroundImages", 0,
					{
						"ImagePath": "content://Authored/Fx/Effects/Abilities/Someone/Someone3.png",
						"Width": 330,
						"Height": 330,
						"XOffset": 0,
						"YOffset": 10
					}
				]
			]
		)",
		R"(
			{
				"DisplayConfig": {
					"BackgroundImages": [
						{
							"ImagePath": "content://Authored/Fx/Effects/Abilities/Someone/Someone3.png",
							"Width": 330,
							"Height": 330,
							"XOffset": 0,
							"YOffset": 10
						},
						{
							"ImagePath": "content://Authored/Fx/Effects/Abilities/Someone/Someone3.png",
							"Width": 330,
							"Height": 330,
							"XOffset": 0,
							"YOffset": 10
						}
					]
				}
			}
		)");

	TestAppend(api,
		R"(
			[
				["$object", "DisplayConfig"],
				["$set", "BackgroundImages", [
					{
						"ImagePath": "content://Authored/Fx/Effects/Abilities/Someone/Someone3.png",
						"Width": 330,
						"Height": 330,
						"XOffset": 0,
						"YOffset": 10
					}
				]]
			]
		)",
		R"(
			[
				["$object", "DisplayConfig"],
				["$set", "BackgroundImages", 1,
					{
						"ImagePath": "content://Authored/Fx/Effects/Abilities/Someone/Someone3.png",
						"Width": 330,
						"Height": 330,
						"XOffset": 0,
						"YOffset": 10
					}
				]
			]
		)",
		R"(
			{
				"DisplayConfig": {
					"BackgroundImages": [
						{
							"ImagePath": "content://Authored/Fx/Effects/Abilities/Someone/Someone3.png",
							"Width": 330,
							"Height": 330,
							"XOffset": 0,
							"YOffset": 10
						},
						{
							"ImagePath": "content://Authored/Fx/Effects/Abilities/Someone/Someone3.png",
							"Width": 330,
							"Height": 330,
							"XOffset": 0,
							"YOffset": 10
						}
					]
				}
			}
		)");

#endif // /#if SEOUL_PLATFORM_WINDOWS
}

void SeoulUtilDllTest::TestCookJson()
{
#if SEOUL_PLATFORM_WINDOWS
	UnitTestsEngineHelper helper;
	SeoulUtilApi api;
	TestCook(api, FilePath::CreateConfigFilePath("UnitTests/DataStoreParser/BasicTest.json"));
	TestCook(api, FilePath::CreateConfigFilePath("UnitTests/DataStoreParser/UnicodeTest.json"));
#endif // /#if SEOUL_PLATFORM_WINDOWS
}

void SeoulUtilDllTest::TestMinifyJson()
{
#if SEOUL_PLATFORM_WINDOWS
	UnitTestsEngineHelper helper;
	SeoulUtilApi api;
	TestMinify(
		api,
		R"(
			{
				// Configuration for the 2D and 3D drawers, used for rendering simple
				// 3D shapes, screen space 2D shapes, and text.
				"Drawer": {
					// Maximum number of font characters/quads the 2D drawer can
					// render in a single frame.
					"Drawer2DMaxQuads": 16384,

					// Effect file that is used for rendering 2D primitives.
					"Drawer2DEffect": "content://Authored/Effects/Batch/Text.fx",

					// Texture that contains the symbols used for rendering
					// by the 2D drawer.
					"Drawer2DGlyphTexture": "content://Authored/Textures/Resources/monkey_font.png",

					// Base scale of font glyphs when rendering them to
					// the screen - this can be used to allow the texture
					// to have "extra resolution" for sizing the font up from
					// its base scale.
					"Drawer2DFontGlyphDrawScale": [0.38, 0.38],

					// ASCII code of the first character in the font texture -
					// characters are expected to be layed out, left to right, in a single
					// row, starting with this character.
					"Drawer2DGlyphTextureFirstCharacterASCIICode": 32,

					// Total number of glyphs in the font texture.
					"Drawer2DGlyphTextureFontCount": 95,

					// Height of a single glyph in the font texture - drawer 2D currently
					// only supports fixed height fonts.
					"Drawer2DGlyphTextureGlyphHeight": 32,

					// Width of a single glyph in the font texture - drawer 2D currently
					// only supports fixed width fonts.
					"Drawer2DGlyphTextureGlyphWidth": 16,

					// Width (in pixels) of the 2D font texture.
					"Drawer2DGlyphTextureWidth": 256,

					// Height (in pixels) of the 2D font texture.
					"Drawer2DGlyphTextureHeight": 256
				}
			}
		)",
		R"({"Drawer":{"Drawer2DEffect":"content://Authored/Effects/Batch/Text.fx","Drawer2DFontGlyphDrawScale":[0.38,0.38],"Drawer2DGlyphTexture":"content://Authored/Textures/Resources/monkey_font.png","Drawer2DGlyphTextureFirstCharacterASCIICode":32,"Drawer2DGlyphTextureFontCount":95,"Drawer2DGlyphTextureGlyphHeight":32,"Drawer2DGlyphTextureGlyphWidth":16,"Drawer2DGlyphTextureHeight":256,"Drawer2DGlyphTextureWidth":256,"Drawer2DMaxQuads":16384}})");

#endif // /#if SEOUL_PLATFORM_WINDOWS
}

void SeoulUtilDllTest::TestGetModifiedTimeOfFileInSar()
{
#if SEOUL_PLATFORM_WINDOWS
	UInt32 uErrors = 0u;
	{
		// Reasonable run time.
		static const UInt32 kuMaxIterations = 20u;

		UnitTestsEngineHelper helper;
		SeoulUtilApi api;
		auto const sPath(Path::Combine(GamePaths::Get()->GetBaseDir(), String::Printf("Data/%s_Config.sar", GetCurrentPlatformName())));
		PackageFileSystem pkg(sPath);
		UInt32 i = 0u;
		auto const uInterval = (pkg.GetFileTable().GetSize() / kuMaxIterations);
		for (auto const& e : pkg.GetFileTable())
		{
			if (i++ % uInterval != 0)
			{
				continue;
			}

			auto const uExpected = e.Second.m_Entry.m_uModifiedTime;
			UInt64 uActual = 0u;
			if (!api.GetModifiedTimeOfFileInSar(sPath, e.First, uActual))
			{
				SEOUL_LOG_UNIT_TEST("Failed reading modified time of %s", e.First.CStr());
				++uErrors;
				continue;
			}

			if (uExpected != uActual)
			{
				SEOUL_LOG_UNIT_TEST("Modified time not equal: '%s'", e.First.CStr());
				++uErrors;
			}
		}
	}

	if (0 != uErrors)
	{
		SEOUL_UNITTESTING_FAIL("%u errors", uErrors);
	}
#endif // /#if SEOUL_PLATFORM_WINDOWS
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
