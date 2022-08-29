/**
 * \file AnimationNetworkDefinition.h
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

#pragma once
#ifndef ANIMATION_NETWORK_DEFINITION_H
#define ANIMATION_NETWORK_DEFINITION_H

#include "ContentHandle.h"
#include "HashTable.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "SharedPtr.h"
namespace Seoul { namespace Animation { class NodeDefinition; } }

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul
{

namespace Animation
{

struct NetworkDefinitionParameter SEOUL_SEALED
{
	NetworkDefinitionParameter()
		: m_fMin(0.0f)
		, m_fMax(1.0f)
		, m_fDefault(0.0f)
	{
	}

	Float m_fMin;
	Float m_fMax;
	Float m_fDefault;
}; // struct NetworkDefinitionParameter

class NetworkDefinition SEOUL_SEALED
{
public:
	typedef HashTable<HString, Bool, MemoryBudgets::Animation> Conditions;
	typedef HashTable<HString, NetworkDefinitionParameter, MemoryBudgets::Animation> Parameters;

	NetworkDefinition();
	~NetworkDefinition();

	const Conditions& GetConditions() const { return m_tConditions; }
	const Parameters& GetParameters() const { return m_tParameters; }
	const SharedPtr<NodeDefinition>& GetRoot() const { return m_pRoot; }

private:
	SEOUL_REFERENCE_COUNTED(NetworkDefinition);
	SEOUL_REFLECTION_FRIENDSHIP(NetworkDefinition);

	Conditions m_tConditions;
	Parameters m_tParameters;
	SharedPtr<NodeDefinition> m_pRoot;

	SEOUL_DISABLE_COPY(NetworkDefinition);
}; // class NetworkDefinition

} // namespace Animation

typedef Content::Handle<Animation::NetworkDefinition> AnimationNetworkContentHandle;

namespace Content
{

/**
 * Specialization of Content::Traits<> for Animation::NetworkDefinition, allows Animation::NetworkDefinition to be managed
 * as loadable content in the content system.
 */
template <>
struct Traits<Animation::NetworkDefinition>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static SharedPtr<Animation::NetworkDefinition> GetPlaceholder(FilePath filePath);
	static Bool FileChange(FilePath filePath, const AnimationNetworkContentHandle& hEntry);
	static void Load(FilePath filePath, const AnimationNetworkContentHandle& hEntry);
	static Bool PrepareDelete(FilePath filePath, Entry<Animation::NetworkDefinition, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<Animation::NetworkDefinition>& pEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<Animation::NetworkDefinition>& p) { return 0u; }
}; // Content::Traits<Animation::NetworkDefinition>

} // namespace Content

} // namespace Seoul

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
