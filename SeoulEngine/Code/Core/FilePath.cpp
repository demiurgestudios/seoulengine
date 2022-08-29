/**
 * \file FilePath.cpp
 * \brief FilePath provides a consistent key for files. Two FilePaths that
 * identify the same asset will be exactly equal. FilePath is also small
 * (4 bytes) and most operations are computationally cheap, except for
 * the Create*FilePath(const String&) creation functions, which are
 * expensive and should only be used to create file paths during level
 * load or object initialization.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HashTable.h"
#include "FileManager.h"
#include "FilePath.h"
#include "GamePaths.h"
#include "Logger.h"
#include "Path.h"

namespace Seoul
{

/**
 * Convert the FileType eFileType to a filename extension representing
 * that FileType in a cooked content folder. If the file extension does not
 * change between source and content, the extension will be the same
 * as the one returned by FileTypeToSourceExtension().
 *
 * The returned string is an all lowercase file extension, including
 * the leading '.'.
 */
const String& FileTypeToCookedExtension(FileType eFileType)
{
	static const String kAvi(".avi");
	static const String kBank(".bank");
	static const String kCs(".cs");
	static const String kCsp(".csp");
	static const String kCsv(".csv");
	static const String kDat(".dat");
	static const String kExe(".exe");
	static const String kFcn(".fcn");
	static const String kFev(".fev");
	static const String kFsb(".fsb");
	static const String kFxb(".fxb");
	static const String kFxc(".fxc");
	static const String kFxhMarker(".fxh_marker");
	static const String kHtml(".html");
	static const String kJson(".json");
	static const String kLbc(".lbc");
	static const String kPb(".pb");
	static const String kPem(".pem");
	static const String kSaf(".saf");
	static const String kSif0(".sif0");
	static const String kSif1(".sif1");
	static const String kSif2(".sif2");
	static const String kSif3(".sif3");
	static const String kSif4(".sif4");
	static const String kSff(".sff");
	static const String kSpf(".spf");
	static const String kSsa(".ssa");
	static const String kTxt(".txt");
	static const String kWav(".wav");
	static const String kXml(".xml");
	static const String kUnknown;

	switch (eFileType)
	{
	case FileType::kUnknown: return kUnknown;
	case FileType::kAnimation2D: return kSaf;
	case FileType::kCs: return kCs;
	case FileType::kCsv: return kCsv;
	case FileType::kEffect: return kFxc;
	case FileType::kEffectHeader: return kFxhMarker;
	case FileType::kExe: return kExe;
	case FileType::kFont: return kSff;
	case FileType::kFxBank: return kFxb;
	case FileType::kHtml: return kHtml;
	case FileType::kJson: return kJson;
	case FileType::kPEMCertificate: return kPem;
	case FileType::kProtobuf: return kPb;
	case FileType::kSaveGame: return kDat;
	case FileType::kSceneAsset: return kSsa;
	case FileType::kScenePrefab: return kSpf;
	case FileType::kScript: return kLbc;
	case FileType::kScriptProject: return kCsp;
	case FileType::kSoundBank: return kBank;
	case FileType::kSoundProject: return kFev;
	case FileType::kTexture0: return kSif0;
	case FileType::kTexture1: return kSif1;
	case FileType::kTexture2: return kSif2;
	case FileType::kTexture3: return kSif3;
	case FileType::kTexture4: return kSif4;
	case FileType::kText: return kTxt;
	case FileType::kUIMovie: return kFcn;
	case FileType::kVideo: return kAvi;
	case FileType::kWav: return kWav;
	case FileType::kXml: return kXml;
	default:
		return kUnknown;
	}
}

/**
 * Convert the FileType eFileType to a filename extension representing
 * that FileType in a cooked content folder. If the file extension does not
 * change between source and content, the extension will be the same
 * as the one returned by FileTypeToSourceExtension().
 *
 * The returned string is an all lowercase file extension, including
 * the leading '.'.
 */
