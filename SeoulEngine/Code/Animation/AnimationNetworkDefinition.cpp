/**
 * \file AnimationNetworkDefinition.cpp
 * \brief Defines an animation network in content. This is read-only
 * data. To play back a network at runtime, you must instantiate an
 * Animation::NetworkInstance with this data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationContentLoader.h"
#include "AnimationNetworkDefinition.h"
#include "AnimationNetworkDefinitionManager.h"
#include "AnimationNodeDefinition.h"
#include "ContentLoadManager.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, Animation::NetworkDefinitionParameter, 1, DefaultHashTableKeyTraits<HString>>)
SEOUL_BEGIN_TYPE(Animation::NetworkDefinitionParameter)
	SEOUL_PROPERTY_N("Default", m_fDefault)
	SEOUL_PROPERTY_N("Min", m_fMin)
	SEOUL_PROPERTY_N("Max", m_fMax)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation::NetworkDefinition, TypeFlags::kDisableNew)
	SEOUL_PROPERTY_N("Conditions", m_tConditions)
	SEOUL_PROPERTY_N("Params", m_tParameters)
	SEOUL_PROPERTY_N("Root", m_pRoot)
SEOUL_END_TYPE()

namespace Animation
{

NetworkDefinition::NetworkDefinition()
	: m_tConditions()
	, m_tParameters()
	, m_pRoot()
{
}

NetworkDefinition::~NetworkDefinition()
{
}

} // namespace Animation

SEOUL_TYPE(AnimationNetworkContentHandle)

SharedPtr<Animation::NetworkDefinition> Content::Traits<Animation::NetworkDefinition>::GetPlaceholder(FilePath filePath)
{
	return SharedPtr<Animation::NetworkDefinition>();
}

Bool Content::Traits<Animation::NetworkDefinition>::FileChange(FilePath filePath, const AnimationNetworkContentHandle& hEntry)
{
	// TODO: Not entirely satisfied with this. It is convenient for network files to just be .json
	// data, but it runs the risk of duplicating data (if someone wants to access the animation network data
	// from script, for example, it will be loaded via SettingsManager).

	if (FileType::kJson == filePath.GetType() &&
		Animation::NetworkDefinitionManager::Get()->IsNetworkLoaded(filePath))
	{
		Load(filePath, hEntry);
		return true;
	}

	return false;
}

void Content::Traits<Animation::NetworkDefinition>::Load(FilePath filePath, const AnimationNetworkContentHandle& hEntry)
{
	Content::LoadManager::Get()->Queue(
		SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) Animation::NetworkContentLoader(filePath, hEntry)));
}

Bool Content::Traits<Animation::NetworkDefinition>::PrepareDelete(FilePath filePath, Content::Entry<Animation::NetworkDefinition, KeyType>& entry)
{
	return true;
}

} // namespace Seoul

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
