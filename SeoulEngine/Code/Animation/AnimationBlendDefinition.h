/**
 * \file AnimationBlendDefinition.h
 * \brief Defines a blend node in an animation graph. This is read-only
 * data at runtime. To evaluate a blend node, you must instantiate
 * an Animation::BlendInstance, which will normally occur as part
 * of creating an Animation::NetworkInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_BLEND_DEFINITION_H
#define ANIMATION_BLEND_DEFINITION_H

#include "AnimationNodeDefinition.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

class BlendDefinition SEOUL_SEALED : public NodeDefinition
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(BlendDefinition);

	BlendDefinition();
	~BlendDefinition();

	virtual NodeInstance* CreateInstance(NetworkInstance& r, const NodeCreateData& creationData) const SEOUL_OVERRIDE;
	virtual NodeType GetType() const SEOUL_OVERRIDE { return NodeType::kBlend; }

	const SharedPtr<NodeDefinition>& GetChildA() const { return m_pChildA; }
	const SharedPtr<NodeDefinition>& GetChildB() const { return m_pChildB; }
	HString GetMixParameterId() const { return m_MixParameterId; }
	Bool GetSynchronizeTime() const { return m_bSynchronizeTime; }

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(BlendDefinition);
	SEOUL_REFLECTION_FRIENDSHIP(BlendDefinition);

	SharedPtr<NodeDefinition> m_pChildA;
	SharedPtr<NodeDefinition> m_pChildB;
	HString m_MixParameterId;
	Bool m_bSynchronizeTime;

	SEOUL_DISABLE_COPY(BlendDefinition);
}; // class BlendDefinition

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
