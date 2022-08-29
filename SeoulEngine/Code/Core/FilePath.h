/**
 * \file FilePath.h
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

#pragma once
#ifndef FILE_PATH_H
#define FILE_PATH_H

#include "FilePathRelativeFilename.h"
#include "HashFunctions.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "Path.h"

// FilePath uses a bitfield to store its raw hstring identifier,
// this is the max number of bits.
#define SEOUL_FILEPATH_HSTRING_VALUE_SIZE (1 << 19)

namespace Seoul
{

/**
 * File types that are supported by FilePath. Types correspond
 * to file extensions.
 *
 * If you want to use FilePath to refer to a new type of file, you
 * must add the extension to this enum and update the following functions:
 *
 * In SeoulEngine/Code/Core/FilePath.cpp:
 * - FileTypeToCookedExtension()
 * - FileTypeToSourceExtension()
 * - ExtensionToFileType()
 *
 * In SeoulTools/Code/Moriarty/Utilities/FileUtility.cs:
 * - Moriarty.Utilities.FileUtility.FileTypeToCookedExtension()
 * - Moriarty.Utilities.FileUtility.FileTypeToSourceExtension()
 * - Moriarty.Utilities.FileUtility.ExtensionToFileType()
 */
enum class FileType : UInt32
{
	kUnknown,

	kAnimation2D,

	kCsv,

	kEffect,
	kEffectHeader,

	kExe,

	kFont,

	kFxBank,

	kHtml,

	kJson,

	kPEMCertificate,

	kProtobuf,

	kSaveGame,
	kSceneAsset,
	kScenePrefab,

	kScript,

	kSoundBank,
	kSoundProject,

	kTexture0,
	kTexture1,
	kTexture2,
	kTexture3,
	kTexture4,

	kText,

	kUIMovie,

	kWav,
	kXml,

	// NOTE: Although tempting to maintain alphabetical order
	// of this list, new types should be added to the end
	// and old types should not be removed. If you need to
	// violate this guideline, you'll need to poke several cooked
	// file types to regenerate them, including animation files,
	// scene prefabs, and any other format that is actually
	// a cooked DataStore under the hood.
	//
	// WARNING WARNING: This also includes save game data, which
	// may be stored persistently for users on the server. This
	// will be much harder to maintain and migrate, so highly
	// recommended to just add values to the end.
	kScriptProject,
	kCs,
	kVideo,

	FILE_TYPE_COUNT,
	FIRST_TEXTURE_TYPE = kTexture0,
	LAST_TEXTURE_TYPE = kTexture4,
};

inline static Bool FileTypeNeedsCooking(FileType eFileType)
{
	switch (eFileType)
	{
	case FileType::kAnimation2D: // fall-through
	case FileType::kEffect: // fall-through
	case FileType::kFont: // fall-through
	case FileType::kFxBank: // fall-through
	case FileType::kProtobuf: // fall-through
	case FileType::kScript: // fall-through
	case FileType::kScriptProject: // fall-through
	case FileType::kSceneAsset: // fall-through
	case FileType::kScenePrefab: // fall-through
	case FileType::kSoundProject: // fall-through
	case FileType::kTexture0: // fall-through
	case FileType::kTexture1: // fall-through
	case FileType::kTexture2: // fall-through
	case FileType::kTexture3: // fall-through
	case FileType::kTexture4: // fall-through
	case FileType::kUIMovie:
		return true;
	default:
		return false;
	};
}

static inline UInt32 GetHash(FileType eFileType) { return GetHash((UInt32)eFileType); }

const String& FileTypeToCookedExtension(FileType eFileType);
const String& FileTypeToSourceExtension(FileType eFileType);
FileType ExtensionToFileType(const String& sExtension);

/**
 * @return True if a file type has only a text based format
 * on disk, not a binary cooked format.
 *
 * Some of these types may still be cooked into a binary
 * format when packaged (e.g. json has an optional DataStore "jsonb"
 * cooking support in the packaging system).
 */
inline static Bool IsTextOnlyFileType(FileType eFileType)
{
	switch (eFileType)
	{
	case FileType::kCsv: // fall-through
	case FileType::kHtml: // fall-through
	case FileType::kJson: // fall-through
	case FileType::kPEMCertificate: // fall-through
	case FileType::kText: // fall-through
	case FileType::kXml:
		return true;
	default:
		return false;
	};
}

inline static Bool IsTextureFileType(FileType eFileType)
{
	switch (eFileType)
	{
	case FileType::kTexture0: // fall-through
	case FileType::kTexture1: // fall-through
	case FileType::kTexture2: // fall-through
	case FileType::kTexture3: // fall-through
	case FileType::kTexture4:
		return true;
	default:
		return false;
	};
}

/**
 * File directories that are supported by FilePath. These are
 * the root paths of any relative files identified by a FilePath.
 *
 * If you want to use FilePath to use a new directory path,
 * you must add the path to this enum and update the GameDirectoryToString
 * global function.
 */
