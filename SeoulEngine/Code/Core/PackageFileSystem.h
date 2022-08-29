/**
 * \file PackageFileSystem.h
 * \brief Specialization of IFileSystem that services file requests
 * into contiguous package files.
 *
 * Packages are read-only. As a result, packages used by SeoulEngine
 * runtime code must be generated offline using the PackageCooker command-line
 * tool.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PACKAGE_FILE_SYSTEM_H
#define PACKAGE_FILE_SYSTEM_H

#include "HashTable.h"
#include "IFileSystem.h"
#include "Mutex.h"
namespace Seoul { struct ZSTDDecompressionDict; }

namespace Seoul
{

// Forward declarations
class PackageSyncFile;

/** Current signature of a package file. */
static const UInt32 kuPackageSignature = 0xDA7F;

/** Current version of a package file. */
static const UInt32 kuPackageVersion = 21;

/** Constants base name used if compressed file systems should use a dictionary. */
static const Byte* ksPackageCompressionDictNameFormat = "pkgcdict_%s.dat";

// TODO: Probably, infer these lists from the package
// configuration file.
static Byte const* const kasStandardContentPackageFmts[] =
{
	"%s_BaseContent.sar",
	"%s_Content.sar",
	"%s_ScriptsDebug.sar",
};

SEOUL_DEFINE_PACKED_STRUCT(
	struct PackageFileEntry
	{
		UInt64 m_uOffsetToFile;
		UInt64 m_uCompressedFileSize;
		UInt64 m_uUncompressedFileSize;
		UInt64 m_uModifiedTime;
		UInt32 m_uCrc32Pre; // Crc32 of internal file data (before optional package compression and obfuscation).

		// This value may be 0/undefined for
		// old packages (pre version 19).
		UInt32 m_uCrc32Post; // Crc32 of file block on disk (after optional package compression and obfuscation).
	});

void EndianSwap(PackageFileEntry& rEntry);

struct PackageFileTableEntry
{
	PackageFileEntry m_Entry;
	UInt32 m_uXorKey;
	UInt32 m_uOrder;
};

enum class SerializedGameDirectory
{
	kUnknown = 0,
	kConfig = 1,
	kContent
};

inline GameDirectory SerializedToGameDirectory(SerializedGameDirectory eSerializedGameDirectory)
{
	switch (eSerializedGameDirectory)
	{
	case SerializedGameDirectory::kConfig: return GameDirectory::kConfig;
	case SerializedGameDirectory::kContent: return GameDirectory::kContent;
	default:
		return GameDirectory::kUnknown;
	};
}

struct PackageFileHeader
{
	// Backwards compatibility support.
	static const UInt32 ku13_OrigVersion = 13u;
	static const UInt32 ku16_LZ4CompressionVersion = 16u;
	static const UInt32 ku17_PreCompressionDictVersion = 17u;
	static const UInt32 ku18_PreDualCrc32Version = 18u;
	static const UInt32 ku19_PreFileTablePostCrc32 = 19u;
	static const UInt32 ku20_PreVariationRestore = 20u;

	SEOUL_DEFINE_PACKED_STRUCT(struct Version13
	{
		UInt64 m_uTotalPackageFileSizeInBytes;
		UInt64 m_uOffsetToFileTableInBytes;
		UInt32 m_uTotalEntriesInFileTable;
		UInt16 m_eGameDirectory;
		UInt16 m_bCompressedFileTable;
		UInt32 m_uSizeOfFileTableInBytes;
		UInt32 m_uBuildVersionMajor;
		UInt32 m_uBuildChangelist;
		UInt16 m_uPackageVariation;
		UInt8  m_bSupportDirectoryQueries;
		UInt8  m_bObfuscated;
	});
	SEOUL_STATIC_ASSERT(sizeof(Version13) == 40);

	SEOUL_DEFINE_PACKED_STRUCT(struct Version16And17
	{
		UInt64 m_uTotalPackageFileSizeInBytes;
		UInt64 m_uOffsetToFileTableInBytes;
		UInt32 m_uTotalEntriesInFileTable;
		UInt16 m_eGameDirectory;
		UInt16 m_bCompressedFileTable;
		UInt32 m_uSizeOfFileTableInBytes;
		UInt32 m_uBuildVersionMajor;
		UInt32 m_uBuildChangelist;
		UInt16  m_bSupportDirectoryQueries;
		UInt16  m_bObfuscated;
	});
	SEOUL_STATIC_ASSERT(sizeof(Version16And17) == 40);

