/**
 * \file AnimationContentLoader.cpp
 * \brief Specialization of Content::LoaderBase for loading animation
 * data and animation network data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationContentLoader.h"
#include "Compress.h"
#include "CookManager.h"
#include "DataStore.h"
#include "DataStoreParser.h"
#include "FileManager.h"
#include "ReflectionDeserialize.h"
#include "SettingsManager.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul
{

namespace Reflection
{

/**
 * Context for deserializing animation data.
 */
class AnimationContext SEOUL_SEALED : public DefaultSerializeContext
{
public:
	AnimationContext(
		const ContentKey& contentKey,
		const DataStore& dataStore,
		const DataNode& table,
		const TypeInfo& typeInfo)
		: DefaultSerializeContext(contentKey, dataStore, table, typeInfo)
	{
	}

	virtual Bool HandleError(SerializeError eError, HString additionalData = HString()) SEOUL_OVERRIDE
	{
		using namespace Reflection;

		// Required and similar errors are always (silently) ignored, no properties
		// in animation data are considered required.
		if (SerializeError::kRequiredPropertyHasNoCorrespondingValue != eError &&
			SerializeError::kDataStoreContainsUndefinedProperty != eError)
		{
			// Use the default handling to issue a warning.
			return DefaultSerializeContext::HandleError(eError, additionalData);
		}

		return true;
	}
}; // class AnimationContext

} // namespace Reflection

namespace Animation
{

NetworkContentLoader::NetworkContentLoader(FilePath filePath, const AnimationNetworkContentHandle& hEntry)
	: Content::LoaderBase(filePath, Content::LoadState::kLoadingOnWorkerThread)
	, m_hEntry(hEntry)
{
	m_hEntry.GetContentEntry()->IncrementLoaderCount();
}

NetworkContentLoader::~NetworkContentLoader()
{
	// Block until this Content::LoaderBase is in a non-loading state.
	WaitUntilContentIsNotLoading();

	InternalReleaseEntry();
}

Content::LoadState NetworkContentLoader::InternalExecuteContentLoadOp()
{
	// Get the data.
	auto pSettings(SettingsManager::Get()->WaitForSettings(GetFilePath()));

	// Error if failed to load.
	if (!pSettings.IsValid())
	{
		return Content::LoadState::kError;
	}

	SharedPtr<NetworkDefinition> pNetwork(SEOUL_NEW(MemoryBudgets::Rendering) NetworkDefinition);
	Reflection::AnimationContext context(
		GetFilePath(),
		*pSettings,
		pSettings->GetRootNode(),
		TypeId<NetworkDefinition>());
	if (Reflection::DeserializeObject(context, *pSettings, pSettings->GetRootNode(), pNetwork.GetPtr()))
	{
		m_hEntry.GetContentEntry()->AtomicReplace(pNetwork);
		InternalReleaseEntry();

		// Done with loading body, decrement the loading count.
		return Content::LoadState::kLoaded;
	}

	// Swap an invalid entry into the slot.
	m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<NetworkDefinition>());

	// Done with loading body, decrement the loading count.
	return Content::LoadState::kError;
}

void NetworkContentLoader::InternalReleaseEntry()
{
	if (m_hEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		AnimationNetworkContentHandle::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

} // namespace Animation

} // namespace Seoul

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
