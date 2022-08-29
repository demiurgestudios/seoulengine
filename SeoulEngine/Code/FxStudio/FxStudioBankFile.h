/**
 * \file FxStudioBankFile.h
 * \brief Shared FxStudio FX data loaded from an *.fxb bank file. Can spawn multiple instances
 * of an FX stored inside the bank.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_BANK_FILE_H
#define FX_STUDIO_BANK_FILE_H

#include "ContentStore.h"
#include "ContentTraits.h"
#include "FilePath.h"
#include "Fx.h"
#include "SharedPtr.h"
#include "Prereqs.h"
#include "SeoulString.h"

#if SEOUL_WITH_FX_STUDIO

// Forward declarations
namespace FxStudio
{
	class FxInstance;
	class Manager;
}

namespace Seoul
{

// Forward declarations
namespace FxStudio { class BankFile; }

namespace Content
{

/**
 * Specialization of Content::Traits<> for FxStudio::BankFile, allows
 * FxStudio::BankFile to be managed as loadable content by the content system.
 */
template <>
struct Traits<FxStudio::BankFile>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static SharedPtr<FxStudio::BankFile> GetPlaceholder(FilePath filePath);
	static Bool FileChange(FilePath filePath, const Handle<FxStudio::BankFile>& pEntry);
	static void Load(FilePath filePath, const Handle<FxStudio::BankFile>& pEntry);
	static Bool PrepareDelete(FilePath filePath, Entry<FxStudio::BankFile, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<FxStudio::BankFile>& pEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<FxStudio::BankFile>& p) { return 0u; }
}; // struct Content::Traits<FxStudioBankFile>

} // namespace Content

namespace FxStudio
{

struct BankFileData SEOUL_SEALED
{
	BankFileData()
		: m_pData(nullptr)
		, m_uDataSizeInBytes(0u)
		, m_FilePath()
		, m_Name()
	{
	}

	void* m_pData;
	UInt32 m_uDataSizeInBytes;
	FilePath m_FilePath;
	HString m_Name;

	void Clear()
	{
		m_pData = nullptr;
		m_uDataSizeInBytes = 0u;
		m_FilePath = FilePath();
		m_Name = HString();
	}

	void DeallocateAndClear()
	{
		MemoryManager::Deallocate(m_pData);
		Clear();
	}
};

typedef Vector<FilePath> FxAssetsVector;
class BankFile SEOUL_SEALED
{
public:
	static Bool PopulateData(
		FilePath filePath,
		void* pData,
		UInt32 uDataSizeInBytes,
		BankFileData& rData);

	// Construct an FxStudio::BankFile with pData, takes ownership of the data
	// and will call MemoryManager::Deallocate() to free it on destruction.
	BankFile(::FxStudio::Manager& rOwner, const BankFileData& data);
	~BankFile();

	// Append all assets that are in used by Fx name to rvAssets.
	void AppendAssetsOfFx(FxAssetsVector& rvAssets);

	// Instantiate a new ::FxStudio::FxInstance from this FxStudio bank.
	const ::FxStudio::FxInstance& CreateFx();

	/**
	 * @return The data blob that defines this FxStudioBankFile.
	 */
	const BankFileData& GetData() const
	{
		return m_Data;
	}

	/**
	 * @return The FilePath of the bank file.
	 */
	FilePath GetFilePath() const
	{
		return m_Data.m_FilePath;
	}

	// Return properties of the Fx encoded in this bank file.
	void GetProperties(FxProperties& rProperties) const;

	// Exchange the data in this FxStudio::BankFile with rData.
	//
	// WARNING: This method can only be called on the main thread.
	void Swap(FilePath& rFilePath, BankFileData& rData);

private:
	BankFileData m_Data;
	::FxStudio::Manager& m_rOwner;
	ScopedPtr<::FxStudio::FxInstance> m_pPreLoadedInstance;
	FxProperties m_FxProperties;

	FxProperties InternalComputeFxProperties() const;

	SEOUL_DISABLE_COPY(BankFile);
	SEOUL_REFERENCE_COUNTED(BankFile);
}; // class FxStudio::BankFile

} // namespace FxStudio

} // namespace Seoul

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