	SEOUL_DEFINE_PACKED_STRUCT(struct Version18And19And20
	{
		UInt64 m_uTotalPackageFileSizeInBytes;
		UInt64 m_uOffsetToFileTableInBytes;
		UInt32 m_uTotalEntriesInFileTable;
		UInt16 m_eGameDirectory;
		UInt16 m_bCompressedFileTable;
		UInt32 m_uSizeOfFileTableInBytes;
		UInt32 m_uBuildVersionMajor;
		UInt32 m_uBuildChangelist;
		UInt16 m_bSupportDirectoryQueries;
		UInt8  m_bObfuscated;
		UInt8  m_ePlatform;
	});
	SEOUL_STATIC_ASSERT(sizeof(Version18And19And20) == 40);

	SEOUL_DEFINE_PACKED_STRUCT(struct Version21
	{
		UInt64 m_uTotalPackageFileSizeInBytes;
		UInt64 m_uOffsetToFileTableInBytes;
		UInt32 m_uTotalEntriesInFileTable;
		UInt16 m_eGameDirectory;
		UInt16 m_bCompressedFileTable;
		UInt32 m_uSizeOfFileTableInBytes;
		UInt16 m_uPackageVariation;
		UInt16 m_uBuildVersionMajor;
		UInt32 m_uBuildChangelist;
		UInt16 m_bSupportDirectoryQueries;
		UInt8  m_bObfuscated;
		UInt8  m_ePlatform;
	});
	SEOUL_STATIC_ASSERT(sizeof(Version21) == 40);

	UInt32 m_uSignature;
	UInt32 m_uVersion;
	union
	{
		Version13           m_V13;
		Version16And17      m_V16a17;
		Version18And19And20 m_V18a19a20;
		Version21           m_V21;
	};

public:
	static void EndianSwap(PackageFileHeader& rHeader);

	/** Equality comparison. */
	Bool operator==(const PackageFileHeader& b) const
	{
		// All fields byte-for-byte equal.
		return (0 == memcmp(this, &b, sizeof(b)));
	}
	Bool operator!=(const PackageFileHeader& b) const
	{
		return !(*this == b);
	}

	/** @return The build changelist stored in the header. */
	UInt32 GetBuildChangelist() const
	{
		if (m_uVersion <= 13u)
		{
			return m_V13.m_uBuildChangelist;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			return m_V16a17.m_uBuildChangelist;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			return m_V18a19a20.m_uBuildChangelist;
		}
		else if (m_uVersion >= 21u)
		{
			return m_V21.m_uBuildChangelist;
		}
		else
		{
			return 0u;
		}
	}

	/** @return The build variation stored in the header. */
	UInt32 GetPackageVariation() const
	{
		if (m_uVersion <= 13u)
		{
			return m_V13.m_uPackageVariation;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			return 0u;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			return 0u;
		}
		else if (m_uVersion >= 21u)
		{
			return m_V21.m_uPackageVariation;
		}
		else
		{
			return 0u;
		}
	}

	/** @return The build version stored in the header. */
	UInt32 GetBuildVersionMajor() const
	{
		if (m_uVersion <= 13u)
		{
			return m_V13.m_uBuildVersionMajor;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			return m_V16a17.m_uBuildVersionMajor;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			return m_V18a19a20.m_uBuildVersionMajor;
		}
		else if (m_uVersion >= 21u)
		{
			return m_V21.m_uBuildVersionMajor;
		}
		else
		{
			return 0u;
		}
	}

	/** @return The game directory stored in the header. */
	SerializedGameDirectory GetGameDirectory() const
	{
		if (m_uVersion <= 13u)
		{
			return (SerializedGameDirectory)m_V13.m_eGameDirectory;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			return (SerializedGameDirectory)m_V16a17.m_eGameDirectory;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			return (SerializedGameDirectory)m_V18a19a20.m_eGameDirectory;
		}
		else if (m_uVersion >= 21u)
		{
			return (SerializedGameDirectory)m_V21.m_eGameDirectory;
		}
		else
		{
			return SerializedGameDirectory::kUnknown;
		}
	}

	/** @return The build platform of this .sar file. */
	Platform GetPlatform() const
	{
		if (m_uVersion >= 21u)
		{
			return (Platform)m_V21.m_ePlatform;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			return (Platform)m_V18a19a20.m_ePlatform;
		}
		else
		{
			// No platform prior to version 18, so just return current
			// platform.
			return keCurrentPlatform;
		}
	}

	/** @return Absolute offset to the package's file table. */
	UInt64 GetOffsetToFileTableInBytes() const
	{
		if (m_uVersion <= 13u)
		{
			return m_V13.m_uOffsetToFileTableInBytes;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			return m_V16a17.m_uOffsetToFileTableInBytes;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			return m_V18a19a20.m_uOffsetToFileTableInBytes;
		}
		else if (m_uVersion >= 21u)
		{
			return m_V21.m_uOffsetToFileTableInBytes;
		}
		else
		{
			return 0u;
		}
	}