const String& FileTypeToSourceExtension(FileType eFileType)
{
	static const String kAvi(".avi");
	static const String kBank(".bank");
	static const String kCs(".cs");
	static const String kCsproj(".csproj");
	static const String kCsv(".csv");
	static const String kDat(".dat");
	static const String kExe(".exe");
	static const String kFbx(".fbx");
	static const String kFdp(".fdp");
	static const String kFspro(".fspro");
	static const String kFx(".fx");
	static const String kFxh(".fxh");
	static const String kHtml(".html");
	static const String kJson(".json");
	static const String kLua(".lua");
	static const String kPem(".pem");
	static const String kPng(".png");
	static const String kPrefab(".prefab");
	static const String kProtobuf(".proto");
	static const String kSif0(".sif0");
	static const String kSif1(".sif1");
	static const String kSif2(".sif2");
	static const String kSif3(".sif3");
	static const String kSif4(".sif4");
	static const String kSon(".son");
	static const String kSwf(".swf");
	static const String kTtf(".ttf");
	static const String kTxt(".txt");
	static const String kUnknown;
	static const String kWav(".wav");
	static const String kXfx(".xfx");
	static const String kXmd(".xmd");
	static const String kXml(".xml");

	switch (eFileType)
	{
	case FileType::kUnknown: return kUnknown;
	case FileType::kAnimation2D: return kSon;
	case FileType::kCs: return kCs;
	case FileType::kCsv: return kCsv;
	case FileType::kEffect: return kFx;
	case FileType::kEffectHeader: return kFxh;
	case FileType::kExe: return kExe;
	case FileType::kFont: return kTtf;
	case FileType::kFxBank: return kXfx;
	case FileType::kHtml: return kHtml;
	case FileType::kJson: return kJson;
	case FileType::kPEMCertificate: return kPem;
	case FileType::kProtobuf: return kProtobuf;
	case FileType::kSaveGame: return kDat;
	case FileType::kSceneAsset: return kFbx;
	case FileType::kScenePrefab: return kPrefab;
	case FileType::kScript: return kLua;
	case FileType::kScriptProject: return kCsproj;
	case FileType::kSoundBank: return kBank;
	case FileType::kSoundProject: return kFspro;
	case FileType::kText: return kTxt;
	case FileType::kTexture0: return kPng;
	case FileType::kTexture1: return kPng;
	case FileType::kTexture2: return kPng;
	case FileType::kTexture3: return kPng;
	case FileType::kTexture4: return kPng;
	case FileType::kUIMovie: return kSwf;
	case FileType::kVideo: return kAvi;
	case FileType::kWav: return kWav;
	case FileType::kXml: return kXml;
	default:
		return kUnknown;
	}
}

static HashTable<String, FileType, MemoryBudgets::TBD> GetExtensionToFileTypeTable()
{
	struct Entry
	{
		String m_sExtension;
		FileType m_eFileType;
	};

	static const Entry s_kaEntries[] =
	{
		{ ".avi", FileType::kVideo },
		{ ".bank", FileType::kSoundBank },
		{ ".cs", FileType::kCs },
		{ ".csp", FileType::kScriptProject },
		{ ".csproj", FileType::kScriptProject },
		{ ".csv", FileType::kCsv },
		{ ".dat", FileType::kSaveGame },
		{ ".exe", FileType::kExe },
		{ ".fbx", FileType::kSceneAsset },
		{ ".fcn", FileType::kUIMovie },
		{ ".fdp", FileType::kSoundProject }, // Old source sound project extension.
		{ ".fev", FileType::kSoundProject },
		{ ".fsb", FileType::kSoundBank }, // Old cooked sound bank extension.
		{ ".fspro", FileType::kSoundProject },
		{ ".fx", FileType::kEffect },
		{ ".fxb", FileType::kFxBank },
		{ ".fxc", FileType::kEffect },
		{ ".fxh", FileType::kEffectHeader },
		{ ".fxh_marker", FileType::kEffectHeader },
		{ ".html", FileType::kHtml },
		{ ".json", FileType::kJson },
		{ ".lua", FileType::kScript },
		{ ".lbc", FileType::kScript },
		{ ".prefab", FileType::kScenePrefab },
		{ ".pb", FileType::kProtobuf },
		{ ".pem", FileType::kPEMCertificate },
		{ ".png", FileType::kTexture0 },
		{ ".proto", FileType::kProtobuf },
		{ ".saf", FileType::kAnimation2D },
		{ ".sff", FileType::kFont },
		{ ".spf", FileType::kScenePrefab },
		{ ".sif0", FileType::kTexture0 },
		{ ".sif1", FileType::kTexture1 },
		{ ".sif2", FileType::kTexture2 },
		{ ".sif3", FileType::kTexture3 },
		{ ".sif4", FileType::kTexture4 },
		{ ".son", FileType::kAnimation2D },
		{ ".ssa", FileType::kSceneAsset },
		{ ".swf", FileType::kUIMovie },
		{ ".ttf", FileType::kFont },
		{ ".txt", FileType::kText },
		{ ".wav", FileType::kWav },
		{ ".xfx", FileType::kFxBank },
		{ ".xml", FileType::kXml },
	};

	HashTable<String, FileType, MemoryBudgets::TBD> tReturn;
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaEntries); ++i)
	{
		SEOUL_VERIFY(tReturn.Insert(s_kaEntries[i].m_sExtension, s_kaEntries[i].m_eFileType).Second);
	}

	return tReturn;
}

