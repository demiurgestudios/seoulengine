/**
 * \file FxStudioBankFile.cpp
 * \brief Shared FxStudio FX data loaded from an *.fxb bank file. Can spawn multiple instances
 * of an FX stored inside the bank.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "FileManager.h"
#include "Fx.h"
#include "FxStudioBankFile.h"
#include "FxStudioContentLoader.h"
#include "FxStudioManager.h"
#include "Logger.h"
#include "SeoulFile.h"
#include "SeoulFileReaders.h"

#if SEOUL_WITH_FX_STUDIO
#include "FxStudioRT.h"

namespace Seoul
{

/**
 * Process the raw file data loaded into pTotalData into a data structure
 * containing pointers to the FxStudio bank data and FxStudio FX file references.
 *
 * @return True if pTotalData and zTotalDataSizeInBytes was
 * successfully processed into rHeader, false otherwise.
 *
 * \warning rData will be modified even if this function fails.
 */
namespace FxStudio
{

Bool BankFile::PopulateData(
	FilePath filePath,
	void* pData,
	UInt32 uDataSizeInBytes,
	BankFileData& rData)
{
	// Cache data.
	rData.m_FilePath = filePath;
	rData.m_Name = HString(Path::GetFileNameWithoutExtension(filePath.GetRelativeFilenameWithoutExtension().ToString()).ToLowerASCII());
	rData.m_pData = pData;
	rData.m_uDataSizeInBytes = uDataSizeInBytes;

	// Validate the bank data.
	if (!Manager::Get()->GetFxStudioManager().ValidateStream(
		(UInt8 const*)rData.m_pData,
		rData.m_uDataSizeInBytes))
	{
		return false;
	}

	return true;
}

BankFile::BankFile(
	::FxStudio::Manager& rOwner,
	const BankFileData& data)
	: m_Data(data)
	, m_rOwner(rOwner)
	, m_pPreLoadedInstance(SEOUL_NEW(MemoryBudgets::Fx) ::FxStudio::FxInstance)
	, m_FxProperties()
{
	SEOUL_ASSERT(IsMainThread());

	// Load the bank data into the owner manager - must always succeed
	// or we have a validation bug.
	SEOUL_VERIFY(m_rOwner.LoadBank(m_Data.m_pData, m_Data.m_uDataSizeInBytes));

	// Get the instance loading.
	*m_pPreLoadedInstance = CreateFx();
	m_pPreLoadedInstance->SetAutoRender(false);
	m_pPreLoadedInstance->SetAutoUpdate(false);

	// Cache properties.
	m_FxProperties = InternalComputeFxProperties();
}

BankFile::~BankFile()
{
	SEOUL_ASSERT(IsMainThread());

	// Release the preload instance if it still exists.
	m_pPreLoadedInstance->Clear();

	// Unload the bank - this must always succeed or there's a reference counting
	// or other bug.
	SEOUL_VERIFY(m_rOwner.UnloadBank(m_Data.m_pData));

	// Free the data.
	m_Data.DeallocateAndClear();
}

/**
 * @return A new FxStudio::FxInstance of the Fx name in this FxStudio bank.
 */
const ::FxStudio::FxInstance& BankFile::CreateFx()
{
	SEOUL_ASSERT(IsMainThread());

	auto const& r = m_rOwner.CreateFx(
		m_Data.m_pData,
		m_Data.m_Name.CStr(),
		(void*)&(m_Data.m_FilePath));

	// Release the preload instance now that something has requested
	// a real instance. NOTE: We don't need mutex locking here
	// due to the IsMainThread() above (FxStudio is not even a little
	// bit thread safe). If that ever changes, then mutex locking
	// will be required to ensure atomic behavior around CreateFx().
	m_pPreLoadedInstance->Clear();

	return r;
}

/** Utility used by FxStudio::BankFile::AppendAssetsOfFx(). */
struct GetAssetsOfFxBinder
{
	GetAssetsOfFxBinder(FxAssetsVector& rv)
		: m_rv(rv)
	{
	}

	static void Bind(void* pUserData, Byte const* sAssetID)
	{
		((GetAssetsOfFxBinder*)pUserData)->BindImpl(sAssetID);
	};

	void BindImpl(Byte const* sAssetID)
	{
		m_rv.PushBack(FilePath::CreateContentFilePath(sAssetID));
	}

	FxAssetsVector& m_rv;
};

void BankFile::AppendAssetsOfFx(FxAssetsVector& rvAssets)
{
	GetAssetsOfFxBinder binder(rvAssets);

	m_rOwner.GetAssets(
		m_Data.m_pData,
		m_Data.m_Name.CStr(),
		&GetAssetsOfFxBinder::Bind,
		&binder);
}

/** @return properties of the Fx encoded in this bank file. */
void BankFile::GetProperties(FxProperties& rProperties) const
{
	rProperties = m_FxProperties;
}

/**
 * Swap the contents of this FxStudio::BankFile with rData,
 * reloads the bank with the new data.
 */
void BankFile::Swap(FilePath& rFilePath, BankFileData& rData)
{
	SEOUL_ASSERT(IsMainThread());

	// Release the preload instance if it still exists.
	m_pPreLoadedInstance->Clear();

	Seoul::Swap(m_Data, rData);

	SEOUL_VERIFY(m_rOwner.LoadBank(
		m_Data.m_pData,
		m_Data.m_uDataSizeInBytes));

	// Get the instance loading.
	*m_pPreLoadedInstance = CreateFx();
	m_pPreLoadedInstance->SetAutoRender(false);
	m_pPreLoadedInstance->SetAutoUpdate(false);

	// Cache properties.
	m_FxProperties = InternalComputeFxProperties();
}

FxProperties BankFile::InternalComputeFxProperties() const
{
	FxProperties ret;
	m_rOwner.GetFxProperties(
		m_Data.m_pData,
		m_Data.m_Name.CStr(),
		ret.m_fDuration,
		ret.m_bHasLoops);

	return ret;
}

} // namespace FxStudio

SharedPtr<FxStudio::BankFile> Content::Traits<FxStudio::BankFile>::GetPlaceholder(FilePath filePath)
{
	return SharedPtr<FxStudio::BankFile>();
}

Bool Content::Traits<FxStudio::BankFile>::FileChange(FilePath filePath, const Content::Handle<FxStudio::BankFile>& pEntry)
{
	// Sanity check that the type is an FxStudio bank type - Content::Store<> should have
	// done most of the filtering for us, making sure that the target already
	// exists in our store.
	if (FileType::kFxBank == filePath.GetType())
	{
		Content::LoadManager::Get()->Queue(
			SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) FxStudio::ContentLoader(filePath, pEntry)));

		return true;
	}

	return false;
}

void Content::Traits<FxStudio::BankFile>::Load(FilePath filePath, const Content::Handle<FxStudio::BankFile>& pEntry)
{
	Content::LoadManager::Get()->Queue(
		SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) FxStudio::ContentLoader(filePath, pEntry)));
}

Bool Content::Traits<FxStudio::BankFile>::PrepareDelete(FilePath filePath, Content::Entry<FxStudio::BankFile, KeyType>& entry)
{
	return FxStudio::Manager::Get()->PrepareDelete(filePath);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_FX_STUDIO