	/** @return Size of the file table on disk (not necessarily equal to the in-memory size of the table is compressed). */
	UInt32 GetSizeOfFileTableInBytes() const
	{
		if (m_uVersion <= 13u)
		{
			return m_V13.m_uSizeOfFileTableInBytes;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			return m_V16a17.m_uSizeOfFileTableInBytes;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			return m_V18a19a20.m_uSizeOfFileTableInBytes;
		}
		else if (m_uVersion >= 21u)
		{
			return m_V21.m_uSizeOfFileTableInBytes;
		}
		else
		{
			return 0u;
		}
	}

	/** @return The number of entries in the package's file table. */
	UInt32 GetTotalEntriesInFileTable() const
	{
		if (m_uVersion <= 13u)
		{
			return m_V13.m_uTotalEntriesInFileTable;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			return m_V16a17.m_uTotalEntriesInFileTable;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			return m_V18a19a20.m_uTotalEntriesInFileTable;
		}
		else if (m_uVersion >= 21u)
		{
			return m_V21.m_uTotalEntriesInFileTable;
		}
		else
		{
			return 0u;
		}
	}

	/** @return Total expected size of the package on disk. */
	UInt64 GetTotalPackageFileSizeInBytes() const
	{
		if (m_uVersion <= 13u)
		{
			return m_V13.m_uTotalPackageFileSizeInBytes;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			return m_V16a17.m_uTotalPackageFileSizeInBytes;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			return m_V18a19a20.m_uTotalPackageFileSizeInBytes;
		}
		else if (m_uVersion >= 21u)
		{
			return m_V21.m_uTotalPackageFileSizeInBytes;
		}
		else
		{
			return 0u;
		}
	}

	/** @return True if the file table of the package is compressed, false otherwise. */
	Bool HasCompressedFileTable() const
	{
		if (m_uVersion <= 13u)
		{
			return (0u != m_V13.m_bCompressedFileTable);
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			return (0u != m_V16a17.m_bCompressedFileTable);
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			return (0u != m_V18a19a20.m_bCompressedFileTable);
		}
		else if (m_uVersion >= 21u)
		{
			return (0u != m_V21.m_bCompressedFileTable);
		}
		else
		{
			return false;
		}
	}

	/**
	 * @return Whether the file table includes a crc32 value to verify its validity.
	 */
	Bool HasFileTablePostCRC32() const
	{
		return (m_uVersion > ku19_PreFileTablePostCrc32);
	}

	/**
	 * @return Whether this header indicates a package that has pre-computed post CRC32
	 * values. "Post CRC32" values are computed after any obfuscation or compression
	 * was applied at the archive level.
	 */
	Bool HasPostCRC32() const
	{
		return (m_uVersion > ku18_PreDualCrc32Version);
	}

	/** @return True if the file table of the package is compressed, false otherwise. */
	Bool HasSupportDirectoryQueries() const
	{
		if (m_uVersion <= 13u)
		{
			return (0u != m_V13.m_bSupportDirectoryQueries);
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			return (0u != m_V16a17.m_bSupportDirectoryQueries);
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			return (0u != m_V18a19a20.m_bSupportDirectoryQueries);
		}
		else if (m_uVersion >= 21u)
		{
			return (0u != m_V21.m_bSupportDirectoryQueries);
		}
		else
		{
			return false;
		}
	}

	/** @return Old header, package uses LZ4 compression. */
	Bool IsOldLZ4Compression() const
	{
		return (m_uVersion == ku16_LZ4CompressionVersion);
	}

	/** @return Whether the archive is obfuscated or not. */
	Bool IsObfuscated() const
	{
		if (m_uVersion <= 13u)
		{
			return (0u != m_V13.m_bObfuscated);
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			return (0u != m_V16a17.m_bObfuscated);
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			return (0u != m_V18a19a20.m_bObfuscated);
		}
		else if (m_uVersion >= 21u)
		{
			return (0u != m_V21.m_bObfuscated);
		}
		else
		{
			return false;
		}
	}

	/** @return True if this header indicates an old package. */
	Bool IsOld() const
	{
		return (m_uVersion != kuPackageVersion);
	}

	/** Head and several backwards compatible versions. */
	Bool IsVersionValid() const
	{
		return (
			ku13_OrigVersion == m_uVersion ||
			ku16_LZ4CompressionVersion == m_uVersion ||
			ku17_PreCompressionDictVersion == m_uVersion ||
			ku18_PreDualCrc32Version == m_uVersion ||
			ku19_PreFileTablePostCrc32 == m_uVersion ||
			ku20_PreVariationRestore == m_uVersion ||
			kuPackageVersion == m_uVersion);
	}

	/**
	 * @return True if this header appears to require
	 * an endian swap (based on the m_uSignature field),
	 * false otherwise.
	 */
	Bool RequiresEndianSwap() const
	{
		return (m_uSignature == EndianSwap32(kuPackageSignature));
	}