/**
 * Convert the String sExtension to a FileType enum that matches
 * the extension specified by sExtension.
 *
 * sExtension must be a recognized, all lowercase file extension
 * including the leading '.', otherwise this function will return
 * FileType::kUnknown.
 */
FileType ExtensionToFileType(const String& sExtension)
{
	static const HashTable<String, FileType, MemoryBudgets::TBD> s_tLookup(GetExtensionToFileTypeTable());

	FileType eReturn = FileType::kUnknown;
	(void)s_tLookup.GetValue(sExtension.ToLowerASCII(), eReturn);
	return eReturn;
}

/**
 * Convert the GameDirectory eGameDirectory to a String
 * that represents the absolute path of the game directory eGameDirectory.
 */
const String& GameDirectoryToString(GameDirectory eGameDirectory)
{
	static const String kUnknown;

	SEOUL_ASSERT(GamePaths::Get() && GamePaths::Get()->IsInitialized());

	switch (eGameDirectory)
	{
	case GameDirectory::kConfig:
		return GamePaths::Get()->GetConfigDir();
	case GameDirectory::kContent:
		return GamePaths::Get()->GetContentDir();
	case GameDirectory::kLog:
		return GamePaths::Get()->GetLogDir();
	case GameDirectory::kSave:
		return GamePaths::Get()->GetSaveDir();
	case GameDirectory::kToolsBin:
		return GamePaths::Get()->GetToolsBinDir();
	case GameDirectory::kVideos:
		return GamePaths::Get()->GetVideosDir();
	default:
		return kUnknown;
	};
}

/**
 * Convert the GameDirectory eGameDirectory to a String
 * that represents the absolute path of the game directory eGameDirectory.
 *
 * For content, returns the explicit platform path given.
 */
const String& GameDirectoryToStringForPlatform(GameDirectory eGameDirectory, Platform ePlatform)
{
	if (GameDirectory::kContent == eGameDirectory)
	{
		return GamePaths::Get()->GetContentDirForPlatform(ePlatform);
	}
	else
	{
		return GameDirectoryToString(eGameDirectory);
	}
}

/**
 * Convert the GameDirectory eGameDirectory to a String
 * that represents the absolute path of the game directory eGameDirectory
 * in the game's source directory.
 */
const String& GameDirectoryToStringInSource(GameDirectory eGameDirectory)
{
	static const String kUnknown;

	SEOUL_ASSERT(GamePaths::Get() && GamePaths::Get()->IsInitialized());

	if (GameDirectory::kContent == eGameDirectory)
	{
		return GamePaths::Get()->GetSourceDir();
	}
	else
	{
		return GameDirectoryToString(eGameDirectory);
	}
}

/**
 * Given an absolute, rooted filename, returns the GameDirectory that
 * contains the file, or GameDirectory::kUnknown if the file is outside
 * any GameDirectory or could not be resolved.
 *
 * sFilename must be a rooted absolute filename or this method
 * will return GameDirectory::kUnknown.
 */
GameDirectory GetGameDirectoryFromAbsoluteFilename(const String& sFilename)
{
	// Sanity check - make sure the GameDirectory enum has the expected
	// layout with kUnknown at 0.
	SEOUL_STATIC_ASSERT(0 == (Int)GameDirectory::kUnknown);

	// Special case handling for Source/
	if (sFilename.StartsWithASCIICaseInsensitive(
		GamePaths::Get()->GetSourceDir()))
	{
		return (GameDirectory::kContent);
	}

	// Compare the filename string to
	// each defined game directory.
	for (Int i = (Int)GameDirectory::kUnknown + 1; i < (Int)GameDirectory::GAME_DIRECTORY_COUNT; ++i)
	{
		// If the game directory string is found at the beginning of
		// the filename, we have a match.
		if (sFilename.StartsWithASCIICaseInsensitive(
			GameDirectoryToString((GameDirectory)i)))
		{
			return (GameDirectory)i;
		}
	}

	return GameDirectory::kUnknown;
}

/**
 * Create a file path in GameDirectory::kConfig.
 */
FilePath FilePath::CreateConfigFilePath(const String& sFilename)
{
	return FilePath::CreateFilePath(
		GameDirectory::kConfig,
		sFilename);
}

