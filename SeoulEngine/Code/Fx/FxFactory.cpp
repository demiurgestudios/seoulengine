/**
 * \file FxFactory.cpp
 * \brief Utility class which handles loading and playback of fx
 * by HString identifier. It is not necessary to use this class to use Fx,
 * it is provided as a convenience when you want more flexibility regarding
 * Fx lifespan.
 *
 * "Named" fx are fx for which the handle persists,
 * and the particular instance of the fx can be manipulated after the event
 * has started. You want to use a named fx for looping events,
 * events with keys, parameters, or an event that you want to stop at a specific
 * time.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FxFactory.h"
#include "FxManager.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "ThreadId.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(FxProperties)
	SEOUL_PROPERTY_N("Duration", m_fDuration)
	SEOUL_PROPERTY_N("HasLoops", m_bHasLoops)
SEOUL_END_TYPE()

FxFactory::FxFactory()
{
}

FxFactory::~FxFactory()
{
	SafeDeleteTable(m_tTemplates);
}

/**
 * Setup the set of fx that can be instanced by a fxId identifier.
 *
 * \warning Calling this method will immediately stop any existing fx
 * in this FxFactory.
 */
Bool FxFactory::Configure(
	const DataStore& dataStore,
	const DataNode& tableNode,
	Bool bAppend /*=false*/,
	FilePath configFilePath /*=FilePath()*/
)
{
	SEOUL_ASSERT(IsMainThread());

	if (!bAppend)
	{
		SafeDeleteTable(m_tTemplates);
		m_tFxLookup.Clear();
	}

	Bool bHitAFailure = false;

	auto const iBegin = dataStore.TableBegin(tableNode);
	auto const iEnd = dataStore.TableEnd(tableNode);
	for (auto i = iBegin; iEnd != i; ++i)
	{
		FilePath filePath;
		if (!dataStore.AsFilePath(i->Second, filePath))
		{
			// Support variation case.
			if (i->Second.IsArray())
			{
				DataNode inner;
				if (!dataStore.GetValueFromArray(i->Second, 0u, inner) ||
					!dataStore.AsFilePath(inner, filePath))
				{
					SEOUL_WARN("Malformed file path in %s for FX %s",
						configFilePath.CStr(), i->First.CStr());
					bHitAFailure = true;
					continue;
				}
			}
			else
			{
				SEOUL_WARN("Malformed file path in %s for FX %s",
					configFilePath.CStr(), i->First.CStr());
				bHitAFailure = true;
				continue;
			}
		}

		// Insert an entry for the main effect - this is the key with no sub key.
		{
			if (bAppend)
			{
				// If appending, make sure to delete any values associated with keys that are about to be overwritten
				Fx* pExistingEntry = nullptr;
				if (m_tTemplates.GetValue(i->First, pExistingEntry))
				{
					SEOUL_VERIFY(m_tTemplates.Erase(i->First));

					SafeDelete(pExistingEntry);
				}
			}

			// Overwrite the lookup.
			SEOUL_VERIFY(m_tFxLookup.Overwrite(i->First, filePath).Second);

			// If loading all, do so now.
			if (m_bPreloadAll)
			{
				Fx* pFx = nullptr;
				if (FxManager::Get()->GetFx(filePath, pFx))
				{
					// It should be impossible for this to fail, since the key is already a key in
					// a table and must be unique.
					SEOUL_VERIFY(m_tTemplates.Insert(i->First, pFx).Second);
				}
			}
		}

		// If the entry has a table of additional variations, add those as well.
		DataNode variationsTable;
		if (dataStore.GetValueFromArray(i->Second, 1u, variationsTable) &&
			variationsTable.IsTable())
		{
			auto const iBegin = dataStore.TableBegin(variationsTable);
			auto const iEnd = dataStore.TableEnd(variationsTable);
			for (auto j = iBegin; iEnd != j; ++j)
			{
				// Value is expected to be a FilePath.
				FilePath variationfilePath;
				if (!dataStore.AsFilePath(j->Second, variationfilePath))
				{
					SEOUL_WARN("Malformed file path in %s for FX %s",
						configFilePath.CStr(), i->First.CStr());
					bHitAFailure = true;
					continue;
				}

				// Setup an FxKey for the sub Fx.
				FxKey fxKey(i->First, j->First);

				if (bAppend)
				{
					// If appending, make sure to delete any values associated with keys that are about to be overwritten
					Fx* pExistingEntry = nullptr;
					if (m_tTemplates.GetValue(fxKey, pExistingEntry))
					{
						SEOUL_VERIFY(m_tTemplates.Erase(fxKey));
						SafeDelete(pExistingEntry);
					}
				}

				// It should be impossible for this to fail, since the key is already a key in
				// a table and must be unique.
				SEOUL_VERIFY(m_tFxLookup.Overwrite(fxKey, variationfilePath).Second);

				// If loading all, do so now.
				if (m_bPreloadAll)
				{
					// Update the data element of the key and get the variation Fx.
					Fx* pFxVariation = nullptr;
					if (FxManager::Get()->GetFx(variationfilePath, pFxVariation))
					{
						// It should be impossible for this to fail, since the key is already a key in
						// a table and must be unique.
						SEOUL_VERIFY(m_tTemplates.Insert(fxKey, pFxVariation).Second);
					}
				}
			}
		}
	}

	if (bHitAFailure)
	{
		return false;
	}

	return true;
}

/**
 * Convenience function - query the duration of a factoried FX based on its template
 * id.
 */