	/** Set the build changelist stored in the header. */
	void SetBuildChangelist(UInt32 uBuildChangelist)
	{
		if (m_uVersion <= 13u)
		{
			m_V13.m_uBuildChangelist = uBuildChangelist;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			m_V16a17.m_uBuildChangelist = uBuildChangelist;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			m_V18a19a20.m_uBuildChangelist = uBuildChangelist;
		}
		else if (m_uVersion >= 21u)
		{
			m_V21.m_uBuildChangelist = uBuildChangelist;
		}
	}

	/** Set the build version stored in the header. */
	void SetBuildVersionMajor(UInt32 uBuildVersionMajor)
	{
		if (m_uVersion <= 13u)
		{
			m_V13.m_uBuildVersionMajor = uBuildVersionMajor;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			m_V16a17.m_uBuildVersionMajor = uBuildVersionMajor;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			m_V18a19a20.m_uBuildVersionMajor = uBuildVersionMajor;
		}
		else if (m_uVersion >= 21u)
		{
			m_V21.m_uBuildVersionMajor = uBuildVersionMajor;
		}
	}

	/** Set the game directory stored in the header. */
	void SetGameDirectory(SerializedGameDirectory eGameDirectory)
	{
		if (m_uVersion <= 13u)
		{
			m_V13.m_eGameDirectory = (UInt16)eGameDirectory;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			m_V16a17.m_eGameDirectory = (UInt16)eGameDirectory;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			m_V18a19a20.m_eGameDirectory = (UInt16)eGameDirectory;
		}
		else if (m_uVersion >= 21u)
		{
			m_V21.m_eGameDirectory = (UInt16)eGameDirectory;
		}
	}

	/** Set true if the file table of the package is compressed, false otherwise. */
	void SetHasCompressedFileTable(Bool bCompressedFileTable)
	{
		if (m_uVersion <= 13u)
		{
			m_V13.m_bCompressedFileTable = (bCompressedFileTable ? 1u : 0u);
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			m_V16a17.m_bCompressedFileTable = (bCompressedFileTable ? 1u : 0u);
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			m_V18a19a20.m_bCompressedFileTable = (bCompressedFileTable ? 1u : 0u);
		}
		else if (m_uVersion >= 21u)
		{
			m_V21.m_bCompressedFileTable = (bCompressedFileTable ? 1u : 0u);
		}
	}

	/**Set to true if the file table of the package is compressed, false otherwise. */
	void SetHasSupportDirectoryQueries(Bool bSupportDirectoryQueries)
	{
		if (m_uVersion <= 13u)
		{
			m_V13.m_bSupportDirectoryQueries = (bSupportDirectoryQueries ? 1u : 0u);
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			m_V16a17.m_bSupportDirectoryQueries = (bSupportDirectoryQueries ? 1u : 0u);
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			m_V18a19a20.m_bSupportDirectoryQueries = (bSupportDirectoryQueries ? 1u : 0u);
		}
		else if (m_uVersion >= 21u)
		{
			m_V21.m_bSupportDirectoryQueries = (bSupportDirectoryQueries ? 1u : 0u);
		}
	}

	/** Update obfuscation and platform. Necessary with version differences. */
	void SetPlatformAndObfuscation(Platform ePlatform, Bool bObfuscation)
	{
		// Old version archive, only set obfuscation bit.
		if (m_uVersion < 18u)
		{
			if (m_uVersion <= 13u)
			{
				m_V13.m_bObfuscated = (bObfuscation ? 1 : 0);
			}
			else if (m_uVersion >= 16u && m_uVersion <= 17u)
			{
				m_V16a17.m_bObfuscated = (bObfuscation ? 1 : 0);
			}
		}
		// New archive, set both.
		else
		{
			if (m_uVersion >= 18u && m_uVersion <= 20u)
			{
				m_V18a19a20.m_bObfuscated = (bObfuscation ? 1 : 0);
				m_V18a19a20.m_ePlatform = (UInt8)ePlatform;
			}
			else
			{
				m_V21.m_bObfuscated = (bObfuscation ? 1 : 0);
				m_V21.m_ePlatform = (UInt8)ePlatform;
			}
		}
	}

	/** Set the absolute offset to the package's file table. */
	void SetOffsetToFileTableInBytes(UInt64 uOffsetToFileTableInBytes)
	{
		if (m_uVersion <= 13u)
		{
			m_V13.m_uOffsetToFileTableInBytes = uOffsetToFileTableInBytes;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			m_V16a17.m_uOffsetToFileTableInBytes = uOffsetToFileTableInBytes;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			m_V18a19a20.m_uOffsetToFileTableInBytes = uOffsetToFileTableInBytes;
		}
		else if (m_uVersion >= 21u)
		{
			m_V21.m_uOffsetToFileTableInBytes = uOffsetToFileTableInBytes;
		}
	}

