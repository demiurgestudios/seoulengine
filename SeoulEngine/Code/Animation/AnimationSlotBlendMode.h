/**
 * \file AnimationSlotBlendMode.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_SLOT_BLEND_MODE_H
#define ANIMATION_SLOT_BLEND_MODE_H

#include "Prereqs.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

enum class SlotBlendMode
{
	/**
	 * In blended transitions, the slot assignment
	 * is "blended" - the source animation is used until
	 * the blend reaches 0.5, at which point the
	 * to target animation is used.
	 */
	kNone,

	/**
	 * In blended transitions, the slot assignment of
	 * the source animation is used only (the slot assignment
	 * of the target is not applied until the blend is complete).
	 */
	kOnlySource,

	/**
	 * In blended transitions, the slot assignment of
	 * the target animation is used only (the slot assignment
	 * of the source is abandoned as soon as the blend starts).
	 */
	kOnlyTarget,
};

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
