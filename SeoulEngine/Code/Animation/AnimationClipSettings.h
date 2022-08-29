/**
 * \file AnimationClipSettings.h
 * \brief Additional clip control settings that are not specified
 * in our animation authoring tool.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_CLIP_SETTINGS_H
#define ANIMATION_CLIP_SETTINGS_H

#include "Prereqs.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

// Additional settings that control clip playback. These
// are defined by an animation network, as our 2D animation
// tool does not support them.
struct ClipSettings SEOUL_SEALED
{
	ClipSettings()
		: m_fEventMixThreshold(0.0f)
	{
	}

	// The blending mix threshold below which event
	// timelines will be suppressed.
	Float m_fEventMixThreshold;
}; // struct ClipSettings

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