	/** Set the size of the file table on disk (not necessarily equal to the in-memory size of the table is compressed). */
	void SetSizeOfFileTableInBytes(UInt32 uSizeOfFileTableInBytes)
	{
		if (m_uVersion <= 13u)
		{
			m_V13.m_uSizeOfFileTableInBytes = uSizeOfFileTableInBytes;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			m_V16a17.m_uSizeOfFileTableInBytes = uSizeOfFileTableInBytes;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			m_V18a19a20.m_uSizeOfFileTableInBytes = uSizeOfFileTableInBytes;
		}
		else if (m_uVersion >= 21u)
		{
			m_V21.m_uSizeOfFileTableInBytes = uSizeOfFileTableInBytes;
		}
	}

	/** Set the number of entries in the package's file table. */
	void SetTotalEntriesInFileTable(UInt32 uTotalEntriesInFileTable)
	{
		if (m_uVersion <= 13u)
		{
			m_V13.m_uTotalEntriesInFileTable = uTotalEntriesInFileTable;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			m_V16a17.m_uTotalEntriesInFileTable = uTotalEntriesInFileTable;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			m_V18a19a20.m_uTotalEntriesInFileTable = uTotalEntriesInFileTable;
		}
		else if (m_uVersion >= 21u)
		{
			m_V21.m_uTotalEntriesInFileTable = uTotalEntriesInFileTable;
		}
	}

	/** Set total expected size of the package on disk. */
	void SetTotalPackageFileSizeInBytes(UInt64 uTotalPackageFileSizeInBytes)
	{
		if (m_uVersion <= 13u)
		{
			m_V13.m_uTotalPackageFileSizeInBytes = uTotalPackageFileSizeInBytes;
		}
		else if (m_uVersion >= 16u && m_uVersion <= 17u)
		{
			m_V16a17.m_uTotalPackageFileSizeInBytes = uTotalPackageFileSizeInBytes;
		}
		else if (m_uVersion >= 18u && m_uVersion <= 20u)
		{
			m_V18a19a20.m_uTotalPackageFileSizeInBytes = uTotalPackageFileSizeInBytes;
		}
		else if (m_uVersion >= 21u)
		{
			m_V21.m_uTotalPackageFileSizeInBytes = uTotalPackageFileSizeInBytes;
		}
	}
};

// WARNING, WARNING: Due to our usage of downloading
// and patching of .sar files, it is not straightforward
// to change the header size. Take care if you decide
// you need to do so, in particular, review
// DownloadablePackageFileSystem and update it to
// handle a variable header size.
SEOUL_STATIC_ASSERT(sizeof(PackageFileHeader) == 48);

static inline void EndianSwap(PackageFileHeader& rHeader)
{
	PackageFileHeader::EndianSwap(rHeader);
}

/** Entry input/output as part of PerformCrc32Check(). */
struct PackageCrc32Entry SEOUL_SEALED
{
	PackageFileEntry m_Entry{};
	FilePath m_FilePath{};
	Bool m_bCrc32Ok{};
};
typedef Vector<PackageCrc32Entry, MemoryBudgets::Io> PackageCrc32Entries;

/** Shared interface of PackageFileSystem instances. */
class IPackageFileSystem SEOUL_ABSTRACT : public IFileSystem
{
public:
	typedef HashTable<FilePath, PackageFileTableEntry, MemoryBudgets::Io> FileTable;

	virtual ~IPackageFileSystem()
	{
	}

	virtual Atomic32Type GetActiveSyncFileCount() const = 0;
	virtual const String& GetAbsolutePackageFilename() const = 0;
	virtual UInt32 GetBuildChangelist() const = 0;
	virtual UInt32 GetPackageVariation() const = 0;
	virtual UInt32 GetBuildVersionMajor() const = 0;
	virtual Bool GetFileTable(FileTable& rtFileTable) const = 0;
	virtual Bool HasPostCrc32() const = 0;
	virtual Bool IsOk() const = 0;
	virtual Atomic32Type GetNetworkFileRequestsIssued() const { return 0; }
	virtual Atomic32Type GetNetworkFileRequestsCompleted() const { return 0; }
	virtual Atomic32Type GetNetworkTimeMillisecond() const { return 0; }
	virtual Atomic32Type GetNetworkBytes() const { return 0; }
	virtual Bool PerformCrc32Check(PackageCrc32Entries* pvInOutEntries = nullptr) = 0;

protected:
	IPackageFileSystem()
	{
	}
}; // class IPackageFileSystem

/**
 * PackageFileSystem services file open requests for
 * files contained in a single, contiguous file on disk.
 */
class PackageFileSystem SEOUL_SEALED : public IPackageFileSystem
{
public:
	// Utility, can be used to verify a valid header in a fully stored or partially stored .sar file.
	static Bool CheckSarHeader(void const* pData, size_t zDataSizeInBytes);

