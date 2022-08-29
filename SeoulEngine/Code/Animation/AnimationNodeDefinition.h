/**
 * \file AnimationNodeDefinition.h
 * \brief Base class of the read-only data of an animation
 * network node. Subclasses of this class are used to fully define
 * an animation network in content.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_NODE_DEFINITION_H
#define ANIMATION_NODE_DEFINITION_H

#include "AnimationNodeType.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "SharedPtr.h"
namespace Seoul { namespace Animation { class NetworkInstance; } }
namespace Seoul { namespace Animation { class NodeInstance; } }

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

struct NodeCreateData
{
	HString m_sOverrideDefaultState;

	NodeCreateData() = default;
	NodeCreateData(const NodeCreateData&) = default;
	NodeCreateData& operator=(const NodeCreateData&) = default;

	explicit NodeCreateData(HString sOverrideDefaultState)
	{
		m_sOverrideDefaultState = sOverrideDefaultState;
	}
};

class NodeDefinition SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(NodeDefinition);

	virtual ~NodeDefinition();

	virtual NodeInstance* CreateInstance(NetworkInstance& r, const NodeCreateData& creationData) const = 0;
	virtual NodeType GetType() const = 0;

protected:
	NodeDefinition();
	SEOUL_REFERENCE_COUNTED(NodeDefinition);

	SEOUL_DISABLE_COPY(NodeDefinition);
}; // class NodeDefinition

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
