/**
 * \file SccFileTypeOptions.h
 * \brief Struct encapsulating options that can be used to modify source
 * control options at the file granularity.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCC_FILE_TYPE_OPTIONS_H
#define SCC_FILE_TYPE_OPTIONS_H

#include "Prereqs.h"

namespace Seoul::Scc
{

/**
 * Optional argument to a subset of source control
 * commands - updates/sets file type options for a file
 * being opened for edit, add, etc.
 */
struct FileTypeOptions SEOUL_SEALED
{
	/**
	 * Describe the base file type of a Perforce file.
	 */
	enum BaseFileType
	{
		/** No explicit type. */
		kDefaultType,

		/** File is ASCII text. */
		kText,

		/** File is considered raw bytes, stored compressed by default. */
		kBinary,

		/** On supported platforms, treated as a symbolic link. */
		kSymlink,

		/** "AppleSingle storage of Mac data fork, resource fork, file type and file creator." */
		kApple,

		/** "The only file type for Mac resource forks in Perforce 99.1 and before. Still supported, but the apple file type is preferred." */
		kResource,

		/** Treats the file as Unicode - only works if the Perforce server is in Unicode mode. */
		kUnicode,

		/**
		* If the Perforce server is in Unicode mode, this is equivalent to the Unicode modifier.
		* If the server is in non-Unicode mode, than this treats files as utf16 on clients.
		*/
		kUtf16,
	}; // enum BaseFileType

	/**
	 * Collection of additional (optional) modifiers that describe the behavior of a file
	 * in Perforce.
	 */
	enum FileTypeModifier
	{
		/** No special modifiers. */
		kNone = 0,

		/** (+w) - file is not made read-only on clients. */
		kAlwaysWriteable = (1 << 0),

		/** (+x) - file is marked as executable on clients. */
		kExecuteBit = (1 << 1),

		/** (+k) - expands revision control system keywords. */
		kRcsKeywordExpansion = (1 << 2),

		/** (+l) - file can only be opened for edit by one user at a time. */
		kExclusiveOpen = (1 << 3),

		/** (+C) - files are stored in compressed form on the server. */
		kStoreCompressedVersionOfEachRevision = (1 << 4),

		/** (+D) - delta storage for text files. */
		kStoreDeltasInRcsFormat = (1 << 5),

		/** (+F) - files are stored uncompressed on the server. */
		kStoreUncompressedVersionOfEachRevision = (1 << 6),

		/** (+m) - file last modified time stored and preserved by the server. */
		kPreserveModificationTime = (1 << 7),

		/** (+X) - the server runs an "archive trigger" to access the file. */
		kArchiveTriggerRequired = (1 << 8)
	}; // enum FileTypeModifier

	/**
	 * Special handling of the +S file type modifier, limits the number of revisions
	 * stored by the server.
	 */
	enum NumberOfRevisions
	{
		/** This is a special code that means "don't set the +S revision limit modifier". */
		kUnlimited = 0,
		k1 = 1,
		k2 = 2,
		k3 = 3,
		k4 = 4,
		k5 = 5,
		k6 = 6,
		k7 = 7,
		k8 = 8,
		k9 = 9,
		k10 = 10,
		k16 = 16,
		k32 = 32,
		k64 = 64,
		k128 = 128,
		k256 = 256,
		k512 = 512,
	}; // enum NumberOfRevisions

	static FileTypeOptions Create(
		BaseFileType eBaseFileType,
		Int32 iModifiers,
		NumberOfRevisions eNumberOfRevisions)
	{
		FileTypeOptions ret;
		ret.m_eBaseFileType = eBaseFileType;
		ret.m_iModifiers = iModifiers;
		ret.m_eNumberOfRevisions = eNumberOfRevisions;
		return ret;
	}

	Bool HasOptions() const
	{
		return
			(kDefaultType != m_eBaseFileType) ||
			(kNone != m_iModifiers) ||
			(kUnlimited != m_eNumberOfRevisions);
	}

	BaseFileType m_eBaseFileType{};
	Int32 m_iModifiers{};
	NumberOfRevisions m_eNumberOfRevisions{};
}; // struct FileTypeOptions

} // namespace Seoul::Scc

#endif // include guard