enum class GameDirectory : UInt32
{
	kUnknown,
	kConfig,
	kContent,
	kLog,
	kSave,
	kToolsBin,
	kVideos,
	GAME_DIRECTORY_COUNT
};

static Byte const* kasGameDirectorySchemes[] =
{
	"",
	"config",
	"content",
	"log",
	"save",
	"tools",
	"videos",
};
SEOUL_STATIC_ASSERT(SEOUL_ARRAY_COUNT(kasGameDirectorySchemes) == (UInt32)GameDirectory::GAME_DIRECTORY_COUNT);

static inline UInt32 GetHash(GameDirectory eGameDirectory) { return GetHash((UInt32)eGameDirectory); }

const String& GameDirectoryToString(GameDirectory eGameDirectory);
const String& GameDirectoryToStringForPlatform(GameDirectory eGameDirectory, Platform ePlatform);
const String& GameDirectoryToStringInSource(GameDirectory eGameDirectory);
GameDirectory GetGameDirectoryFromAbsoluteFilename(const String& sFilename);

// warning C4201: nonstandard extension used : nameless struct/union
// For the m_uType, m_uDirectory pair in FilePath.
#if defined(_MSC_VER)
#	pragma warning (push)
#	pragma warning (disable : 4201)
#endif

/**
 * FilePath is a structure used to uniquely identify a file. Two FilePaths
 * that refer to the same file are exactly equal. FilePaths are also small (4 bytes)
 * and most operations, except for the Create*FilePath(const String& sFilename)
 * functions, are computationally inexpensive. The Create* functions should be
 * used during load or during irregular object initialization to cache FilePaths,
 * they should not be called regularly in normal gameplay code.
 */
class FilePath SEOUL_SEALED
{
public:
	static FilePath CreateConfigFilePath(const String& sFilename);
	static FilePath CreateContentFilePath(const String& sFilename);
	static FilePath CreateLogFilePath(const String& sFilename);
	static FilePath CreateSaveFilePath(const String& sFilename);
	static FilePath CreateToolsBinFilePath(const String& sFilename);
	static FilePath CreateVideosFilePath(const String& sFilename);

	static FilePath CreateFilePath(
		GameDirectory eDirectory,
		const String& sFilename);

	// Creates a FilePath of type kUnknown without removing the extension from
	// the relative path.  This should only be used in very rare circumstances,
	// such as when you need to load a file type not supported by
	// FileType, or if you need to load a raw, uncooked texture.
	static FilePath CreateRawFilePath(
		GameDirectory eDirectory,
		const String& sFilename);

	FilePath()
		: m_uData(0u)
		// Note: Don't try to use HString().CStr() here. Although this is valid
		// there is no guarantee that the empty HString() is constructed until
		// another HString() has been constructed. This can cause this constructor
		// to fail during static FilePath construction.
#if SEOUL_DEBUG
		, m_pCStringFilePath("")
#endif
	{
	}

	FilePath(const FilePath& b)
		: m_uData(b.m_uData)
#if SEOUL_DEBUG
		, m_pCStringFilePath(GetRelativeFilenameInternal().CStr())
#endif
	{
	}

	/**
	 * Two file paths are equal if their directory, file type, and
	 * relative filenames match.
	 */
	Bool operator==(const FilePath& b) const
	{
		return (m_uData == b.m_uData);
	}

	Bool operator!=(const FilePath& b) const
	{
		return !(*this == b);
	}

	/**
	 * FilePaths are not sorted lexographically.
	 */
	Bool operator<(const FilePath& b) const
	{
		return (m_uData < b.m_uData);
	}

	Bool operator>(const FilePath& b) const
	{
		return (m_uData > b.m_uData);
	}

	/**
	 * True if this FilePath is non-zero (at least one of its game directory
	 * or relative filename fields has been assigned).
	 */
	Bool IsValid() const
	{
		return (0u != m_uDirectory || 0u != m_uRawHStringRelativeFilename);
	}

	/**
	 * Generates a unique hash code for this FilePath. Allows
	 * FilePath to be used as keys in hashing key-value structures.
	 */
	UInt32 GetHash() const
	{
		return Seoul::GetHash(m_uData);
	}

	// Use this method when a filename is needed for disk access.
	String GetAbsoluteFilename() const;

	// Equivalent to GetAbsoluteFilename(), but for an explicit
	// platform.
	String GetAbsoluteFilenameForPlatform(Platform ePlatform) const;

	// Use this method when you want to access the raw source asset
	// for a cooked content file. With no current exceptions, this is always
	// a developer-focused action, for hot loading and cooking. You will never
	// do this for loading game assets.
	String GetAbsoluteFilenameInSource() const;

	/**
	 * Generates a string that is the relative filename of the file path
	 * described by this FilePath. The filename is relative to the GameDirectory
	 * of this FilePath.
	 *
	 * This method should be called when serializing a FilePath. It should
	 * not be used for accessing a file from disk
	 */
	String GetRelativeFilename() const
	{
		return ToString() +
			FileTypeToCookedExtension((FileType)m_uType);
	}

