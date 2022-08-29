/**
 * \file FalconFCNFile.h
 * \brief An FCN file exactly corresponds to a Flash SWF file,
 * with some Falcon specific extensions.
 *
 * An FCN is a cooked SWF. More or less, the structure of the FCN
 * is identical to the SWF, except the data is never GZIP compressed,
 * but rather uses ZSTD compression on the entire file.
 *
 * Further, embedded image tags never exist in an FCN file. They
 * are always replaced with an FCN extension, the "external image
 * reference".
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_FCN_FILE_H
#define FALCON_FCN_FILE_H

#include "FalconDefinition.h"
#include "FalconMovieClipDefinition.h"
#include "FalconTypes.h"
#include "FilePath.h"
#include "HashSet.h"
#include "HashTable.h"
#include "SharedPtr.h"
#include "Mutex.h"
#include "SeoulHString.h"

namespace Seoul::Falcon
{

// forward declarations
class FCNFile;
class FCNLibraryAnchor;
class SwfReader;

static const UInt32 kFCNSignature = 0xF17AB839;
static const UInt32 kFCNVersion = 1;

class FCNFile SEOUL_SEALED
{
public:
	/** Tool/convenience utility to retrieve dependencies of an FCN file. */
	typedef Vector<FilePath, MemoryBudgets::Falcon> FCNDependencies;
	static Bool GetFCNFileDependencies(
		FilePath filePath,
		void const* pData,
		UInt32 uSizeInBytes,
		FCNDependencies& rv);

	FCNFile(
		const HString& sURL,
		UInt8 const* pData,
		UInt32 uSizeInBytes);
	~FCNFile();

	RGBA GetBackgroundColor() const
	{
		return m_BackgroundColor;
	}

	Rectangle GetBounds() const
	{
		return m_Bounds;
	}

	template <typename T>
	Bool GetDefinition(UInt16 uDefinitionID, SharedPtr<T>& rp) const
	{
		SharedPtr<Definition> p;
		if (!GetDefinition(uDefinitionID, p))
		{
			return false;
		}

		if (!p.IsValid() || DefinitionTypeOf<T>::Value != p->GetType())
		{
			return false;
		}

		SharedPtr<T> pt((T*)p.GetPtr());
		rp.Swap(pt);
		return true;
	}

	Bool GetDefinition(UInt16 uDefinitionID, SharedPtr<Definition>& rp) const
	{
		if (0 == uDefinitionID)
		{
			rp = m_pRootMovieClip;
			return true;
		}

		SharedPtr<Definition> p;
		if (!m_tDictionary.GetValue(uDefinitionID, p))
		{
			return false;
		}
		rp.Swap(p);
		return true;
	}

	template <typename T>
	Bool GetExportedDefinition(const HString& sName, SharedPtr<T>& rp) const
	{
		SharedPtr<Definition> p;
		if (!GetExportedDefinition(sName, p))
		{
			return false;
		}

		if (!p.IsValid() || DefinitionTypeOf<T>::Value != p->GetType())
		{
			return false;
		}

		rp.Reset((T*)p.GetPtr());
		p.Reset();
		return true;
	}

	Bool GetExportedDefinition(const HString& sName, SharedPtr<Definition>& rp) const
	{
		UInt16 uDefinitionID = 0;
		if (!m_tExportedSymbols.GetValue(sName, uDefinitionID))
		{
			return false;
		}

		return GetDefinition(uDefinitionID, rp);
	}

	Float GetFramesPerSecond() const
	{
		return m_fFramesPerSeconds;
	}

	template <typename T>
	Bool GetImportedDefinition(const HString& sName, SharedPtr<T>& rp, Bool bCheckNested = false) const
	{
		SharedPtr<Definition> p;
		if (!GetImportedDefinition(sName, p, bCheckNested))
		{
			return false;
		}

		if (!p.IsValid() || DefinitionTypeOf<T>::Value != p->GetType())
		{
			return false;
		}

		SharedPtr<T> pt((T*)p.GetPtr());
		rp.Swap(pt);
		return true;
	}

	Bool GetImportedDefinition(const HString& sName, SharedPtr<Definition>& rp, Bool bCheckNested = false) const;

	const SharedPtr<MovieClipDefinition>& GetRoot() const
	{
		return m_pRootMovieClip;
	}

	const HString& GetURL() const
	{
		return m_sURL;
	}

	Bool IsLibrary() const
	{
		return (0 != m_LibraryReferenceCount);
	}

	Bool IsOk() const
	{
		return m_bOk;
	}

	Bool Validate() const;