/**
 * Create a file path in GameDirectory::kToolsBin.
 */
FilePath FilePath::CreateToolsBinFilePath(const String& sFilename)
{
	return FilePath::CreateFilePath(
		GameDirectory::kToolsBin,
		sFilename);
}

/**
 * Create a file path in GameDirectory::kVideos.
 */
FilePath FilePath::CreateVideosFilePath(const String& sFilename)
{
	return FilePath::CreateFilePath(
		GameDirectory::kVideos,
		sFilename);
}

/**
 * Create a file path in GameDirectory::kSave.
 */
FilePath FilePath::CreateSaveFilePath(const String& sFilename)
{
	return FilePath::CreateFilePath(
		GameDirectory::kSave,
		sFilename);
}

/**
 * Create a file path in GameDirectory::kContent.
 */
FilePath FilePath::CreateContentFilePath(const String& sFilename)
{
	return FilePath::CreateFilePath(
		GameDirectory::kContent,
		sFilename);
}

/**
 * Create a file path in GameDirectory::kLog.
 */
FilePath FilePath::CreateLogFilePath(const String& sFilename)
{
	return FilePath::CreateFilePath(
		GameDirectory::kLog,
		sFilename);
}

/**
 * Create a FilePath within game directory eDirectory, based on
 * filename sFilename.
 *
 * The FileType of the returned FilePath will be determined based on
 * the extension in sFilename. The returned FileType will be kUnknown
 * if the extension could not be extracted, is not present, or is not
 * recognized.
 */
FilePath FilePath::CreateFilePath(
	GameDirectory eDirectory,
	const String& sFilename)
{
	return InternalCreateFilePath(eDirectory, sFilename, true);
}

/**
 * Creates a FilePath of type kUnknown without removing the extension from
 * the relative path.  This should only be used in very rare circumstances,
 * such as when you need to load a file type not supported by
 * FileType, or if you need to load a raw, uncooked texture.
 */
FilePath FilePath::CreateRawFilePath(
	GameDirectory eDirectory,
	const String& sFilename)
{
	return InternalCreateFilePath(eDirectory, sFilename, false);
}

/**
 * Helper function to create a FilePath
 */
FilePath FilePath::InternalCreateFilePath(
	GameDirectory eDirectory,
	const String& sFilename,
	Bool bDetermineFileTypeFromExtension)
{
	// Handle empty filenames.
	if (sFilename.IsEmpty())
	{
		FilePath ret;
		ret.m_uDirectory = (UInt32)eDirectory;
		return ret;
	}

	// Normalize the path.
	String sRelativePath = Path::Normalize(sFilename);

	// Strip the trailing slash, if there is one.
	if (!sRelativePath.IsEmpty() &&
		sRelativePath[sRelativePath.GetSize() - 1] == Path::kDirectorySeparatorChar)
	{
		sRelativePath.PopBack();
	}

	FileType eFileType;

	if (bDetermineFileTypeFromExtension)
	{
		// Extract the extension.
		auto const sExtension(Path::GetExtension(sRelativePath));

		// Try to determine the FileType. It's ok if this is kUnknown.
		eFileType = ExtensionToFileType(sExtension);

		// If eFileType is unknown but the extension is not empty,
		// return an invalid file path.
		if (FileType::kUnknown == eFileType && !sExtension.IsEmpty())
		{
			return FilePath();
		}

		// The relative path is the path specified in sFilename without the
		// extension.
		sRelativePath = Path::GetPathWithoutExtension(sRelativePath);
	}
	else
	{
		eFileType = FileType::kUnknown;
	}

	// Further processing if the path is absolute.
	if (GameDirectory::kUnknown != eDirectory && Path::IsRooted(sRelativePath))
	{
		// Get the string representing the game directory of the filename.
		String const* psFileBase = &GameDirectoryToString(eDirectory);

		// Find the index of the base directory in the normalized path.
		Bool bStartsWith = sRelativePath.StartsWithASCIICaseInsensitive(*psFileBase);

		// Next try, directory case - this occurs if the argument is the root
		// game directory with no trailing slash.
		if (!bStartsWith &&
			FileType::kUnknown == eFileType &&
			sRelativePath.GetSize() + 1 == psFileBase->GetSize() &&
			0 == strncmp(sRelativePath.CStr(), psFileBase->CStr(), sRelativePath.GetSize()))
		{
			FilePath ret;
			ret.SetDirectory(eDirectory);
			return ret;
		}

		// Special consideration for Source/ paths
		if (!bStartsWith && GameDirectory::kContent == eDirectory)
		{
			psFileBase = &GamePaths::Get()->GetSourceDir();
			bStartsWith = sRelativePath.StartsWithASCIICaseInsensitive(*psFileBase);

			if (!bStartsWith)
			{
				// Check other platforms as a final fallback.
				for (auto i = (Int)Platform::SEOUL_PLATFORM_FIRST; i <= (Int)Platform::SEOUL_PLATFORM_LAST; ++i)
				{
					if (i == (Int)keCurrentPlatform)
					{
						continue;
					}

					psFileBase = &GameDirectoryToStringForPlatform(eDirectory, (Platform)i);
					bStartsWith = sRelativePath.StartsWithASCIICaseInsensitive(*psFileBase);

					if (bStartsWith)
					{
						break;
					}
				}
			}
		}

		// Sanity check, make sure we're not getting weird eDirectory values.
		SEOUL_ASSERT(!psFileBase->IsEmpty());

		// If the base path exists and its total size is less than that of the absolute path,
		// relativize the absolute path.
		if (bStartsWith)
		{
			// Update the relative path to exclude the absolute directory portion.
			sRelativePath = sRelativePath.Substring(psFileBase->GetSize());
		}
		else
		{
			// Otherwise there is hinky going on and we return an invalid path.
			return FilePath();
		}
	}

	// If we get here, either the path was already relative, or it was
	// successfully made relative.

	// Combine the path with an empty root in order to simplify
	// away patterns such as "..\" and ".\"
	if (!Path::CombineAndSimplify(
		String(),
		sRelativePath,
		sRelativePath))
	{
		return FilePath();
	}

	// Initialize the file path and return it. Use the case insensitive
	// constructor so that the relative filename is case insensitive.
	FilePath ret;
	ret.SetRelativeFilenameInternal(FilePathRelativeFilename(sRelativePath));
	ret.m_uDirectory = (UInt32)eDirectory;
	ret.m_uType = (UInt32)eFileType;
#if SEOUL_DEBUG
	ret.m_pCStringFilePath = ret.GetRelativeFilenameInternal().CStr();
#endif
	return ret;
}