Bool FxFactory::GetFxDuration(HString id, Float& rfDuration)
{
	Fx* pFx = GetOrCreateTemplate(id);
	if (nullptr != pFx)
	{
		FxProperties props;
		if (pFx->GetProperties(props))
		{
			rfDuration = props.m_fDuration;
			return true;
		}
	}

	return false;
}

/** Append any assets that the Fx defined by fxId (and its variations) are dependent on. */
Bool FxFactory::AppendAssets(HString fxId, FxAssetsVector& rvAssets) const
{
	SEOUL_ASSERT(IsMainThread());

	// A bit expensive, but not typically a critical path, so it's ok.
	auto const iBegin = m_tTemplates.Begin();
	auto const iEnd = m_tTemplates.End();

	Bool bReturn = true;
	for (auto i = iBegin; iEnd != i; ++i)
	{
		if (i->First.m_FxId == fxId)
		{
			bReturn = i->Second->AppendAssets(rvAssets) && bReturn;
		}
	}

	return bReturn;
}

/**
 * Utility used to add additional FX to the factory beyond
 * the initial configuration.
 */
void FxFactory::AppendFx(HString fxId, FilePath filePath)
{
	{
		// Delete any existing entry when appending.
		Fx* pExistingEntry = nullptr;
		if (m_tTemplates.GetValue(fxId, pExistingEntry))
		{
			SEOUL_VERIFY(m_tTemplates.Erase(fxId));
			SafeDelete(pExistingEntry);
		}
	}

	// Overwrite.
	SEOUL_VERIFY(m_tFxLookup.Overwrite(fxId, filePath).Second);
	
	// If preloading all, do so now.
	if (m_bPreloadAll)
	{
		// Get the Fx.
		Fx* pFx = nullptr;
		if (FxManager::Get()->GetFx(filePath, pFx))
		{
			// It should be impossible for this to fail, since the key is already a key in
			// a table and must be unique.
			SEOUL_VERIFY(m_tTemplates.Insert(fxId, pFx).Second);
		}
	}
}

Fx* FxFactory::GetOrCreateTemplate(const FxKey& fxKey)
{
	SEOUL_ASSERT(IsMainThread());
	Fx* pFxTemplate = nullptr;

	// Using the fxKey as the key includes the sub id.
	if (m_tTemplates.GetValue(fxKey, pFxTemplate))
	{
		// If we already have a template, just return that.
		return pFxTemplate;
	}
	// If we don't have a template for the fx + sub id, try to create one.
	FilePath filePath;
	if (m_tFxLookup.GetValue(fxKey, filePath)
		&& FxManager::Get()->GetFx(filePath, pFxTemplate))
	{
		// Insertion must succeed.
		SEOUL_VERIFY(m_tTemplates.Insert(fxKey, pFxTemplate).Second);
		return pFxTemplate;
	}

	// If we failed getting an entry for the specified fxKey,
	// try again with just the fxId (no sub id).
	if (m_tTemplates.GetValue(fxKey.m_FxId, pFxTemplate))
	{
		return pFxTemplate;
	}
	
	// If we are falling back on the fxId without sub id and that template also did not exist, create it.
	if (!m_tFxLookup.GetValue(fxKey.m_FxId, filePath))
	{
		return nullptr;
	}

	// Retrieve from the FxManager.
	if (!FxManager::Get()->GetFx(filePath, pFxTemplate))
	{
		return nullptr;
	}

	// Insertion must succeed.
	SEOUL_VERIFY(m_tTemplates.Insert(fxKey.m_FxId, pFxTemplate).Second);
	return pFxTemplate;
}

/**
 * Trigger a one-off fx - must be a finite fx that does not loop,
 * as you will have no control over the event once this method returns.
 *
 * @return True if the event was successfully started, false otherwise.
 */
Fx* FxFactory::CreateFx(const FxKey& fxKey)
{
	SEOUL_ASSERT(IsMainThread());

	auto pFxTemplate = GetOrCreateTemplate(fxKey);

	// At this point, we either have a valid or can re-query
	// after it was created above.
	if (nullptr != pFxTemplate ||
		m_tTemplates.GetValue(fxKey, pFxTemplate) ||
		m_tTemplates.GetValue(fxKey.m_FxId, pFxTemplate))
	{
		return pFxTemplate->Clone();
	}

	return nullptr;
}

/**
 * @return True if the FX in this factory are still being loaded,
 * false otherwise.
 */
Bool FxFactory::IsLoading() const
{
	for (auto const& e : m_tTemplates)
	{
		if (e.Second->IsLoading())
		{
			return true;
		}
	}

	return false;
}

/** Get the Fx async loading for use in the short term. */
void FxFactory::Prefetch(const FxKey& fxKey)
{
	Fx* pTemplateFx = nullptr;
	if (m_tTemplates.GetValue(fxKey, pTemplateFx))
	{
		// Done if we already have a template.
		return;
	}

	// Get the mapping.
	FilePath filePath;
	if (!m_tFxLookup.GetValue(fxKey, filePath))
	{
		return;
	}
		
	// Retrieve from FxManager.
	if (!FxManager::Get()->GetFx(filePath, pTemplateFx))
	{
		return;
	}

	// Insertion must succeed.
	SEOUL_VERIFY(m_tTemplates.Overwrite(fxKey, pTemplateFx).Second);
}

/**
 * Swap the contents of this factory with another.
 */
void FxFactory::Swap(FxFactory& rFactory)
{
	m_tFxLookup.Swap(rFactory.m_tFxLookup);
	m_tTemplates.Swap(rFactory.m_tTemplates);
	Seoul::Swap(m_bPreloadAll, rFactory.m_bPreloadAll);
}

} // namespace Seoul