	// Utility, generate the xor key for an obfuscation operation.
	static UInt32 GenerateObfuscationKey(Byte const* s, UInt32 u);
	static inline UInt32 GenerateObfuscationKey(const String& s)
	{
		return GenerateObfuscationKey(s.CStr(), s.GetSize());
	}

	// Utility, used for obfuscation operations on files and the file table.
	static void Obfuscate(UInt32 uXorKey, void *pData, size_t zDataSize, Int64 iFileOffset);

	// Utility, read a PackageFileHeader from a complete or partial byte stream, expected to
	// point at the head of a .sar file.
	static Bool ReadPackageHeader(void const* pData, size_t zDataSizeInBytes, PackageFileHeader& rHeader);

	PackageFileSystem(
		void* pInMemoryPackageData,
		UInt32 uPackageFileSizeInBytes,
		Bool bTakeOwnershipOfData = true,
		const String& sAbsolutePackageFilename = String());
	PackageFileSystem(
		const String& sAbsolutePackageFilename,
		Bool bLoadIntoMemory = false,
		Bool bOpenPackageFileWithWritePermissions = false,
		Bool bDeferCompressionDictProcessing = false);
	~PackageFileSystem();

	// Low level operation - advanced usage only!
	//
	// Commits an update to the sar data on disk. No validation is performed, this method assumes
	// that you know what you are doing.
	Bool CommitChangeToSarFile(
		void const* pData,
		UInt32 uSizeInBytes,
		Int64 iWritePositionFromStartOfArchive);

	// Low level operation - advanced usage only!
	//
	// Force a blocking flush (fsync) for any writes.
	Bool FlushChanges();

	// Low level operation - advanced usage only!
	//
	// Read data directly from the file that backs this PackageFileSystem. If the package has compression, obfuscation, etc.
	// enabled, the data is left compressed/obfuscated/etc.
	Bool ReadRaw(
		UInt64 uOffsetToDataInFile,
		void* pBuffer,
		UInt32 uSizeInBytes);

	/**
	 * Post CRC32 valid or not.
	 *
	 * Post CRC32 is a CRC32 computed after any compression or obfuscation
	 * has been applied at package level. This value was not added until
	 * version 19 so it may be undefined if a package is older.
	 */
	virtual Bool HasPostCrc32() const SEOUL_OVERRIDE
	{
		return m_bHasPostCrc32;
	}

	/**
	 * @return True if the PackageFileSystem was fully loaded successfully, false otherwise.
	 *
	 * This is a basic (inexpensive) check that verifies that the PackageFileSystem
	 * file tables and header appear intact. To perform a full (expensive) Crc32 verification
	 * of the package, call PerformCrc32Check().
	 */
	virtual Bool IsOk() const SEOUL_OVERRIDE
	{
		return m_bOk;
	}

	/** If !IsOk(), return details of the load failure. */
	const String& GetLoadError() const
	{
		return m_sLoadError;
	}