	String GetRelativeFilenameInSource() const
	{
		return ToString() +
			FileTypeToSourceExtension((FileType)m_uType);
	}

	/**
	 * Restores this FilePath to a default, invalid FilePath.
	 */
	void Reset()
	{
		m_uData = 0u;

#if SEOUL_DEBUG
		m_pCStringFilePath = "";
#endif
	}

	/**
	 * Return a cstring representing this FilePath.
	 *
	 * The cstring returned by this method does not include the
	 * GameDirectory or FileType, only the relative filename. It should
	 * not be used to for accessing a file from disk
	 */
	Byte const* CStr() const
	{
		return GetRelativeFilenameInternal().CStr();
	}

	/**
	 * Generates a String representing this FilePath.
	 *
	 * The String returned by this method does not include the
	 * GameDirectory or FileType, only the relative filename. It should
	 * not be used to for accessing a file from disk
	 */
	String ToString() const
	{
		auto const relative = GetRelativeFilenameInternal();
		return String(relative.CStr(), relative.GetSizeInBytes());
	}

	/**
	 * @return The GameDirectory of this FilePath.
	 */
	GameDirectory GetDirectory() const
	{
		return (GameDirectory)m_uDirectory;
	}

	/**
	 * @return The base relative filename, without extension.
	 */
	FilePathRelativeFilename GetRelativeFilenameWithoutExtension() const
	{
		return GetRelativeFilenameInternal();
	}

	/**
	 * @return The FileType of this FilePath.
	 */
	FileType GetType() const
	{
		return (FileType)m_uType;
	}

	/**
	 * Set the Directory of this FilePath.
	 */
	void SetDirectory(GameDirectory eDirectory)
	{
		m_uDirectory = (UInt8)eDirectory;
	}

	/**
	 * Set the relative filename of this FilePath.
	 *
	 * This method expects relativeFilename to be normalized
	 * according to FilePath's normalization rules - see FilePath::CreateFilePath().
	 */
	void SetRelativeFilenameWithoutExtension(FilePathRelativeFilename relativeFilenameWithoutExtension)
	{
		SetRelativeFilenameInternal(relativeFilenameWithoutExtension);
#if SEOUL_DEBUG
		m_pCStringFilePath = GetRelativeFilenameInternal().CStr();
#endif
	}

	/**
	 * Set the FileType of this FilePath.
	 *
	 * This method is useful for swapping the file type of a FilePath
	 * for cases where two files have exactly the same GameDirectory and
	 * relative filename but a different file type (i.e. SIF0 and SIF2).
	 */
	void SetType(FileType eFileType)
	{
		m_uType = (UInt8)eFileType;
	}

	// Return this FilePath as a serializable SeoulEngine
	// content URL (e.g. config://FooBar.json).
	String ToSerializedUrl() const;

private:
	FilePathRelativeFilename GetRelativeFilenameInternal() const
	{
		FilePathRelativeFilename ret;
		ret.SetHandleValue(m_uRawHStringRelativeFilename);
		return ret;
	}

	void SetRelativeFilenameInternal(FilePathRelativeFilename relative)
	{
		m_uRawHStringRelativeFilename = relative.GetHandleValue();
	}

	static FilePath InternalCreateFilePath(
		GameDirectory eDirectory,
		const String& sFilename,
		Bool bDetermineFileTypeFromExtension);

	SEOUL_STATIC_ASSERT(HStringDataProperties<HStringData::InternalIndexType>::GlobalArraySize <= SEOUL_FILEPATH_HSTRING_VALUE_SIZE);

	union
	{
		struct
		{
			UInt32 m_uDirectory : 3;
			UInt32 m_uType : 5;
			UInt32 m_uRawHStringRelativeFilename : 24;
		};
		UInt32 m_uData;
	};

#if SEOUL_DEBUG
	// Debug-only member to allow easier debugging of FilePaths.
	Byte const* m_pCStringFilePath;
#endif
}; // class FilePath

// warning C4201: nonstandard extension used : nameless struct/union
// For the m_uType, m_uDirectory pair in FilePath.
#if defined(_MSC_VER)
#	pragma warning (pop)
#endif

#if SEOUL_DEBUG
#	if SEOUL_PLATFORM_64
		SEOUL_STATIC_ASSERT(sizeof(FilePath) == 16);
#	else
		SEOUL_STATIC_ASSERT(sizeof(FilePath) == 8);
#	endif
#else
	SEOUL_STATIC_ASSERT(sizeof(FilePath) == 4);
#endif

/**
 * Helper function to allow FilePaths to be used as
 * keys in key-value data structures that use hash values on
 * the key.
 */
inline UInt32 GetHash(const FilePath& key)
{
	return key.GetHash();
}

template <>
struct DefaultHashTableKeyTraits<FilePath>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static FilePath GetNullKey()
	{
		return FilePath();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

} // namespace Seoul

#endif // include guard