/**
 * Generates a string that is the fully resolved, absolute filename
 * of the file path described by this FilePath.
 *
 * This method should be called when a filename is needed for
 * any disk access.
 */
String FilePath::GetAbsoluteFilename() const
{
	return
		GameDirectoryToString((GameDirectory)m_uDirectory) +
		ToString() +
		FileTypeToCookedExtension((FileType)m_uType);
}

/**
 * Equivalent to GetAbsoluteFilename(), but for an explicit
 * platform.
 */
String FilePath::GetAbsoluteFilenameForPlatform(Platform ePlatform) const
{
	return
		GameDirectoryToStringForPlatform((GameDirectory)m_uDirectory, ePlatform) +
		ToString() +
		FileTypeToCookedExtension((FileType)m_uType);
}

/**
 * Resolves this FilePath to an absolute filename string.
 */
String FilePath::GetAbsoluteFilenameInSource() const
{
	String sReturn;
	if (GetDirectory() == GameDirectory::kContent)
	{
		sReturn = Path::Combine(GamePaths::Get()->GetSourceDir(), ToString()) +
			FileTypeToSourceExtension((FileType)m_uType);
	}
	else
	{
		sReturn = Path::Combine(GameDirectoryToString((GameDirectory)m_uDirectory), ToString()) +
			FileTypeToSourceExtension((FileType)m_uType);
	}

	return sReturn;
}

/**
 * @return this FilePath as a serializable SeoulEngine
 * content URL (e.g. config://FooBar.json).
 */
String FilePath::ToSerializedUrl() const
{
	String sSerializedUrl;
	auto const eDirectory = GetDirectory();
	if ((Int)eDirectory >= 0 && (Int)eDirectory < (Int)GameDirectory::GAME_DIRECTORY_COUNT)
	{
		sSerializedUrl.Append(kasGameDirectorySchemes[(UInt32)eDirectory]);
	}

	sSerializedUrl.Append("://");

	// FilePaths are always represented using the '/' in the <root>://<path> format,
	// so replace the backslash if it's the directory seperator for the current platform.
	String s(GetRelativeFilenameInSource());
	if (Path::DirectorySeparatorChar()[0] == '\\')
	{
		s = s.ReplaceAll(Path::DirectorySeparatorChar(), Path::AltDirectorySeparatorChar());
	}
	sSerializedUrl.Append(s);

	return sSerializedUrl;
}

} // namespace Seoul