private:
	Bool Read(SwfReader& rBuffer);
	Bool ReadTags(SwfReader& rBuffer, const SharedPtr<MovieClipDefinition>& pMovieClip);
	Bool ReadTag(SwfReader& rBuffer, const SharedPtr<MovieClipDefinition>& pMovieClip, UInt32& ruCurrentFrame, TagId& reTagId);

	typedef HashSet<HString, MemoryBudgets::Falcon> ValidateSymbolsSet;
	typedef HashTable<HString, HString, MemoryBudgets::Falcon> ValidateSymbolsTable;
	Bool ValidateTimelines() const;
	Bool ValidateUniqueSymbols(HString rootUrl, ValidateSymbolsSet& visited, ValidateSymbolsTable& t) const;

	SEOUL_REFERENCE_COUNTED(FCNFile);

	SharedPtr<MovieClipDefinition> m_pRootMovieClip;

	typedef HashTable< UInt16, SharedPtr<Definition>, MemoryBudgets::Falcon > Dictionary;
	Dictionary m_tDictionary;

	typedef HashTable< HString, UInt16, MemoryBudgets::Falcon > Exports;
	Exports m_tExportedSymbols;

	typedef HashTable<HString, SimpleActions, MemoryBudgets::Falcon> AllSimpleActions;
	AllSimpleActions m_tAllSimpleActions;

	struct ImportEntry
	{
		ImportEntry()
			: m_sURL()
			, m_uDefinitionID(0)
		{
		}

		HString m_sURL;
		UInt16 m_uDefinitionID;
	};
	typedef HashTable< HString, ImportEntry, MemoryBudgets::Falcon > Imports;
	Imports m_tImports;
	typedef HashTable< HString, FCNLibraryAnchor*, MemoryBudgets::Falcon > ImportSources;
	ImportSources m_tImportSources;

	typedef HashTable< UInt32, HString, MemoryBudgets::Falcon, LabelsTraits > Labels;
	Labels m_tMainTimelineFrameLabels;
	Labels m_tMainTimelineSceneLabels;

	friend class FCNLibraryAnchor;
	Atomic32 m_LibraryReferenceCount;
	Rectangle m_Bounds;
	RGBA m_BackgroundColor;
	Float m_fFramesPerSeconds;
	HString m_sURL;
	Bool m_bOk;

	SEOUL_DISABLE_COPY(FCNFile);
}; // class FCNFile

class FCNLibraryAnchor SEOUL_ABSTRACT
{
public:
	virtual ~FCNLibraryAnchor()
	{
		if (m_p.IsValid())
		{
			--(m_p->m_LibraryReferenceCount);
		}
	}

	const SharedPtr<FCNFile>& GetPtr() const
	{
		return m_p;
	}

protected:
	FCNLibraryAnchor(const SharedPtr<FCNFile>& p)
		: m_p(p)
	{
		if (m_p.IsValid())
		{
			++(m_p->m_LibraryReferenceCount);
		}
	}

private:
	SharedPtr<FCNFile> m_p;

	SEOUL_DISABLE_COPY(FCNLibraryAnchor);
}; // class FCNLibraryAnchor

} // namespace Seoul::Falcon

#endif // include guard