	/**
	 * @return Always false - PackageFileSystem is not mutable.
	 */
	virtual Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite = false) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem is not mutable.
	 */
	virtual Bool Copy(const String& sFrom, const String& sTo, Bool bAllowOverwrite = false) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem is not mutable.
	 */
	virtual Bool CreateDirPath(FilePath dirPath) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem is not mutable.
	 */
	virtual Bool CreateDirPath(const String& sDirPath) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem is not mutable.
	 */
	virtual Bool DeleteDirectory(FilePath dirPath, Bool bRecursive) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem is not mutable.
	 */
	virtual Bool DeleteDirectory(const String& sAbsoluteDirPath, Bool bRecursive) SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Bool GetFileSize(
		FilePath filePath,
		UInt64& ruFileSize) const SEOUL_OVERRIDE;

	virtual Bool GetFileSizeForPlatform(Platform ePlatform, FilePath filePath, UInt64& ruFileSize) const SEOUL_OVERRIDE
	{
		if (m_Header.GetPlatform() != ePlatform)
		{
			return false;
		}

		return GetFileSize(filePath, ruFileSize);
	}

	/**
	 * @return Always false - PackageFileSystem cannot service requests for
	 * files that cannot be tagged with a FilePath.
	 */
	virtual Bool GetFileSize(
		const String& sAbsoluteFilename,
		UInt64& ruFileSize) const  SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return The game directory that was the source of this PackageFileSystem.
	 */
	GameDirectory GetPackageGameDirectory() const
	{
		return m_ePackageGameDirectory;
	}

	virtual Bool GetModifiedTime(
		FilePath filePath,
		UInt64& ruModifiedTime) const SEOUL_OVERRIDE;

	virtual Bool GetModifiedTimeForPlatform(Platform ePlatform, FilePath filePath, UInt64& ruModifiedTime) const SEOUL_OVERRIDE
	{
		if (m_Header.GetPlatform() != ePlatform)
		{
			return false;
		}

		return GetModifiedTime(filePath, ruModifiedTime);
	}

	/**
	 * @return Always false - PackageFileSystem cannot service requests for
	 * files that cannot be tagged with a FilePath.
	 */
	virtual Bool GetModifiedTime(const String& sAbsoluteFilename, UInt64& ruModifiedTime) const  SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem is not mutable.
	 */
	virtual Bool Rename(FilePath from, FilePath to) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem is not mutable.
	 */
	virtual Bool Rename(const String& sFrom, const String& sTo) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem is not mutable.
	 */
	virtual Bool SetModifiedTime(FilePath filePath, UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem is not mutable.
	 */
	virtual Bool SetModifiedTime(const String& sAbsoluteFilename, UInt64 uModifiedTime) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem is not mutable.
	 */
	virtual Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnly) SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem is not mutable.
	 */
	virtual Bool SetReadOnlyBit(const String& sAbsoluteFilename, Bool bReadOnly) SEOUL_OVERRIDE
	{
		return false;
	}

	// Attempt to delete filePath, return true on success.
	virtual Bool Delete(FilePath filePath) SEOUL_OVERRIDE
	{
		// Not supported.
		return false;
	}

	// Attempt to delete sAbsoluteFilename, return true on success.
	virtual Bool Delete(const String& sAbsoluteFilename) SEOUL_OVERRIDE
	{
		// Not supported.
		return false;
	}

	virtual Bool Exists(FilePath filePath) const SEOUL_OVERRIDE;
	virtual Bool ExistsForPlatform(Platform ePlatform, FilePath filePath) const SEOUL_OVERRIDE
	{
		if (m_Header.GetPlatform() != ePlatform)
		{
			return false;
		}

		return Exists(filePath);
	}

	/**
	 * @return Always false - PackageFileSystem cannot open files which cannot be
	 * described by a FilePath.
	 */
	virtual Bool Exists(const String& sAbsoluteFilename) const SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem does not store
	 * directories.
	 */
	virtual Bool IsDirectory(FilePath filePath) const SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return Always false - PackageFileSystem cannot open files which cannot be
	 * described by a FilePath.
	 */
	virtual Bool IsDirectory(const String& sAbsoluteFilename) const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Bool Open(
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE;

	virtual Bool OpenForPlatform(
		Platform ePlatform,
		FilePath filePath,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		if (m_Header.GetPlatform() != ePlatform)
		{
			return false;
		}

		return Open(filePath, eMode, rpFile);
	}

	/**
	 * @return Always false - PackageFileSystem cannot open files which cannot be
	 * described by a FilePath.
	 */
	virtual Bool Open(
		const String& sAbsoluteFilename,
		File::Mode eMode,
		ScopedPtr<SyncFile>& rpFile) SEOUL_OVERRIDE
	{
		return false;
	}

	// Attempt to populate rvResults with a list of files and (optionally)
	// directories contained within the directory represented by dirPath
	virtual Bool GetDirectoryListing(
		FilePath dirPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE;

	/**
	 * @return Always false - PackageFileSystem cannot open files which cannot be
	 * described by a FilePath.
	 */
	virtual Bool GetDirectoryListing(
		const String& sAbsoluteDirectoryPath,
		Vector<String>& rvResults,
		Bool bIncludeDirectoriesInResults = true,
		Bool bRecursive = true,
		const String& sFileExtension = String()) const SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * @return The number of open sync file handles to this PackageSyncFile.
	 */
	virtual Atomic32Type GetActiveSyncFileCount() const SEOUL_OVERRIDE
	{
		return m_ActiveSyncFileCount;
	}

	/**
	 * @return The absolute path to the .sar file associated with this PackageFileSystem
	 * at load.
	 */
	virtual const String& GetAbsolutePackageFilename() const SEOUL_OVERRIDE
	{
		return m_sAbsolutePackageFilename;
	}

	/**
	 * @return The changelist of the content stored in this PackageFileSystem, or 0u if
	 * no changelist was specified when the archive was created.
	 */
	virtual UInt32 GetBuildChangelist() const SEOUL_OVERRIDE
	{
		return m_Header.GetBuildChangelist();
	}

	/**
	 * @return The build variation of the generated .sar. Typically 0 for no variation.
	 */
	virtual UInt32 GetPackageVariation() const SEOUL_OVERRIDE
	{
		return m_Header.GetPackageVariation();
	}

	/**
	 * @return The build major version of the content stored in this PackageFileSystem, or 0u if
	 * no changelist was specified when the archive was created.
	 */
	virtual UInt32 GetBuildVersionMajor() const SEOUL_OVERRIDE
	{
		return m_Header.GetBuildVersionMajor();
	}

	typedef HashTable<FilePath, PackageFileTableEntry, MemoryBudgets::Io> FileTable;

	/**
	 * @return A read-only reference to the table of file entries
	 * inside this PackageFileSystem.
	 */
	const FileTable& GetFileTable() const
	{
		return m_tFileTable;
	}

	// Convenience, return the entire file table as a PackageCrc32Entries vector, properly sorted
	// so that entries are in file offset order.
	void GetFileTableAsEntries(PackageCrc32Entries& rv) const;

	/**
	 * Populate rtFileTable with this PackageFileSystem's
	 * table and return true, or leave rtFileTable unmodified
	 * and return false.
	 */
	virtual Bool GetFileTable(FileTable& rtFileTable) const SEOUL_OVERRIDE
	{
		rtFileTable = GetFileTable();
		return true;
	}

	/**
	 * @return The file header data of this PackageFileSystem.
	 */
	const PackageFileHeader& GetHeader() const
	{
		return m_Header;
	}

	/**
	 * @return True if the package has an obfuscation XOR key applied, false otherwise.
	 */
	Bool IsObfuscated() const
	{
		return m_Header.IsObfuscated();
	}

	// Perform a CRC-32 check on all files in this PackageFileSystem, returns true if all
	// pass, false otherwise.
	//
	// Exact behavior of this method depends on the value of pvInOutEntries:
	// - if null, this function checks all files until a file fails the
	//   CRC32 check, at which point it early outs and returns false. Otherwise,
	//   it returns true.
	// - if non-null but empty, this function behaves similarly to when
	//   the pvInOutEntries argument is null, except that is does not early
	//   out. Also, pvInOutEntries will be populated with all files in
	//   this archive and their crc32 state (true or false depending on
	//   the state on disk).
	// - if non-null and not empty, this function only checks crc32 for
	//   the files listed. On output, crc32 state and entry state will
	//   be popualted.
	//
	// This method is expensive - you probably want to call it off the main thread.
	virtual Bool PerformCrc32Check(PackageCrc32Entries* pvInOutEntries = nullptr) SEOUL_OVERRIDE;

	// Perform a CRC-32 check on an individual file in this PackageFileSystem.
	//
	// This method returns false if filePath is not present in the PackageFileSystem.
	Bool PerformCrc32Check(FilePath filePath);

	// FilePath of the compression dict inside this archive. Will be empty if no
	// compression dict is being used.
	FilePath GetCompressionDictFilePath() const { return m_CompressionDictFilePath; }

	// True or false if a compression dict has been processed. Remains false if
	// GetCompressionDictFilePath() is not valid.
	Bool IsCompressionDictProcessed() const { return m_bProcessedCompressionDict; }

	// Compression dictionary - usually empty, unless compression is enabled on this
	// package and the enabled compression uses a compression dictionary.
	ZSTDDecompressionDict const* GetDecompressionDict() const { return m_pDecompressionDict; }

	// Used in special cases to read the compression dict for this
	// package. Nop if already completed successfully.
	Bool ProcessCompressionDict();

private:
	friend class PackageSyncFile;

	String m_sAbsolutePackageFilename;
	ScopedPtr<SyncFile> m_pPackageFile;
	GameDirectory m_ePackageGameDirectory;
	Int64 m_iCurrentFileOffset;

	Bool InsideLockComputeCrc32Post(const PackageFileEntry& entry, void* pBuffer, UInt32& ruCrc32);
	Bool InternalPerformPreCrc32Check(PackageCrc32Entries& rv, Bool bEntriesOut);

	void InternalProcessPackageFile(Bool bDeferCompressionDictProcessing);
	Bool InternalProcessPackageFileTable(
		const PackageFileHeader& header,
		Bool& rbHasPostCrc32,
		Bool bEndianSwap);

	FileTable m_tFileTable;

	// Sorted list of normalized, relative filenames; only valid if this package
	// file system supports directory queries
	typedef Vector<FilePath, MemoryBudgets::Io> FileList;
	FileList m_vSortedFileList;

	// Compression dictionary used for decompression, if it was enabled
	// for this package.
	typedef Vector<Byte, MemoryBudgets::Io> DictMemory;
	DictMemory m_vDictMemory;
	ZSTDDecompressionDict* m_pDecompressionDict;

	PackageFileHeader m_Header;
	FilePath m_CompressionDictFilePath;
	Mutex m_Mutex;
	Atomic32 m_ActiveSyncFileCount;
	Atomic32Value<Bool> m_bProcessedCompressionDict;
	String m_sLoadError;
	Bool m_bHasPostCrc32;
	Bool m_bOk;

private:
	SEOUL_DISABLE_COPY(PackageFileSystem);
}; // class PackageFileSystem

} // namespace Seoul

#endif // include guard
