/**
 * \file AnimationPlayClipDefinition.h
 * \brief Defines a clip playback node in an animation graph. This is read-only
 * data at runtime. To evaluate a play clip node, you must instantiate
 * an Animation::PlayClipInstance, which will normally occur as part
 * of creating an Animation::NetworkInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_PLAY_CLIP_DEFINITION_H
#define ANIMATION_PLAY_CLIP_DEFINITION_H

#include "AnimationClipSettings.h"
#include "AnimationNodeDefinition.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

class PlayClipDefinition SEOUL_SEALED : public NodeDefinition
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(PlayClipDefinition);

	PlayClipDefinition();
	~PlayClipDefinition();

	virtual NodeInstance* CreateInstance(NetworkInstance& r, const NodeCreateData& creationData) const SEOUL_OVERRIDE;
	virtual NodeType GetType() const SEOUL_OVERRIDE { return NodeType::kPlayClip; }

	Bool GetLoop() const { return m_bLoop; }
	HString GetName() const { return m_Name; }
	HString GetOnComplete() const { return m_OnComplete; }

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(PlayClipDefinition);
	SEOUL_REFLECTION_FRIENDSHIP(PlayClipDefinition);

	HString m_Name;
	HString m_OnComplete;
	Float m_fEventMixThreshold;
	Bool m_bLoop;

	SEOUL_DISABLE_COPY(PlayClipDefinition);
}; // class PlayClipDefinition

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
